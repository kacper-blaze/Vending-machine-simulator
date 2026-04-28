#include "vending_machine.h"
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>

// GUI Widgets
static GtkWidget *main_window;
static GtkWidget *balance_label;
static GtkWidget *product_grid;
static GtkWidget *product_buttons[MAX_PRODUCTS];
static GtkWidget *product_labels[MAX_PRODUCTS][3]; // name, price, quantity
static GtkWidget *coin_buttons[6];
static GtkWidget *status_label;
static GtkWidget *admin_dialog;
static GtkWidget *admin_pin_entry;

// Data
static VendingMachine vm;
static int main_running = 1;

// Coin data
extern coin accepted_coins[];

// Function declarations
static void update_product_display();
static void update_balance_display();
static void on_coin_clicked(GtkWidget *widget, gpointer data);
static void on_product_clicked(GtkWidget *widget, gpointer data);
static void on_return_change_clicked(GtkWidget *widget, gpointer data);
static void on_admin_clicked(GtkWidget *widget, gpointer data);
static void show_admin_dialog();
static void create_admin_dialog();
static void show_admin_panel();
static void on_admin_remove_product(GtkWidget *widget, gpointer data);
static void on_admin_change_price(GtkWidget *widget, gpointer data);
static void on_admin_change_quantity(GtkWidget *widget, gpointer data);
static void on_admin_add_product(GtkWidget *widget, gpointer data);
static void on_admin_show_coins(GtkWidget *widget, gpointer data);
static gboolean update_display(gpointer user_data);

