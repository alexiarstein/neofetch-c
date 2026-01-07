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
    struct passwd pwd;
    struct passwd *result;
    char buf[1024];
    
    if (getpwuid_r(getuid(), &pwd, buf, sizeof(buf), &result) == 0 && result != NULL) {
        strncpy(info->user, pwd.pw_name, sizeof(info->user) - 1);
        info->user[sizeof(info->user) - 1] = '\0';
    } else {
        strncpy(info->user, "unknown", sizeof(info->user) - 1);
    }
    
    // Get hostname
    if (gethostname(info->hostname, sizeof(info->hostname)) != 0) {
        strncpy(info->hostname, "unknown", sizeof(info->hostname) - 1);
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
            if (strncmp(line, "PRETTY_NAME=", 12) == 0) {
                sscanf(line, "PRETTY_NAME=\"%127[^\"]\"", pretty_name);
            } else if (strncmp(line, "NAME=", 5) == 0) {
                sscanf(line, "NAME=\"%63[^\"]\"", id);
            } else if (strncmp(line, "VERSION_ID=", 11) == 0) {
                sscanf(line, "VERSION_ID=\"%63[^\"]\"", version_id);
            }
        }
        fclose(file);
        
        if (strlen(pretty_name) > 0) {
            strncpy(info->distro, pretty_name, sizeof(info->distro) - 1);
        } else if (strlen(id) > 0) {
            if (strlen(version_id) > 0) {
                snprintf(info->distro, sizeof(info->distro), "%s %s", id, version_id);
            } else {
                strncpy(info->distro, id, sizeof(info->distro) - 1);
            }
        }
        info->distro[sizeof(info->distro) - 1] = '\0';
        return;
    }
    
    // Fallback: try lsb_release
    const char *result = execute_command("lsb_release -d 2>/dev/null | cut -f2");
    if (result && strlen(result) > 0) {
        strncpy(info->distro, result, sizeof(info->distro) - 1);
        info->distro[sizeof(info->distro) - 1] = '\0';
        return;
    }
    
    // Last resort
    strncpy(info->distro, "Linux", sizeof(info->distro) - 1);
    info->distro[sizeof(info->distro) - 1] = '\0';
}

void get_architecture(system_info_t *info) {
    char *output = execute_command("uname -m");
    if (output && strlen(output) > 0) {
        // Remove newline and copy architecture
        output[strcspn(output, "\n")] = '\0';
        
        // Map common architectures to more descriptive names
        if (strcmp(output, "x86_64") == 0) {
            strncpy(info->architecture, "x86-64", sizeof(info->architecture) - 1);
        } else if (strcmp(output, "i386") == 0 || strcmp(output, "i686") == 0) {
            strncpy(info->architecture, "x86", sizeof(info->architecture) - 1);
        } else if (strcmp(output, "aarch64") == 0) {
            strncpy(info->architecture, "ARM64", sizeof(info->architecture) - 1);
        } else if (strncmp(output, "arm", 3) == 0) {
            strncpy(info->architecture, "ARM", sizeof(info->architecture) - 1);
        } else {
            strncpy(info->architecture, output, sizeof(info->architecture) - 1);
        }
        info->architecture[sizeof(info->architecture) - 1] = '\0';
    } else {
        strncpy(info->architecture, "Unknown", sizeof(info->architecture) - 1);
        info->architecture[sizeof(info->architecture) - 1] = '\0';
    }
}

// Helper function to check if string is valid (not empty, not placeholder)
static int is_valid_hardware_string(const char *str) {
    return str && strlen(str) > 0 && strcmp(str, "To be filled by O.E.M.") != 0;
}

// Helper function to safely copy hardware info
static void copy_hardware_info(char *dest, size_t dest_size, const char *src) {
    if (is_valid_hardware_string(src)) {
        strncpy(dest, src, dest_size - 1);
        dest[dest_size - 1] = '\0';
    }
}

