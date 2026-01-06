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

- **dpkg** (Debian, Ubuntu)
- **rpm** (Red Hat, Fedora, SUSE)
- **pacman** (Arch Linux)
- **emerge** (Gentoo)
- **xbps** (Void Linux)
- **apk** (Alpine Linux)

## Project Structure

```
neofetch-c/
├── src/
│   ├── main.c         # Main application entry point
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

# Install to /usr/local/bin
make install
```

## Usage

Simply run the executable:

```bash
./neofetch
```

The application will automatically detect your system information and display it alongside the appropriate ASCII art for your distribution.

## Implementation Details

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

## Contributing

To add support for additional distributions, package managers, or improve system detection:

1. Add distribution-specific logic to `src/sysinfo.c`
2. Add corresponding ASCII art to the `ascii/` directory
3. Update the distribution mapping in `src/ascii.c`

## License

This project maintains compatibility with the original neofetch licensing.

## Acknowledgments

- Original neofetch by Dylan Araps
- Maintained fork by Alexia Michelle
- ASCII art collection from the original neofetch project