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
#include <ctype.h>
#include <stdbool.h>

// Forward declarations for internal helper functions
static void build_progress_bar(char *bar, size_t bar_size, int percentage, int bar_length);
static void get_cpu_load_string(char *cpu_load_str, size_t str_size, int bar_length);

// Helper function to safely set string with null termination
static void safe_set_string(char *dest, const char *src, size_t dest_size) {
    safe_strcpy(dest, src, dest_size);
    dest[dest_size - 1] = '\0';
}

// Helper function to check environment variable and set if found
static bool try_env_and_set(const char *env_var, char *dest, size_t dest_size) {
    char *value = getenv(env_var);
    if (value && strnlen(value, dest_size) > 0) {
        safe_set_string(dest, value, dest_size);
        return true;
    }
    return false;
}

// Helper function to format window manager names consistently
static void format_wm_name(const char *raw_name, char *dest, size_t dest_size) {
    if (strcmp(raw_name, "kwin") == 0 || strcmp(raw_name, "kwin_x11") == 0) {
        safe_set_string(dest, "KWin", dest_size);
    } else if (strcmp(raw_name, "mutter") == 0) {
        safe_set_string(dest, "Mutter", dest_size);
    } else if (strcmp(raw_name, "xfwm4") == 0) {
        safe_set_string(dest, "Xfwm4", dest_size);
    } else if (strcmp(raw_name, "openbox") == 0) {
        safe_set_string(dest, "Openbox", dest_size);
    } else if (strcmp(raw_name, "i3") == 0) {
        safe_set_string(dest, "i3", dest_size);
    } else if (strcmp(raw_name, "bspwm") == 0) {
        safe_set_string(dest, "bspwm", dest_size);
    } else if (strcmp(raw_name, "awesome") == 0) {
        safe_set_string(dest, "Awesome", dest_size);
    } else {
        // Capitalize first letter for other WMs
        char formatted_name[64];
        safe_strcpy(formatted_name, raw_name, sizeof(formatted_name));
        formatted_name[0] = toupper(formatted_name[0]);
        safe_set_string(dest, formatted_name, dest_size);
    }
}

// Helper function to execute command and try to set result
static bool try_command_and_set(const char *command, char *dest, size_t dest_size) {
    char *result = execute_command(command);
    if (result && strnlen(result, dest_size) > 0) {
        safe_set_string(dest, result, dest_size);
        return true;
    }
    return false;
}

// Helper function to check if process is running
static bool is_process_running(const char *process_name) {
    char command[256];
    snprintf(command, sizeof(command), "pgrep -x %s >/dev/null 2>&1", process_name);
    char *result = execute_command(command);
    return (result && atoi(result) == 0);
}

// Helper function for setting WM based on process check
static bool try_wm_process_check(const char *process_name, const char *wm_name, char *dest, size_t dest_size) {
    if (is_process_running(process_name)) {
        safe_set_string(dest, wm_name, dest_size);
        return true;
    }
    return false;
}

// refactored this to make it thread-safe.
void get_user_hostname(system_info_t *info) {
    struct passwd pwd = {0};
    struct passwd *result = NULL;
    char buf[1024];
    
    if (getpwuid_r(getuid(), &pwd, buf, sizeof(buf), &result) == 0 && result != NULL) {
        safe_set_string(info->user, pwd.pw_name, sizeof(info->user));
    } else {
        safe_set_string(info->user, "unknown", sizeof(info->user));
    }
    
    if (gethostname(info->hostname, sizeof(info->hostname)) != 0) {
        safe_set_string(info->hostname, "unknown", sizeof(info->hostname));
    } else {
        info->hostname[sizeof(info->hostname) - 1] = '\0';
    }
}