void signalHandler(int sig) {
    switch (sig) {
        case SIGINT:
        case SIGTERM:
            main_running = 0;
            if (main_window) {
                gtk_window_close(GTK_WINDOW(main_window));
            }
            break;
        case SIGCHLD:
            int status;
            pid_t pid = waitpid(-1, &status, WNOHANG);
            if (pid == maintenance_pid) {
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

static void update_product_display() {
    char buffer[256];
    
    for (int i = 0; i < MAX_PRODUCTS; i++) {
        if (i < vm.product_count) {
            // Update product name
            snprintf(buffer, sizeof(buffer), "%s", vm.products[i].name);
            gtk_label_set_text(GTK_LABEL(product_labels[i][0]), buffer);
            
            // Update price
            snprintf(buffer, sizeof(buffer), "%.2f zł", vm.products[i].price_gr / 100.0);
            gtk_label_set_text(GTK_LABEL(product_labels[i][1]), buffer);
            
            // Update quantity
            snprintf(buffer, sizeof(buffer), "Ilość: %d", vm.products[i].quantity);
            gtk_label_set_text(GTK_LABEL(product_labels[i][2]), buffer);
            
            // Show and enable button if product is available
            gtk_widget_set_visible(product_buttons[i], TRUE);
            gtk_widget_set_sensitive(product_buttons[i], vm.products[i].quantity > 0);
        } else {
            // Hide empty slots
            gtk_widget_set_visible(product_buttons[i], FALSE);
        }
    }
}

static void update_balance_display() {
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "Saldo: %.2f zł", vm.balance_gr / 100.0);
    gtk_label_set_text(GTK_LABEL(balance_label), buffer);
}

static void on_coin_clicked(GtkWidget *widget, gpointer data) {
    int coin_index = GPOINTER_TO_INT(data);
    int amount_gr = accepted_coins[coin_index].value_gr;
    
    insertMoney(&vm, amount_gr);
    update_balance_display();
    update_product_display();
    
    char status_msg[256];
    snprintf(status_msg, sizeof(status_msg), "Przyjęto monetę: %s", accepted_coins[coin_index].key);
    gtk_label_set_text(GTK_LABEL(status_label), status_msg);
}

static void on_product_clicked(GtkWidget *widget, gpointer data) {
    int product_index = GPOINTER_TO_INT(data) + 1; // Convert to 1-based index
    
    char status_msg[256];
    
    if (product_index < 1 || product_index > vm.product_count) {
        snprintf(status_msg, sizeof(status_msg), "Nieprawidłowy produkt");
        gtk_label_set_text(GTK_LABEL(status_label), status_msg);
        return;
    }
    
    Product *product = &vm.products[product_index - 1];
    
    if (product->quantity == 0) {
        snprintf(status_msg, sizeof(status_msg), "Produkt niedostępny: %s", product->name);
        gtk_label_set_text(GTK_LABEL(status_label), status_msg);
        return;
    }
    
    if (vm.balance_gr < product->price_gr) {
        snprintf(status_msg, sizeof(status_msg), 
                "Niewystarczająca kwota. Brakuje: %.2f zł", 
                (product->price_gr - vm.balance_gr) / 100.0);
        gtk_label_set_text(GTK_LABEL(status_label), status_msg);
        return;
    }
    
    // Process purchase via core function (handles logging and auto change return)
    selectProduct(&vm, product_index);
    
    snprintf(status_msg, sizeof(status_msg), 
            "Zakupiono: %s", 
            product->name);
    gtk_label_set_text(GTK_LABEL(status_label), status_msg);
    
    update_balance_display();
    update_product_display();
}

static void on_return_change_clicked(GtkWidget *widget, gpointer data) {
    (void)widget;
    (void)data;
    
    if (vm.balance_gr == 0) {
        gtk_label_set_text(GTK_LABEL(status_label), "Brak reszty do zwrotu");
        return;
    }
    
    returnChange(&vm);
    
    gtk_label_set_text(GTK_LABEL(status_label), "Zwrócono resztę");
    update_balance_display();
    update_product_display();
}

static void on_admin_clicked(GtkWidget *widget, gpointer data) {
    show_admin_dialog();
}

static void create_admin_dialog() {
    admin_dialog = gtk_dialog_new_with_buttons("Panel Admina", 
                                               GTK_WINDOW(main_window),
                                               GTK_DIALOG_MODAL,
                                               "Anuluj", GTK_RESPONSE_CANCEL,
                                               "OK", GTK_RESPONSE_OK,
                                               NULL);
    
    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(admin_dialog));
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    gtk_container_set_border_width(GTK_CONTAINER(grid), 10);
    gtk_container_add(GTK_CONTAINER(content_area), grid);
    
    // PIN entry
    GtkWidget *pin_label = gtk_label_new("Podaj PIN admina:");
    gtk_grid_attach(GTK_GRID(grid), pin_label, 0, 0, 1, 1);
    
    admin_pin_entry = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(admin_pin_entry), FALSE);
    gtk_entry_set_max_length(GTK_ENTRY(admin_pin_entry), 4);
    gtk_grid_attach(GTK_GRID(grid), admin_pin_entry, 1, 0, 1, 1);
    
    gtk_widget_show_all(admin_dialog);
}

static void show_admin_dialog() {
    create_admin_dialog();
    
    gint response = gtk_dialog_run(GTK_DIALOG(admin_dialog));
    
    if (response == GTK_RESPONSE_OK) {
        const char *pin = gtk_entry_get_text(GTK_ENTRY(admin_pin_entry));
        
        if (strcmp(pin, "1111") == 0) {
            gtk_label_set_text(GTK_LABEL(status_label), "Dostęp admina udzielony");
            
            char log_msg[200];
            sprintf(log_msg, "Próba dostępu admina: sukces");
            writeLog(LOG_ADMIN, log_msg);
            
            show_admin_panel();
        } else {
            gtk_label_set_text(GTK_LABEL(status_label), "Nieprawidłowy PIN!");
            
            char log_msg[200];
            sprintf(log_msg, "Próba dostępu admina: niepowodzenie");
            writeLog(LOG_ADMIN, log_msg);
        }
    }
    
    gtk_widget_destroy(admin_dialog);
    admin_dialog = NULL;
}

