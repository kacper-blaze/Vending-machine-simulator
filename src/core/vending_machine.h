#pragma once

#include <time.h>
#include <sys/types.h>

#define MAX_PRODUCTS 10
#define MAX_QUANTITY 30
#define LOG_FILE "vending_machine.log"
#define PIPE_PATH "/tmp/vm_pipe"
#define MAX_LOG_ENTRY 256

typedef struct {
    const char* key;
    int value_gr;
} coin;

extern coin accepted_coins[];

typedef struct {
    char name[50];
    int price_gr;
    int quantity;
} Product;

typedef struct {
    Product products[MAX_PRODUCTS];
    int product_count;
    int balance_gr;
    int coin_inventory[6];
} VendingMachine;

typedef struct {
    time_t timestamp;
    char event_type[50];
    char description[200];
} LogEntry;

typedef enum {
    LOG_TRANSACTION,
    LOG_ALERT,
    LOG_ADMIN,
    LOG_COIN,
    LOG_PRODUCT,
    LOG_SYSTEM
} LogType;

// Index mapping for coin_inventory array:
// 0: 10gr
// 1: 20gr
// 2: 50gr
// 3: 1zł (100gr)
// 4: 2zł (200gr)
// 5: 5zł (500gr)

void initMachine(VendingMachine *vm);
void displayProducts(const VendingMachine *vm);
void insertMoney(VendingMachine *vm, int amount_gr);
void addProduct(VendingMachine *vm, Product product, int additional_quantity);
void selectProduct(VendingMachine *vm, int index);
void returnChange(VendingMachine *vm);

int parseCoinInput(const char* input);
void insertMoneyFromConsole(VendingMachine *vm);
void showMenu();

void removeProduct(VendingMachine *vm, int index);
void updateProductPrice(VendingMachine *vm, int index, int new_price_gr);
void updateProductQuantity(VendingMachine *vm, int index, int new_quantity);

// Logging functions
void writeLog(LogType type, const char* description);
void initLogger();
void closeLogger();

// Maintenance process functions
void startMaintenanceProcess();
void maintenanceProcess();
void checkCoinLevels(const VendingMachine *vm);
void checkProductLevels(const VendingMachine *vm);
void checkHardwareStatus();

// Communication functions
void sendMessageToMaintenance(const char* message);
void setupPipe();

// External variables
extern pid_t maintenance_pid;