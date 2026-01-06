#include "neofetch.h"

int main(int argc __attribute__((unused)), char *argv[] __attribute__((unused))) {
    system_info_t info = {0};
    ascii_art_t art = {0};
    
    // Gather all system information
    get_user_hostname(&info);
    get_distro(&info);
    get_architecture(&info);
    get_hardware(&info);
    get_kernel(&info);
    get_uptime(&info);
    get_packages(&info);
    get_shell(&info);
    get_resolution(&info);
    get_desktop_environment(&info);
    get_display_manager(&info);
    get_window_manager(&info);
    get_wm_theme(&info);
    get_theme(&info);
    get_icons(&info);
    get_terminal(&info);
    get_terminal_font(&info);
    get_cpu(&info);
    get_gpu(&info);
    get_memory(&info);
    
    // Load ASCII art based on distro
    load_ascii_art(info.distro, &art);
    
    // Print the information with ASCII art
    print_info_with_ascii(&info, &art);
    
    return 0;
}