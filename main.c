#include "vending_machine.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int getIntInput() {
    char buffer[100];
    fgets(buffer, sizeof(buffer), stdin);
    return atoi(buffer);
}

int verifyAdminPin() {
    printf("Podaj 4-cyfrowy PIN admina: ");

    char input[10];
    fgets(input, sizeof(input), stdin);
    input[strcspn(input, "\n")] = 0;

    return strcmp(input, "1111") == 0;
}

void showAdminMenu() {
    printf("\n=== PANEL ADMINA ===\n");
    printf("1. Usuń produkt\n");
    printf("2. Zmień cenę produktu\n");
    printf("3. Zmień ilość produktu\n");
    printf("4. Dodaj nowy produkt\n");
    printf("5. Pokaż stan monet\n");
    printf("6. Powrót do głównego menu\n");
    printf("Wybierz opcję: ");
}

void showMenu() {
    printf("\n=== AUTOMAT VENDINGOWY ===\n");
    printf("1. Wyświetl produkty\n");
    printf("2. Wrzuć monetę\n");
    printf("3. Wybierz produkt\n");
    printf("4. Zwrot reszty\n");
    printf("5. Panel admina\n");
    printf("6. Wyjście\n");
    printf("Wybierz opcję: ");
}

int parseCoinInput(const char* input) {
    if (strcmp(input, "10gr") == 0) return 10;
    if (strcmp(input, "20gr") == 0) return 20;
    if (strcmp(input, "50gr") == 0) return 50;
    if (strcmp(input, "1zł") == 0) return 100;
    if (strcmp(input, "2zł") == 0) return 200;
    if (strcmp(input, "5zł") == 0) return 500;
    return 0;
}

void insertMoneyFromConsole(VendingMachine *vm) {
    printf("\nDostępne monety: 10gr, 20gr, 50gr, 1zł, 2zł, 5zł\n");
    printf("Wpisz nominał: ");

    char input[10];
    fgets(input, sizeof(input), stdin);

    input[strcspn(input, "\n")] = 0;

    int amount_gr = parseCoinInput(input);
    if (amount_gr > 0) {
        insertMoney(vm, amount_gr);
    } else {
        printf("Nieprawidłowy format. Spróbuj ponownie.\n");
    }
}

void adminMenu(VendingMachine *vm) {
    int admin_choice;
    do {
        showAdminMenu();
        admin_choice = getIntInput();

        switch(admin_choice) {
            case 1:
                printf("Podaj numer produktu do usunięcia: ");
                int index = getIntInput();
                removeProduct(vm, index);
                break;
            case 2:
                printf("Podaj numer produktu: ");
                int price_index = getIntInput();
                printf("Podaj nową cenę w groszach: ");
                int new_price = getIntInput();
                updateProductPrice(vm, price_index, new_price);
                break;
            case 3:
                printf("Podaj numer produktu: ");
                int qty_index = getIntInput();
                printf("Podaj nową ilość: ");
                int new_qty = getIntInput();
                updateProductQuantity(vm, qty_index, new_qty);
                break;
            case 4:
                if (vm->product_count >= MAX_PRODUCTS) {
                    printf("Automat jest pełny. Nie można dodać więcej produktów.\n");
                    break;
                }

                printf("Podaj nazwę produktu: ");
                char name[50];
                fgets(name, sizeof(name), stdin);
                name[strcspn(name, "\n")] = 0;

                printf("Podaj cenę w groszach: ");
                int price = getIntInput();

                printf("Podaj ilość: ");
                int quantity = getIntInput();

                Product new_product;
                strcpy(new_product.name, name);
                new_product.price_gr = price;
                new_product.quantity = quantity;

                addProduct(vm, new_product, quantity);
                break;
            case 5:
                printf("\n=== STAN MONET W AUTOMACIE ===\n");
                for (int i = 0; i < 6; i++) {
                    printf("%s: %d szt (%.2f zł)\n",
                           accepted_coins[i].key,
                           vm->coin_inventory[i],
                           (vm->coin_inventory[i] * accepted_coins[i].value_gr) / 100.0);
                }

                int total_value_gr = 0;
                for (int i = 0; i < 6; i++) {
                    total_value_gr += vm->coin_inventory[i] * accepted_coins[i].value_gr;
                }
                printf("\nCałkowita wartość monet: %.2f zł\n", total_value_gr / 100.0);
                break;
            case 6: printf("Powrót do głównego menu\n"); break;
            default: printf("Nieprawidłowa opcja\n");
        }
    } while (admin_choice != 6);
}

int main() {
    VendingMachine vm;
    initMachine(&vm);

    int choice;
    do {
        showMenu();
        choice = getIntInput();

        switch(choice) {
            case 1: displayProducts(&vm); break;
            case 2: insertMoneyFromConsole(&vm); break;
            case 3:
                printf("Wybierz numer produktu: ");
                int product_num = getIntInput();
                selectProduct(&vm, product_num);
                break;
            case 4: returnChange(&vm); break;
            case 5:
                if (verifyAdminPin()) {
                    adminMenu(&vm);
                } else {
                    printf("Nieprawidłowy PIN!\n");
                }
                break;
            case 6: printf("Dziękujemy!\n"); break;
            default: printf("Nieprawidłowa opcja\n");
        }
    } while (choice != 6);

    return 0;
}