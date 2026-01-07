/*
 * neofetch-c - A fast system information tool written in C
 * Copyright (C) 2026 Alexia Michelle <https://github.com/alexiarstein/neofetch-c>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "neofetch.h"

void get_user_hostname(system_info_t *info) {
    // Get username using thread-safe version
    struct passwd pwd = {0};
    struct passwd *result = NULL;
    char buf[1024];
    
    if (getpwuid_r(getuid(), &pwd, buf, sizeof(buf), &result) == 0 && result != NULL) {
        safe_strcpy(info->user, pwd.pw_name, sizeof(info->user));
        info->user[sizeof(info->user) - 1] = '\0';
    } else {
        safe_strcpy(info->user, "unknown", sizeof(info->user));
    }
    
    // Get hostname
    if (gethostname(info->hostname, sizeof(info->hostname)) != 0) {
        safe_strcpy(info->hostname, "unknown", sizeof(info->hostname));
    }
    info->hostname[sizeof(info->hostname) - 1] = '\0';
}

void get_distro(system_info_t *info) {
    FILE *file;
    char line[256];
    char id[64] = {0};
    char version_id[64] = {0};
    char pretty_name[128] = {0};
    
    // Try to read /etc/os-release
    file = fopen("/etc/os-release", "r");
    if (!file) {
        file = fopen("/usr/lib/os-release", "r");
    }
    
    if (file) {
        while (fgets(line, sizeof(line), file)) {
            // Locate '=' and extract the value part to handle quoted and unquoted values
            if (strncmp(line, "PRETTY_NAME=", 12) == 0 || strncmp(line, "NAME=", 5) == 0 || strncmp(line, "VERSION_ID=", 11) == 0) {
                char *eq = strchr(line, '=');
                if (!eq) continue;
                char *val = eq + 1;
                // Trim whitespace/newline
                val = trim_whitespace(val);
                // Strip surrounding quotes if present
                if (*val == '"') {
                    char *endq = strchr(val + 1, '"');
                    if (endq) *endq = '\0';
                    val++;
                } else {
                    // Remove trailing newline if any
                    char *nl = strchr(val, '\n');
                    if (nl) *nl = '\0';
                }

                if (strncmp(line, "PRETTY_NAME=", 12) == 0) {
                    safe_strcpy(pretty_name, val, sizeof(pretty_name));
                } else if (strncmp(line, "NAME=", 5) == 0) {
                    safe_strcpy(id, val, sizeof(id));
                } else if (strncmp(line, "VERSION_ID=", 11) == 0) {
                    safe_strcpy(version_id, val, sizeof(version_id));
                }
            }
        }
        fclose(file);
        
        if (strnlen(pretty_name, sizeof(pretty_name)) > 0) {
            safe_strcpy(info->distro, pretty_name, sizeof(info->distro));
        } else if (strnlen(id, sizeof(id)) > 0) {
            if (strnlen(version_id, sizeof(version_id)) > 0) {
                snprintf(info->distro, sizeof(info->distro), "%s %s", id, version_id);
            } else {
                safe_strcpy(info->distro, id, sizeof(info->distro));
            }
        }
        info->distro[sizeof(info->distro) - 1] = '\0';
        return;
    }
    
    // Fallback: try lsb_release
    const char *result = execute_command("lsb_release -d 2>/dev/null | cut -f2");
    if (result && strnlen(result, sizeof(info->distro)) > 0) {
        safe_strcpy(info->distro, result, sizeof(info->distro));
        info->distro[sizeof(info->distro) - 1] = '\0';
        return;
    }
    
    // Last resort
    safe_strcpy(info->distro, "Linux", sizeof(info->distro));
    info->distro[sizeof(info->distro) - 1] = '\0';
}

void get_architecture(system_info_t *info) {
    char *output = execute_command("uname -m");
    if (output && strnlen(output, sizeof(info->architecture)) > 0) {
        // Remove newline and copy architecture
        output[strcspn(output, "\n")] = '\0';
        
        // Map common architectures to more descriptive names
        if (strcmp(output, "x86_64") == 0) {
            safe_strcpy(info->architecture, "x86-64", sizeof(info->architecture));
        } else if (strcmp(output, "i386") == 0 || strcmp(output, "i686") == 0) {
            safe_strcpy(info->architecture, "x86", sizeof(info->architecture));
        } else if (strcmp(output, "aarch64") == 0) {
            safe_strcpy(info->architecture, "ARM64", sizeof(info->architecture));
        } else if (strncmp(output, "arm", 3) == 0) {
            safe_strcpy(info->architecture, "ARM", sizeof(info->architecture));
        } else {
            safe_strcpy(info->architecture, output, sizeof(info->architecture));
        }
        info->architecture[sizeof(info->architecture) - 1] = '\0';
    } else {
        safe_strcpy(info->architecture, "Unknown", sizeof(info->architecture));
        info->architecture[sizeof(info->architecture) - 1] = '\0';
    }
}

// Helper function to check if string is valid (not empty, not placeholder)
static int is_valid_hardware_string(const char *str) {
    return str && strnlen(str, 256) > 0 && strcmp(str, "To be filled by O.E.M.") != 0;
}

// Helper function to safely copy hardware info
static void copy_hardware_info(char *dest, size_t dest_size, const char *src) {
    if (is_valid_hardware_string(src)) {
        safe_strcpy(dest, src, dest_size - 1);
        dest[dest_size - 1] = '\0';
    }
}

// Helper function to extract value after colon
static void extract_value_after_colon(const char *line, char *dest, size_t dest_size) {
    const char *value_start = strstr(line, ":");
    if (value_start) {
        value_start++;
        while (*value_start == ' ') value_start++;
        safe_strcpy(dest, value_start, dest_size - 1);
        dest[dest_size - 1] = '\0';
    }
}

// Helper function to try getting hardware info from DMI
static void get_hardware_from_dmi(char *vendor, size_t vendor_size, char *model, size_t model_size) {
    const char *vendor_output = execute_command("cat /sys/class/dmi/id/sys_vendor 2>/dev/null || cat /sys/class/dmi/id/board_vendor 2>/dev/null");
    if (vendor_output) {
        copy_hardware_info(vendor, vendor_size, vendor_output);
    }
    
    const char *model_output = execute_command("cat /sys/class/dmi/id/product_name 2>/dev/null || cat /sys/class/dmi/id/board_name 2>/dev/null");
    if (model_output) {
        copy_hardware_info(model, model_size, model_output);
    }
}

// Helper function to try getting hardware info from hostnamectl
static void get_hardware_from_hostnamectl(char *vendor, size_t vendor_size, char *model, size_t model_size) {
    if (strcmp(vendor, "Unknown") != 0 && strcmp(model, "Unknown") != 0) {
        return; // Already have both values
    }
    
    char *hostnamectl_output = execute_command("hostnamectl 2>/dev/null | grep -E 'Hardware (Vendor|Model)' | head -2");
    if (!hostnamectl_output || strnlen(hostnamectl_output, 512) == 0) {
        return;
    }
    
    char *saveptr;
    const char *line = strtok_r(hostnamectl_output, "\n", &saveptr);
    while (line != NULL) {
        if (strstr(line, "Hardware Vendor:") && strcmp(vendor, "Unknown") == 0) {
            extract_value_after_colon(line, vendor, vendor_size);
        } else if (strstr(line, "Hardware Model:") && strcmp(model, "Unknown") == 0) {
            extract_value_after_colon(line, model, model_size);
        }
        line = strtok_r(NULL, "\n", &saveptr);
    }
}

void get_hardware(system_info_t *info) {
    char vendor[128] = "Unknown";
    char model[128] = "Unknown";
    
    // Try DMI first
    get_hardware_from_dmi(vendor, sizeof(vendor), model, sizeof(model));
    
    // Try hostnamectl as fallback
    get_hardware_from_hostnamectl(vendor, sizeof(vendor), model, sizeof(model));
    
    // Store results
    safe_strcpy(info->hardware, vendor, sizeof(info->hardware));
    info->hardware[sizeof(info->hardware) - 1] = '\0';
    
    safe_strcpy(info->model, model, sizeof(info->model));
    info->model[sizeof(info->model) - 1] = '\0';
}

void get_kernel(system_info_t *info) {
    struct utsname uts;
    if (uname(&uts) == 0) {
        // Truncate to fit in buffer
        snprintf(info->kernel, sizeof(info->kernel), "%.60s %.60s", uts.sysname, uts.release);
    } else {
        safe_strcpy(info->kernel, "Unknown", sizeof(info->kernel));
    }
    info->kernel[sizeof(info->kernel) - 1] = '\0';
}

void get_uptime(system_info_t *info) {
    struct sysinfo s_info;
    if (sysinfo(&s_info) == 0) {
        format_uptime(s_info.uptime, info->uptime, sizeof(info->uptime));
    } else {
        safe_strcpy(info->uptime, "Unknown", sizeof(info->uptime));
    }
    info->uptime[sizeof(info->uptime) - 1] = '\0';
}

// Helper function to count packages for a specific package manager
static int count_packages(const char *command) {
    char *result = execute_command(command);
    return (result && atoi(result) > 0) ? atoi(result) : 0;
}

// Helper function to add package count to breakdown string
static void add_package_breakdown(char *breakdown, size_t breakdown_size, 
                                   int *breakdown_count, const char *pm_name, int count) {
    if (count > 0) {
        size_t current_len = strnlen(breakdown, breakdown_size);
        snprintf(breakdown + current_len, breakdown_size - current_len, 
                 "%s\033[36m%s\033[0m: %d", *breakdown_count > 0 ? " - " : "", pm_name, count);
        (*breakdown_count)++;
    }
}

void get_packages(system_info_t *info) {
    int total_packages = 0;
    char breakdown[384] = "";
    int breakdown_count = 0;
    
    // Check various package managers
    int dpkg_count = count_packages("dpkg -l 2>/dev/null | grep '^ii' | wc -l");
    int rpm_count = count_packages("rpm -qa 2>/dev/null | wc -l");
    int pacman_count = count_packages("pacman -Q 2>/dev/null | wc -l");
    int emerge_count = count_packages("ls -d /var/db/pkg/*/* 2>/dev/null | wc -l");
    int xbps_count = count_packages("xbps-query -l 2>/dev/null | wc -l");
    int apk_count = count_packages("apk list --installed 2>/dev/null | wc -l");
    int flatpak_count = count_packages("flatpak list 2>/dev/null | wc -l");
    int snap_count = count_packages("snap list 2>/dev/null | tail -n +2 | wc -l");
    int brew_count = count_packages("brew list --formula 2>/dev/null | wc -l");
    
    // Add to breakdown and total
    total_packages += dpkg_count;
    add_package_breakdown(breakdown, sizeof(breakdown), &breakdown_count, ".deb", dpkg_count);
    
    total_packages += rpm_count;
    add_package_breakdown(breakdown, sizeof(breakdown), &breakdown_count, ".rpm", rpm_count);
    
    total_packages += pacman_count;
    add_package_breakdown(breakdown, sizeof(breakdown), &breakdown_count, "pacman", pacman_count);
    
    total_packages += emerge_count;
    add_package_breakdown(breakdown, sizeof(breakdown), &breakdown_count, "emerge", emerge_count);
    
    total_packages += xbps_count;
    add_package_breakdown(breakdown, sizeof(breakdown), &breakdown_count, "xbps", xbps_count);
    
    total_packages += apk_count;
    add_package_breakdown(breakdown, sizeof(breakdown), &breakdown_count, "apk", apk_count);
    
    total_packages += flatpak_count;
    add_package_breakdown(breakdown, sizeof(breakdown), &breakdown_count, "flatpak", flatpak_count);
    
    total_packages += snap_count;
    add_package_breakdown(breakdown, sizeof(breakdown), &breakdown_count, "snap", snap_count);
    
    total_packages += brew_count;
    add_package_breakdown(breakdown, sizeof(breakdown), &breakdown_count, "brew", brew_count);
    
    if (total_packages > 0) {
        if (breakdown_count > 0) {
            snprintf(info->packages, sizeof(info->packages), "%d  | %s", total_packages, breakdown);
        } else {
            snprintf(info->packages, sizeof(info->packages), "%d", total_packages);
        }
    } else {
        safe_strcpy(info->packages, "Unknown", sizeof(info->packages));
    }
    info->packages[sizeof(info->packages) - 1] = '\0';
}