// Helper function to extract value after colon
static void extract_value_after_colon(const char *line, char *dest, size_t dest_size) {
    const char *value_start = strstr(line, ":");
    if (value_start) {
        value_start++;
        while (*value_start == ' ') value_start++;
        strncpy(dest, value_start, dest_size - 1);
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
    if (!hostnamectl_output || strlen(hostnamectl_output) == 0) {
        return;
    }
    
    char *saveptr;
    char *line = strtok_r(hostnamectl_output, "\n", &saveptr);
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
    strncpy(info->hardware, vendor, sizeof(info->hardware) - 1);
    info->hardware[sizeof(info->hardware) - 1] = '\0';
    
    strncpy(info->model, model, sizeof(info->model) - 1);
    info->model[sizeof(info->model) - 1] = '\0';
}

void get_kernel(system_info_t *info) {
    struct utsname uts;
    if (uname(&uts) == 0) {
        // Truncate to fit in buffer
        snprintf(info->kernel, sizeof(info->kernel), "%.60s %.60s", uts.sysname, uts.release);
    } else {
        strncpy(info->kernel, "Unknown", sizeof(info->kernel) - 1);
    }
    info->kernel[sizeof(info->kernel) - 1] = '\0';
}

void get_uptime(system_info_t *info) {
    struct sysinfo s_info;
    if (sysinfo(&s_info) == 0) {
        format_uptime(s_info.uptime, info->uptime, sizeof(info->uptime));
    } else {
        strncpy(info->uptime, "Unknown", sizeof(info->uptime) - 1);
    }
    info->uptime[sizeof(info->uptime) - 1] = '\0';
}

void get_packages(system_info_t *info) {
    int total_packages = 0;
    char *result;
    char breakdown[384] = "";
    int breakdown_count = 0;
    
    // Check various package managers
    
    // dpkg (Debian/Ubuntu)
    result = execute_command("dpkg -l 2>/dev/null | grep '^ii' | wc -l");
    int dpkg_count = (result && atoi(result) > 0) ? atoi(result) : 0;
    if (dpkg_count > 0) {
        total_packages += dpkg_count;
        snprintf(breakdown + strlen(breakdown), sizeof(breakdown) - strlen(breakdown), 
                 "%s\033[36m.deb\033[0m: %d", breakdown_count > 0 ? " - " : "", dpkg_count);
        breakdown_count++;
    }
    
    // rpm (Red Hat/Fedora)
    result = execute_command("rpm -qa 2>/dev/null | wc -l");
    int rpm_count = (result && atoi(result) > 0) ? atoi(result) : 0;
    if (rpm_count > 0) {
        total_packages += rpm_count;
        snprintf(breakdown + strlen(breakdown), sizeof(breakdown) - strlen(breakdown), 
                 "%s\033[36m.rpm\033[0m: %d", breakdown_count > 0 ? " - " : "", rpm_count);
        breakdown_count++;
    }
    
    // pacman (Arch)
    result = execute_command("pacman -Q 2>/dev/null | wc -l");
    int pacman_count = (result && atoi(result) > 0) ? atoi(result) : 0;
    if (pacman_count > 0) {
        total_packages += pacman_count;
        snprintf(breakdown + strlen(breakdown), sizeof(breakdown) - strlen(breakdown), 
                 "%s\033[36mpacman\033[0m: %d", breakdown_count > 0 ? " - " : "", pacman_count);
        breakdown_count++;
    }
    
    // emerge (Gentoo)
    result = execute_command("ls -d /var/db/pkg/*/* 2>/dev/null | wc -l");
    int emerge_count = (result && atoi(result) > 0) ? atoi(result) : 0;
    if (emerge_count > 0) {
        total_packages += emerge_count;
        snprintf(breakdown + strlen(breakdown), sizeof(breakdown) - strlen(breakdown), 
                 "%s\033[36memerge\033[0m: %d", breakdown_count > 0 ? " - " : "", emerge_count);
        breakdown_count++;
    }
    
    // xbps (Void Linux)
    result = execute_command("xbps-query -l 2>/dev/null | wc -l");
    int xbps_count = (result && atoi(result) > 0) ? atoi(result) : 0;
    if (xbps_count > 0) {
        total_packages += xbps_count;
        snprintf(breakdown + strlen(breakdown), sizeof(breakdown) - strlen(breakdown), 
                 "%s\033[36mxbps\033[0m: %d", breakdown_count > 0 ? " - " : "", xbps_count);
        breakdown_count++;
    }
    
    // apk (Alpine)
    result = execute_command("apk list --installed 2>/dev/null | wc -l");
    int apk_count = (result && atoi(result) > 0) ? atoi(result) : 0;
    if (apk_count > 0) {
        total_packages += apk_count;
        snprintf(breakdown + strlen(breakdown), sizeof(breakdown) - strlen(breakdown), 
                 "%s\033[36mapk\033[0m: %d", breakdown_count > 0 ? " - " : "", apk_count);
        breakdown_count++;
    }
    
    // flatpak
    result = execute_command("flatpak list 2>/dev/null | wc -l");
    int flatpak_count = (result && atoi(result) > 0) ? atoi(result) : 0;
    if (flatpak_count > 0) {
        total_packages += flatpak_count;
        snprintf(breakdown + strlen(breakdown), sizeof(breakdown) - strlen(breakdown), 
                 "%s\033[36mflatpak\033[0m: %d", breakdown_count > 0 ? " - " : "", flatpak_count);
        breakdown_count++;
    }
    
    // snap
    result = execute_command("snap list 2>/dev/null | tail -n +2 | wc -l");
    int snap_count = (result && atoi(result) > 0) ? atoi(result) : 0;
    if (snap_count > 0) {
        total_packages += snap_count;
        snprintf(breakdown + strlen(breakdown), sizeof(breakdown) - strlen(breakdown), 
                 "%s\033[36msnap\033[0m: %d", breakdown_count > 0 ? " - " : "", snap_count);
        breakdown_count++;
    }
    
    // brew (Homebrew)
    result = execute_command("brew list --formula 2>/dev/null | wc -l");
    int brew_count = (result && atoi(result) > 0) ? atoi(result) : 0;
    if (brew_count > 0) {
        total_packages += brew_count;
        snprintf(breakdown + strlen(breakdown), sizeof(breakdown) - strlen(breakdown), 
                 "%s\033[36mbrew\033[0m: %d", breakdown_count > 0 ? " - " : "", brew_count);
        breakdown_count++;
    }
    
    if (total_packages > 0) {
        if (breakdown_count > 0) {
            snprintf(info->packages, sizeof(info->packages), "%d  | %s", total_packages, breakdown);
        } else {
            snprintf(info->packages, sizeof(info->packages), "%d", total_packages);
        }
    } else {
        strncpy(info->packages, "Unknown", sizeof(info->packages) - 1);
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
                strncpy(info->shell, shell_name, sizeof(info->shell) - 1);
                info->shell[sizeof(info->shell) - 1] = '\0';
                return;
            }
            
            if (version_result && strlen(version_result) > 0) {
                snprintf(info->shell, sizeof(info->shell), "%s %s", shell_name, version_result);
            } else {
                strncpy(info->shell, shell_name, sizeof(info->shell) - 1);
            }
        } else {
            strncpy(info->shell, shell, sizeof(info->shell) - 1);
        }
        info->shell[sizeof(info->shell) - 1] = '\0';
    } else {
        strncpy(info->shell, "Unknown", sizeof(info->shell) - 1);
    }
}

