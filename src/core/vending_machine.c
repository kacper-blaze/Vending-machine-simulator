#include "vending_machine.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

extern coin accepted_coins[] = {
    {"10gr", 10},
    {"20gr", 20},
    {"50gr", 50},
    {"1zł", 100},
    {"2zł", 200},
    {"5zł", 500}
};

void initMachine(VendingMachine *vm) {
    vm->balance_gr = 0;
    int initial_coins[6] = {50, 50, 30, 20, 10, 5}; //[10gr, 20gr, 50gr, 1zł, 2zł 5zł]
    for (int i = 0; i < 6; i++) {
        vm->coin_inventory[i] = initial_coins[i];
    }

    Product default_products[] = {
        {"Cola", 350, 10},      // 3.50 zł
        {"Woda", 200, 15},      // 2.00 zł
        {"Sok", 400, 8},        // 4.00 zł
        {"Baton", 250, 12},     // 2.50 zł
        {"Chipsy", 450, 6},      // 4.50 zł
        {"baton proteinowy", 400, 20},  // 4.00 zł
        {"paluszki z sezamem", 350, 12},    // 3.50 zł
        {"paluszki z solą", 300, 10},   // 3.00 zł
        {"guma balonowa", 150, 30},  // 1.50 zł
        {"napój energetyczny", 450, 5} // 4.50 zł
    };

    vm->product_count = 0;
    int num_products = sizeof(default_products) / sizeof(default_products[0]);
    for (int i = 0; i < num_products && i < MAX_PRODUCTS; i++) {
        vm->products[i] = default_products[i];
        vm->product_count++;
    }
}

void displayProducts(const VendingMachine *vm) {
    printf("balans środków: %.2f zł\n\n", vm->balance_gr / 100.0);
    for (int i = 0; i < vm->product_count; i++) {
        printf("%d. %s - %.2f zł, ilość: %d\n", i + 1, vm->products[i].name, vm->products[i].price_gr / 100.0, vm->products[i].quantity);
    }
}

void insertMoney(VendingMachine *vm, int amount_gr) {
    if (amount_gr <= 0) {
        printf("Nieprawidłowa kwota\n");
        printf("Moneta wróciła do Ciebie\n"); //fizyczny zwrot, automat nie przyjął nominału
        
        char log_msg[200];
        sprintf(log_msg, "Odrzucono nieprawidłową monetę: %d", amount_gr);
        writeLog(LOG_COIN, log_msg);
        return;
    }

    for (int i = 0; i < 6; i++) {
        if (amount_gr == accepted_coins[i].value_gr) {
            vm->coin_inventory[i]++;
            vm->balance_gr += amount_gr;
            printf("Przyjęto monetę: %s\n", accepted_coins[i].key);
            printf("balans środków: %.2f zł\n\n", vm->balance_gr / 100.0);
            
            char log_msg[200];
            sprintf(log_msg, "Przyjęto monetę: %s (saldo: %.2f zł)", 
                   accepted_coins[i].key, vm->balance_gr / 100.0);
            writeLog(LOG_COIN, log_msg);
            
            // Wysłanie wiadomości do procesu maintenance
            char pipe_msg[256];
            sprintf(pipe_msg, "COIN:Przyjęto monetę %s", accepted_coins[i].key);
            sendMessageToMaintenance(pipe_msg);
            return;
        }
    }

    printf("Nieprzyjęta moneta, moneta wróciła do Ciebie\n");
    
    char log_msg[200];
    sprintf(log_msg, "Odrzucono nieznany nominał: %d", amount_gr);
    writeLog(LOG_COIN, log_msg);
}

void addProduct(VendingMachine *vm, Product product, int additional_quantity) {
    for (int i = 0; i < vm->product_count; i++) {
        if (strcmp(vm->products[i].name, product.name) == 0) {
            int space_available = MAX_QUANTITY - vm->products[i].quantity;
            if (space_available <= 0) {
                printf("Produkt '%s' jest już pełny (%d/%d)\n", vm->products[i].name, vm->products[i].quantity, MAX_QUANTITY);
                return;
            }

            int to_add = (additional_quantity > space_available) ? space_available : additional_quantity;
            vm->products[i].quantity += to_add;

            if (to_add < additional_quantity) {
                printf("Dodano %d sztuk produktu '%s' (maksimum osiągnięte: %d/%d)\n",
                       to_add, vm->products[i].name, vm->products[i].quantity, MAX_QUANTITY);
            } else {
                printf("Dodano %d sztuk produktu '%s' (łącznie: %d/%d)\n",
                       to_add, vm->products[i].name, vm->products[i].quantity, MAX_QUANTITY);
            }
            return;
        }
    }

    if (vm->product_count >= MAX_PRODUCTS) {
        printf("Maszyna jest pełna\n");
        return;
    }

    if (product.quantity <= 0) {
        printf("Nieprawidłowa ilość produktu\n");
        return;
    }

    product.quantity = additional_quantity;
    vm->products[vm->product_count] = product;
    vm->product_count++;
}