void get_shell(system_info_t *info) {
    char *shell = getenv("SHELL");
    if (shell) {
        // Extract just the shell name from the path
        char *shell_name = strrchr(shell, '/');
        if (shell_name) {
            shell_name++; // Skip the '/'
            
            // Try to get shell version
            char *version_result = NULL;
            
            if (strcmp(shell_name, "bash") == 0) {
                version_result = execute_command("bash --version 2>/dev/null | head -1 | grep -oP 'version \\K[0-9.]+'");
            } else if (strcmp(shell_name, "zsh") == 0) {
                version_result = execute_command("zsh --version 2>/dev/null | grep -oP '[0-9.]+' | head -1");
            } else if (strcmp(shell_name, "fish") == 0) {
                version_result = execute_command("fish --version 2>/dev/null | grep -oP '[0-9.]+'");
            } else if (strcmp(shell_name, "dash") == 0 || strcmp(shell_name, "sh") == 0) {
                // dash/sh often don't have easy version flags, just use name
                safe_strcpy(info->shell, shell_name, sizeof(info->shell));
                info->shell[sizeof(info->shell) - 1] = '\0';
                return;
            }
            
            if (version_result && strnlen(version_result, sizeof(info->shell)) > 0) {
                snprintf(info->shell, sizeof(info->shell), "%s %s", shell_name, version_result);
            } else {
                safe_strcpy(info->shell, shell_name, sizeof(info->shell));
            }
        } else {
            safe_strcpy(info->shell, shell, sizeof(info->shell));
        }
        info->shell[sizeof(info->shell) - 1] = '\0';
    } else {
        safe_strcpy(info->shell, "Unknown", sizeof(info->shell));
    }
}