void get_resolution(system_info_t *info) {
    char *result;
    
    // Try xrandr first (for X11) - get all connected monitor resolutions
    result = execute_command("xrandr --current 2>/dev/null | grep ' connected' | grep -o '[0-9]\\+x[0-9]\\+' | tr '\\n' ', ' | sed 's/,$//' | sed 's/,/, /g'");
    if (result && strlen(result) > 0) {
        strncpy(info->resolution, result, sizeof(info->resolution) - 1);
        info->resolution[sizeof(info->resolution) - 1] = '\0';
        return;
    }
    
    // Try wlr-randr (for Wayland)
    result = execute_command("wlr-randr 2>/dev/null | grep -o '[0-9]\\+x[0-9]\\+' | head -1");
    if (result && strlen(result) > 0) {
        strncpy(info->resolution, result, sizeof(info->resolution) - 1);
        info->resolution[sizeof(info->resolution) - 1] = '\0';
        return;
    }
    
    // Check /sys/class/drm
    result = execute_command("find /sys/class/drm/*/modes -type f -exec cat {} \\; 2>/dev/null | head -1");
    if (result && strlen(result) > 0) {
        strncpy(info->resolution, result, sizeof(info->resolution) - 1);
        info->resolution[sizeof(info->resolution) - 1] = '\0';
        return;
    }
    
    strncpy(info->resolution, "Unknown", sizeof(info->resolution) - 1);
    info->resolution[sizeof(info->resolution) - 1] = '\0';
}

