# GhostShell

GhostShell is a lightweight Linux system monitor built with C++17 and ncurses. It provides live CPU, RAM, swap, disk, network, process, uptime, load, hardware, and kernel information through a clean, responsive terminal interface.

## Overview

GhostShell brings the most useful host and process information together in a focused terminal dashboard. It reads standard Linux interfaces such as `/proc`, `/sys`, `sysinfo()`, and `uname()` without requiring a graphical environment or heavyweight runtime dependencies.

The dashboard is designed for real terminals: it adapts to available space, handles terminal resizing, uses persistent ncurses windows for smooth frame updates, and falls back safely on small terminals.

## Features

- Live overall CPU usage and per-core CPU usage
- CPU history visualization on larger terminals
- RAM and swap usage
- Disk capacity and disk I/O totals
- Network receive/transmit totals
- Sortable process table
- Process sorting by CPU, memory, or PID
- Process navigation and selection
- Uptime and load average
- Hostname, kernel, hardware, and core-count information
- Responsive layouts for wide, medium, and small terminals
- Flicker-free rendering with batched ncurses updates
- Graceful handling of terminal resize and signals
- Y2K-inspired GhostShell cloud identity

## Screenshots

Screenshots are not currently included. Run GhostShell in a terminal to see the live dashboard.

## Requirements

- Linux
- C++17-compatible compiler
- CMake 3.16 or newer
- ncurses development libraries

GhostShell is developed and tested on CachyOS/Arch Linux. Other Linux distributions should work with their equivalent C++17, CMake, and ncurses packages.

## Installation

On Arch Linux or CachyOS:

```sh
sudo pacman -S --needed base-devel cmake ncurses
```

## Building

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## Running

```sh
./build/ghostshell
```

## Testing

```sh
ctest --test-dir build --output-on-failure
```

## Controls

| Key | Action |
| --- | --- |
| `q` | Quit |
| `?` | Show or hide help |
| `s` | Cycle process sorting: CPU, memory, PID |
| `1` | Sort processes by CPU |
| `2` | Sort processes by memory |
| `3` | Sort processes by PID |
| `↑` / `↓` | Move the selected process |
| `j` / `k` | Move the selected process |
| `g` | Select the first process |
| `G` | Select the last process |

## Project Structure

- `include/` - Public headers and data structures
- `src/` - Dashboard UI, Linux metric collection, process collection, and utilities
- `tests/` - Parsing and formatting tests
- `.github/workflows/` - Continuous integration workflows
- `CMakeLists.txt` - Build and test configuration

## Contributing

1. Fork the repository.
2. Create a feature branch.
3. Make the change and preserve Linux/C++17 compatibility.
4. Build and test locally:

   ```sh
   cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
   cmake --build build
   ctest --test-dir build --output-on-failure
   ```

5. Open a pull request with a clear description of the change.

Please keep changes focused, avoid committing generated build output, and include tests when changing parsing or platform-facing behavior.

## License

GhostShell is available under the [MIT License](LICENSE).