void get_resolution(system_info_t *info) {
    char *result;
    
    // Try xrandr first (for X11) - get all connected monitor resolutions
    result = execute_command("xrandr --current 2>/dev/null | grep ' connected' | grep -o '[0-9]\\+x[0-9]\\+' | tr '\\n' ', ' | sed 's/,$//' | sed 's/,/, /g'");
    if (result && strnlen(result, sizeof(info->resolution)) > 0) {
        safe_strcpy(info->resolution, result, sizeof(info->resolution));
        info->resolution[sizeof(info->resolution) - 1] = '\0';
        return;
    }
    
    // Try wlr-randr (for Wayland)
    result = execute_command("wlr-randr 2>/dev/null | grep -o '[0-9]\\+x[0-9]\\+' | head -1");
    if (result && strnlen(result, sizeof(info->resolution)) > 0) {
        safe_strcpy(info->resolution, result, sizeof(info->resolution));
        info->resolution[sizeof(info->resolution) - 1] = '\0';
        return;
    }
    
    // Check /sys/class/drm
    result = execute_command("find /sys/class/drm/*/modes -type f -exec cat {} \\; 2>/dev/null | head -1");
    if (result && strnlen(result, sizeof(info->resolution)) > 0) {
        safe_strcpy(info->resolution, result, sizeof(info->resolution));
        info->resolution[sizeof(info->resolution) - 1] = '\0';
        return;
    }
    
    safe_strcpy(info->resolution, "Unknown", sizeof(info->resolution));
    info->resolution[sizeof(info->resolution) - 1] = '\0';
}