void get_desktop_environment(system_info_t *info) {
    char *de = NULL;
    char de_with_display[128] = {0};
    char version_info[64] = {0};
    
    // Check environment variables
    if ((de = getenv("XDG_CURRENT_DESKTOP"))) {
        strncpy(de_with_display, de, sizeof(de_with_display) - 1);
    } else if ((de = getenv("DESKTOP_SESSION"))) {
        strncpy(de_with_display, de, sizeof(de_with_display) - 1);
    } else if ((de = getenv("XDG_SESSION_DESKTOP"))) {
        strncpy(de_with_display, de, sizeof(de_with_display) - 1);
    } else if (getenv("KDE_FULL_SESSION")) {
        strncpy(de_with_display, "KDE", sizeof(de_with_display) - 1);
    } else if (getenv("GNOME_DESKTOP_SESSION_ID")) {
        strncpy(de_with_display, "GNOME", sizeof(de_with_display) - 1);
    } else {
        strncpy(de_with_display, "Unknown", sizeof(de_with_display) - 1);
    }
    
    // Get version information for specific DEs
    if (strstr(de_with_display, "KDE") || strstr(de_with_display, "plasma")) {
        char *plasma_version = execute_command("plasmashell --version 2>/dev/null | grep -o '[0-9]\\+\\.[0-9]\\+\\.[0-9]\\+'");
        if (plasma_version && strlen(plasma_version) > 0) {
            snprintf(version_info, sizeof(version_info), "Plasma %.20s", plasma_version);
        } else {
            strncpy(version_info, "KDE", sizeof(version_info) - 1);
        }
    } else if (strstr(de_with_display, "GNOME")) {
        char *gnome_version = execute_command("gnome-shell --version 2>/dev/null | grep -o '[0-9]\\+\\.[0-9]\\+'");
        if (gnome_version && strlen(gnome_version) > 0) {
            snprintf(version_info, sizeof(version_info), "GNOME %.20s", gnome_version);
        } else {
            strncpy(version_info, "GNOME", sizeof(version_info) - 1);
        }
    } else if (strstr(de_with_display, "XFCE")) {
        char *xfce_version = execute_command("xfce4-session --version 2>/dev/null | head -1 | grep -o '[0-9]\\+\\.[0-9]\\+'");
        if (xfce_version && strlen(xfce_version) > 0) {
            snprintf(version_info, sizeof(version_info), "Xfce %.20s", xfce_version);
        } else {
            strncpy(version_info, de_with_display, sizeof(version_info) - 1);
        }
    } else if (strstr(de_with_display, "MATE")) {
        char *mate_version = execute_command("mate-session --version 2>/dev/null | head -1 | grep -o '[0-9]\\+\\.[0-9]\\+'");
        if (mate_version && strlen(mate_version) > 0) {
            snprintf(version_info, sizeof(version_info), "MATE %.20s", mate_version);
        } else {
            strncpy(version_info, de_with_display, sizeof(version_info) - 1);
        }
    } else if (strstr(de_with_display, "Cinnamon")) {
        char *cinnamon_version = execute_command("cinnamon --version 2>/dev/null | grep -o '[0-9]\\+\\.[0-9]\\+'");
        if (cinnamon_version && strlen(cinnamon_version) > 0) {
            snprintf(version_info, sizeof(version_info), "Cinnamon %.15s", cinnamon_version);
        } else {
            strncpy(version_info, de_with_display, sizeof(version_info) - 1);
        }
    } else {
        strncpy(version_info, de_with_display, sizeof(version_info) - 1);
    }
    
    // Detect display server (X11/Wayland)
    if (getenv("WAYLAND_DISPLAY")) {
        if (strlen(version_info) > 0 && strcmp(version_info, "Unknown") != 0) {
            snprintf(info->de, sizeof(info->de), "%.50s (Wayland)", version_info);
        } else {
            strncpy(info->de, "Wayland", sizeof(info->de) - 1);
        }
    } else if (getenv("DISPLAY")) {
        if (strlen(version_info) > 0 && strcmp(version_info, "Unknown") != 0) {
            snprintf(info->de, sizeof(info->de), "%.50s (X11)", version_info);
        } else {
            strncpy(info->de, "X11", sizeof(info->de) - 1);
        }
    } else {
        strncpy(info->de, version_info, sizeof(info->de) - 1);
    }
    
    info->de[sizeof(info->de) - 1] = '\0';
}

