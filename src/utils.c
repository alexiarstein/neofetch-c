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

// Safe string copy that guarantees null-termination
void safe_strcpy(char *dest, const char *src, size_t dest_size) {
    if (dest_size == 0) return;
    size_t src_len = strnlen(src, dest_size - 1);
    memcpy(dest, src, src_len);
    dest[src_len] = '\0';
}

char *trim_whitespace(char *str) {
    char *end;
    
    // Trim leading space
    while (*str == ' ' || *str == '\t' || *str == '\n' || *str == '\r') str++;
    
    if (*str == 0) // All spaces?
        return str;
    
    // Trim trailing space
    size_t len = strnlen(str, 1024);
    if (len == 0) return str;
    
    end = str + len - 1;
    while (end > str && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) end--;
    
    // Write new null terminator character
    end[1] = '\0';
    
    return str;
}

char *read_file_content(const char *filepath) {
    FILE *file = fopen(filepath, "r");
    if (!file) return NULL;
    
    static char buffer[1024];
    if (fgets(buffer, sizeof(buffer), file) != NULL) {
        fclose(file);
        return trim_whitespace(buffer);
    }
    
    fclose(file);
    return NULL;
}

char *execute_command(const char *command) {
    static char result[1024];
    FILE *fp = popen(command, "r");
    if (fp == NULL) return NULL;
    
    if (fgets(result, sizeof(result), fp) != NULL) {
        pclose(fp);
        return trim_whitespace(result);
    }
    
    pclose(fp);
    return NULL;
}

void detect_distro_name(char *distro_name, size_t size) {
    char *content;
    
    // Try different methods to detect distro name
    
    // Method 1: /etc/os-release
    content = read_file_content("/etc/os-release");
    if (!content) {
        goto try_lsb_release;
    }
    
    char *name_start = strstr(content, "NAME=");
    if (!name_start) {
        goto try_lsb_release;
    }
    
    name_start += 5; // Skip "NAME="
    if (*name_start == '"') name_start++; // Skip quote
    
    char *name_end = strchr(name_start, '"');
    if (!name_end) {
        name_end = strchr(name_start, '\n');
    }
    if (name_end) {
        *name_end = '\0';
    }
    
    safe_strcpy(distro_name, name_start, size - 1);
    distro_name[size - 1] = '\0';
    return;
    
try_lsb_release:
    
    // Method 2: lsb_release
    content = execute_command("lsb_release -si 2>/dev/null");
    if (content && strnlen(content, size) > 0) {
        safe_strcpy(distro_name, content, size - 1);
        distro_name[size - 1] = '\0';
        return;
    }
    
    // Method 3: Check specific files
    if (access("/etc/arch-release", F_OK) == 0) {
        safe_strcpy(distro_name, "Arch Linux", size - 1);
        return;
    }
    if (access("/etc/gentoo-release", F_OK) == 0) {
        safe_strcpy(distro_name, "Gentoo", size - 1);
        return;
    }
    if (access("/etc/fedora-release", F_OK) == 0) {
        safe_strcpy(distro_name, "Fedora", size - 1);
        return;
    }
    
    // Default
    safe_strcpy(distro_name, "Linux", size - 1);
    distro_name[size - 1] = '\0';
}

void format_memory(unsigned long bytes, char *output, size_t size) {
    double mb = bytes / 1024.0 / 1024.0;
    double gb = mb / 1024.0;
    
    if (gb >= 1.0) {
        snprintf(output, size, "%.1f GB", gb);
    } else {
        snprintf(output, size, "%.0f MB", mb);
    }
}

void format_uptime(long uptime_seconds, char *output, size_t size) {
    long days = uptime_seconds / (24 * 3600);
    long hours = (uptime_seconds % (24 * 3600)) / 3600;
    long minutes = (uptime_seconds % 3600) / 60;
    
    if (days > 0) {
        snprintf(output, size, "%ld days, %ld hours, %ld mins", days, hours, minutes);
    } else if (hours > 0) {
        snprintf(output, size, "%ld hours, %ld mins", hours, minutes);
    } else {
        snprintf(output, size, "%ld mins", minutes);
    }
}