void get_distro(system_info_t *info) {
    FILE *file;
    char line[256];
    char id[64] = {0};
    char version_id[64] = {0};
    char pretty_name[128] = {0};
    
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
                val = trim_whitespace(val);
                if (*val == '"') {
                    char *endq = strchr(val + 1, '"');
                    if (endq) *endq = '\0';
                    val++;
                } else {
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
            safe_set_string(info->distro, pretty_name, sizeof(info->distro));
        } else if (strnlen(id, sizeof(id)) > 0) {
            if (strnlen(version_id, sizeof(version_id)) > 0) {
                snprintf(info->distro, sizeof(info->distro), "%s %s", id, version_id);
                info->distro[sizeof(info->distro) - 1] = '\0';
            } else {
                safe_set_string(info->distro, id, sizeof(info->distro));
            }
        }
        return;
    }
    
    // NEW: Fallback if /etc/os-release is not present. We try to get it from lsb_release -- Alexia
    if (try_command_and_set("lsb_release -d 2>/dev/null | cut -f2", info->distro, sizeof(info->distro))) {
        return;
    }
    
    // If all else fails, we display it as "Linux" (not ideal but good enough for now)
    safe_set_string(info->distro, "Linux", sizeof(info->distro));
}

void get_architecture(system_info_t *info) {
    char *output = execute_command("uname -m");
    if (output && strnlen(output, sizeof(info->architecture)) > 0) {
        output[strcspn(output, "\n")] = '\0';
        
        // 'beautifying' arch outputs to something more human-friendly
        if (strcmp(output, "x86_64") == 0) {
            safe_set_string(info->architecture, "x86-64", sizeof(info->architecture));
        } else if (strcmp(output, "i386") == 0 || strcmp(output, "i686") == 0) {
            safe_set_string(info->architecture, "x86", sizeof(info->architecture));
        } else if (strcmp(output, "aarch64") == 0) {
            safe_set_string(info->architecture, "ARM64", sizeof(info->architecture));
        } else if (strncmp(output, "arm", 3) == 0) {
            safe_set_string(info->architecture, "ARM", sizeof(info->architecture));
        } else {
            safe_set_string(info->architecture, output, sizeof(info->architecture));
        }
    } else {
        safe_set_string(info->architecture, "Unknown", sizeof(info->architecture));
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

static void extract_value_after_colon(const char *line, char *dest, size_t dest_size) {
    const char *value_start = strstr(line, ":");
    if (value_start) {
        value_start++;
        while (*value_start == ' ') value_start++;
        safe_strcpy(dest, value_start, dest_size - 1);
        dest[dest_size - 1] = '\0';
    }
}

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
    
    // First we try DMI, otherwise we fallback to hostnamectl -- Alexia
    get_hardware_from_dmi(vendor, sizeof(vendor), model, sizeof(model));
    get_hardware_from_hostnamectl(vendor, sizeof(vendor), model, sizeof(model));
    
    // Storing results using the new function safe_strcpy to avoid involuntary overflows
    safe_set_string(info->hardware, vendor, sizeof(info->hardware));
    safe_set_string(info->model, model, sizeof(info->model));
}

void get_kernel(system_info_t *info) {
    struct utsname uts;
    if (uname(&uts) == 0) {
        // Truncate to fit in buffer
        snprintf(info->kernel, sizeof(info->kernel), "%.60s %.60s", uts.sysname, uts.release);
        info->kernel[sizeof(info->kernel) - 1] = '\0';
    } else {
        safe_set_string(info->kernel, "Unknown", sizeof(info->kernel));
    }
}

void get_uptime(system_info_t *info) {
    struct sysinfo s_info;
    if (sysinfo(&s_info) == 0) {
        format_uptime(s_info.uptime, info->uptime, sizeof(info->uptime));
    } else {
        safe_set_string(info->uptime, "Unknown", sizeof(info->uptime));
    }
}

static int count_packages(const char *command) {
    char *result = execute_command(command);
    return (result && atoi(result) > 0) ? atoi(result) : 0;
}

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
        char *shell_name = strrchr(shell, '/');
        if (shell_name) {
            shell_name++; // Skip the '/'
            
            char *version_result = NULL;
            
            if (strcmp(shell_name, "bash") == 0) {
                version_result = execute_command("bash --version 2>/dev/null | head -1 | grep -oP 'version \\K[0-9.]+'");
            } else if (strcmp(shell_name, "zsh") == 0) {
                version_result = execute_command("zsh --version 2>/dev/null | grep -oP '[0-9.]+' | head -1");
            } else if (strcmp(shell_name, "fish") == 0) {
                version_result = execute_command("fish --version 2>/dev/null | grep -oP '[0-9.]+'");
            } else if (strcmp(shell_name, "dash") == 0 || strcmp(shell_name, "sh") == 0) {
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
    
// NEW: We try to get all connected monitors and their resolutions. We try first with xrandr (X11) and
// then we try with wlr-rander for Wayland, and finally we check /sys/class/drm -- Alexia

    result = execute_command("xrandr --current 2>/dev/null | grep ' connected' | grep -o '[0-9]\\+x[0-9]\\+' | tr '\\n' ', ' | sed 's/,$//' | sed 's/,/, /g'");
    if (result && strnlen(result, sizeof(info->resolution)) > 0) {
        safe_strcpy(info->resolution, result, sizeof(info->resolution));
        info->resolution[sizeof(info->resolution) - 1] = '\0';
        return;
    }
    
    result = execute_command("wlr-randr 2>/dev/null | grep -o '[0-9]\\+x[0-9]\\+' | head -1");
    if (result && strnlen(result, sizeof(info->resolution)) > 0) {
        safe_strcpy(info->resolution, result, sizeof(info->resolution));
        info->resolution[sizeof(info->resolution) - 1] = '\0';
        return;
    }
    
    result = execute_command("find /sys/class/drm/*/modes -type f -exec cat {} \\; 2>/dev/null | head -1");
    if (result && strnlen(result, sizeof(info->resolution)) > 0) {
        safe_strcpy(info->resolution, result, sizeof(info->resolution));
        info->resolution[sizeof(info->resolution) - 1] = '\0';
        return;
    }
    
    safe_strcpy(info->resolution, "Unknown", sizeof(info->resolution));
    info->resolution[sizeof(info->resolution) - 1] = '\0';
}

static void detect_de_name(char *de_with_display, size_t size) {
    if (try_env_and_set("XDG_CURRENT_DESKTOP", de_with_display, size) ||
        try_env_and_set("DESKTOP_SESSION", de_with_display, size) ||
        try_env_and_set("XDG_SESSION_DESKTOP", de_with_display, size)) {
        return;
    } else if (getenv("KDE_FULL_SESSION")) {
        safe_set_string(de_with_display, "KDE", size);
    } else if (getenv("GNOME_DESKTOP_SESSION_ID")) {
        safe_set_string(de_with_display, "GNOME", size);
    } else {
        safe_set_string(de_with_display, "Unknown", size);
    }
}

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
                         "KDE", version_info, size, "KDE Plasma", 10); // NEW: Shows "KDE Plasma instead of just 'Plasma' -- Alexia"
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


static void format_de_with_session_and_wm(system_info_t *info, const char *version_info) {
    char session_type[16] = "Unknown";
    
    if (getenv("WAYLAND_DISPLAY")) {
        safe_strcpy(session_type, "Wayland", sizeof(session_type));
    } else if (getenv("DISPLAY")) {
        safe_strcpy(session_type, "X11", sizeof(session_type));
    }
    
    // Format the comprehensive DE line
    if (strnlen(version_info, sizeof(info->de)) > 0 && strcmp(version_info, "Unknown") != 0) {
        if (strnlen(info->wm, sizeof(info->wm)) > 0 && strcmp(info->wm, "Unknown") != 0) {
            snprintf(info->de, sizeof(info->de), "%.30s | \033[1;36mSession\033[0m: %s | \033[1;36mWM\033[0m: %s", 
                     version_info, session_type, info->wm);
        } else {
            snprintf(info->de, sizeof(info->de), "%.40s | \033[1;36mSession\033[0m: %s", 
                     version_info, session_type);
        }
    } else {
        if (strnlen(info->wm, sizeof(info->wm)) > 0 && strcmp(info->wm, "Unknown") != 0) {
            snprintf(info->de, sizeof(info->de), "\033[1;36mSession\033[0m: %s | \033[1;36mWM\033[0m: %s", 
                     session_type, info->wm);
        } else {
            snprintf(info->de, sizeof(info->de), "\033[1;36mSession\033[0m: %s", session_type);
        }
    }
    
    info->de[sizeof(info->de) - 1] = '\0';
}

void get_desktop_environment(system_info_t *info) {
    char de_with_display[128] = {0};
    char version_info[64] = {0};
    
    // First get window manager info (needed for the combined display)
    get_window_manager(info);
    
    detect_de_name(de_with_display, sizeof(de_with_display));
    get_de_version(de_with_display, version_info, sizeof(version_info));
    format_de_with_session_and_wm(info, version_info);
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
    
    // Method 1: Try wmctrl first (most reliable when available)
    result = execute_command("wmctrl -m 2>/dev/null | grep 'Name:' | cut -d' ' -f2-");
    if (result && strnlen(result, sizeof(info->wm)) > 0) {
        safe_strcpy(info->wm, result, sizeof(info->wm));
        info->wm[sizeof(info->wm) - 1] = '\0';
        return;
    }
    
    // Method 2: Check for Wayland compositors
    if (getenv("WAYLAND_DISPLAY")) {
        if (getenv("SWAYSOCK") || try_wm_process_check("sway", "Sway", info->wm, sizeof(info->wm))) {
            return;
        } else if (try_wm_process_check("kwin_wayland", "KWin", info->wm, sizeof(info->wm)) ||
                   try_wm_process_check("mutter", "Mutter", info->wm, sizeof(info->wm)) ||
                   try_wm_process_check("weston", "Weston", info->wm, sizeof(info->wm))) {
            return;
        } else {
            safe_set_string(info->wm, "Unknown", sizeof(info->wm));
        }
    } else {
        // Method 3: Check for X11 window managers via process detection
        result = execute_command("ps aux | grep -E '(kwin|mutter|xfwm|openbox|i3|bspwm|awesome|dwm|fluxbox|jwm|herbstluftwm|qtile|xmonad|spectrwm)' | grep -v grep | head -1 | awk '{print $11}' | xargs basename");
        if (result && strnlen(result, sizeof(info->wm)) > 0) {
            format_wm_name(result, info->wm, sizeof(info->wm));
            return;
        }
        
        // Method 4: Fall back to checking specific processes individually
        if (execute_command("pgrep -x i3 >/dev/null 2>&1; echo $?") && 
            atoi(execute_command("pgrep -x i3 >/dev/null 2>&1; echo $?")) == 0) {
            safe_strcpy(info->wm, "i3", sizeof(info->wm));
        } else if (execute_command("pgrep -x bspwm >/dev/null 2>&1; echo $?") && 
                   atoi(execute_command("pgrep -x bspwm >/dev/null 2>&1; echo $?")) == 0) {
            safe_strcpy(info->wm, "bspwm", sizeof(info->wm));
        } else {
            safe_strcpy(info->wm, "Unknown", sizeof(info->wm));
        }
    }
    
    info->wm[sizeof(info->wm) - 1] = '\0';
}

void get_wm_theme(system_info_t *info) {
    // This is quite complex and varies by WM/DE
    safe_set_string(info->wm_theme, "Unknown", sizeof(info->wm_theme));
}

void get_theme(system_info_t *info) {
    char *result;
    
    // Try gsettings for GTK theme
    result = execute_command("gsettings get org.gnome.desktop.interface gtk-theme 2>/dev/null | tr -d \"'\"");
    if (result && strnlen(result, sizeof(info->theme)) > 0 && strcmp(result, "''") != 0) {
        safe_set_string(info->theme, result, sizeof(info->theme));
        return;
    }
    
    safe_set_string(info->theme, "Unknown", sizeof(info->theme));
}

void get_icons(system_info_t *info) {
    char *result;
    
    // Try gsettings for icon theme
    result = execute_command("gsettings get org.gnome.desktop.interface icon-theme 2>/dev/null | tr -d \"'\"");
    if (result && strnlen(result, sizeof(info->icons)) > 0 && strcmp(result, "''") != 0) {
        safe_set_string(info->icons, result, sizeof(info->icons));
        return;
    }
    
    safe_set_string(info->icons, "Unknown", sizeof(info->icons));
}

void get_terminal(system_info_t *info) {
    char *term = getenv("TERM");
    char *term_program = getenv("TERM_PROGRAM");
    char *colorterm = getenv("COLORTERM");
    
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
    
    char cpu_bar[128];  // Increased size to accommodate ANSI codes
    build_progress_bar(cpu_bar, sizeof(cpu_bar), cpu_percentage, bar_length);
    snprintf(cpu_load_str, str_size, " | \033[1;36mCPU Load\033[0m: %s %d%%", cpu_bar, cpu_percentage);
}

void get_terminal_font(system_info_t *info) {
    // This varies greatly by terminal emulator
    safe_set_string(info->term_font, "Unknown", sizeof(info->term_font));
}

void get_cpu(system_info_t *info) {
    FILE *file = fopen("/proc/cpuinfo", "r");
    if (!file) {
        safe_set_string(info->cpu, "Unknown", sizeof(info->cpu));
        return;
    }
    
    char line[256];
    char cpu_name[256] = "Unknown";
    
    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "model name", 10) == 0) {
            char *colon = strchr(line, ':');
            if (colon) {
                char *name = trim_whitespace(colon + 1);
                safe_strcpy(cpu_name, name, sizeof(cpu_name));
                cpu_name[sizeof(cpu_name) - 1] = '\0';
                break;
            }
        }
    }
    
    fclose(file);
    
    char cpu_load_str[128] = "";  
    const int bar_length = 8;
    get_cpu_load_string(cpu_load_str, sizeof(cpu_load_str), bar_length);
    int cpu_name_len = strnlen(cpu_name, sizeof(info->cpu) - 60);  // Leave space for load info
    snprintf(info->cpu, sizeof(info->cpu), "%.*s%s", cpu_name_len, cpu_name, cpu_load_str);
    info->cpu[sizeof(info->cpu) - 1] = '\0';
}