void get_display_manager(system_info_t *info) {
    char *dm_result = NULL;
    
    // Method 1: Check systemd for active display manager
    dm_result = execute_command("systemctl list-units --type=service --state=active | grep -E '(gdm|sddm|lightdm|xdm|kdm|mdm|lxdm|slim)' | head -1 | awk '{print $1}' | sed 's/\\.service$//'");
    if (dm_result && strlen(dm_result) > 0) {
        // Capitalize and format the DM name
        if (strstr(dm_result, "gdm")) {
            strncpy(info->dm, "GDM", sizeof(info->dm) - 1);
        } else if (strstr(dm_result, "sddm")) {
            strncpy(info->dm, "SDDM", sizeof(info->dm) - 1);
        } else if (strstr(dm_result, "lightdm")) {
            strncpy(info->dm, "LightDM", sizeof(info->dm) - 1);
        } else if (strstr(dm_result, "xdm")) {
            strncpy(info->dm, "XDM", sizeof(info->dm) - 1);
        } else if (strstr(dm_result, "kdm")) {
            strncpy(info->dm, "KDM", sizeof(info->dm) - 1);
        } else if (strstr(dm_result, "mdm")) {
            strncpy(info->dm, "MDM", sizeof(info->dm) - 1);
        } else if (strstr(dm_result, "lxdm")) {
            strncpy(info->dm, "LXDM", sizeof(info->dm) - 1);
        } else if (strstr(dm_result, "slim")) {
            strncpy(info->dm, "SLiM", sizeof(info->dm) - 1);
        } else {
            strncpy(info->dm, dm_result, sizeof(info->dm) - 1);
        }
        info->dm[sizeof(info->dm) - 1] = '\0';
        return;
    }
    
    // Method 2: Check running processes
    dm_result = execute_command("ps -eo comm | grep -E '^(gdm|gdm3|sddm|lightdm|xdm|kdm|mdm|lxdm|slim)$' | head -1");
    if (dm_result && strlen(dm_result) > 0) {
        if (strstr(dm_result, "gdm")) {
            strncpy(info->dm, "GDM", sizeof(info->dm) - 1);
        } else if (strstr(dm_result, "sddm")) {
            strncpy(info->dm, "SDDM", sizeof(info->dm) - 1);
        } else if (strstr(dm_result, "lightdm")) {
            strncpy(info->dm, "LightDM", sizeof(info->dm) - 1);
        } else {
            strncpy(info->dm, dm_result, sizeof(info->dm) - 1);
        }
        info->dm[sizeof(info->dm) - 1] = '\0';
        return;
    }
    
    // Method 3: Check for display manager configuration files
    if (access("/etc/gdm/gdm.conf", F_OK) == 0 || access("/etc/gdm3/daemon.conf", F_OK) == 0) {
        strncpy(info->dm, "GDM", sizeof(info->dm) - 1);
    } else if (access("/etc/sddm.conf", F_OK) == 0 || access("/etc/sddm/sddm.conf", F_OK) == 0) {
        strncpy(info->dm, "SDDM", sizeof(info->dm) - 1);
    } else if (access("/etc/lightdm/lightdm.conf", F_OK) == 0) {
        strncpy(info->dm, "LightDM", sizeof(info->dm) - 1);
    } else {
        strncpy(info->dm, "Unknown", sizeof(info->dm) - 1);
    }
    
    info->dm[sizeof(info->dm) - 1] = '\0';
}

