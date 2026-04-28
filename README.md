# Vending Machine Simulator

C-based vending machine simulator with product management, coin handling, and admin panel. Features Polish currency support and inventory tracking. Available in both console and GUI versions.

## Features

- **Product Management**: Add, remove, and update products with prices and quantities
- **Coin Handling**: Support for Polish coins (10gr, 20gr, 50gr, 1zł, 2zł, 5zł)
- **Admin Panel**: Secure admin access with PIN protection for inventory management
- **Change Calculation**: Automatic change calculation and return
- **Inventory Tracking**: Real-time tracking of products and coins in the machine
- **Background Maintenance**: Separate maintenance process for system monitoring
- **Comprehensive Logging**: All operations logged to vending_machine.log
- **Inter-Process Communication**: Named pipe communication between processes
- **Signal Handling**: Graceful shutdown with SIGINT/SIGTERM support
- **Real-time Balance Display**: Shows current balance after coin insertion
- **Graphical Interface**: GTK+3-based GUI version with interactive buttons and visual feedback

## Building

This project uses CMake for building. Two versions are available:

### Console Version (Always Builds)
```bash
mkdir build
cd build
cmake ..
make
```

### GUI Version (Requires GTK+3)
To build the GUI version, you need to install GTK+3 development libraries first:

**Ubuntu/Debian:**
```bash
sudo apt install libgtk-3-dev
```

**Fedora/RHEL:**
```bash
sudo dnf install gtk3-devel
```

**Arch Linux:**
```bash
sudo pacman -S gtk3
```

After installing GTK+3, rebuild the project:
```bash
cd build
cmake ..
make
```

The build system will automatically detect GTK+3 and build both versions:
- `vendingMachineSimulator` - Console version
- `vendingMachineSimulatorGUI` - GUI version

If GTK+3 is not found, only the console version will be built.

## Usage

### Console Version
```bash
./vendingMachineSimulator
```

### GUI Version
```bash
./vendingMachineSimulatorGUI
```

### Main Menu Options (Console):
1. **Display Products** - View available products with prices and quantities
2. **Insert Money** - Add coins to your balance
3. **Select Product** - Purchase a product
4. **Return Change** - Get your remaining balance back
5. **Admin Panel** - Access admin functions (PIN: 1111)

### GUI Features:
- **Product Grid**: Visual display of all products with prices and quantities
- **Coin Buttons**: Click to insert coins (10gr, 20gr, 50gr, 1zł, 2zł, 5zł)
- **Product Selection**: Click on product buttons to purchase
- **Balance Display**: Real-time balance updates
- **Return Change**: Automatic change calculation and display
- **Admin Access**: PIN-protected admin panel (PIN: 1111)
- **Status Messages**: Real-time feedback on all operations

### Admin Functions (Both Versions):
- Remove products
- Change product prices
- Update product quantities
- Add new products
- View coin inventory status

## Logging and Monitoring

The simulator includes a comprehensive logging system:

- **Log File**: All operations are logged to `vending_machine.log`
- **Log Types**: TRANSACTION, ADMIN, COIN, PRODUCT, SYSTEM, ALERT
- **Background Process**: Maintenance process runs independently for monitoring
- **Real-time Logging**: Events are logged immediately as they occur
- **Process Communication**: Uses named pipes (`/tmp/vm_pipe`) for IPC

### Log Format:
```
[YYYY-MM-DD HH:MM:SS] [TYPE] Description
```

Example log entries:
```
[2024-04-28 14:01:25] [SYSTEM] System uruchomiony - logger zainicjalizowany
[2024-04-28 14:01:30] [COIN] Przyjęto monetę: 5zł (saldo: 5.00 zł)
[2024-04-28 14:02:15] [TRANSACTION] Zakup: Cola za 3.50 zł
```

## Technical Details

- **Language**: C
- **Build System**: CMake
- **Currency**: Polish Złoty (PLN)
- **Max Products**: 10
- **Max Quantity per Product**: 30
- **Processes**: Main process + background maintenance process
- **IPC**: Named pipes (`/tmp/vm_pipe`)
- **Signals**: SIGINT, SIGTERM, SIGCHLD, SIGPIPE handling

## License

This project is open source. Feel free to contribute or modify for your own use.