static void show_admin_panel() {
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Panel Admina",
                                                    GTK_WINDOW(main_window),
                                                    GTK_DIALOG_MODAL,
                                                    "Zamknij", GTK_RESPONSE_CLOSE,
                                                    NULL);
    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(box), 10);
    gtk_container_add(GTK_CONTAINER(content), box);

    GtkWidget *btn_remove = gtk_button_new_with_label("Usuń produkt");
    g_signal_connect(btn_remove, "clicked", G_CALLBACK(on_admin_remove_product), dialog);
    gtk_box_pack_start(GTK_BOX(box), btn_remove, FALSE, FALSE, 0);

    GtkWidget *btn_price = gtk_button_new_with_label("Zmień cenę produktu");
    g_signal_connect(btn_price, "clicked", G_CALLBACK(on_admin_change_price), dialog);
    gtk_box_pack_start(GTK_BOX(box), btn_price, FALSE, FALSE, 0);

    GtkWidget *btn_qty = gtk_button_new_with_label("Zmień ilość produktu");
    g_signal_connect(btn_qty, "clicked", G_CALLBACK(on_admin_change_quantity), dialog);
    gtk_box_pack_start(GTK_BOX(box), btn_qty, FALSE, FALSE, 0);

    GtkWidget *btn_add = gtk_button_new_with_label("Dodaj nowy produkt");
    g_signal_connect(btn_add, "clicked", G_CALLBACK(on_admin_add_product), dialog);
    gtk_box_pack_start(GTK_BOX(box), btn_add, FALSE, FALSE, 0);

    GtkWidget *btn_coins = gtk_button_new_with_label("Pokaż stan monet");
    g_signal_connect(btn_coins, "clicked", G_CALLBACK(on_admin_show_coins), dialog);
    gtk_box_pack_start(GTK_BOX(box), btn_coins, FALSE, FALSE, 0);

    gtk_widget_show_all(dialog);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

static void on_admin_remove_product(GtkWidget *widget, gpointer data) {
    (void)widget;
    GtkWidget *parent = GTK_WIDGET(data);

    GtkWidget *dialog = gtk_dialog_new_with_buttons("Usuń produkt",
                                                    GTK_WINDOW(parent),
                                                    GTK_DIALOG_MODAL,
                                                    "Anuluj", GTK_RESPONSE_CANCEL,
                                                    "OK", GTK_RESPONSE_OK,
                                                    NULL);
    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 5);
    gtk_container_set_border_width(GTK_CONTAINER(grid), 10);
    gtk_container_add(GTK_CONTAINER(content), grid);

    GtkWidget *label = gtk_label_new("Numer produktu:");
    gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 1, 1);

    GtkWidget *entry = gtk_entry_new();
    gtk_grid_attach(GTK_GRID(grid), entry, 1, 0, 1, 1);

    gtk_widget_show_all(dialog);
    gint response = gtk_dialog_run(GTK_DIALOG(dialog));
    if (response == GTK_RESPONSE_OK) {
        const char *text = gtk_entry_get_text(GTK_ENTRY(entry));
        int index = atoi(text);
        if (index >= 1 && index <= vm.product_count) {
            char log_msg[200];
            sprintf(log_msg, "Admin usunął produkt nr %d (%s)", index, vm.products[index-1].name);
            writeLog(LOG_ADMIN, log_msg);
            removeProduct(&vm, index);
            update_product_display();
            gtk_label_set_text(GTK_LABEL(status_label), "Produkt usunięty");
        } else {
            gtk_label_set_text(GTK_LABEL(status_label), "Nieprawidłowy numer produktu");
        }
    }
    gtk_widget_destroy(dialog);
}

