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
#include <dirent.h>
#include <sys/stat.h>

// NOTE: MacOS Support is not yet implemented. It will, though. I'm working on it. 
// It sort of works modifying sysinfo.c but won't be able to properly detect aqua, quartz compositor, etc. 
// but I'll get it working soonish. -- Alexia


/* 
 Another note (this one for contributors)
 The simplest way to add support for a new distro's ASCII art is to add an entry
 to the `distro_mappings` array below, mapping a normalized distro name pattern
 to the corresponding ASCII art filename located in the `ascii/` directory.

 And please, please, please... Follow the alphabetic order. Don't put a distro starting with the letter Z next to GoldenDog, (heh)
 Also note that most of the ascii art has been salvaged from the original neofetch script by Dylan Araps (now abandoned)
 But things change, distros change, some logos change.
 So if your distro is not here or the logo needs some love, please fix and send a pull request or open an issue. 
 Some do need recoloring. 
 I will try to update some as time allows it. But my main focus is Debian and GoldenDog for obvious reasons.
*/

typedef struct {
    const char *distro_pattern;
    const char *ascii_file;
} distro_mapping_t;
static const distro_mapping_t distro_mappings[] = {
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
    {NULL, NULL} 
};

static void normalize_string(const char *input, char *output, size_t size) {
    size_t i = 0, j = 0;
    
    while (input[i] && j < size - 1) {
        if (isalnum(input[i])) {
            output[j++] = tolower(input[i]);
        } else if ((input[i] == ' ' || input[i] == '_' || input[i] == '-') && j > 0 && output[j-1] != ' ') {
            output[j++] = ' ';
        }
        i++;
    }
    
    if (j > 0 && output[j-1] == ' ') j--;
    output[j] = '\0';
}

static int file_exists_in_paths(const char *filename) {
    char filepath[512];
    struct stat st;
    //NEW: Paths. Installed neofetch-c will deploy ascii/ in /usr/share/neofetch/ascii
    // if we cant find it there, we check local,
    // otherwise we fall back to the relative path, for debugging, development, etc
    // (to avoid sudo make install during testing, mostly.) -- Alexia

    const char *search_dirs[] = {
        "/usr/share/neofetch/ascii",
        "/usr/local/share/neofetch/ascii",
        "./ascii",
        NULL
    };
    
    for (int i = 0; search_dirs[i] != NULL; i++) {
        snprintf(filepath, sizeof(filepath), "%s/%s", search_dirs[i], filename);
        if (stat(filepath, &st) == 0 && S_ISREG(st.st_mode)) {
            return 1;
        }
    }
    return 0;
}

void map_distro_to_ascii_file(const char *distro, char *filename, size_t size) {
    char normalized_distro[256];
    
    normalize_string(distro, normalized_distro, sizeof(normalized_distro));
    
    for (int i = 0; distro_mappings[i].distro_pattern != NULL; i++) {
        if (strcmp(normalized_distro, distro_mappings[i].distro_pattern) == 0 &&
            file_exists_in_paths(distro_mappings[i].ascii_file)) {
            safe_strcpy(filename, distro_mappings[i].ascii_file, size - 1);
            filename[size - 1] = '\0';
            return;
        }
    }
    
    // Second pass: substring matches
    for (int i = 0; distro_mappings[i].distro_pattern != NULL; i++) {
        if (strstr(normalized_distro, distro_mappings[i].distro_pattern) != NULL &&
            file_exists_in_paths(distro_mappings[i].ascii_file)) {
            safe_strcpy(filename, distro_mappings[i].ascii_file, size - 1);
            filename[size - 1] = '\0';
            return;
        }
    }
    
    char fallback_name[249];  // Reduced to leave room for ".ascii" suffix (249 + 6 + 1 = 256)
    safe_strcpy(fallback_name, normalized_distro, sizeof(fallback_name));
    fallback_name[sizeof(fallback_name) - 1] = '\0';
    
    // Replace spaces with underscores for filename
    for (int i = 0; fallback_name[i]; i++) {
        if (fallback_name[i] == ' ') {
            fallback_name[i] = '_';
        }
    }
    
    char fallback_file[256];
    snprintf(fallback_file, sizeof(fallback_file), "%s.ascii", fallback_name);
    if (file_exists_in_paths(fallback_file)) {
        safe_strcpy(filename, fallback_file, size - 1);
        filename[size - 1] = '\0';
        return;
    }
    
    // Default to linux.ascii if no match found
    safe_strcpy(filename, "linux.ascii", size - 1);
    filename[size - 1] = '\0';
}
    
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

