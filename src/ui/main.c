#include "vending_machine.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>

static int main_running = 1;

void signalHandler(int sig) {
    switch (sig) {
        case SIGINT:
            printf("\nOtrzymano SIGINT - Bezpieczne zamykanie...\n");
            main_running = 0;
            break;
        case SIGTERM:
            printf("\nOtrzymano SIGTERM - zamykanie systemu...\n");
            main_running = 0;
            break;
        case SIGCHLD:
            // Obsługa zakończenia procesu potomnego
            int status;
            pid_t pid = waitpid(-1, &status, WNOHANG);
            if (pid == maintenance_pid) {
                printf("Proces maintenance zakończony\n");
                maintenance_pid = -1;
            }
            break;
    }
}

void cleanup() {
    if (maintenance_pid > 0) {
        kill(maintenance_pid, SIGTERM);
        waitpid(maintenance_pid, NULL, 0);
    }
    closeLogger();
    unlink(PIPE_PATH);
}

int getIntInput() {
    char buffer[100];
    if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
        return -1; // EOF
    }

    buffer[strcspn(buffer, "\n")] = 0;
    if (strlen(buffer) > 0) {
        return atoi(buffer);
    }
    
    return 0; // Pusty input
}

int verifyAdminPin() {
    printf("Podaj 4-cyfrowy PIN admina: ");

    char input[10];
    fgets(input, sizeof(input), stdin);
    input[strcspn(input, "\n")] = 0;

    int result = strcmp(input, "1111") == 0;
    
    char log_msg[200];
    sprintf(log_msg, "Próba dostępu admina: %s", result ? "sukces" : "niepowodzenie");
    writeLog(LOG_ADMIN, log_msg);
    
    if (result) {
        char pipe_msg[256];
        sprintf(pipe_msg, "ADMIN:Otwarto panel admina");
        sendMessageToMaintenance(pipe_msg);
    }
    
    return result;
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
            case 1: {
                printf("Podaj numer produktu do usunięcia: ");
                int index = getIntInput();
                removeProduct(vm, index);
                
                char log_msg[200];
                sprintf(log_msg, "Admin usunął produkt nr %d", index);
                writeLog(LOG_ADMIN, log_msg);
                
                char pipe_msg[256];
                sprintf(pipe_msg, "ADMIN:Usunięto produkt nr %d", index);
                sendMessageToMaintenance(pipe_msg);
                break;
            }
            case 2: {
                printf("Podaj numer produktu: ");
                int price_index = getIntInput();
                printf("Podaj nową cenę w groszach: ");
                int new_price = getIntInput();
                updateProductPrice(vm, price_index, new_price);
                
                char price_log_msg[200];
                sprintf(price_log_msg, "Admin zmienił cenę produktu nr %d na %d gr", price_index, new_price);
                writeLog(LOG_ADMIN, price_log_msg);
                break;
            }
            case 3: {
                printf("Podaj numer produktu: ");
                int qty_index = getIntInput();
                printf("Podaj nową ilość: ");
                int new_qty = getIntInput();
                updateProductQuantity(vm, qty_index, new_qty);
                
                char qty_log_msg[200];
                sprintf(qty_log_msg, "Admin zmienił ilość produktu nr %d na %d", qty_index, new_qty);
                writeLog(LOG_ADMIN, qty_log_msg);
                break;
            }
            case 4: {
                char name[50];
                int price;
                int quantity;
                Product new_product;

                if (vm->product_count >= MAX_PRODUCTS) {
                    printf("Automat jest pełny. Nie można dodać więcej produktów.\n");
                    break;
                }

                printf("Podaj nazwę produktu: ");
                fgets(name, sizeof(name), stdin);
                name[strcspn(name, "\n")] = 0;

                printf("Podaj cenę w groszach: ");
                price = getIntInput();

                printf("Podaj ilość: ");
                quantity = getIntInput();

                strcpy(new_product.name, name);
                new_product.price_gr = price;
                new_product.quantity = quantity;

                addProduct(vm, new_product, quantity);
                break;
            }
            case 5: {
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
            }
            case 6: printf("Powrót do głównego menu\n"); break;
            default: printf("Nieprawidłowa opcja\n");
        }
    } while (admin_choice != 6);
}

int main() {
    // Ustawienie obsługi sygnałów
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    signal(SIGCHLD, signalHandler);
    
    // Inicjalizacja systemu
    setupPipe();
    initLogger();
    
    // Uruchomienie procesu maintenance
    startMaintenanceProcess();
    
    VendingMachine vm;
    initMachine(&vm);

    printf("=== AUTOMAT VENDINGOWY URUCHOMIONY ===\n");
    printf("Proces maintenance działa w tle (PID: %d)\n", maintenance_pid);
    // printf("Logi zapisywane do pliku: %s\n", LOG_FILE);
    
    int choice;
    while (main_running) {
        showMenu();
        choice = getIntInput();
        
        if (choice == -1) {
            // EOF - zakończ program
            main_running = 0;
            break;
        }
        
        if (choice == 0) {
            // Pusty input - pomiń
            continue;
        }

        switch(choice) {
            case 1: displayProducts(&vm); break;
            case 2: insertMoneyFromConsole(&vm); break;
            case 3: {
                printf("Wybierz numer produktu: ");
                int product_num = getIntInput();
                selectProduct(&vm, product_num);
                break;
            }
            case 4: returnChange(&vm); break;
            case 5:
                if (verifyAdminPin()) {
                    adminMenu(&vm);
                } else {
                    printf("Nieprawidłowy PIN!\n");
                }
                break;
            case 6: 
                printf("Dziękujemy!\n");
                main_running = 0;
                break;
            default: printf("Nieprawidłowa opcja\n");
        }
    }

    // Sprzątanie przed wyjściem
    cleanup();
    printf("System zakończony.\n");
    
    return 0;
}