static void on_admin_change_price(GtkWidget *widget, gpointer data) {
    (void)widget;
    GtkWidget *parent = GTK_WIDGET(data);

    GtkWidget *dialog = gtk_dialog_new_with_buttons("Zmień cenę produktu",
                                                    GTK_WINDOW(parent),
                                                    GTK_DIALOG_MODAL,
                                                    "Anuluj", GTK_RESPONSE_CANCEL,
                                                    "OK", GTK_RESPONSE_OK,
                                                    NULL);
    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 5);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 5);
    gtk_container_set_border_width(GTK_CONTAINER(grid), 10);
    gtk_container_add(GTK_CONTAINER(content), grid);

    GtkWidget *label1 = gtk_label_new("Numer produktu:");
    gtk_grid_attach(GTK_GRID(grid), label1, 0, 0, 1, 1);
    GtkWidget *entry_idx = gtk_entry_new();
    gtk_grid_attach(GTK_GRID(grid), entry_idx, 1, 0, 1, 1);

    GtkWidget *label2 = gtk_label_new("Nowa cena (gr):");
    gtk_grid_attach(GTK_GRID(grid), label2, 0, 1, 1, 1);
    GtkWidget *entry_price = gtk_entry_new();
    gtk_grid_attach(GTK_GRID(grid), entry_price, 1, 1, 1, 1);

    gtk_widget_show_all(dialog);
    gint response = gtk_dialog_run(GTK_DIALOG(dialog));
    if (response == GTK_RESPONSE_OK) {
        int index = atoi(gtk_entry_get_text(GTK_ENTRY(entry_idx)));
        int price = atoi(gtk_entry_get_text(GTK_ENTRY(entry_price)));
        if (index >= 1 && index <= vm.product_count && price > 0) {
            char log_msg[200];
            sprintf(log_msg, "Admin zmienił cenę produktu nr %d na %d gr", index, price);
            writeLog(LOG_ADMIN, log_msg);
            updateProductPrice(&vm, index, price);
            update_product_display();
            gtk_label_set_text(GTK_LABEL(status_label), "Cena zmieniona");
        } else {
            gtk_label_set_text(GTK_LABEL(status_label), "Nieprawidłowe dane");
        }
    }
    gtk_widget_destroy(dialog);
}

static void on_admin_change_quantity(GtkWidget *widget, gpointer data) {
    (void)widget;
    GtkWidget *parent = GTK_WIDGET(data);

    GtkWidget *dialog = gtk_dialog_new_with_buttons("Zmień ilość produktu",
                                                    GTK_WINDOW(parent),
                                                    GTK_DIALOG_MODAL,
                                                    "Anuluj", GTK_RESPONSE_CANCEL,
                                                    "OK", GTK_RESPONSE_OK,
                                                    NULL);
    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 5);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 5);
    gtk_container_set_border_width(GTK_CONTAINER(grid), 10);
    gtk_container_add(GTK_CONTAINER(content), grid);

    GtkWidget *label1 = gtk_label_new("Numer produktu:");
    gtk_grid_attach(GTK_GRID(grid), label1, 0, 0, 1, 1);
    GtkWidget *entry_idx = gtk_entry_new();
    gtk_grid_attach(GTK_GRID(grid), entry_idx, 1, 0, 1, 1);

    GtkWidget *label2 = gtk_label_new("Nowa ilość:");
    gtk_grid_attach(GTK_GRID(grid), label2, 0, 1, 1, 1);
    GtkWidget *entry_qty = gtk_entry_new();
    gtk_grid_attach(GTK_GRID(grid), entry_qty, 1, 1, 1, 1);

    gtk_widget_show_all(dialog);
    gint response = gtk_dialog_run(GTK_DIALOG(dialog));
    if (response == GTK_RESPONSE_OK) {
        int index = atoi(gtk_entry_get_text(GTK_ENTRY(entry_idx)));
        int qty = atoi(gtk_entry_get_text(GTK_ENTRY(entry_qty)));
        if (index >= 1 && index <= vm.product_count && qty >= 0 && qty <= MAX_QUANTITY) {
            char log_msg[200];
            sprintf(log_msg, "Admin zmienił ilość produktu nr %d na %d", index, qty);
            writeLog(LOG_ADMIN, log_msg);
            updateProductQuantity(&vm, index, qty);
            update_product_display();
            gtk_label_set_text(GTK_LABEL(status_label), "Ilość zmieniona");
        } else {
            gtk_label_set_text(GTK_LABEL(status_label), "Nieprawidłowe dane");
        }
    }
    gtk_widget_destroy(dialog);
}

