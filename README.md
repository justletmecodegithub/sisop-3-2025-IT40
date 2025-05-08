# Soal Nomor 1
Dikerjakan oleh 5027241024

## Skrip image_server.c
## 1. Library yang Diperlukan
```c
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
```
## 3. Konstanta dan Prototipe Fungsi
```c
#define LISTEN_PORT 9090
#define MAX_BUFFER 8192
#define STORAGE_DIR "./data_store/"


void handle_connection(int client_fd);
void process_decryption(int client_fd, const char *hex_input);
void process_download(int client_fd, const char *file_name);
int convert_hex_to_bin(const char *hex_str, unsigned char **bin_data);
```

## 4. Fungsi Utama: main

```cint main() {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    signal(SIGCHLD, SIG_IGN);  // Ignore SIGCHLD to prevent zombie processes

    server_fd = socket(AF_INET, SOCK_STREAM, 0);  // Create server socket
    if (server_fd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options for address reuse
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // Bind to the specified port
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(LISTEN_PORT);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Binding failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Start listening for incoming connections
    if (listen(server_fd, 10) < 0) {
        perror("Listening failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server is operational on port %d\n", LISTEN_PORT);

    while (1) {
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0) {
            perror("Accept failed");
            continue;
        }

        pid_t pid = fork();
        if (pid == 0) {
            close(server_fd);
            handle_connection(client_fd);
            close(client_fd);
            exit(0);
        } else if (pid > 0) {
            close(client_fd);
        } else {
            perror("Fork failed");
            close(client_fd);
        }
    }

    close(server_fd);
    return 0;
}
```

## 5. Fungsi handle_connection
Fungsi ini menangani komunikasi dengan klien setelah koneksi diterima.
```c
void handle_connection(int client_fd) {
    char buffer[MAX_BUFFER];
    ssize_t bytes_received;

    while ((bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytes_received] = '\0';
        char *command = strtok(buffer, "::");
        char *argument = strtok(NULL, "::");

        if (command && argument) {
            if (strcmp(command, "DECRYPT") == 0) {
                process_decryption(client_fd, argument);
            } else if (strcmp(command, "DOWNLOAD") == 0) {
                process_download(client_fd, argument);
            } else {
                const char *msg = "Invalid command\n";
                send(client_fd, msg, strlen(msg), 0);
            }
        } else {
            const char *msg = "Malformed request\n";
            send(client_fd, msg, strlen(msg), 0);
        }
    }
}
```
## 6. Fungsi process_decryption
Fungsi ini memproses permintaan dekripsi dan menyimpan hasilnya sebagai file.

```c
void process_decryption(int client_fd, const char *hex_input) {
    unsigned char *binary_data = NULL;
    int binary_length = convert_hex_to_bin(hex_input, &binary_data);

    if (binary_length <= 0) {
        const char *msg = "Decryption failed\n";
        send(client_fd, msg, strlen(msg), 0);
        return;
    }

    time_t current_time = time(NULL);
    char file_path[256];
    snprintf(file_path, sizeof(file_path), "%s%ld_output.jpeg", STORAGE_DIR, current_time);

    int file_fd = open(file_path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (file_fd < 0) {
        perror("File creation failed");
        const char *msg = "File saving failed\n";
        send(client_fd, msg, strlen(msg), 0);
        free(binary_data);
        return;
    }

    write(file_fd, binary_data, binary_length);
    close(file_fd);
    free(binary_data);

    const char *msg = "Decryption and saving successful\n";
    send(client_fd, msg, strlen(msg), 0);
}
```
## 7. Fungsi process_download
Fungsi ini menangani permintaan pengunduhan file dari server.
```c
void process_download(int client_fd, const char *file_name) {
    char file_path[256];
    snprintf(file_path, sizeof(file_path), "%s%s", STORAGE_DIR, file_name);

    int file_fd = open(file_path, O_RDONLY);
    if (file_fd < 0) {
        const char *msg = "Requested file not found\n";
        send(client_fd, msg, strlen(msg), 0);
        return;
    }

    char buffer[MAX_BUFFER];
    ssize_t bytes_read;
    while ((bytes_read = read(file_fd, buffer, sizeof(buffer))) > 0) {
        send(client_fd, buffer, bytes_read, 0);
    }

    close(file_fd);
}
```
## 8. Fungsi convert_hex_to_bin
Fungsi ini mengonversi string hex menjadi data biner.
```c
int convert_hex_to_bin(const char *hex_str, unsigned char **bin_data) {
    size_t hex_len = strlen(hex_str);
    if (hex_len % 2 != 0) return -1;

    size_t bin_len = hex_len / 2;
    *bin_data = malloc(bin_len);
    if (!*bin_data) return -1;

    for (size_t i = 0; i < bin_len; ++i) {
        if (sscanf(hex_str + 2 * i, "%2hhx", &((*bin_data)[i])) != 1) {
            free(*bin_data);
            return -1;
        }
    }

    return bin_len;
}
```
