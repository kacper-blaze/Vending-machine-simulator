#include "vending_machine.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>

extern coin accepted_coins[];
pid_t maintenance_pid = -1;

static int maintenance_running = 1;

void maintenanceSignalHandler(int sig) {
    if (sig == SIGTERM) {
        maintenance_running = 0;
        writeLog(LOG_SYSTEM, "Proces maintenance otrzymał SIGTERM - kończenie pracy");
    }
}

void checkCoinLevels(const VendingMachine *vm) {
    const int low_coin_threshold[] = {20, 15, 10, 8, 5, 3}; // Próg niskiego stanu dla każdego nominału
    
    for (int i = 0; i < 6; i++) {
        if (vm->coin_inventory[i] <= low_coin_threshold[i]) {
            char alert_msg[200];
            sprintf(alert_msg, "ALERT: Niski stan monet %s - %d szt (próg: %d)", 
                   accepted_coins[i].key, vm->coin_inventory[i], low_coin_threshold[i]);
            writeLog(LOG_ALERT, alert_msg);
        }
    }
}

void checkProductLevels(const VendingMachine *vm) {
    const int low_product_threshold = 3;
    
    for (int i = 0; i < vm->product_count; i++) {
        if (vm->products[i].quantity <= low_product_threshold) {
            char alert_msg[200];
            sprintf(alert_msg, "ALERT: Niski stan produktu '%s' - %d szt (próg: %d)", 
                   vm->products[i].name, vm->products[i].quantity, low_product_threshold);
            writeLog(LOG_ALERT, alert_msg);
        }
    }
}

void checkHardwareStatus() {
    // Symulacja testów sprzętowych
    static int test_counter = 0;
    test_counter++;
    
    if (test_counter % 10 == 0) { // Co 10 cykli
        writeLog(LOG_SYSTEM, "Testy sprzętowe zakończone pomyślnie");
        
        // Symulacja 1% szansy na "błąd sprzętowy"
        if (rand() % 100 == 0) {
            writeLog(LOG_ALERT, "Wykryto błąd czujnika temperatury - wymaga serwisu");
        }
    }
}

void maintenanceProcess() {
    // Inicjalizacja loggera w procesie potomnym
    initLogger();
    
    // Ustawienie obsługi sygnałów
    signal(SIGTERM, maintenanceSignalHandler);
    
    writeLog(LOG_SYSTEM, "Proces maintenance uruchomiony - rozpoczynam pętlę");
    
    while (maintenance_running) {
        
        // Spróbuj otworzyć potok
        int pipe_fd = open(PIPE_PATH, O_RDONLY | O_NONBLOCK);
        if (pipe_fd != -1) {
            char message[256];
            ssize_t bytes_read = read(pipe_fd, message, sizeof(message) - 1);
            
            if (bytes_read > 0) {
                message[bytes_read] = '\0';
                message[strcspn(message, "\n")] = 0;
                
                // Przetwarzanie wiadomości
                if (strncmp(message, "TRANSACTION:", 12) == 0) {
                    writeLog(LOG_TRANSACTION, message + 12);
                } else if (strncmp(message, "ADMIN:", 6) == 0) {
                    writeLog(LOG_ADMIN, message + 6);
                } else if (strncmp(message, "COIN:", 5) == 0) {
                    writeLog(LOG_COIN, message + 5);
                } else if (strncmp(message, "PRODUCT:", 8) == 0) {
                    writeLog(LOG_PRODUCT, message + 8);
                } else {
                    writeLog(LOG_SYSTEM, message);
                }
            }
            
            close(pipe_fd);
        }
        
        sleep(1); // Sprawdzaj co 1 sekundę
    }
    
    writeLog(LOG_SYSTEM, "Proces maintenance zakończony");
    closeLogger();
}

void startMaintenanceProcess() {
    maintenance_pid = fork();
    
    if (maintenance_pid == 0) {
        // Proces potomny - proces maintenance
        maintenanceProcess();
        exit(0);
    } else if (maintenance_pid < 0) {
        perror("Nie można uruchomić procesu maintenance");
        exit(1);
    }
    // Proces nadrzędny kontynuuje normalną pracę
}
