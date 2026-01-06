#include "neofetch.h"
#include <ctype.h>
#include <dirent.h>
#include <sys/stat.h>

// ANSI color codes (removed static array - colors now defined in load_ascii_art)

// Structure for distro mapping
typedef struct {
    const char *distro_pattern;
    const char *ascii_file;
} distro_mapping_t;

// Distro mappings with patterns and their corresponding ASCII files
static const distro_mapping_t distro_mappings[] = {
    // Exact matches first (highest priority)
    {"aix", "aix.ascii"},
    {"almalinux", "almalinux.ascii"},
    {"alma", "almalinux.ascii"},
    {"alpine", "alpine.ascii"},
    {"amazon linux", "amazon.ascii"},
    {"amazon", "amazon.ascii"},
    {"anarchy", "anarchy.ascii"},
    {"android", "android.ascii"},
    {"antergos", "antergos.ascii"},
    {"antix", "antix.ascii"},
    {"aosc os", "aosc_os.ascii"},
    {"aosc", "aosc_os.ascii"},
    {"aperio gnu/linux", "aperio_gnu_linux.ascii"},
    {"aperio", "aperio_gnu_linux.ascii"},
    {"apricity", "apricity.ascii"},
    {"arch linux", "arch.ascii"},
    {"archcraft", "archcraft.ascii"},
    {"archlabs", "archlabs.ascii"},
    {"archmerge", "archmerge.ascii"},
    {"archstrike", "archstrike.ascii"},
    {"archbox", "archbox.ascii"},
    {"arcolinux", "arcolinux.ascii"},
    {"arco", "arcolinux.ascii"},
    {"blackarch", "blackarch.ascii"},
    {"artix", "artix.ascii"},
    {"arch", "arch.ascii"},
    {"arya", "arya.ascii"},
    {"asteroidos", "asteroidos.ascii"},
    {"bedrock", "bedrock.ascii"},
    {"bitrig", "bitrig.ascii"},
    {"blag", "blag.ascii"},
    {"blankon", "blankon.ascii"},
    {"bluelight", "bluelight.ascii"},
    {"bodhi", "bodhi.ascii"},
    {"bonsai", "bonsai.ascii"},
    {"bsd", "bsd.ascii"},
    {"bunsenlabs", "bunsenlabs.ascii"},
    {"calculate", "calculate.ascii"},
    {"carbs", "carbs.ascii"},
    {"cbl-mariner", "cbl-mariner.ascii"},
    {"mariner", "cbl-mariner.ascii"},
    {"celos", "celos.ascii"},
    {"centos", "centos.ascii"},
    {"chakra", "chakra.ascii"},
    {"chaletos", "chaletos.ascii"},
    {"chapeau", "chapeau.ascii"},
    {"chrome os", "chrom.ascii"},
    {"chromium os", "chrom.ascii"},
    {"chrome", "chrom.ascii"},
    {"chromium", "chrom.ascii"},
    {"cleanjaro", "cleanjaro.ascii"},
    {"clear linux", "clear_linux_os.ascii"},
    {"clearlinux", "clear_linux_os.ascii"},
    {"clearos", "clearos.ascii"},
    {"clover", "clover.ascii"},
    {"condres", "condres.ascii"},
    {"container linux", "container_linux_by_coreos.ascii"},
    {"coreos", "container_linux_by_coreos.ascii"},
    {"crux", "crux.ascii"},
    {"crystal linux", "crystal_linux.ascii"},
    {"crystal", "crystal_linux.ascii"},
    {"cucumber", "cucumber.ascii"},
    {"cyberos", "cyberos.ascii"},
    {"dahlia", "dahlia.ascii"},
    {"darkos", "darkos.ascii"},
    {"debian", "debian.ascii"},
    {"deepin", "deepin.ascii"},
    {"desaos", "desaos.ascii"},
    {"devuan", "devuan.ascii"},
    {"dracos", "dracos.ascii"},
    {"dragonfly bsd", "dragonfly.ascii"},
    {"dragonfly", "dragonfly.ascii"},
    {"drauger", "drauger.ascii"},
    {"elementary os", "elementary.ascii"},
    {"elementary", "elementary.ascii"},
    {"endeavouros", "endeavouros.ascii"},
    {"endeavour", "endeavouros.ascii"},
    {"endless", "endless.ascii"},
    {"eurolinux", "eurolinux.ascii"},
    {"exherbo", "exherbo.ascii"},
    {"fedora", "fedora.ascii"},
    {"feren", "feren.ascii"},
    {"freebsd", "freebsd_small.ascii"},
    {"freemint", "freemint.ascii"},
    {"frugalware", "frugalware.ascii"},
    {"funtoo", "funtoo.ascii"},
    {"galliumos", "galliumos.ascii"},
    {"garuda", "garuda.ascii"},
    {"gentoo", "gentoo.ascii"},
    {"gnewsense", "gnewsense.ascii"},
    {"gnu", "gnu.ascii"},
    {"gnome", "gnome.ascii"},
    {"gobolinux", "gobolinux.ascii"},
    {"goldendog", "goldendog.ascii"},
    {"golden dog", "goldendog.ascii"},
    {"grombyang", "grombyang.ascii"},
    {"guix", "guix.ascii"},
    {"haiku", "haiku.ascii"},
    {"hash", "hash.ascii"},
    {"huayra", "huayra.ascii"},
    {"hydroos", "hydroos.ascii"},
    {"hyperbola", "hyperbola.ascii"},
    {"iglunix", "iglunux.ascii"},
    {"instantos", "instantos.ascii"},
    {"irix", "irix.ascii"},
    {"itc", "itc.ascii"},
    {"januslinux", "januslinux.ascii"},
    {"janus", "januslinux.ascii"},
    {"kaisen", "kaisen.ascii"},
    {"kali", "kali.ascii"},
    {"kaos", "kaos.ascii"},
    {"kde neon", "kde.ascii"},
    {"kde", "kde.ascii"},
    {"kibojoe", "kibojoe.ascii"},
    {"kogaion", "kogaion.ascii"},
    {"korora", "korora.ascii"},
    {"kslinux", "kslinux.ascii"},
    {"kubuntu", "kubuntu.ascii"},
    {"langitketujuh", "langitketujuh.ascii"},
    {"laxeros", "laxeros.ascii"},
    {"lede", "lede.ascii"},
    {"libreelec", "libreelec.ascii"},
    {"linux lite", "linux_lite.ascii"},
    {"linux mint", "linux_mint.ascii"},
    {"live raizo", "live_raizo.ascii"},
    {"lmde", "lmde.ascii"},
    {"lubuntu", "lubuntu.ascii"},
    {"lunar", "lunar.ascii"},
    {"mageia", "mageia.ascii"},
    {"magpieos", "magpieos.ascii"},
    {"mandriva", "mandriva.ascii"},
    {"manjaro", "manjaro.ascii"},
    {"maui", "maui.ascii"},
    {"mer", "mer.ascii"},
    {"minix", "minix.ascii"},
    {"mx linux", "mx.ascii"},
    {"mx", "mx.ascii"},
    {"namib", "namib.ascii"},
    {"neptune", "neptune.ascii"},
    {"netbsd", "netbsd.ascii"},
    {"netrunner", "netrunner.ascii"},
    {"nitrux", "nitrux.ascii"},
    {"nixos", "nixos.ascii"},
    {"nix", "nixos.ascii"},
    {"nurunner", "nurunner.ascii"},
    {"nutyx", "nutyx.ascii"},
    {"obarun", "obarun.ascii"},
    {"obrevenge", "obrevenge.ascii"},
    {"openbsd", "openbsd.ascii"},
    {"openeuler", "openeuler.ascii"},
    {"openindiana", "openindiana.ascii"},
    {"openmamba", "openmamba.ascii"},
    {"openmandriva", "openmandriva.ascii"},
    {"opensuse leap", "opensuse_leap.ascii"},
    {"opensuse tumbleweed", "opensuse_tumbleweed.ascii"},
    {"opensuse", "opensuse.ascii"},
    {"suse", "opensuse.ascii"},
    {"openstage", "openstage.ascii"},
    {"openwrt", "openwrt.ascii"},
    {"oracle linux", "oracle.ascii"},
    {"oracle", "oracle.ascii"},
    {"os elbrus", "os_elbrus.ascii"},
    {"pacbsd", "pacbsd.ascii"},
    {"parabola", "parabola.ascii"},
    {"pardus", "pardus.ascii"},
    {"parrot", "parrot.ascii"},
    {"parsix", "parsix.ascii"},
    {"pcbsd", "pcbsd.ascii"},
    {"pclinuxos", "pclinuxos.ascii"},
    {"pengwin", "pengwin.ascii"},
    {"pentoo", "pentoo.ascii"},
    {"peppermint", "peppermint.ascii"},
    {"pisi", "pisi.ascii"},
    {"pnm linux", "pnm_linux.ascii"},
    {"pop!_os", "pop_os.ascii"},
    {"pop os", "pop_os.ascii"},
    {"popos", "pop_os.ascii"},
    {"porteus", "porteus.ascii"},
    {"postmarketos", "postmarketos.ascii"},
    {"proxmox", "proxmox.ascii"},
    {"puffos", "puffos.ascii"},
    {"puppy", "puppy.ascii"},
    {"pureos", "pureos.ascii"},
    {"qubyt", "qubyt.ascii"},
    {"qubes", "qubes.ascii"},
    {"quibian", "quibian.ascii"},
    {"radix", "radix.ascii"},
    {"raspbian", "raspbian.ascii"},
    {"reborn os", "reborn_os.ascii"},
    {"red star", "red_star.ascii"},
    {"redcore", "redcore.ascii"},
    {"red hat", "redhat.ascii"},
    {"redhat", "redhat.ascii"},
    {"rhel", "redhat.ascii"},
    {"refracted devuan", "refracted_devuan.ascii"},
    {"regata", "regata.ascii"},
    {"regolith", "regolith.ascii"},
    {"rocky linux", "rocky.ascii"},
    {"rocky", "rocky.ascii"},
    {"rosa", "rosa.ascii"},
    {"sabayon", "sabayon.ascii"},
    {"sabotage", "sabotage.ascii"},
    {"sailfish", "sailfish.ascii"},
    {"salentos", "salentos.ascii"},
    {"scientific", "scientific.ascii"},
    {"semc", "semc.ascii"},
    {"septor", "septor.ascii"},
    {"serene", "serene.ascii"},
    {"sharklinux", "sharklinux.ascii"},
    {"siduction", "siduction.ascii"},
    {"skiffos", "skiffos.ascii"},
    {"slackware", "slackware.ascii"},
    {"slitaz", "slitaz.ascii"},
    {"smartos", "smartos.ascii"},
    {"solus", "solus.ascii"},
    {"source mage", "source_mage.ascii"},
    {"sparky", "sparky.ascii"},
    {"star", "star.ascii"},
    {"steamos", "steamos.ascii"},
    {"swagarch", "swagarch.ascii"},
    {"t2", "t2.ascii"},
    {"tails", "tails.ascii"},
    {"tearch", "tearch.ascii"},
    {"trisquel", "trisquel.ascii"},
    {"ubuntu budgie", "ubuntu_budgie.ascii"},
    {"ubuntu cinnamon", "ubuntu_cinnamon.ascii"},
    {"ubuntu gnome", "ubuntu-gnome.ascii"},
    {"ubuntu mate", "ubuntu_mate.ascii"},
    {"ubuntu", "ubuntu.ascii"},
    {"univention", "univention.ascii"},
    {"venom", "venom.ascii"},
    {"void", "void.ascii"},
    {"vnux", "vnux.ascii"},
    {"windows", "windows.ascii"},
    {"xubuntu", "xubuntu.ascii"},
    {"xferience", "xferience.ascii"},
    {"zorin", "zorin.ascii"},
    {NULL, NULL} // End marker
};