static void on_admin_add_product(GtkWidget *widget, gpointer data) {
    (void)widget;
    GtkWidget *parent = GTK_WIDGET(data);

    if (vm.product_count >= MAX_PRODUCTS) {
        GtkWidget *msg = gtk_message_dialog_new(GTK_WINDOW(parent),
                                                GTK_DIALOG_MODAL,
                                                GTK_MESSAGE_INFO,
                                                GTK_BUTTONS_OK,
                                                "Automat jest pełny. Nie można dodać więcej produktów.");
        gtk_dialog_run(GTK_DIALOG(msg));
        gtk_widget_destroy(msg);
        return;
    }

    GtkWidget *dialog = gtk_dialog_new_with_buttons("Dodaj nowy produkt",
                                                    GTK_WINDOW(parent),
                                                    GTK_DIALOG_MODAL,
                                                    "Anuluj", GTK_RESPONSE_CANCEL,
                                                    "OK", GTK_RESPONSE_OK,
                                                    NULL);
    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 5);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 5);
    gtk_container_set_border_width(GTK_CONTAINER(grid), 10);
    gtk_container_add(GTK_CONTAINER(content), grid);

    GtkWidget *label1 = gtk_label_new("Nazwa:");
    gtk_grid_attach(GTK_GRID(grid), label1, 0, 0, 1, 1);
    GtkWidget *entry_name = gtk_entry_new();
    gtk_grid_attach(GTK_GRID(grid), entry_name, 1, 0, 1, 1);

    GtkWidget *label2 = gtk_label_new("Cena (gr):");
    gtk_grid_attach(GTK_GRID(grid), label2, 0, 1, 1, 1);
    GtkWidget *entry_price = gtk_entry_new();
    gtk_grid_attach(GTK_GRID(grid), entry_price, 1, 1, 1, 1);

    GtkWidget *label3 = gtk_label_new("Ilość:");
    gtk_grid_attach(GTK_GRID(grid), label3, 0, 2, 1, 1);
    GtkWidget *entry_qty = gtk_entry_new();
    gtk_grid_attach(GTK_GRID(grid), entry_qty, 1, 2, 1, 1);

    gtk_widget_show_all(dialog);
    gint response = gtk_dialog_run(GTK_DIALOG(dialog));
    if (response == GTK_RESPONSE_OK) {
        const char *name = gtk_entry_get_text(GTK_ENTRY(entry_name));
        int price = atoi(gtk_entry_get_text(GTK_ENTRY(entry_price)));
        int qty = atoi(gtk_entry_get_text(GTK_ENTRY(entry_qty)));

        if (strlen(name) > 0 && price > 0 && qty > 0 && qty <= MAX_QUANTITY) {
            Product new_product;
            strcpy(new_product.name, name);
            new_product.price_gr = price;
            new_product.quantity = qty;
            addProduct(&vm, new_product, qty);
            update_product_display();
            gtk_label_set_text(GTK_LABEL(status_label), "Produkt dodany");
        } else {
            gtk_label_set_text(GTK_LABEL(status_label), "Nieprawidłowe dane");
        }
    }
    gtk_widget_destroy(dialog);
}

static void on_admin_show_coins(GtkWidget *widget, gpointer data) {
    (void)widget;
    GtkWidget *parent = GTK_WIDGET(data);

    char msg[512] = "Stan monet w automacie:\n";
    char line[64];
    int total_value_gr = 0;

    for (int i = 0; i < 6; i++) {
        snprintf(line, sizeof(line), "%s: %d szt (%.2f zł)\n",
                 accepted_coins[i].key,
                 vm.coin_inventory[i],
                 (vm.coin_inventory[i] * accepted_coins[i].value_gr) / 100.0);
        strcat(msg, line);
        total_value_gr += vm.coin_inventory[i] * accepted_coins[i].value_gr;
    }

    snprintf(line, sizeof(line), "\nCałkowita wartość: %.2f zł", total_value_gr / 100.0);
    strcat(msg, line);

    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(parent),
                                             GTK_DIALOG_MODAL,
                                             GTK_MESSAGE_INFO,
                                             GTK_BUTTONS_OK,
                                             "%s", msg);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

static gboolean update_display(gpointer user_data) {
    update_product_display();
    update_balance_display();
    return TRUE; // Continue calling this function
}