// Helper function to detect desktop environment name
static void detect_de_name(char *de_with_display, size_t size) {
    char *de = NULL;
    
    if ((de = getenv("XDG_CURRENT_DESKTOP"))) {
        safe_strcpy(de_with_display, de, size - 1);
    } else if ((de = getenv("DESKTOP_SESSION"))) {
        safe_strcpy(de_with_display, de, size - 1);
    } else if ((de = getenv("XDG_SESSION_DESKTOP"))) {
        safe_strcpy(de_with_display, de, size - 1);
    } else if (getenv("KDE_FULL_SESSION")) {
        safe_strcpy(de_with_display, "KDE", size - 1);
    } else if (getenv("GNOME_DESKTOP_SESSION_ID")) {
        safe_strcpy(de_with_display, "GNOME", size - 1);
    } else {
        safe_strcpy(de_with_display, "Unknown", size - 1);
    }
    de_with_display[size - 1] = '\0';
}

// Helper function to get DE version with fallback
static void get_de_version_info(const char *version_cmd, const char *fallback, 
                                 char *version_info, size_t size) {
    char *version = execute_command(version_cmd);
    if (version && strnlen(version, size) > 0) {
        snprintf(version_info, size, "%s", version);
    } else {
        safe_strcpy(version_info, fallback, size - 1);
        version_info[size - 1] = '\0';
    }
}