// Helper function to process color placeholders in ASCII art
static void process_color_placeholder(const char **src, char **dst, const ascii_art_t *art) {
    *src += 3; // Skip "${c"
    
    if (**src < '0' || **src > '7') {
        // Invalid color code, copy as is
        *(*dst)++ = '$';
        *(*dst)++ = '{';
        *(*dst)++ = 'c';
        return;
    }
    
    int color_num = **src - '0';
    (*src)++; // Skip digit
    
    if (**src != '}') {
        // Invalid format, copy as is
        *(*dst)++ = '$';
        *(*dst)++ = '{';
        *(*dst)++ = 'c';
        return;
    }
    
    (*src)++; // Skip '}'
    
    // Add color code
    if (color_num <= 7) {
        size_t color_len = strnlen(art->colors[color_num], sizeof(art->colors[color_num]));
        memcpy(*dst, art->colors[color_num], color_len);
        *dst += color_len;
    }
}

// Helper function to process line with color codes
static void process_line_colors(const char *line, char *processed_line, const ascii_art_t *art) {
    const char *src = line;
    char *dst = processed_line;

    // Allow expansion for ANSI color codes; keep one byte for final NUL
    while (*src && (dst - processed_line) < (MAX_ASCII_WIDTH - 1)) {
        if (strncmp(src, "${c", 3) == 0) {
            process_color_placeholder(&src, &dst, art);
        } else {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
}

// Helper function to setup default ASCII art
static void setup_default_ascii(ascii_art_t *art) {
    art->line_count = 6;
    art->max_width = 20;
    snprintf(art->lines[0], sizeof(art->lines[0]), "      /\\      ");
    snprintf(art->lines[1], sizeof(art->lines[1]), "     /  \\     ");
    snprintf(art->lines[2], sizeof(art->lines[2]), "    /____\\    ");
    snprintf(art->lines[3], sizeof(art->lines[3]), "   /      \\   ");
    snprintf(art->lines[4], sizeof(art->lines[4]), "  /        \\  ");
    snprintf(art->lines[5], sizeof(art->lines[5]), " /__________\\ ");
    
    snprintf(art->colors[0], sizeof(art->colors[0]), "\033[0m");   // reset
    snprintf(art->colors[1], sizeof(art->colors[1]), "\033[34m");  // blue
    snprintf(art->colors[2], sizeof(art->colors[2]), "\033[36m");  // cyan
}

// Helper function to setup color mappings
static void setup_color_mappings(ascii_art_t *art) {
    snprintf(art->colors[0], sizeof(art->colors[0]), "\033[0m");   // reset
    snprintf(art->colors[1], sizeof(art->colors[1]), "\033[31m");  // red
    snprintf(art->colors[2], sizeof(art->colors[2]), "\033[32m");  // green  
    snprintf(art->colors[3], sizeof(art->colors[3]), "\033[33m");  // yellow
    snprintf(art->colors[4], sizeof(art->colors[4]), "\033[34m");  // blue
    snprintf(art->colors[5], sizeof(art->colors[5]), "\033[35m");  // magenta
    snprintf(art->colors[6], sizeof(art->colors[6]), "\033[36m");  // cyan
    snprintf(art->colors[7], sizeof(art->colors[7]), "\033[37m");  // white
}

// Helper function to process ASCII file line
static int should_skip_line(const char *line, int line_count) {
    // Skip comment lines
    if (line[0] == '#') {
        return 1;
    }
    
    // Skip empty lines at the beginning
    if (line_count == 0 && strnlen(line, MAX_ASCII_WIDTH) == 0) {
        return 1;
    }
    
    return 0;
}

int load_ascii_art(const char *distro_name, ascii_art_t *art) {
    char filename[64];
    char filepath[512];
    
    map_distro_to_ascii_file(distro_name, filename, sizeof(filename));
    
    FILE *file = NULL;
    const char *search_dirs[] = {
        "/usr/share/neofetch/ascii",
        "/usr/local/share/neofetch/ascii",
        "./ascii",
        NULL
    };
    
    // Try each path in order
    for (int i = 0; search_dirs[i] != NULL; i++) {
        snprintf(filepath, sizeof(filepath), "%s/%s", search_dirs[i], filename);
        file = fopen(filepath, "r");
        if (file) break;
    }
    
    if (!file) {
        setup_default_ascii(art);
        return 0;
    }
    
    art->line_count = 0;
    art->max_width = 0;
    char line[MAX_ASCII_WIDTH];
    
    setup_color_mappings(art);
    
    while (fgets(line, sizeof(line), file) && art->line_count < MAX_ASCII_LINES) {
        // Remove newline safely
        size_t len = strcspn(line, "\n");
        if (len < sizeof(line)) {
            line[len] = '\0';
        }
        
        if (should_skip_line(line, art->line_count)) {
            continue;
        }
        
        // Process color placeholders
        char processed_line[MAX_ASCII_WIDTH];
        process_line_colors(line, processed_line, art);
        
        snprintf(art->lines[art->line_count], sizeof(art->lines[art->line_count]), "%s", processed_line);
        
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

// Helper structure for display entries
typedef struct {
    const char *label;
    const char *content;
} info_entry_t;

// Helper function to check if info line should be displayed
static int should_display_info(const char *content) {
    return strnlen(content, 512) > 0 && strcmp(content, "Unknown") != 0;
}

// Helper function to build display entries
static int build_display_entries(const system_info_t *info, info_entry_t *entries, int max_entries) {
    const char *info_lines[] = {
        info->user, info->distro, info->architecture, info->hardware,
        info->model, info->kernel, info->uptime, info->packages,
        info->shell, info->resolution, info->de, info->dm,
        info->theme, info->icons, info->terminal, info->gpu,
        info->cpu, info->memory
    };
    
    const char *info_labels[] = {
        "", "OS", "Architecture", "Host", "Model", "Kernel",
        "Uptime", "Packages", "Shell", "Resolution", "DE", "Display Manager",
        "Theme", "Icons", "Terminal", "GPU", "CPU", "Memory"
    };
    
    int entry_count = 0;
    int num_info_lines = sizeof(info_lines) / sizeof(info_lines[0]);
    
    // First entry is always user@hostname
    entries[entry_count].label = "";
    entries[entry_count].content = info->user;
    entry_count++;
    
    // Add other entries if they have valid content
    for (int i = 1; i < num_info_lines && entry_count < max_entries; i++) {
        if (should_display_info(info_lines[i])) {
            entries[entry_count].label = info_labels[i];
            entries[entry_count].content = info_lines[i];
            entry_count++;
        }
    }
    
    return entry_count;
}

// Helper function to print ASCII art line with padding
static void print_ascii_line(const ascii_art_t *art, int line_idx, int padding) {
    if (line_idx < art->line_count) {
        printf("%s", art->lines[line_idx]);
        int current_width = calculate_display_width(art->lines[line_idx]);
        for (int j = current_width; j < padding; j++) {
            printf(" ");
        }
    } else {
        for (int j = 0; j < padding; j++) {
            printf(" ");
        }
    }
}

// Helper function to print info line
static void print_info_line(const system_info_t *info, const info_entry_t *entries, int line_idx) {
    if (line_idx == 0) {
        printf("\033[1m%s@%s\033[0m", info->user, info->hostname);
    } else if (line_idx == 1) {
        int title_len = strnlen(info->user, sizeof(info->user)) + strnlen(info->hostname, sizeof(info->hostname)) + 1;
        for (int j = 0; j < title_len; j++) {
            printf("-");
        }
    } else {
        int entry_idx = line_idx - 1; // shift because line 1 is the underline
        printf("\033[1;36m%s\033[0m: %s", entries[entry_idx].label, entries[entry_idx].content);
    }
}

void print_info_with_ascii(const system_info_t *info, const ascii_art_t *art) {
    // Calculate ASCII display width
    int ascii_display_width = 0;
    for (int i = 0; i < art->line_count; i++) {
        int line_width = calculate_display_width(art->lines[i]);
        if (line_width > ascii_display_width) {
            ascii_display_width = line_width;
        }
    }
    
    int padding = ascii_display_width + 4;
    
    // Build list of entries to display
    info_entry_t display_entries[20];
    int entry_count = build_display_entries(info, display_entries, 20);
    
    int max_lines = (art->line_count > (entry_count + 1)) ? art->line_count : (entry_count + 1);

    for (int i = 0; i < max_lines; i++) {
        print_ascii_line(art, i, padding);

        if (i <= entry_count) {
            print_info_line(info, display_entries, i);
        }

        printf("\n");
    }
    
    printf("\033[0m"); // Reset colors
}
