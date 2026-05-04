# Vending Machine Simulator

A C-based vending machine simulator with product management, coin handling, and an admin panel. Features Polish currency support, real-time inventory tracking, and both console and GUI interfaces. Built with multi-process architecture using named pipes for IPC and comprehensive logging.

## Features

- **Product Management** — Add, remove, and update products with prices and quantities
- **Coin Handling** — Support for Polish coins (10gr, 20gr, 50gr, 1zł, 2zł, 5zł)
- **Admin Panel** — Secure admin access with PIN protection (`1111`)
- **Change Calculation** — Automatic change calculation and return using available coin inventory
- **Inventory Tracking** — Real-time tracking of products and coins in the machine
- **Background Maintenance** — Separate monitoring process for system health checks
- **Comprehensive Logging** — All operations logged to `vending_machine.log` with categorized event types
- **Inter-Process Communication** — Named pipe communication between main and maintenance processes
- **Signal Handling** — Graceful shutdown with `SIGINT`/`SIGTERM`, `SIGCHLD`, and `SIGPIPE` support
- **Real-time Balance Display** — Shows current balance after each coin insertion
- **Graphical Interface** — GTK+3-based GUI with interactive buttons, visual feedback, and CSS styling

## Project Structure

```
vendingMachineSimulator/
├── CMakeLists.txt              # Build configuration
├── README.md                   # This file
├── .gitignore                  # Git ignore rules
├── resources/
│   └── style.css               # GTK+3 GUI stylesheet
├── src/
│   ├── core/                   # Core business logic
│   │   ├── vending_machine.h   # Data structures, constants, and function declarations
│   │   ├── vending_machine.c   # Product/coin operations and machine state
│   │   ├── logger.c            # File logging with multiple fallback paths
│   │   └── maintenance.c       # Background monitoring process (IPC, health checks)
│   └── ui/                     # User interfaces
│       ├── main.c              # Console (terminal) application
│       └── gui_main.c          # GTK+3 graphical application
└── build/                      # CMake build output (generated)
```

## Prerequisites

| Component | Required For | Notes |
|-----------|-------------|-------|
| C99 compiler | Both | `gcc` or `clang` |
| CMake 3.10+ | Both | Build system |
| GTK+3 dev libs | GUI only | Optional — console builds without it |
| pthread | Both | Linked automatically |

### Installing GTK+3 (GUI only)

**Ubuntu / Debian**
```bash
sudo apt install libgtk-3-dev
```

**Fedora / RHEL**
```bash
sudo dnf install gtk3-devel
```

**Arch Linux**
```bash
sudo pacman -S gtk3
```

## Building

```bash
mkdir build && cd build
cmake ..
make
```

CMake auto-detects GTK+3 and builds available targets:

| Target | File | Description |
|--------|------|-------------|
| `vendingMachineSimulator` | `src/ui/main.c` | Console version (always built) |
| `vendingMachineSimulatorGUI` | `src/ui/gui_main.c` | GTK+3 GUI (only if GTK+3 found) |

If GTK+3 is missing, only the console version is compiled and a warning is printed.

## Usage

### Console Version
```bash
./vendingMachineSimulator
```

**Main Menu**
1. **Display Products** — View available products with prices and quantities
2. **Insert Money** — Add coins to your balance (e.g. `1zł`, `5zł`)
3. **Select Product** — Purchase a product by number
4. **Return Change** — Get your remaining balance back in coins
5. **Admin Panel** — Access admin functions (PIN: `1111`)
6. **Exit** — Quit the application

### GUI Version
```bash
./vendingMachineSimulatorGUI
```

**Interface**
- **Product Grid** — Visual buttons showing name, price, and quantity
- **Coin Buttons** — Click to insert coins
- **Return Change** — Button to return remaining balance
- **Admin Panel** — PIN-protected admin access (`1111`)
- **Status Bar** — Real-time feedback on every operation
- **CSS Styling** — Custom themes via `resources/style.css`

### Admin Functions (Both Versions)
- Remove products from inventory
- Change product prices
- Update product quantities
- Add new products (up to `MAX_PRODUCTS`)
- View full coin inventory status and total value

## Default Inventory

The machine is pre-loaded with the following products:

| # | Product | Price | Qty |
|---|---------|-------|-----|
| 1 | Cola | 3.50 zł | 10 |
| 2 | Woda | 2.00 zł | 15 |
| 3 | Sok | 4.00 zł | 8 |
| 4 | Baton | 2.50 zł | 12 |
| 5 | Chipsy | 4.50 zł | 6 |
| 6 | Baton proteinowy | 4.00 zł | 20 |
| 7 | Paluszki z sezamem | 3.50 zł | 12 |
| 8 | Paluszki z solą | 3.00 zł | 10 |
| 9 | Guma balonowa | 1.50 zł | 30 |
| 10 | Napój energetyczny | 4.50 zł | 5 |

