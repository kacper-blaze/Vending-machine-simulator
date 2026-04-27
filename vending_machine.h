#pragma once

#define MAX_PRODUCTS 10
#define MAX_QUANTITY 30

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