// Fast string normalization for comparison
static void normalize_string(const char *input, char *output, size_t size) {
    size_t i = 0, j = 0;
    
    while (input[i] && j < size - 1) {
        if (isalnum(input[i])) {
            output[j++] = tolower(input[i]);
        } else if (input[i] == ' ' || input[i] == '_' || input[i] == '-') {
            if (j > 0 && output[j-1] != ' ') {
                output[j++] = ' ';
            }
        }
        i++;
    }
    
    // Remove trailing space
    if (j > 0 && output[j-1] == ' ') j--;
    output[j] = '\0';
}

// Check if file exists
static int file_exists(const char *filepath) {
    struct stat st;
    return (stat(filepath, &st) == 0 && S_ISREG(st.st_mode));
}

void map_distro_to_ascii_file(const char *distro, char *filename, size_t size) {
    char normalized_distro[256];
    char full_path[512];
    
    // Normalize the input distro name
    normalize_string(distro, normalized_distro, sizeof(normalized_distro));
    
    // First pass: exact matches
    for (int i = 0; distro_mappings[i].distro_pattern != NULL; i++) {
        if (strcmp(normalized_distro, distro_mappings[i].distro_pattern) == 0) {
            snprintf(full_path, sizeof(full_path), "ascii/%s", distro_mappings[i].ascii_file);
            if (file_exists(full_path)) {
                strncpy(filename, distro_mappings[i].ascii_file, size - 1);
                filename[size - 1] = '\0';
                return;
            }
        }
    }
    
    // Second pass: substring matches
    for (int i = 0; distro_mappings[i].distro_pattern != NULL; i++) {
        if (strstr(normalized_distro, distro_mappings[i].distro_pattern) != NULL) {
            snprintf(full_path, sizeof(full_path), "ascii/%s", distro_mappings[i].ascii_file);
            if (file_exists(full_path)) {
                strncpy(filename, distro_mappings[i].ascii_file, size - 1);
                filename[size - 1] = '\0';
                return;
            }
        }
    }
    
    // Fallback: try to construct filename directly from normalized distro name
    char fallback_name[256];
    strncpy(fallback_name, normalized_distro, sizeof(fallback_name) - 1);
    fallback_name[sizeof(fallback_name) - 1] = '\0';
    
    // Replace spaces with underscores for filename
    for (int i = 0; fallback_name[i]; i++) {
        if (fallback_name[i] == ' ') {
            fallback_name[i] = '_';
        }
    }
    
    snprintf(full_path, sizeof(full_path), "ascii/%s.ascii", fallback_name);
    if (file_exists(full_path)) {
        snprintf(filename, size, "%s.ascii", fallback_name);
        return;
    }
    
    // Final fallback: use a default or indicate unknown
    strncpy(filename, "arch.ascii", size - 1);
    filename[size - 1] = '\0';
}
    
    // (removed duplicate hardcoded if/else mapping - replaced by `distro_mappings` and
    // the lookup logic above which selects an ASCII filename based on normalized distro
    // names and available files under the `ascii/` directory)