void get_window_manager(system_info_t *info) {
    char *result;
    
    // Try to get WM from various methods
    result = execute_command("wmctrl -m 2>/dev/null | grep 'Name:' | cut -d' ' -f2-");
    if (result && strlen(result) > 0) {
        strncpy(info->wm, result, sizeof(info->wm) - 1);
        info->wm[sizeof(info->wm) - 1] = '\0';
        return;
    }
    
    // Check for common WMs
    if (getenv("SWAYSOCK")) {
        strncpy(info->wm, "sway", sizeof(info->wm) - 1);
    } else if (execute_command("pgrep -x i3 >/dev/null 2>&1; echo $?") && atoi(execute_command("pgrep -x i3 >/dev/null 2>&1; echo $?")) == 0) {
        strncpy(info->wm, "i3", sizeof(info->wm) - 1);
    } else if (execute_command("pgrep -x bspwm >/dev/null 2>&1; echo $?") && atoi(execute_command("pgrep -x bspwm >/dev/null 2>&1; echo $?")) == 0) {
        strncpy(info->wm, "bspwm", sizeof(info->wm) - 1);
    } else {
        strncpy(info->wm, "Unknown", sizeof(info->wm) - 1);
    }
    
    info->wm[sizeof(info->wm) - 1] = '\0';
}

void get_wm_theme(system_info_t *info) {
    // This is quite complex and varies by WM/DE
    strncpy(info->wm_theme, "Unknown", sizeof(info->wm_theme) - 1);
    info->wm_theme[sizeof(info->wm_theme) - 1] = '\0';
}

void get_theme(system_info_t *info) {
    char *result;
    
    // Try gsettings for GTK theme
    result = execute_command("gsettings get org.gnome.desktop.interface gtk-theme 2>/dev/null | tr -d \"'\"");
    if (result && strlen(result) > 0 && strcmp(result, "''") != 0) {
        strncpy(info->theme, result, sizeof(info->theme) - 1);
        info->theme[sizeof(info->theme) - 1] = '\0';
        return;
    }
    
    strncpy(info->theme, "Unknown", sizeof(info->theme) - 1);
    info->theme[sizeof(info->theme) - 1] = '\0';
}

void get_icons(system_info_t *info) {
    char *result;
    
    // Try gsettings for icon theme
    result = execute_command("gsettings get org.gnome.desktop.interface icon-theme 2>/dev/null | tr -d \"'\"");
    if (result && strlen(result) > 0 && strcmp(result, "''") != 0) {
        strncpy(info->icons, result, sizeof(info->icons) - 1);
        info->icons[sizeof(info->icons) - 1] = '\0';
        return;
    }
    
    strncpy(info->icons, "Unknown", sizeof(info->icons) - 1);
    info->icons[sizeof(info->icons) - 1] = '\0';
}

void get_terminal(system_info_t *info) {
    char *term = getenv("TERM");
    char *term_program = getenv("TERM_PROGRAM");
    char *colorterm = getenv("COLORTERM");
    
    // Try to get more specific terminal information
    if (term_program) {
        strncpy(info->terminal, term_program, sizeof(info->terminal) - 1);
    } else if (colorterm) {
        strncpy(info->terminal, colorterm, sizeof(info->terminal) - 1);
    } else if (term) {
        // Remove common suffixes to get cleaner names
        if (strstr(term, "xterm-256color")) {
            strncpy(info->terminal, "xterm", sizeof(info->terminal) - 1);
        } else if (strstr(term, "screen")) {
            strncpy(info->terminal, "screen", sizeof(info->terminal) - 1);
        } else {
            strncpy(info->terminal, term, sizeof(info->terminal) - 1);
        }
    } else {
        strncpy(info->terminal, "Unknown", sizeof(info->terminal) - 1);
    }
    
    info->terminal[sizeof(info->terminal) - 1] = '\0';
}

