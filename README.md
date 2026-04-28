# Vending Machine Simulator

C-based vending machine simulator with product management, coin handling, and admin panel. Features Polish currency support and inventory tracking.

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

## Building

This project uses CMake for building:

```bash
mkdir build
cd build
cmake ..
make
```

## Usage

Run the executable:
```bash
./vending_machine_simulator
```

### Main Menu Options:
1. **Display Products** - View available products with prices and quantities
2. **Insert Money** - Add coins to your balance
3. **Select Product** - Purchase a product
4. **Return Change** - Get your remaining balance back
5. **Admin Panel** - Access admin functions (PIN: 1111)

### Admin Functions:
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