void removeProduct(VendingMachine *vm, int index) {
    if (index < 1 || index > vm->product_count) {
        printf("Nieprawidłowy numer produktu\n");
        return;
    }

    printf("Usunięto produkt: %s\n", vm->products[index-1].name);

    for (int i = index-1; i < vm->product_count-1; i++) {
        vm->products[i] = vm->products[i+1];
    }

    vm->product_count--;
}

void updateProductPrice(VendingMachine *vm, int index, int new_price_gr) {
    if (index < 1 || index > vm->product_count) {
        printf("Nieprawidłowy numer produktu\n");
        return;
    }

    if (new_price_gr <= 0) {
        printf("Cena musi być dodatnia\n");
        return;
    }

    printf("Zmieniono cenę produktu '%s': %.2f zł -> %.2f zł\n",
           vm->products[index-1].name,
           vm->products[index-1].price_gr / 100.0,
           new_price_gr / 100.0);

    vm->products[index-1].price_gr = new_price_gr;
}

void updateProductQuantity(VendingMachine *vm, int index, int new_quantity) {
    if (index < 1 || index > vm->product_count) {
        printf("Nieprawidłowy numer produktu\n");
        return;
    }

    if (new_quantity < 0 || new_quantity > MAX_QUANTITY) {
        printf("Ilość musi być między 0 a %d\n", MAX_QUANTITY);
        return;
    }

    printf("Zmieniono ilość produktu '%s': %d -> %d\n",
           vm->products[index-1].name,
           vm->products[index-1].quantity,
           new_quantity);

    vm->products[index-1].quantity = new_quantity;
}

void selectProduct(VendingMachine *vm, int index) {
    if (index < 1 || index > vm->product_count) {
        printf("Nieprawidłowy numer produktu\n");
        
        char log_msg[200];
        sprintf(log_msg, "Nieprawidłowy wybór produktu: %d", index);
        writeLog(LOG_TRANSACTION, log_msg);
        return;
    }

    printf("twój balans: %.2f zł\n", vm->balance_gr / 100.0);
    printf("wybrano %d -> %s\n", index, vm->products[index-1].name);

    if (vm->products[index-1].quantity == 0) {
        printf("Produkt '%s' jest niedostępny\n", vm->products[index-1].name);
        
        char log_msg[200];
        sprintf(log_msg, "Próba zakupu niedostępnego produktu: %s (ilość: 0)", vm->products[index-1].name);
        writeLog(LOG_TRANSACTION, log_msg);
        return;
    }

    printf("do zapłaty %.2f zł\n", vm->products[index-1].price_gr / 100.0);

    if (vm->balance_gr < vm->products[index-1].price_gr) {
        printf("Niewystarczająca kwota. Brakuje: %.2f zł\n",
            (vm->products[index-1].price_gr - vm->balance_gr) / 100.0);
        
        char log_msg[200];
        sprintf(log_msg, "Niewystarczająca kwota dla produktu %s: brak %.2f zł", 
               vm->products[index-1].name, 
               (vm->products[index-1].price_gr - vm->balance_gr) / 100.0);
        writeLog(LOG_TRANSACTION, log_msg);
        return;
    }

    vm->balance_gr -= vm->products[index-1].price_gr;
    vm->products[index-1].quantity--;

    printf("Zakup udany! Produkt '%s' wydany.\n", vm->products[index-1].name);
    
    char log_msg[200];
    sprintf(log_msg, "Zakup: %s za %.2f zł (saldo: %.2f zł, pozostało: %d szt)", 
           vm->products[index-1].name, 
           vm->products[index-1].price_gr / 100.0,
           vm->balance_gr / 100.0,
           vm->products[index-1].quantity);
    writeLog(LOG_TRANSACTION, log_msg);
    
    // Wysłanie wiadomości do procesu maintenance
    char pipe_msg[256];
    sprintf(pipe_msg, "TRANSACTION:Zakup %s za %.2f zł", 
           vm->products[index-1].name, vm->products[index-1].price_gr / 100.0);
    sendMessageToMaintenance(pipe_msg);

    if (vm->balance_gr > 0) {
        returnChange(vm);
    }
}

void returnChange(VendingMachine *vm) {
    if (vm->balance_gr == 0) {
        printf("Brak reszty do zwrotu\n");
        return;
    }

    int change_to_return = vm->balance_gr;
    printf("Zwracanie reszty: %.2f zł\n", change_to_return / 100.0);

    for (int i = 5; i >= 0; i--) {
        int coin_value = accepted_coins[i].value_gr;
        int coins_needed = change_to_return / coin_value;
        int coins_available = vm->coin_inventory[i];
        int coins_to_use = (coins_needed < coins_available) ? coins_needed : coins_available;

        if (coins_to_use > 0) {
            printf("%dx %s\n", coins_to_use, accepted_coins[i].key);
            change_to_return -= coins_to_use * coin_value;
            vm->coin_inventory[i] -= coins_to_use;
        }
    }

    if (change_to_return > 0) {
        printf("Ostrzeżenie: Nie można zwrócić pełnej reszty (%.2f zł)\n", change_to_return / 100.0);
    }

    vm->balance_gr = 0;
}