void get_terminal_font(system_info_t *info) {
    // This varies greatly by terminal emulator
    strncpy(info->term_font, "Unknown", sizeof(info->term_font) - 1);
    info->term_font[sizeof(info->term_font) - 1] = '\0';
}

void get_cpu(system_info_t *info) {
    FILE *file = fopen("/proc/cpuinfo", "r");
    if (!file) {
        strncpy(info->cpu, "Unknown", sizeof(info->cpu) - 1);
        info->cpu[sizeof(info->cpu) - 1] = '\0';
        return;
    }
    
    char line[256];
    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "model name", 10) == 0) {
            char *colon = strchr(line, ':');
            if (colon) {
                char *cpu_name = trim_whitespace(colon + 1);
                strncpy(info->cpu, cpu_name, sizeof(info->cpu) - 1);
                info->cpu[sizeof(info->cpu) - 1] = '\0';
                fclose(file);
                return;
            }
        }
    }
    
    fclose(file);
    strncpy(info->cpu, "Unknown", sizeof(info->cpu) - 1);
    info->cpu[sizeof(info->cpu) - 1] = '\0';
}

void get_gpu(system_info_t *info) {
    char *result;
    
    // Try lspci
    result = execute_command("lspci | grep -i vga | head -1 | cut -d: -f3");
    if (result && strlen(result) > 0) {
        strncpy(info->gpu, trim_whitespace(result), sizeof(info->gpu) - 1);
        info->gpu[sizeof(info->gpu) - 1] = '\0';
        return;
    }
    
    // Try nvidia-smi
    result = execute_command("nvidia-smi --query-gpu=name --format=csv,noheader,nounits 2>/dev/null | head -1");
    if (result && strlen(result) > 0) {
        strncpy(info->gpu, result, sizeof(info->gpu) - 1);
        info->gpu[sizeof(info->gpu) - 1] = '\0';
        return;
    }
    
    strncpy(info->gpu, "Unknown", sizeof(info->gpu) - 1);
    info->gpu[sizeof(info->gpu) - 1] = '\0';
}

