# Vending Machine Simulator

C-based vending machine simulator with product management, coin handling, and admin panel. Features Polish currency support and inventory tracking.

## Features

- **Product Management**: Add, remove, and update products with prices and quantities
- **Coin Handling**: Support for Polish coins (10gr, 20gr, 50gr, 1zł, 2zł, 5zł)
- **Admin Panel**: Secure admin access with PIN protection for inventory management
- **Change Calculation**: Automatic change calculation and return
- **Inventory Tracking**: Real-time tracking of products and coins in the machine

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

## Technical Details

- **Language**: C
- **Build System**: CMake
- **Currency**: Polish Złoty (PLN)
- **Max Products**: 10
- **Max Quantity per Product**: 30

## License

This project is open source. Feel free to contribute or modify for your own use.
