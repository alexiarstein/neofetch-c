# Neofetch-C

A C implementation of neofetch - a fast, highly customizable system information script that displays information about your operating system, software, and hardware in an aesthetically pleasing way.

## Features

This C version implements the core functionality of neofetch, including:

- **System Information Display:**
  - OS/Distribution detection and display
  - Kernel version
  - System uptime
  - Package count (supports multiple package managers)
  - Shell information
  - Desktop Environment (DE)
  - Window Manager (WM)
  - Display resolution
  - Hardware information (CPU, GPU, RAM)
  - System model/hostname

- **ASCII Art Support:**
  - Automatic distribution logo detection
  - Support for 100+ different ASCII logos
  - Color support using ANSI escape sequences

## Supported Package Managers

The application automatically detects and counts packages from various package managers:

- **dpkg** (Debian, GoldenDog, Ubuntu)
- **rpm** (Red Hat, Fedora, SUSE)
- **pacman** (Arch Linux)
- **emerge** (Gentoo)
- **xbps** (Void Linux)
- **apk** (Alpine Linux)

## Project Structure

```
neofetch-c/
├── src/
│   ├── neofetch.c         # Main application entry point
│   ├── sysinfo.c      # System information gathering functions
│   ├── ascii.c        # ASCII art loading and display
│   └── utils.c        # Utility functions
├── include/
│   └── neofetch.h     # Header file with all declarations
├── ascii/             # ASCII art files for various distributions
├── Makefile           # Build configuration
└── README.md          # This file
```

## Building

### Prerequisites

- GCC compiler
- GNU Make
- Standard C libraries

### Compilation

```bash
# Build the application
make

# Build with debug symbols
make debug

# Clean build artifacts
make clean

# Full Install (copies neofetch bin to /usr/bin and neofetch ascii art to /usr/share/neofetch)
sudo make install

# Full removal (removes binary and ascii art from the system)
sudo make uninstall
```

## Usage

Simply run the executable:

```
neofetch
```

The application will automatically detect your system information and display it alongside the appropriate ASCII art for your distribution.

## Implementation Details

Artwork rework is ongoing. Distros with fixed artwork are:
- Debian
- Red Hat Enterprise Linux
- GoldenDog Linux
- Linux Mint
- LMDE
- Rocky Linux

Please send your corrected artwork or open an issue if your distro is displaying an ugly ASCII logo. Thanks!

### System Information Gathering

The application gathers system information through various methods:

1. **File System Access**: Reading from `/proc/`, `/sys/`, and `/etc/` directories
2. **System Calls**: Using `uname()`, `sysinfo()`, `gethostname()` etc.
3. **Command Execution**: Running system commands like `lspci`, `gsettings`, etc.
4. **Environment Variables**: Reading `$SHELL`, `$XDG_CURRENT_DESKTOP`, etc.

### Distribution Detection

Distribution detection follows this priority order:

1. `/etc/os-release` or `/usr/lib/os-release`
2. `lsb_release` command
3. Distribution-specific release files
4. Fallback to "Linux"

### ASCII Art Loading

ASCII art files are loaded from the `ascii/` directory with automatic mapping from distribution names to appropriate ASCII art files. Color codes in the ASCII files (like `${c1}`, `${c2}`) are replaced with appropriate ANSI color escape sequences.

## Differences from Original Neofetch

This C implementation focuses on the core functionality of the original bash neofetch:

- **Performance**: Significantly faster execution due to compiled nature
- **Dependencies**: Minimal external dependencies 
- **Portability**: Easier to compile and distribute as a single binary
- **Memory**: Lower memory footprint
- **Features**: Implements the most commonly used features

## License

This project is licensed under the GNU General Public License v3.0 - see the [LICENSE](LICENSE) file for details.

## Author

**Alexia Michelle**
- Copyright (C) 2026

## Acknowledgments

- Original neofetch by Dylan Araps
- ASCII art collection from the original neofetch project

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

Some ASCII art needs reworking and coloring, feel free to correct the logos you wish and send me them.

To add a new distro to neofetch-c you can prepare it yourself and send me a pull request (adding the art in ascii/ and editing ascii.c)

Or open an issue on this project and paste the art and the name of the distro as its shown in lsb_release and /etc/os-release

Thanks!