// Helper to process DE version with consistent formatting
static void format_de_version(const char *version_cmd, const char *fallback,
                               char *version_info, size_t size, const char *display_prefix, size_t prefix_len) {
    char temp[64];
    get_de_version_info(version_cmd, fallback, temp, sizeof(temp));
    if (strstr(temp, ".")) {
        snprintf(version_info, size, "%.*s %.20s", (int)prefix_len, display_prefix, temp);
    } else {
        safe_strcpy(version_info, temp, size - 1);
    }
}

// Helper function to get version information for specific DEs
static void get_de_version(const char *de_with_display, char *version_info, size_t size) {
    if (strstr(de_with_display, "KDE") || strstr(de_with_display, "plasma")) {
        format_de_version("plasmashell --version 2>/dev/null | grep -o '[0-9]\\+\\.[0-9]\\+\\.[0-9]\\+'",
                         "KDE", version_info, size, "Plasma", 6);
    } else if (strstr(de_with_display, "GNOME")) {
        format_de_version("gnome-shell --version 2>/dev/null | grep -o '[0-9]\\+\\.[0-9]\\+'",
                         "GNOME", version_info, size, "GNOME", 5);
    } else if (strstr(de_with_display, "XFCE")) {
        format_de_version("xfce4-session --version 2>/dev/null | head -1 | grep -o '[0-9]\\+\\.[0-9]\\+'",
                         de_with_display, version_info, size, "Xfce", 4);
    } else if (strstr(de_with_display, "MATE")) {
        format_de_version("mate-session --version 2>/dev/null | head -1 | grep -o '[0-9]\\+\\.[0-9]\\+'",
                         de_with_display, version_info, size, "MATE", 4);
    } else if (strstr(de_with_display, "Cinnamon")) {
        format_de_version("cinnamon --version 2>/dev/null | grep -o '[0-9]\\+\\.[0-9]\\+'",
                         de_with_display, version_info, size, "Cinnamon", 8);
    } else {
        safe_strcpy(version_info, de_with_display, size - 1);
        version_info[size - 1] = '\0';
    }
}

// Helper function to format DE with display server
static void format_de_with_server(system_info_t *info, const char *version_info) {
    if (getenv("WAYLAND_DISPLAY")) {
        if (strnlen(version_info, sizeof(info->de)) > 0 && strcmp(version_info, "Unknown") != 0) {
            snprintf(info->de, sizeof(info->de), "%.50s (Wayland)", version_info);
        } else {
            safe_strcpy(info->de, "Wayland", sizeof(info->de));
        }
    } else if (getenv("DISPLAY")) {
        if (strnlen(version_info, sizeof(info->de)) > 0 && strcmp(version_info, "Unknown") != 0) {
            snprintf(info->de, sizeof(info->de), "%.50s (X11)", version_info);
        } else {
            safe_strcpy(info->de, "X11", sizeof(info->de));
        }
    } else {
        safe_strcpy(info->de, version_info, sizeof(info->de));
    }
    info->de[sizeof(info->de) - 1] = '\0';
}

void get_desktop_environment(system_info_t *info) {
    char de_with_display[128] = {0};
    char version_info[64] = {0};
    
    detect_de_name(de_with_display, sizeof(de_with_display));
    get_de_version(de_with_display, version_info, sizeof(version_info));
    format_de_with_server(info, version_info);
}

// Helper function to map display manager to canonical name
static const char* get_dm_name(const char *dm_str) {
    if (strstr(dm_str, "gdm")) return "GDM";
    if (strstr(dm_str, "sddm")) return "SDDM";
    if (strstr(dm_str, "lightdm")) return "LightDM";
    if (strstr(dm_str, "xdm")) return "XDM";
    if (strstr(dm_str, "kdm")) return "KDM";
    if (strstr(dm_str, "mdm")) return "MDM";
    if (strstr(dm_str, "lxdm")) return "LXDM";
    if (strstr(dm_str, "slim")) return "SLiM";
    return dm_str;
}