// Calculate actual display width of a string (excluding ANSI escape sequences)
int calculate_display_width(const char *str) {
    int width = 0;
    int i = 0;
    while (str[i]) {
        if (str[i] == '\033') {
            // Skip ANSI escape sequence
            while (str[i] && str[i] != 'm') i++;
            if (str[i]) i++; // Skip the 'm'
        } else {
            width++;
            i++;
        }
    }
    return width;
}

int load_ascii_art(const char *distro_name, ascii_art_t *art) {
    char filename[64];
    char filepath[512];
    
    map_distro_to_ascii_file(distro_name, filename, sizeof(filename));
    
    // Try to construct the full path to the ASCII file
    snprintf(filepath, sizeof(filepath), "./ascii/%s", filename);
    
    FILE *file = fopen(filepath, "r");
    if (!file) {
        // Try alternative path
        snprintf(filepath, sizeof(filepath), "/usr/local/share/neofetch/ascii/%s", filename);
        file = fopen(filepath, "r");
    }
    
    if (!file) {
        // Use a simple default ASCII art
        art->line_count = 6;
        art->max_width = 20;
        strcpy(art->lines[0], "      /\\      ");
        strcpy(art->lines[1], "     /  \\     ");
        strcpy(art->lines[2], "    /____\\    ");
        strcpy(art->lines[3], "   /      \\   ");
        strcpy(art->lines[4], "  /        \\  ");
        strcpy(art->lines[5], " /__________\\ ");
        
        // Set default colors
        strcpy(art->colors[0], "\033[0m");   // reset
        strcpy(art->colors[1], "\033[34m");  // blue
        strcpy(art->colors[2], "\033[36m");  // cyan
        return 0;
    }
    
    art->line_count = 0;
    art->max_width = 0;
    char line[MAX_ASCII_WIDTH];
    
    // Set up color mappings - matching neofetch colors
    strcpy(art->colors[0], "\033[0m");   // reset
    strcpy(art->colors[1], "\033[31m");  // red
    strcpy(art->colors[2], "\033[32m");  // green  
    strcpy(art->colors[3], "\033[33m");  // yellow (for goldendog)
    strcpy(art->colors[4], "\033[34m");  // blue
    strcpy(art->colors[5], "\033[35m");  // magenta
    strcpy(art->colors[6], "\033[36m");  // cyan
    strcpy(art->colors[7], "\033[37m");  // white
    
    while (fgets(line, sizeof(line), file) && art->line_count < MAX_ASCII_LINES) {
        // Remove newline
        line[strcspn(line, "\n")] = '\0';
        
        // Skip comment lines that start with '#'
        if (line[0] == '#') {
            continue;
        }
        
        // Skip empty lines at the beginning
        if (art->line_count == 0 && strlen(line) == 0) {
            continue;
        }
        
        // Process color placeholders like ${c1}, ${c2}, etc.
        char processed_line[MAX_ASCII_WIDTH];
        char *src = line;
        char *dst = processed_line;
        
        while (*src && (dst - processed_line) < MAX_ASCII_WIDTH - 20) {
            if (strncmp(src, "${c", 3) == 0) {
                src += 3; // Skip "${c"
                if (*src >= '0' && *src <= '7') {
                    int color_num = *src - '0';
                    src++; // Skip digit
                    if (*src == '}') {
                        src++; // Skip '}'
                        // Add color code
                        if (color_num <= 7) {
                            strcpy(dst, art->colors[color_num]);
                            dst += strlen(art->colors[color_num]);
                        }
                        continue;
                    }
                }
                // If not a valid color code, copy as is
                *dst++ = '$';
                *dst++ = '{';
                *dst++ = 'c';
            } else {
                *dst++ = *src++;
            }
        }
        *dst = '\0';
        
        strcpy(art->lines[art->line_count], processed_line);
        
        // Calculate display width excluding ANSI escape sequences
        int actual_len = calculate_display_width(processed_line);
        
        if (actual_len > art->max_width) {
            art->max_width = actual_len;
        }
        
        art->line_count++;
    }
    
    fclose(file);
    return 1;
}