void get_memory(system_info_t *info) {
    struct sysinfo s_info;
    if (sysinfo(&s_info) == 0) {
        unsigned long total_mem = s_info.totalram * s_info.mem_unit;
        unsigned long free_mem = s_info.freeram * s_info.mem_unit;
        unsigned long used_mem = total_mem - free_mem;
        
        char used_str[32], total_str[32];
        format_memory(used_mem, used_str, sizeof(used_str));
        format_memory(total_mem, total_str, sizeof(total_str));
        
        // Calculate percentage
        int percentage = (int)((double)used_mem / (double)total_mem * 100.0);
        
        // Create visual progress bar
        const int bar_length = 8;   // Shorter bar to fit in memory field
        int filled_blocks = (percentage * bar_length) / 100;
        char progress_bar[128];  // Increased buffer size for ANSI codes (8 blocks * ~9 bytes each + overhead)
        
        // Build the progress bar with color coding using ASCII characters
        int bar_pos = 0;
        bar_pos += snprintf(progress_bar + bar_pos, sizeof(progress_bar) - bar_pos, "\033[36m[");
        
        for (int i = 0; i < bar_length && bar_pos < (int)(sizeof(progress_bar) - 1); i++) {
            if (i < filled_blocks) {
                if (percentage < 60) {
                    bar_pos += snprintf(progress_bar + bar_pos, sizeof(progress_bar) - bar_pos, "\033[32m#");
                } else if (percentage < 80) {
                    bar_pos += snprintf(progress_bar + bar_pos, sizeof(progress_bar) - bar_pos, "\033[33m#");
                } else {
                    bar_pos += snprintf(progress_bar + bar_pos, sizeof(progress_bar) - bar_pos, "\033[31m#");
                }
            } else {
                bar_pos += snprintf(progress_bar + bar_pos, sizeof(progress_bar) - bar_pos, "\033[90m-");
            }
        }
        
        snprintf(progress_bar + bar_pos, sizeof(progress_bar) - bar_pos, "\033[36m]\033[0m");
        
        // Get CPU load average (1 minute average)
        double loadavg[3];
        char cpu_load_str[192] = "";
        if (getloadavg(loadavg, 1) != -1) {
            // Get number of CPU cores
            long num_cores = sysconf(_SC_NPROCESSORS_ONLN);
            if (num_cores > 0) {
                // Calculate CPU load percentage based on 1-minute load average
                int cpu_percentage = (int)((loadavg[0] / num_cores) * 100.0);
                if (cpu_percentage > 100) cpu_percentage = 100; // Cap at 100%
                
                // Create CPU load bar (exact size: opening + 8 blocks + closing)
                int cpu_filled = (cpu_percentage * bar_length) / 100;
                char cpu_bar[96];  // Sufficient for progress bar with ANSI codes
                int cpu_bar_pos = 0;
                cpu_bar_pos += snprintf(cpu_bar + cpu_bar_pos, sizeof(cpu_bar) - cpu_bar_pos, "\033[36m[");
                
                for (int i = 0; i < bar_length && cpu_bar_pos < (int)(sizeof(cpu_bar) - 1); i++) {
                    if (i < cpu_filled) {
                        if (cpu_percentage < 60) {
                            cpu_bar_pos += snprintf(cpu_bar + cpu_bar_pos, sizeof(cpu_bar) - cpu_bar_pos, "\033[32m#");
                        } else if (cpu_percentage < 80) {
                            cpu_bar_pos += snprintf(cpu_bar + cpu_bar_pos, sizeof(cpu_bar) - cpu_bar_pos, "\033[33m#");
                        } else {
                            cpu_bar_pos += snprintf(cpu_bar + cpu_bar_pos, sizeof(cpu_bar) - cpu_bar_pos, "\033[31m#");
                        }
                    } else {
                        cpu_bar_pos += snprintf(cpu_bar + cpu_bar_pos, sizeof(cpu_bar) - cpu_bar_pos, "\033[90m-");
                    }
                }
                
                snprintf(cpu_bar + cpu_bar_pos, sizeof(cpu_bar) - cpu_bar_pos, "\033[36m]\033[0m");
                snprintf(cpu_load_str, sizeof(cpu_load_str), " | \033[36mCPU Load\033[0m: %s%d%%", cpu_bar, cpu_percentage);
            }
        }
        
        snprintf(info->memory, sizeof(info->memory), "%.12s/%.12s %s%d%%%s", used_str, total_str, progress_bar, percentage, cpu_load_str);
        info->memory_bar[0] = '\0'; // Clear since we combined it
    } else {
        strncpy(info->memory, "Unknown", sizeof(info->memory) - 1);
        strncpy(info->memory_bar, "Unknown", sizeof(info->memory_bar) - 1);
    }
    info->memory[sizeof(info->memory) - 1] = '\0';
    info->memory_bar[sizeof(info->memory_bar) - 1] = '\0';
}

void get_model(system_info_t *info) {
    char *result;
    
    // Try DMI information
    result = read_file_content("/sys/devices/virtual/dmi/id/product_name");
    if (result && strlen(result) > 0 && strcmp(result, "To Be Filled By O.E.M.") != 0) {
        char *vendor = read_file_content("/sys/devices/virtual/dmi/id/sys_vendor");
        if (vendor && strlen(vendor) > 0) {
            snprintf(info->model, sizeof(info->model), "%s %s", vendor, result);
        } else {
            strncpy(info->model, result, sizeof(info->model) - 1);
        }
        info->model[sizeof(info->model) - 1] = '\0';
        return;
    }
    
    strncpy(info->model, "Unknown", sizeof(info->model) - 1);
    info->model[sizeof(info->model) - 1] = '\0';
}