**Initial Coin Inventory**

| Coin | Count |
|------|-------|
| 10 gr | 50 |
| 20 gr | 50 |
| 50 gr | 30 |
| 1 zł | 20 |
| 2 zł | 10 |
| 5 zł | 5 |

## Logging & Monitoring

A background maintenance process starts automatically and communicates with the main process via a named pipe at `/tmp/vm_pipe`.

**Log file:** `vending_machine.log` (searches current dir, parent, and grandparent)

**Log types:** `TRANSACTION`, `ADMIN`, `COIN`, `PRODUCT`, `SYSTEM`, `ALERT`

**Format:**
```
[YYYY-MM-DD HH:MM:SS] [TYPE] Description
```

**Example entries:**
```
[2024-04-28 14:01:25] [SYSTEM] System uruchomiony - logger zainicjalizowany
[2024-04-28 14:01:30] [COIN] Przyjęto monetę: 5zł (saldo: 5.00 zł)
[2024-04-28 14:02:15] [TRANSACTION] Zakup: Cola za 3.50 zł
```

The maintenance process monitors coin levels, product stock, and hardware status, sending alerts through the pipe when thresholds are crossed.

## Programming Constructions

### Data Structures

**Structures for State Management**
- `VendingMachine` - Central data structure containing products array, balance, and coin inventory
- `Product` - Encapsulates product information (name, price in grosz, quantity) 
- `coin` - Key-value pairs for coin denominations and their grosz values
- `LogEntry` - Timestamped log entries with event categorization

**Why:** Structures provide logical grouping of related data and enable efficient memory management. Using grosz (integer) instead of złoty (float) prevents floating-point precision errors in financial calculations.

### Process Architecture

**Multi-Process Design**
- Main process handles user interaction and core vending logic
- Background maintenance process monitors system health
- Inter-Process Communication via named pipes (`/tmp/vm_pipe`)

**Why:** Separation of concerns improves reliability - maintenance process can continue monitoring even if main process crashes. Named pipes provide simple, reliable IPC without complex synchronization.

### Signal Handling

**Comprehensive Signal Management**
```c
signal(SIGINT, signalHandler);    // Graceful shutdown on Ctrl+C
signal(SIGTERM, signalHandler);   // Termination from system
signal(SIGCHLD, signalHandler);   // Child process cleanup
signal(SIGPIPE, SIG_IGN);         // Ignore broken pipes
```

**Why:** Ensures proper resource cleanup and prevents zombie processes. Ignoring SIGPIPE prevents crashes when pipe communication fails.

### Memory Management

**Static Arrays vs Dynamic Allocation**
- Fixed-size arrays for products (`MAX_PRODUCTS`) and coins
- Stack allocation for most data structures
- Minimal heap usage to prevent memory leaks

**Why:** Predictable memory usage simplifies debugging and eliminates memory leak risks in embedded-like environments.

### Enumerated Types

**Type Safety for Log Categories**
```c
typedef enum {
    LOG_TRANSACTION,
    LOG_ALERT,
    LOG_ADMIN,
    LOG_COIN,
    LOG_PRODUCT,
    LOG_SYSTEM
} LogType;
```

**Why:** Provides compile-time type checking and self-documenting code compared to magic numbers or strings.

### Function Pointers & Callbacks

**GTK+3 Event Handling**
```c
g_signal_connect(button, "clicked", G_CALLBACK(on_coin_clicked), coin_value);
```

**Why:** Event-driven architecture essential for GUI responsiveness. Callbacks enable loose coupling between UI and business logic.

### File I/O Patterns

**Robust Logging with Fallback Paths**
- Searches multiple directories for log file location
- Thread-safe file operations
- Structured log format with timestamps and categories

**Why:** Ensures logging works across different deployment scenarios and provides audit trail for debugging.

### Error Handling

**Defensive Programming**
- Input validation for coin parsing and product selection
- Boundary checking for array access
- Graceful degradation when resources unavailable

**Why:** Prevents crashes from invalid user input and system resource issues.

## Technical Details

| Property | Value |
|----------|-------|
| Language | C99 |
| Build System | CMake 3.10+ |
| Currency | Polish Złoty (PLN) |
| Max Products | 10 |
| Max Quantity / Product | 30 |
| Architecture | Main process + background maintenance process |
| IPC | Named pipe (`/tmp/vm_pipe`) |
| Signals | `SIGINT`, `SIGTERM`, `SIGCHLD`, `SIGPIPE` |
| Threading | pthread (logger & IPC) |
| GUI Toolkit | GTK+3 |

## Configuration Limits

Constants are defined in `src/core/vending_machine.h`:

```c
#define MAX_PRODUCTS 10       // Maximum number of different products
#define MAX_QUANTITY 30       // Maximum units per product
```

Change these before building if you need a larger inventory.

## License

This project is open source. Feel free to contribute or modify for your own use.