void get_display_manager(system_info_t *info) {
    char *dm_result = NULL;
    
    // Method 1: Check systemd for active display manager
    dm_result = execute_command("systemctl list-units --type=service --state=active | grep -E '(gdm|sddm|lightdm|xdm|kdm|mdm|lxdm|slim)' | head -1 | awk '{print $1}' | sed 's/\\.service$//'");
    if (dm_result && strnlen(dm_result, sizeof(info->dm)) > 0) {
        safe_strcpy(info->dm, get_dm_name(dm_result), sizeof(info->dm));
        info->dm[sizeof(info->dm) - 1] = '\0';
        return;
    }
    
    // Method 2: Check running processes
    dm_result = execute_command("ps -eo comm | grep -E '^(gdm|gdm3|sddm|lightdm|xdm|kdm|mdm|lxdm|slim)$' | head -1");
    if (dm_result && strnlen(dm_result, sizeof(info->dm)) > 0) {
        safe_strcpy(info->dm, get_dm_name(dm_result), sizeof(info->dm));
        info->dm[sizeof(info->dm) - 1] = '\0';
        return;
    }
    
    // Method 3: Check for display manager configuration files
    if (access("/etc/gdm/gdm.conf", F_OK) == 0 || access("/etc/gdm3/daemon.conf", F_OK) == 0) {
        safe_strcpy(info->dm, "GDM", sizeof(info->dm));
    } else if (access("/etc/sddm.conf", F_OK) == 0 || access("/etc/sddm/sddm.conf", F_OK) == 0) {
        safe_strcpy(info->dm, "SDDM", sizeof(info->dm));
    } else if (access("/etc/lightdm/lightdm.conf", F_OK) == 0) {
        safe_strcpy(info->dm, "LightDM", sizeof(info->dm));
    } else {
        safe_strcpy(info->dm, "Unknown", sizeof(info->dm));
    }
    
    info->dm[sizeof(info->dm) - 1] = '\0';
}

void get_window_manager(system_info_t *info) {
    char *result;
    
    // Try to get WM from various methods
    result = execute_command("wmctrl -m 2>/dev/null | grep 'Name:' | cut -d' ' -f2-");
    if (result && strnlen(result, sizeof(info->wm)) > 0) {
        safe_strcpy(info->wm, result, sizeof(info->wm));
        info->wm[sizeof(info->wm) - 1] = '\0';
        return;
    }
    
    // Check for common WMs
    if (getenv("SWAYSOCK")) {
        safe_strcpy(info->wm, "sway", sizeof(info->wm));
    } else if (execute_command("pgrep -x i3 >/dev/null 2>&1; echo $?") && atoi(execute_command("pgrep -x i3 >/dev/null 2>&1; echo $?")) == 0) {
        safe_strcpy(info->wm, "i3", sizeof(info->wm));
    } else if (execute_command("pgrep -x bspwm >/dev/null 2>&1; echo $?") && atoi(execute_command("pgrep -x bspwm >/dev/null 2>&1; echo $?")) == 0) {
        safe_strcpy(info->wm, "bspwm", sizeof(info->wm));
    } else {
        safe_strcpy(info->wm, "Unknown", sizeof(info->wm));
    }
    
    info->wm[sizeof(info->wm) - 1] = '\0';
}

void get_wm_theme(system_info_t *info) {
    // This is quite complex and varies by WM/DE
    safe_strcpy(info->wm_theme, "Unknown", sizeof(info->wm_theme));
    info->wm_theme[sizeof(info->wm_theme) - 1] = '\0';
}

void get_theme(system_info_t *info) {
    char *result;
    
    // Try gsettings for GTK theme
    result = execute_command("gsettings get org.gnome.desktop.interface gtk-theme 2>/dev/null | tr -d \"'\"");
    if (result && strnlen(result, sizeof(info->theme)) > 0 && strcmp(result, "''") != 0) {
        safe_strcpy(info->theme, result, sizeof(info->theme));
        info->theme[sizeof(info->theme) - 1] = '\0';
        return;
    }
    
    safe_strcpy(info->theme, "Unknown", sizeof(info->theme));
    info->theme[sizeof(info->theme) - 1] = '\0';
}