void get_gpu(system_info_t *info) {
    char *result;
    
    // Try lspci
    result = execute_command("lspci | grep -i vga | head -1 | cut -d: -f3");
    if (result && strnlen(result, sizeof(info->gpu)) > 0) {
        safe_set_string(info->gpu, trim_whitespace(result), sizeof(info->gpu));
        return;
    }
    
    // Try nvidia-smi
    result = execute_command("nvidia-smi --query-gpu=name --format=csv,noheader,nounits 2>/dev/null | head -1");
    if (result && strnlen(result, sizeof(info->gpu)) > 0) {
        safe_set_string(info->gpu, result, sizeof(info->gpu));
        return;
    }
    
    safe_set_string(info->gpu, "Unknown", sizeof(info->gpu));
}

static void build_progress_bar(char *bar, size_t bar_size, int percentage, int bar_length) {
    int filled_blocks = (percentage * bar_length) / 100;
    int bar_pos = 0;
    
    // Opening bracket
    bar_pos += snprintf(bar + bar_pos, bar_size - bar_pos, "\033[1;36m[");
    
    // Progress blocks
    for (int i = 0; i < bar_length && bar_pos < (int)(bar_size - 20); i++) {  // Leave extra space for closing
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
    
    // Closing bracket with proper space check
    if (bar_pos < (int)(bar_size - 10)) {
        snprintf(bar + bar_pos, bar_size - bar_pos, "\033[1;36m]\033[0m");
    }
}

void get_memory(system_info_t *info) {
    struct sysinfo s_info;
    
    if (sysinfo(&s_info) != 0) {
        safe_set_string(info->memory, "Unknown", sizeof(info->memory));
        safe_set_string(info->memory_bar, "Unknown", sizeof(info->memory_bar));
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
    
    snprintf(info->memory, sizeof(info->memory), "%.12s/%.12s %s %d%%", 
             used_str, total_str, progress_bar, percentage);
    info->memory_bar[0] = '\0';
    info->memory[sizeof(info->memory) - 1] = '\0';
}

void get_model(system_info_t *info) {
    char *result;
    
    result = read_file_content("/sys/devices/virtual/dmi/id/product_name");
    if (result && strnlen(result, sizeof(info->model)) > 0 && strcmp(result, "To Be Filled By O.E.M.") != 0) {
        char *vendor = read_file_content("/sys/devices/virtual/dmi/id/sys_vendor");
        if (vendor && strnlen(vendor, 256) > 0) {
            snprintf(info->model, sizeof(info->model), "%s %s", vendor, result);
            info->model[sizeof(info->model) - 1] = '\0';
        } else {
            safe_set_string(info->model, result, sizeof(info->model));
        }
        return;
    }
    
    safe_set_string(info->model, "Unknown", sizeof(info->model));
}