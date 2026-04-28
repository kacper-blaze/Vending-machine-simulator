#include "vending_machine.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>

static FILE* log_file = NULL;

void initLogger() {
    log_file = fopen(LOG_FILE, "a");
    if (log_file == NULL) {
        perror("Nie można otworzyć pliku logu");
        exit(1);
    }
    
    // Ustawienie buforowania liniami dla natychmiastowego zapisu
    setvbuf(log_file, NULL, _IOLBF, 0);
    
    writeLog(LOG_SYSTEM, "System uruchomiony - logger zainicjalizowany");
}

void closeLogger() {
    if (log_file != NULL) {
        writeLog(LOG_SYSTEM, "System zamykany - logger zakończony");
        fclose(log_file);
        log_file = NULL;
    }
}

void writeLog(LogType type, const char* description) {
    if (log_file == NULL) {
        return;
    }
    
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
    
    const char* type_strings[] = {
        "TRANSACTION",
        "ALERT", 
        "ADMIN",
        "COIN",
        "PRODUCT",
        "SYSTEM"
    };
    
    fprintf(log_file, "[%s] [%s] %s\n", timestamp, type_strings[type], description);
    fflush(log_file);
}

void setupPipe() {
    // Usunięcie istniejącego potoku jeśli istnieje
    unlink(PIPE_PATH);
    
    // Utworzenie nowego potoku nazwanego
    if (mkfifo(PIPE_PATH, 0666) == -1) {
        perror("Nie można utworzyć potoku");
        exit(1);
    }
}

void sendMessageToMaintenance(const char* message) {
    // Ignorowanie SIGPIPE, aby program nie crashował
    signal(SIGPIPE, SIG_IGN);
    
    int pipe_fd = open(PIPE_PATH, O_WRONLY | O_NONBLOCK);
    if (pipe_fd != -1) {
        ssize_t written = write(pipe_fd, message, strlen(message));
        if (written > 0) {
            // newline dla łatwiejszego odczytu
            write(pipe_fd, "\n", 1);
        }
        close(pipe_fd);
    } else {
        // Jeśli potok nie jest dostępny, zapis bezpośrednio do logu
        writeLog(LOG_SYSTEM, message);
    }
    // Przywrócenie domyślnej obsługi SIGPIPE
    signal(SIGPIPE, SIG_DFL);
}