void print_info_with_ascii(const system_info_t *info, const ascii_art_t *art) {
    // Prepare info lines - remove empty entries to compact display
    const char *info_lines[] = {
        info->user,
        info->distro,
        info->kernel,
        info->uptime,
        info->packages,
        info->shell,
        info->resolution,
        info->de,
        info->dm,
        info->theme,
        info->icons,
        info->terminal,
        info->cpu,
        info->gpu,
        info->memory,  // Now includes the progress bar
        info->memory_bar // Add back as separate line since we have room
    };
    
    const char *info_labels[] = {
        "",                // user@hostname (special case)
        "OS",
        "Kernel",
        "Uptime",
        "Packages",
        "Shell",
        "Resolution",
        "DE",
        "DM",
        "Theme",
        "Icons",
        "Terminal",
        "CPU",
        "GPU",
        "Memory",
        "Memory Usage"
    };
    
    // Calculate proper spacing - use actual display width
    int ascii_display_width = 0;
    for (int i = 0; i < art->line_count; i++) {
        int line_width = calculate_display_width(art->lines[i]);
        if (line_width > ascii_display_width) {
            ascii_display_width = line_width;
        }
    }
    
    // Add padding for better alignment
    int padding = ascii_display_width + 4;
    
    // Build list of non-empty info lines to display compactly
    typedef struct {
        const char *label;
        const char *content;
    } info_entry_t;
    
    info_entry_t display_entries[20];
    int entry_count = 0;
    
    for (int i = 0; i < 16; i++) {
        if (i == 0) {
            // Special case for user@hostname - always include
            display_entries[entry_count].label = "";
            display_entries[entry_count].content = info->user; // Will be handled specially
            entry_count++;
        } else {
            int idx = i - 1;
            // Always include Memory Usage (memory_bar) even if empty
            if (strcmp(info_labels[idx], "Memory Usage") == 0) {
                display_entries[entry_count].label = info_labels[idx];
                display_entries[entry_count].content = info_lines[idx];
                entry_count++;
            } else if (strlen(info_lines[idx]) > 0 && strcmp(info_lines[idx], "Unknown") != 0) {
                display_entries[entry_count].label = info_labels[idx];
                display_entries[entry_count].content = info_lines[idx];
                entry_count++;
            }
        }
    }
    
    int max_lines = (art->line_count > entry_count) ? art->line_count : entry_count;
    
    for (int i = 0; i < max_lines; i++) {
        // Print ASCII art line
        if (i < art->line_count) {
            printf("%s", art->lines[i]);
            // Add padding to align info column
            int current_width = calculate_display_width(art->lines[i]);
            for (int j = current_width; j < padding; j++) {
                printf(" ");
            }
        } else {
            // Empty line with proper padding
            for (int j = 0; j < padding; j++) {
                printf(" ");
            }
        }
        
        // Print info line if we have one
        if (i < entry_count) {
            if (i == 0) {
                // Special case for user@hostname
                printf("\033[1m%s@%s\033[0m", info->user, info->hostname);
            } else if (i == 1) {
                // Print underline after user@hostname
                int title_len = strlen(info->user) + strlen(info->hostname) + 1;
                for (int j = 0; j < title_len; j++) {
                    printf("-");
                }
            } else {
                // Regular info line
                printf("\033[36m%s\033[0m: %s", display_entries[i].label, display_entries[i].content);
            }
        }
        printf("\n");
    }
    
    printf("\033[0m"); // Reset colors
}