void get_icons(system_info_t *info) {
    char *result;
    
    // Try gsettings for icon theme
    result = execute_command("gsettings get org.gnome.desktop.interface icon-theme 2>/dev/null | tr -d \"'\"");
    if (result && strnlen(result, sizeof(info->icons)) > 0 && strcmp(result, "''") != 0) {
        safe_strcpy(info->icons, result, sizeof(info->icons));
        info->icons[sizeof(info->icons) - 1] = '\0';
        return;
    }
    
    safe_strcpy(info->icons, "Unknown", sizeof(info->icons));
    info->icons[sizeof(info->icons) - 1] = '\0';
}

void get_terminal(system_info_t *info) {
    char *term = getenv("TERM");
    char *term_program = getenv("TERM_PROGRAM");
    char *colorterm = getenv("COLORTERM");
    
    // Try to get more specific terminal information
    if (term_program) {
        safe_strcpy(info->terminal, term_program, sizeof(info->terminal));
    } else if (colorterm) {
        safe_strcpy(info->terminal, colorterm, sizeof(info->terminal));
    } else if (term) {
        // Remove common suffixes to get cleaner names
        if (strstr(term, "xterm-256color")) {
            safe_strcpy(info->terminal, "xterm", sizeof(info->terminal));
        } else if (strstr(term, "screen")) {
            safe_strcpy(info->terminal, "screen", sizeof(info->terminal));
        } else {
            safe_strcpy(info->terminal, term, sizeof(info->terminal));
        }
    } else {
        safe_strcpy(info->terminal, "Unknown", sizeof(info->terminal));
    }
    
    info->terminal[sizeof(info->terminal) - 1] = '\0';
}

void get_terminal_font(system_info_t *info) {
    // This varies greatly by terminal emulator
    safe_strcpy(info->term_font, "Unknown", sizeof(info->term_font));
    info->term_font[sizeof(info->term_font) - 1] = '\0';
}

void get_cpu(system_info_t *info) {
    FILE *file = fopen("/proc/cpuinfo", "r");
    if (!file) {
        safe_strcpy(info->cpu, "Unknown", sizeof(info->cpu));
        info->cpu[sizeof(info->cpu) - 1] = '\0';
        return;
    }
    
    char line[256];
    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "model name", 10) == 0) {
            char *colon = strchr(line, ':');
            if (colon) {
                char *cpu_name = trim_whitespace(colon + 1);
                safe_strcpy(info->cpu, cpu_name, sizeof(info->cpu));
                info->cpu[sizeof(info->cpu) - 1] = '\0';
                fclose(file);
                return;
            }
        }
    }
    
    fclose(file);
    safe_strcpy(info->cpu, "Unknown", sizeof(info->cpu));
    info->cpu[sizeof(info->cpu) - 1] = '\0';
}

void get_gpu(system_info_t *info) {
    char *result;
    
    // Try lspci
    result = execute_command("lspci | grep -i vga | head -1 | cut -d: -f3");
    if (result && strnlen(result, sizeof(info->gpu)) > 0) {
        safe_strcpy(info->gpu, trim_whitespace(result), sizeof(info->gpu));
        info->gpu[sizeof(info->gpu) - 1] = '\0';
        return;
    }
    
    // Try nvidia-smi
    result = execute_command("nvidia-smi --query-gpu=name --format=csv,noheader,nounits 2>/dev/null | head -1");
    if (result && strnlen(result, sizeof(info->gpu)) > 0) {
        safe_strcpy(info->gpu, result, sizeof(info->gpu));
        info->gpu[sizeof(info->gpu) - 1] = '\0';
        return;
    }
    
    safe_strcpy(info->gpu, "Unknown", sizeof(info->gpu));
    info->gpu[sizeof(info->gpu) - 1] = '\0';
}