static void activate(GtkApplication* app, gpointer user_data) {
    // Initialize main window
    main_window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(main_window), "Automat Vendingowy");
    gtk_window_set_default_size(GTK_WINDOW(main_window), 800, 600);
    gtk_window_set_resizable(GTK_WINDOW(main_window), FALSE);
    
    // Main container
    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(main_box), 10);
    gtk_container_add(GTK_CONTAINER(main_window), main_box);
    
    // Title
    GtkWidget *title_label = gtk_label_new("=== AUTOMAT VENDINGOWY ===");
    gtk_widget_set_name(title_label, "title");
    gtk_box_pack_start(GTK_BOX(main_box), title_label, FALSE, FALSE, 0);
    
    // Balance display
    balance_label = gtk_label_new("Saldo: 0.00 zł");
    gtk_widget_set_name(balance_label, "balance");
    gtk_box_pack_start(GTK_BOX(main_box), balance_label, FALSE, FALSE, 0);
    
    // Product grid
    product_grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(product_grid), 5);
    gtk_grid_set_column_spacing(GTK_GRID(product_grid), 10);
    gtk_box_pack_start(GTK_BOX(main_box), product_grid, TRUE, TRUE, 0);
    
    // Create product buttons and labels
    for (int i = 0; i < MAX_PRODUCTS; i++) {
        GtkWidget *button = gtk_button_new();
        gtk_widget_set_size_request(button, 200, 100);
        g_signal_connect(button, "clicked", G_CALLBACK(on_product_clicked), GINT_TO_POINTER(i));
        
        GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
        gtk_container_add(GTK_CONTAINER(button), box);
        
        product_labels[i][0] = gtk_label_new(""); // Name
        product_labels[i][1] = gtk_label_new(""); // Price
        product_labels[i][2] = gtk_label_new(""); // Quantity
        
        gtk_box_pack_start(GTK_BOX(box), product_labels[i][0], FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(box), product_labels[i][1], FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(box), product_labels[i][2], FALSE, FALSE, 0);
        
        product_buttons[i] = button;
        
        int row = i / 5;
        int col = i % 5;
        gtk_grid_attach(GTK_GRID(product_grid), button, col, row, 1, 1);
    }
    
    // Coin buttons
    GtkWidget *coin_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(main_box), coin_box, FALSE, FALSE, 0);
    
    for (int i = 0; i < 6; i++) {
        char button_text[32];
        snprintf(button_text, sizeof(button_text), "%s", accepted_coins[i].key);
        coin_buttons[i] = gtk_button_new_with_label(button_text);
        g_signal_connect(coin_buttons[i], "clicked", G_CALLBACK(on_coin_clicked), GINT_TO_POINTER(i));
        gtk_box_pack_start(GTK_BOX(coin_box), coin_buttons[i], TRUE, TRUE, 0);
    }
    
    // Control buttons
    GtkWidget *control_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(main_box), control_box, FALSE, FALSE, 0);
    
    GtkWidget *return_button = gtk_button_new_with_label("Zwróć resztę");
    g_signal_connect(return_button, "clicked", G_CALLBACK(on_return_change_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(control_box), return_button, TRUE, TRUE, 0);
    
    GtkWidget *admin_button = gtk_button_new_with_label("Panel Admina");
    g_signal_connect(admin_button, "clicked", G_CALLBACK(on_admin_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(control_box), admin_button, TRUE, TRUE, 0);
    
    // Status label
    status_label = gtk_label_new("Witaj! Wybierz produkt lub wrzuć monetę");
    gtk_box_pack_start(GTK_BOX(main_box), status_label, FALSE, FALSE, 0);
    
    // Initialize display
    update_product_display();
    update_balance_display();
    
    // Set up periodic update
    g_timeout_add(1000, update_display, NULL);
    
    gtk_widget_show_all(main_window);
}

int main(int argc, char **argv) {
    // Set up signal handlers
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    signal(SIGCHLD, signalHandler);
    
    // Initialize system
    setupPipe();
    initLogger();
    startMaintenanceProcess();
    initMachine(&vm);
    
    // Create GTK application
    GtkApplication *app = gtk_application_new("com.example.vendingmachine", G_APPLICATION_FLAGS_NONE);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    
    // Cleanup
    cleanup();
    main_running = 0;
    
    return status;
}
