#ifndef NEOFETCH_H
#define NEOFETCH_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <sys/sysinfo.h>
#include <pwd.h>
#include <limits.h>
#include <dirent.h>
#include <sys/stat.h>
#include <ctype.h>

#define MAX_LINE_LENGTH 256
#define MAX_PATH_LENGTH 512
#define MAX_DISTRO_NAME 64
#define MAX_ASCII_LINES 50
#define MAX_ASCII_WIDTH 80

/* System information structure */
typedef struct {
    char user[64];
    char hostname[128];
    char distro[128];
    char architecture[64];
    char hardware[256];
    char kernel[128];
    char uptime[64];
    char packages[64];
    char shell[128];
    char resolution[64];
    char de[64];
    char dm[64];  // Display Manager
    char wm[64];
    char wm_theme[64];
    char theme[64];
    char icons[64];
    char terminal[128];
    char term_font[64];
    char cpu[256];
    char gpu[256];
    char memory[256];      // Increased to fit progress bar with ANSI colors
    char memory_bar[128];
    char model[128];
} system_info_t;

/* ASCII art structure */
typedef struct {
    char lines[MAX_ASCII_LINES][MAX_ASCII_WIDTH];
    int line_count;
    int max_width;
    char colors[8][16]; // ANSI color codes
} ascii_art_t;

/* Function declarations */

/* System information gathering functions */
void get_user_hostname(system_info_t *info);
void get_distro(system_info_t *info);
void get_architecture(system_info_t *info);
void get_hardware(system_info_t *info);
void get_kernel(system_info_t *info);
void get_uptime(system_info_t *info);
void get_packages(system_info_t *info);
void get_shell(system_info_t *info);
void get_resolution(system_info_t *info);
void get_desktop_environment(system_info_t *info);
void get_display_manager(system_info_t *info);
void get_window_manager(system_info_t *info);
void get_wm_theme(system_info_t *info);
void get_theme(system_info_t *info);
void get_icons(system_info_t *info);
void get_terminal(system_info_t *info);
void get_terminal_font(system_info_t *info);
void get_cpu(system_info_t *info);
void get_gpu(system_info_t *info);
void get_memory(system_info_t *info);
void get_model(system_info_t *info);

/* ASCII art functions */
int load_ascii_art(const char *distro_name, ascii_art_t *art);
void print_info_with_ascii(const system_info_t *info, const ascii_art_t *art);

/* Utility functions */
char *trim_whitespace(char *str);
char *read_file_content(const char *filepath);
char *execute_command(const char *command);
void detect_distro_name(char *distro_name, size_t size);
void format_memory(unsigned long bytes, char *output, size_t size);
void format_uptime(long uptime_seconds, char *output, size_t size);

#endif /* NEOFETCH_H */