// Helper function to build a colored progress bar
static void build_progress_bar(char *bar, size_t bar_size, int percentage, int bar_length) {
    int filled_blocks = (percentage * bar_length) / 100;
    int bar_pos = 0;
    
    bar_pos += snprintf(bar + bar_pos, bar_size - bar_pos, "\033[36m[");
    
    for (int i = 0; i < bar_length; i++) {
        if (bar_pos >= (int)(bar_size - 1)) {
            break;
        }
        
        const char *color;
        if (i < filled_blocks) {
            if (percentage < 60) {
                color = "\033[32m#";  // Green
            } else if (percentage < 80) {
                color = "\033[33m#";  // Yellow
            } else {
                color = "\033[31m#";  // Red
            }
        } else {
            color = "\033[90m-";  // Dark gray
        }
        bar_pos += snprintf(bar + bar_pos, bar_size - bar_pos, "%s", color);
    }
    
    snprintf(bar + bar_pos, bar_size - bar_pos, "\033[36m]\033[0m");
}

// Helper function to get CPU load string with progress bar
static void get_cpu_load_string(char *cpu_load_str, size_t str_size, int bar_length) {
    double loadavg[3];
    cpu_load_str[0] = '\0';
    
    if (getloadavg(loadavg, 1) == -1) {
        return;
    }
    
    long num_cores = sysconf(_SC_NPROCESSORS_ONLN);
    if (num_cores <= 0) {
        return;
    }
    
    int cpu_percentage = (int)((loadavg[0] / num_cores) * 100.0);
    if (cpu_percentage > 100) cpu_percentage = 100;
    
    char cpu_bar[96];
    build_progress_bar(cpu_bar, sizeof(cpu_bar), cpu_percentage, bar_length);
    snprintf(cpu_load_str, str_size, " | \033[36mCPU Load\033[0m: %s%d%%", cpu_bar, cpu_percentage);
}

void get_memory(system_info_t *info) {
    struct sysinfo s_info;
    
    if (sysinfo(&s_info) != 0) {
        safe_strcpy(info->memory, "Unknown", sizeof(info->memory));
        safe_strcpy(info->memory_bar, "Unknown", sizeof(info->memory_bar));
        info->memory[sizeof(info->memory) - 1] = '\0';
        info->memory_bar[sizeof(info->memory_bar) - 1] = '\0';
        return;
    }
    
    unsigned long total_mem = s_info.totalram * s_info.mem_unit;
    unsigned long free_mem = s_info.freeram * s_info.mem_unit;
    unsigned long used_mem = total_mem - free_mem;
    
    char used_str[32], total_str[32];
    format_memory(used_mem, used_str, sizeof(used_str));
    format_memory(total_mem, total_str, sizeof(total_str));
    
    int percentage = (int)((double)used_mem / (double)total_mem * 100.0);
    
    const int bar_length = 8;
    char progress_bar[128];
    build_progress_bar(progress_bar, sizeof(progress_bar), percentage, bar_length);
    
    char cpu_load_str[192] = "";
    get_cpu_load_string(cpu_load_str, sizeof(cpu_load_str), bar_length);
    
    snprintf(info->memory, sizeof(info->memory), "%.12s/%.12s %s%d%%%s", 
             used_str, total_str, progress_bar, percentage, cpu_load_str);
    info->memory_bar[0] = '\0';
    info->memory[sizeof(info->memory) - 1] = '\0';
    info->memory_bar[sizeof(info->memory_bar) - 1] = '\0';
}

void get_model(system_info_t *info) {
    char *result;
    
    // Try DMI information
    result = read_file_content("/sys/devices/virtual/dmi/id/product_name");
    if (result && strnlen(result, sizeof(info->model)) > 0 && strcmp(result, "To Be Filled By O.E.M.") != 0) {
        char *vendor = read_file_content("/sys/devices/virtual/dmi/id/sys_vendor");
        if (vendor && strnlen(vendor, 256) > 0) {
            snprintf(info->model, sizeof(info->model), "%s %s", vendor, result);
        } else {
            safe_strcpy(info->model, result, sizeof(info->model));
        }
        info->model[sizeof(info->model) - 1] = '\0';
        return;
    }
    
    safe_strcpy(info->model, "Unknown", sizeof(info->model));
    info->model[sizeof(info->model) - 1] = '\0';
}