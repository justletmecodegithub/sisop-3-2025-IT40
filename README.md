# Soal Nomor 1
Repository ini dibuat untuk menyelesaikan soal 1 shift Modul 3 Sistem Operasi 2025. Di sini kami membangun sistem client-server sederhana yang bisa mengubah file teks terenkripsi jadi file JPEG menggunakan socket. Program client punya menu interaktif, dan server-nya berjalan di background sebagai daemon.
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

```c
int main() {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    signal(SIGCHLD, SIG_IGN);  

    server_fd = socket(AF_INET, SOCK_STREAM, 0);  
    if (server_fd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    
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
# Skrip image_client.c

## Library
```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
```
```c
## Konstanta
#define SERVER_IP "127.0.0.1"
#define PORT 9090
#define BUFFER_SIZE 8192
#define SECRET_PATH "client/secrets/"
#define OUTPUT_PATH "client/"
```
```
## Fungsi reverse string
void reverse(char *str) {
    int len = strlen(str);
    for (int i = 0; i < len / 2; i++) {
        char tmp = str[i];
        str[i] = str[len - 1 - i];
        str[len - 1 - i] = tmp;
    }
}
```

## Menu
```c
void show_menu() {
    printf("															                                                                                        		░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░\n");
    printf("                                                                                                                                                            ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░\n");
    printf("                                                                                    .*((/(//(((((((((((((((##(*.                                            ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░\n");
    printf("                                                                             ,((///////////(((((((((((((((((((((####(,                                      ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░\n");
    printf("                                                                      *((((/////////////((((((((((((((((((((((((########(.                                  ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░\n");
    printf("                                                                 /(///////*/******//////((((((((//(((((((((################/                                ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░\n");
    printf("                                                             ,(/////////*********////////(((((//(((((((((((###################                              ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░\n");
    printf("                                                           *////////***********///////((((((((((((((((((((######################                            ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░\n");
    printf("                                                         *///////********/**///////**/(///((((((((((((((##########################                          ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░\n");
    printf("                                                       .///////**/*****/////////*/*////((((((((((((((((#########################%##/                        ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░\n");
    printf("                                                      *//////***////**///////////////////((((((((((((###########################%%##%.                      ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░\n");
    printf("                                                     *////////*////////////*////*////////(((((((((((#############%%%%%%#####%%########/                     ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░\n");
    printf("                                                    /////////////////////////*///////////(((((((((#############%%%%%%%%%%%%%%%##%%%%%###                    ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░\n");
    printf("                                                  .///***/***///**/**////*////////**////(((((((############%%%%%%%%%%%%%%%%%%%%%%%%%%%%#%                   ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░\n");
    printf("                                                 ,/******************///**//////******//(((((##((#######%%####%%%%%%%%%%%%%%%%%%%%%%%%%%%*                  ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░\n");
    printf("                                                ,//*******************//////////******//((((((((((########%%%%###%%%%%%%%%%%%%%%%%%%%%%%%%                  ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░\n");
    printf("                                               ,//*********************//////**//**///////((((((((((########%%%%%%%##%%%%%%%%%%%%%%%%%%%%%/                 ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░\n");
    printf("                                               .*/************************/////////*///////#####(((((((#####%%#%##%%%%%%##%%%%%%%%%%%%%%%%%%                ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░\n"); 
    printf("                                              (/***********************//**///***/(((         %%%#%%%########%%%%%%%%%%%%##%%%%#%%%%%%%%%%%*                ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░\n");
    printf("                                             //*********************/******/***//(((            #%%%%%%%%%####%%%%%%%%#####%%%####%%%%%%%%%#                ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░\n");
    printf("                                   //(/*(#, ,#%/****,***************************//    (((((((      %%%&&&&%%%%%%%%%%%%%%%###%#####%%%%%%%%%%                ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░\n");
    printf("                                  *##****/(##%(****************************,,***   (((((########     ##%%%&&&%%%%%%%%%%%%%##%%###%%%%%%%%%%%                ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░\n");
    printf("                                 ,#%%/*,.,/(((//*****************************,*//((##%%%&&%&&&&&&&%##   ((#%%&&&&&%%%%%%%&%%%%%%#%%%%%%%%%%%                ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░\n");
    printf("                                 /#%&/,,*//,*//******************//(/*****,**/((##%%&%##((((((#%%%&&@@@   %###%&%&%%%%%&&&%%%&%%%%&&%&&&&&%%                ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░\n");
    printf("                                 /%%(**///(,,**,,****************/*******///((#%%&%#/*/(   0000   %%%&&@@@   &&##%%%&&&&&%###############&&&&               ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░\n");  
    printf("                                 *#/*//**(/****,,**********************///((##%%%&%##(,   000000   &&&&&@@@@&&%###%%&&%(/(# ==%%%%%%%%%%=&&&%               ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░\n");  
    printf("                                 .(/####%%%(***,,*********************//(((######%(//(/    000000    ###&@@@@&&%(((##(//(#%&@@  0000 %%%%&&&&               ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░\n");  
    printf("                                  /#%%&&&&#****,,**,*,,,***,********////(((######((/,,(     0000     %&@@@&@@&&&%(//////(#%&@@@  00 @@&&&&&&                ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░ \n");
    printf("                                  /##%%%&#**//,,**,,**,,,,,********/////(((((##((((((((((        #&&&&&@@&&&&%##(/////(#%&&@@@@@@@@@@@@@@@@*                ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░ \n");
    printf("                                  /**,*/(##,**,***************,**//////(((((((((((((((#####%%%%&&&&&&&&&&%%#((((/**//(#%%@@@@@@@@@@@@&&&&#                  ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░\n");
    printf("                                  *(,,,*/(%,,,,,,****************////((((((((((((((((((####%%%%%&&&&&%%####((//*****/(###&@@@@@@@@@@@@&@/                   ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░\n");
    printf("                                   *,,*,,,,**,,,,,************,***///((((((((((((((((#####%%%%%%%%%#((((((((///****///(##%@@@@@@@@@@@@@                     ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░\n");
    printf("                                    *,,,,*/***,**************,,,,,///((((###############%%%%%%##((((((((((((////****//(##%&&&@@@@@@@&&                      ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░\n");
    printf("                                     **,,***(/,**********/**,,,,,**/((#######################((((((((((((((////****///((#%&&&&&&&&&&&#                      ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░\n");
    printf("                                      .***/(#/************/****,,*//(((############%#############((((/(((//***********(##%%%&&&&&&&&&#                      ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░\n");
    printf("                                       .***/*********************//((((##########################(((///***************((##%&%&&&&&&&%%                      ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░\n");
    printf("                                       ****/**********************/(((##########(######((#######(((//********,,****/*/((##%%%%&&&&&&&#                      ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░\n");
    printf("                                       ****/**************,*****//((((##%#######(((##(((((######(((/****/////*,****///(###%%&&&&&&&&&.                      ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░\n");
    printf("                                      ,****//****************,***///((######((((((((((((((#######(///***///((/****////((###%&&&&&&&&*                       ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░\n");
    printf("                                      ******/*************,,*,****//(((((##(((((////////((((((##(/**,,,**///*******//(((######&&&&@,                        ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░\n");
    printf("                                      ******////***,*,******,*,***////(/(((((((////*///*/////((#(*,,,,,*///////**//((####%%%%%%&@&                          ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░\n");
    printf("                                      *******///******,*,,,,******//////**/////***/*/******//((##(**/((#%&&&&#(///((##%&&&&&&&&@.                           ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░\n");
    printf("                                      *********/******,,,,,,*****////(********************////(((%%#(%&&&##(((#%(/(##%%&&&&&@&#                             ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░\n");
    printf("                                      ********//*******,,*,,,****///(***,*****//,,,,**/**/////(((#%%%%&%%##((#%%#(#%%&&&&&&&&/                              ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░\n");
    printf("                                     .**************,,,,,,,,,***////***,**,*//,,,,,*****/////((######%%%&%%%%%&&@@@@@&&&&%&&(                               ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░\n");
    printf("                                     /***************,,,,,,,,****///*,,,*****,,,,*****//(((((((######%%%&&&%%%&@@@@@@&&&&%%%                                ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░\n");
    printf("                                   ,**********//*****,,,,,,,,***///***,,,,,,,,,,**,**///((((((###%%%%%%%&&&%&&&@@@@&&&&&&&&                                 ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░\n");
    printf("                                 ./********///////****,,,,,,**/*/****,,,,,,,,,*****//((###((#(##%##%%%%%%%%%%&&&&&&&&&&&&&,                                 ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░\n");
    printf("                                ****/******/////////**********/***,,,,,,,,,,,*////((#%%%&&%%%%%%%&&___-----------&&@&&&&@.                                  ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░\n");
    printf("                              */**********///////////***********/**,,,,,,,,*//(####%%%%%%%&&&&&%##-===============@@@@@&                                    ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░\n");
    printf("                            /***/*********///////////////******/**,*,,*,,**(((######(((((((###%%===%%&&&&&&&&&&&@@@@&&%                                     ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░\n");
    printf("                         */****/*********///*///////////*//*******,,,,****/(######(((/(((####((####%%%&&&&&&&&&&&&&&&&#/.                                   ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░\n");
    printf("                     .//////*///**********//////(/////(///*////*****,*****((#####((///(((##%%%%%%%%%%%&&&&&&&&&&&&&&&&%(%%%%%#,                             ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░\n");
    printf("                 .////**/*/////***********///////(((/////((//////*******//(((###(#((//((##%%%%%%%&&&@@@@@@@@@&&&&&&&%%&&##%#%##%&%(                         ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░\n");
    printf("             .*/*/***///*//((/**********//////////((///((((((((((///*****//(((######((###%#####%%%%&&&&&&&&&&&&&&&&&&&&&&%%%%%##%%%%%%(                     ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░\n");
    printf("         .*/*********/////((//**/*******/////////((//(((((((#####((/////**//(/(#(#(((((((((#######%%%&&&&&&&&&&&&&&&&&&&&@(%%%%##%%%%%%%%%                  ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░\n");
    printf("     */************///(((##((////*//***//**/*////((((((((########(((((////****/((((((((///((####%%%%%%%&&&&&&&&&&&&&&&&&@@#(&&###%%%%%#%%%%%&*              ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░\n");
    printf(" */ ,/**********///((((#####(//////////////***////(((((#(####%%##(/((((((///*////(#((#(//(####%%%%%%%&&&&&&&&&&&&&&&&@@@@@%((&%##%%%%&&%%#%%#%%%%%%(,       ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░\n");
    printf(".///**** */*******//(((###%%%%##((((((//////**///////((((((#%#%##%%%#(((((###%%#(((///(##((##(####%%%%%%&&&&&&&&&&&&&&&&&&@@@@&(((%%#%%%%%%%%%%%#%%%%%%%%%%%%&%#░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░\n");
    printf("********* /(/**///((###%%%%%&%%%####(///////**////////((((####%%&&&%%#((####%%%%&@&#((((#####(#%#%%%&&&&&&&&&&&&&&&&&&&&&&@@@@&#(((##%%&%%%%%%%%%%%%%%%%%%%&&%%%░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░\n");
    printf("*********../((((((###%%%%&&&&&&%%###((////*////////(((((###%%%%%&&&&&%###%%%%&&&@@@@@@&#########%%%&&&&&&&&&&&&&&&&&&&&&&&@@@@&#((((#%%%%%%%%%%%%%%%%%###%%%%%%&░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░\n");
    printf("**********../#####%%%%%&&&&&&&%%%%#((((///////////((((####%%%%%&&&&&&&&&&%%&&&@@@@@@@@@@@@@&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&@@@&#(((/(##%%%%%%%%%%%%#%%%%%%%%%%%%░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░\n");
    printf("***/*****// ,/##%%%%%%&&&&%%%%%%%##(((((///////////((####%%%%%&&&&&&@@@@@&&@@@@@@@@@@@@@@&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&@%#(((/(/###%%#%%%%%%%%##%%%%%%%%%%░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░\n");
    printf("*/*///////(( ,*#%%%%%&&%%%%%%%%%##(/((((((///////((((####%%%%%&&&&&@@@@@@@@@@@@@@@@@@@@@&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&@%#(((((//###%%%###%%%%###%%%%%%%%%░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░\n");
    printf("\n+---------------------------------------+\n");
    printf("|         IMAGE CLIENT MENU             |\n");
    printf("+---------------------------------------+\n");
    printf("| 1. Kirim File untuk diubah jadi JPEG  |\n");
    printf("| 2. Download file JPEG dari server     |\n");
    printf("| 3. Keluar                             |\n");
    printf("+---------------------------------------+\n");
    printf("Pilih menu: ");
}
```

## Koneksi ke server
```c
int connect_to_server() {
    int sock;
    struct sockaddr_in serv_addr;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Gagal membuat socket");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    serv_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Gagal terhubung ke server");
        close(sock);
        return -1;
    }
    return sock;
}
```

## Kirim file untuk didekripsi
```c
void kirim_file() {
    char namafile[100];
    printf("Masukkan nama file di folder secrets (contoh: input_1.txt): ");
    scanf("%s", namafile);

    char filepath[256];
    snprintf(filepath, sizeof(filepath), "%s%s", SECRET_PATH, namafile);

    FILE *fp = fopen(filepath, "r");
    if (!fp) {
        printf("File tidak ditemukan!\n");
        return;
    }

    char teks[BUFFER_SIZE] = {0};
    fread(teks, 1, sizeof(teks), fp);
    fclose(fp);

    // Reverse teks sebelum dikirim
    reverse(teks);

    int sock = connect_to_server();
    if (sock == -1) return;

    char message[BUFFER_SIZE * 2];
    snprintf(message, sizeof(message), "DECRYPT::%s", teks);
    send(sock, message, strlen(message), 0);

    char respon[BUFFER_SIZE] = {0};
    int len = recv(sock, respon, sizeof(respon) - 1, 0);
    if (len > 0) {
        respon[len] = '\0';
        printf("Respon dari server: %s\n", respon);
    } else {
        printf("Tidak ada respon dari server.\n");
    }

    close(sock);
}
```
## Download file JPEG
```c
void download_file() {
    char filename[100];
    printf("Masukkan nama file JPEG (contoh: 1744401234.jpeg): ");
    scanf("%s", filename);

    int sock = connect_to_server();
    if (sock == -1) return;

    char request[256];
    snprintf(request, sizeof(request), "DOWNLOAD::%s", filename);
    send(sock, request, strlen(request), 0);

    unsigned char data[BUFFER_SIZE] = {0};
    int received = recv(sock, data, sizeof(data), 0);
    if (received <= 0) {
        printf("Gagal download atau file tidak ditemukan.\n");
    } else {
        char path[256];
        snprintf(path, sizeof(path), "%s%s", OUTPUT_PATH, filename);
        FILE *out = fopen(path, "wb");
        if (!out) {
            printf("Gagal menyimpan file.\n");
        } else {
            fwrite(data, 1, received, out);
            fclose(out);
            printf("File disimpan sebagai %s\n", path);
        }
    }

    close(sock);
}
```


## Main Fucnttion
```c
int main() {
    int pilih;
    while (1) {
        show_menu();
        scanf("%d", &pilih);
        getchar(); // Untuk konsumsi newline

        if (pilih == 1) {
            kirim_file();
        } else if (pilih == 2) {
            download_file();
        } else if (pilih == 3) {
            printf("Keluar dari program.\n");
            break;
        } else {
            printf("Pilihan tidak valid!\n");
        }
    }
    return 0;
}

```
Semua fitur utama seperti dekripsi, download file JPEG, logging, dan penanganan error sudah kami coba implementasikan sesuai permintaan soal. Kalau ada kekurangan, semoga masih bisa dimaklumi. Terima kasih sudah membaca!

# Soal Nomer 2 
dikerjakan oleh 5027241024

### Penjelasan Umum
Sistem ini terdiri dari dua program utama: `delivery_agent.c` (untuk pengiriman Express) dan `dispatcher.c` (untuk manajemen pesanan oleh pengguna). Sistem menggunakan **shared memory** untuk menyimpan data pesanan dari file CSV dan mengelola pengiriman otomatis (Express) maupun manual (Reguler).

---

### Struktur Kode
- **delivery_agent.c**: Menangani pengiriman otomatis oleh 3 agen (A, B, C) untuk pesanan bertipe Express.
- **dispatcher.c**: Program utama untuk:
  - Mengunduh dan memuat data pesanan ke shared memory.
  - Menjalankan perintah pengiriman Reguler, pengecekan status, dan daftar pesanan.
  - Mencatat log pengiriman.

---

### Spesifikasi Solusi
#### a. Mengunduh File CSV dan Shared Memory
- **Mekanisme**:  
  - `dispatcher.c` mengunduh file `delivery_order.csv` menggunakan `curl`.  
  - Data CSV dimuat ke shared memory dengan key `1234` untuk diakses oleh `delivery_agent.c` dan `dispatcher.c`.  
  - Setiap pesanan disimpan dalam struktur `Pesanan` (nama, alamat, tipe, status).  

#### b. Pengiriman Express oleh Agen
- **Mekanisme**:  
  - Tiga thread (Agen A, B, C) dijalankan oleh `delivery_agent.c`.  
  - Setiap agen mencari pesanan Express dengan status **"Pending"** secara atomic (menggunakan `pthread_mutex`).  
  - Jika ditemukan, status diubah menjadi nama agen (misal: "AGENT A"), dan log dicatat di `delivery.log`.  

#### c. Pengiriman Reguler oleh Pengguna
- **Mekanisme**:  
  - Pengguna menjalankan `./dispatcher -deliver [Nama]` untuk mengirim pesanan Reguler.  
  - Proses fork dibuat untuk menangani pengiriman, dengan nama agen sesuai argumen `[Nama]`.  
  - Status pesanan diubah ke nama agen, dan log dicatat.  

#### d. Fitur Tambahan
- **Cek Status**:  
  `./dispatcher -status [Nama]` → Menampilkan status pengiriman pesanan.  
- **Daftar Pesanan**:  
  `./dispatcher -list` → Menampilkan semua pesanan beserta statusnya.  
- **Log**:  
  `./dispatcher -log` → Menampilkan seluruh isi `delivery.log`.  

---

### Logging Format
- **Express**:  
  `[dd/mm/yyyy hh:mm:ss] [AGENT A/B/C] Express package delivered to [Nama] in [Alamat]`  
- **Reguler**:  
  `[dd/mm/yyyy hh:mm:ss] [AGENT <user>] Reguler package delivered to [Nama] in [Alamat]`  

---

### Cara Menjalankan Program
1. **Kompilasi**:
   ```bash
   gcc dispatcher.c -o dispatcher
   gcc delivery_agent.c -o delivery_agent -lpthread
Jalankan Dispatcher (untuk memuat data ke shared memory):

bash
./dispatcher
Jalankan Delivery Agent (untuk pengiriman Express):

bash
./delivery_agent
Perintah Pengguna:

Kirim pesanan Reguler:

bash
./dispatcher -deliver "Nama Agent" "Nama Pesanan"
Cek status:

bash
./dispatcher -status "Nama Pesanan"
Lihat daftar pesanan:

bash
./dispatcher -list
Catatan Penting
Shared Memory:

Key: 1234.

Pastikan tidak ada program lain yang menggunakan key yang sama.

Thread Safety:

Mutex digunakan untuk menghindari race condition saat agen mengakses shared memory.

Batasan:

Kapasitas maksimal pesanan: 100 (sesuai #define MAKS_PESANAN).

Jika file CSV tidak ditemukan, program dispatcher akan gagal.

Known Issues:

Jika ada pesanan dengan nama yang sama, hanya pesanan pertama yang diproses.

Delivery agent tidak berhenti otomatis setelah semua pesanan selesai (loop infinit).


# soal 3
Dikerjakan oleh Muhammad Ahsani Taqwiim Rakhman/5027241099

### Penjelasan Umum
Sistem ini terdiri dari tiga program utama:  
- **dungeon.c**: Server untuk menangani koneksi pemain, logika dungeon, dan battle.  
- **player.c**: Client untuk interaksi pemain dengan server.  
- **shop.c**: Modul toko senjata dan inventory.  

Fitur utama:  
- Multiplayer via socket (RPC-like).  
- Toko senjata dengan 5 pilihan (2 memiliki efek pasif).  
- Mode pertarungan dengan musuh acak, health bar, dan sistem damage.  
- Manajemen inventory dan stat pemain.  


### Struktur Kode
1. **dungeon.c**  
   - Menjadi server yang menerima koneksi dari `player.c`.  
   - Menangani logika: stat pemain, battle, shop, dan inventory.  
   - Port: **8080** (default).  

2. **player.c**  
   - Client GUI berbasis CLI dengan menu interaktif.  
   - Terhubung ke `dungeon.c` untuk mengirim/menerima data.  

3. **shop.c**  
   - Mengelola daftar senjata, pembelian, dan equip senjata.  
   - Terintegrasi dengan `dungeon.c`.  

## dungeon.c
```c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>

#define PORT 8080
#define MAX_CLIENTS 5
#define BUFFER_SIZE 1024

typedef struct {
    int gold;
    int base_damage;
    char equipped_weapon[50];
    int kills;
    int inventory[10];
    int inventory_size;
} Player;

typedef struct {
    int health;
    int max_health;
} Enemy;

Player players[MAX_CLIENTS];
Enemy current_enemies[MAX_CLIENTS];

extern void init_shop();
extern char* get_shop_menu();
extern void buy_weapon(int weapon_id, Player* player, char* result);
extern void get_inventory(Player* player, char* inventory);
extern void equip_weapon(int weapon_id, Player* player, char* result);

void send_battle_prompt(int client_socket, Enemy* enemy) {
    char health_bar[11];
    int filled_segments = (enemy->health * 10) / enemy->max_health;
    
    for (int i = 0; i < 10; i++) {
        health_bar[i] = (i < filled_segments) ? '=' : ' ';
    }
    health_bar[10] = '\0';
    
    char prompt[BUFFER_SIZE];
    snprintf(prompt, BUFFER_SIZE, 
        "BATTLE STARTED ===\n"
        "Enemy appeared with:\n"
        "[%s] %d/%d HP\n"
        "Type 'attack' to attack or 'exit' to leave battle.\n"
        "> ",
        health_bar, enemy->health, enemy->max_health);
    
    send(client_socket, prompt, strlen(prompt), 0);
}

void send_battle_result(int client_socket, Enemy* enemy, int damage, int is_critical) {
    char health_bar[11];
    int filled_segments = (enemy->health * 10) / enemy->max_health;
    
    for (int i = 0; i < 10; i++) {
        health_bar[i] = (i < filled_segments) ? '=' : ' ';
    }
    health_bar[10] = '\0';
    
    char result[BUFFER_SIZE];
    if (is_critical) {
        snprintf(result, BUFFER_SIZE,
            "== CRITICAL HIT! ==\n"
            "You dealt %d damage!\n"
            "=== ENEMY STATUS ===\n"
            "Enemy health: [%s] %d/%d HP\n"
            "> ",
            damage, health_bar, enemy->health, enemy->max_health);
    } else {
        snprintf(result, BUFFER_SIZE,
            "You dealt %d damage!\n"
            "=== ENEMY STATUS ===\n"
            "Enemy health: [%s] %d/%d HP\n"
            "> ",
            damage, health_bar, enemy->health, enemy->max_health);
    }
    
    send(client_socket, result, strlen(result), 0);
}

void handle_battle(int client_socket, int player_id) {
    char buffer[BUFFER_SIZE] = {0};
    
    current_enemies[player_id].max_health = 50 + (rand() % 151);
    current_enemies[player_id].health = current_enemies[player_id].max_health;
    
    send_battle_prompt(client_socket, &current_enemies[player_id]);
    
    while(1) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_read = read(client_socket, buffer, BUFFER_SIZE-1);
        if (bytes_read <= 0) break;
        
        buffer[strcspn(buffer, "\r\n")] = 0;
        
        if(strcmp(buffer, "attack") == 0) {
            if(current_enemies[player_id].health <= 0) {
                send(client_socket, "No enemy to attack!\n> ", 21, 0);
                continue;
            }
            
            int damage = players[player_id].base_damage + (rand() % (players[player_id].base_damage + 1));
            int is_critical = (rand() % 10 == 0);
            if (is_critical) damage *= 2;
            
            current_enemies[player_id].health -= damage;
            
            if(current_enemies[player_id].health <= 0) {
                players[player_id].kills++;
                int reward = 50 + (rand() % 101);
                players[player_id].gold += reward;
                
                char victory_msg[BUFFER_SIZE];
                snprintf(victory_msg, BUFFER_SIZE, 
                    "You defeated the enemy!\n"
                    "=== REWARD ===\n"
                    "You earned %d gold!\n"
                    "=== NEW ENEMY ===\n"
                    "> ",
                    reward);
                send(client_socket, victory_msg, strlen(victory_msg), 0);
                
                current_enemies[player_id].max_health = 50 + (rand() % 151);
                current_enemies[player_id].health = current_enemies[player_id].max_health;
            } else {
                send_battle_result(client_socket, &current_enemies[player_id], damage, is_critical);
            }
        }
        else if(strcmp(buffer, "exit") == 0) {
            send(client_socket, "Exiting battle...\n", 18, 0);
            break;
        }
        else {
            send(client_socket, "Invalid command. Type 'attack' or 'exit'.\n> ", 42, 0);
        }
    }
}

void handle_client(int client_socket) {
    char buffer[BUFFER_SIZE] = {0};
    int player_id = client_socket % MAX_CLIENTS;
    
    players[player_id].gold = 500;
    strcpy(players[player_id].equipped_weapon, "Fists");
    players[player_id].base_damage = 5;
    players[player_id].kills = 0;
    players[player_id].inventory_size = 0;
    
    init_shop();
    
    while(1) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_read = read(client_socket, buffer, BUFFER_SIZE-1);
        if (bytes_read <= 0) break;
        
        buffer[strcspn(buffer, "\r\n")] = 0;
        
        if(strcmp(buffer, "stats") == 0) {
            char stats[BUFFER_SIZE];
            snprintf(stats, BUFFER_SIZE, 
                "Gold: %d | Equipped Weapon: %s | Base Damage: %d | Kills: %d",
                players[player_id].gold, 
                players[player_id].equipped_weapon, 
                players[player_id].base_damage, 
                players[player_id].kills);
            send(client_socket, stats, strlen(stats), 0);
        }
        else if(strcmp(buffer, "shop") == 0) {
            send(client_socket, get_shop_menu(), strlen(get_shop_menu()), 0);
            
            
            memset(buffer, 0, BUFFER_SIZE);
            bytes_read = read(client_socket, buffer, BUFFER_SIZE-1);
            if (bytes_read <= 0) break;
            buffer[strcspn(buffer, "\r\n")] = 0;
            
            char result[BUFFER_SIZE];
            buy_weapon(atoi(buffer), &players[player_id], result);
            send(client_socket, result, strlen(result), 0);
        }
        else if(strcmp(buffer, "inventory") == 0) {
            char inventory[BUFFER_SIZE];
            get_inventory(&players[player_id], inventory);
            send(client_socket, inventory, strlen(inventory), 0);
            
            
            memset(buffer, 0, BUFFER_SIZE);
            bytes_read = read(client_socket, buffer, BUFFER_SIZE-1);
            if (bytes_read <= 0) break;
            buffer[strcspn(buffer, "\r\n")] = 0;
            
            char equip_result[BUFFER_SIZE];
            equip_weapon(atoi(buffer), &players[player_id], equip_result);
            send(client_socket, equip_result, strlen(equip_result), 0);
        }
        else if(strcmp(buffer, "battle") == 0) {
            handle_battle(client_socket, player_id);
        }
        else if(strcmp(buffer, "exit") == 0) {
            break;
        }
    }
    
    close(client_socket);
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    
    srand(time(NULL));
    
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    
    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    
    printf("Dungeon server running on port %d\n", PORT);
    
    while(1) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            continue;
        }
        
        printf("New connection from %s:%d\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));
        handle_client(new_socket);
    }
    
    return 0;
}

```

library yang diperlukan
```c
#include <stdio.h>     
#include <stdlib.h>      
#include <string.h>     
#include <unistd.h>      
#include <arpa/inet.h>
#include <time.h>      
```
`stdio.h` = Untuk fungsi input/output
`stdlib.h `= Untuk fungsi umum
`string.h` = Untuk manipulasi string
`unistd.h` = Untuk Fungsi sitem
`arpa/inet.h` = untuk komunikasi socket
`time.h` = untuk waktu seperti strand

define 
Digunakan untuk membuat konstanta atau alias.
```c
#define PORT 8080            
#define MAX_CLIENTS 5         
#define BUFFER_SIZE 1024      
```
Tujuannya agar:

Kode lebih mudah dibaca.

Mudah dimodifikasi (cukup ubah definisi satu kali).

Struktur Data

1. typedef struct Player

Mewakili data pemain:
```c
typedef struct {
    int gold;
    int base_damage;
    char equipped_weapon[50];
    int kills;
    int inventory[10];
    int inventory_size;
} Player;
```
`gold`: Emas pemain.

`base_damage`: Kerusakan dasar.

`equipped_weapon`: Senjata yang sedang dipakai.

`kills`: Jumlah musuh yang dikalahkan.

`inventory`: Daftar ID senjata yang dimiliki.

`inventory_size`: Jumlah senjata dalam inventaris.


2. typedef struct Enemy

Mewakili data musuh:
```c
typedef struct {
    int health;
    int max_health;
} Enemy;
```
health: HP saat ini.

max_health: HP maksimum.

fungsi eksternal
```c
extern void init_shop();
extern char* get_shop_menu();
extern void buy_weapon(int weapon_id, Player* player, char* result);
extern void get_inventory(Player* player, char* inventory);
extern void equip_weapon(int weapon_id, Player* player, char* result);
```
Ini didefinisikan di file lain:

`init_shop()`: Inisialisasi daftar senjata di toko.

`get_shop_menu()`: Mengembalikan string daftar senjata yang bisa dibeli.

`buy_weapon()`: Melakukan pembelian senjata, cek gold, dan update inventory.

`get_inventory()`: Mengembalikan daftar senjata milik pemain.

`equip_weapon()`: Mengatur senjata terpakai dan update base_damage.

Fungsi-Fungsi Utama

1. send_battle_prompt(int client_socket, Enemy* enemy)
   ```c
   void send_battle_prompt(int client_socket, Enemy* enemy) {
    char health_bar[11];
    int filled_segments = (enemy->health * 10) / enemy->max_health;
    
    for (int i = 0; i < 10; i++) {
        health_bar[i] = (i < filled_segments) ? '=' : ' ';
    }
    health_bar[10] = '\0';
    
    char prompt[BUFFER_SIZE];
    snprintf(prompt, BUFFER_SIZE, 
        "BATTLE STARTED ===\n"
        "Enemy appeared with:\n"
        "[%s] %d/%d HP\n"
        "Type 'attack' to attack or 'exit' to leave battle.\n"
        "> ",
        health_bar, enemy->health, enemy->max_health);
    
    send(client_socket, prompt, strlen(prompt), 0);
   }
   ```

Mengirim prompt pertempuran kepada pemain.

Membuat bar HP musuh (grafik sederhana).

Mengirim pesan seperti:

```bash
BATTLE STARTED ===
Enemy appeared with:
[====      ] 40/100 HP
Type 'attack' to attack or 'exit' to leave battle.
>
```

2. send_battle_result(int client_socket, Enemy* enemy, int damage, int is_critical)
```c
void send_battle_result(int client_socket, Enemy* enemy, int damage, int is_critical) {
    char health_bar[11];
    int filled_segments = (enemy->health * 10) / enemy->max_health;
    
    for (int i = 0; i < 10; i++) {
        health_bar[i] = (i < filled_segments) ? '=' : ' ';
    }
    health_bar[10] = '\0';
    
    char result[BUFFER_SIZE];
    if (is_critical) {
        snprintf(result, BUFFER_SIZE,
            "== CRITICAL HIT! ==\n"
            "You dealt %d damage!\n"
            "=== ENEMY STATUS ===\n"
            "Enemy health: [%s] %d/%d HP\n"
            "> ",
            damage, health_bar, enemy->health, enemy->max_health);
    } else {
        snprintf(result, BUFFER_SIZE,
            "You dealt %d damage!\n"
            "=== ENEMY STATUS ===\n"
            "Enemy health: [%s] %d/%d HP\n"
            "> ",
            damage, health_bar, enemy->health, enemy->max_health);
    }
send(client_socket, result, strlen(result), 0);
}
```
Mengirim hasil serangan setelah pemain menyerang musuh.

Menampilkan `damage` yang diberikan.

Jika `is_critical` (serangan kritis), damage digandakan dan ditampilkan berbeda.

Menampilkan bar HP musuh yang diperbarui.


3. handle_battle(int client_socket, int player_id)
```c
void handle_battle(int client_socket, int player_id) {
    char buffer[BUFFER_SIZE] = {0};
    
    current_enemies[player_id].max_health = 50 + (rand() % 151);
    current_enemies[player_id].health = current_enemies[player_id].max_health;
    
    send_battle_prompt(client_socket, &current_enemies[player_id]);
    
    while(1) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_read = read(client_socket, buffer, BUFFER_SIZE-1);
        if (bytes_read <= 0) break;
        
        buffer[strcspn(buffer, "\r\n")] = 0;
        
        if(strcmp(buffer, "attack") == 0) {
            if(current_enemies[player_id].health <= 0) {
                send(client_socket, "No enemy to attack!\n> ", 21, 0);
                continue;
            }
            
            int damage = players[player_id].base_damage + (rand() % (players[player_id].base_damage + 1));
            int is_critical = (rand() % 10 == 0);
            if (is_critical) damage *= 2;
            
            current_enemies[player_id].health -= damage;
            
            if(current_enemies[player_id].health <= 0) {
                players[player_id].kills++;
                int reward = 50 + (rand() % 101);
                players[player_id].gold += reward;
                
                char victory_msg[BUFFER_SIZE];
                snprintf(victory_msg, BUFFER_SIZE, 
                    "You defeated the enemy!\n"
                    "=== REWARD ===\n"
                    "You earned %d gold!\n"
                    "=== NEW ENEMY ===\n"
                    "> ",
                    reward);
                send(client_socket, victory_msg, strlen(victory_msg), 0);
                
                current_enemies[player_id].max_health = 50 + (rand() % 151);
                current_enemies[player_id].health = current_enemies[player_id].max_health;
            } else {
                send_battle_result(client_socket, &current_enemies[player_id], damage, is_critical);
            }
        }
        else if(strcmp(buffer, "exit") == 0) {
            send(client_socket, "Exiting battle...\n", 18, 0);
            break;
        }
        else {
            send(client_socket, "Invalid command. Type 'attack' or 'exit'.\n> ", 42, 0);
        }
    }
}
```
Loop utama pertarungan pemain dengan musuh.

Inisialisasi musuh baru.

Menerima perintah `attack` atau `exit`:

`attack`: Hitung damage acak berdasarkan base_damage, kadang critical hit.

Jika musuh kalah:

Tambah `kill` dan `gold`.

Kirim pesan kemenangan.

Spawn musuh baru.


Jika musuh belum mati:

Kirim update status musuh.


`exit`: Keluar dari mode battle.



4. handle_client(int client_socket)
```c
void handle_client(int client_socket) {
    char buffer[BUFFER_SIZE] = {0};
    int player_id = client_socket % MAX_CLIENTS;
    
    players[player_id].gold = 500;
    strcpy(players[player_id].equipped_weapon, "Fists");
    players[player_id].base_damage = 5;
    players[player_id].kills = 0;
    players[player_id].inventory_size = 0;
    
    init_shop();
    
    while(1) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_read = read(client_socket, buffer, BUFFER_SIZE-1);
        if (bytes_read <= 0) break;
        
        buffer[strcspn(buffer, "\r\n")] = 0;
        
        if(strcmp(buffer, "stats") == 0) {
            char stats[BUFFER_SIZE];
            snprintf(stats, BUFFER_SIZE, 
                "Gold: %d | Equipped Weapon: %s | Base Damage: %d | Kills: %d",
                players[player_id].gold, 
                players[player_id].equipped_weapon, 
                players[player_id].base_damage, 
                players[player_id].kills);
            send(client_socket, stats, strlen(stats), 0);
        }
        else if(strcmp(buffer, "shop") == 0) {
            send(client_socket, get_shop_menu(), strlen(get_shop_menu()), 0);
            
            
            memset(buffer, 0, BUFFER_SIZE);
            bytes_read = read(client_socket, buffer, BUFFER_SIZE-1);
            if (bytes_read <= 0) break;
            buffer[strcspn(buffer, "\r\n")] = 0;
            
            char result[BUFFER_SIZE];
            buy_weapon(atoi(buffer), &players[player_id], result);
            send(client_socket, result, strlen(result), 0);
        }
        else if(strcmp(buffer, "inventory") == 0) {
            char inventory[BUFFER_SIZE];
            get_inventory(&players[player_id], inventory);
            send(client_socket, inventory, strlen(inventory), 0);
            
            
            memset(buffer, 0, BUFFER_SIZE);
            bytes_read = read(client_socket, buffer, BUFFER_SIZE-1);
            if (bytes_read <= 0) break;
            buffer[strcspn(buffer, "\r\n")] = 0;
            
            char equip_result[BUFFER_SIZE];
            equip_weapon(atoi(buffer), &players[player_id], equip_result);
            send(client_socket, equip_result, strlen(equip_result), 0);
        }
        else if(strcmp(buffer, "battle") == 0) {
            handle_battle(client_socket, player_id);
        }
        else if(strcmp(buffer, "exit") == 0) {
            break;
        }
    }
    
    close(client_socket);
}
```
Fungsi utama untuk menangani satu klien.

Menginisialisasi data pemain (gold, weapon, dll.).

Menangani perintah-perintah dari pemain:

`stats`: Menampilkan statistik pemain.

`shop`: Tampilkan menu toko, kemudian beli item.

`inventory`: Tampilkan inventaris, lalu bisa memilih untuk equip senjata.

`battle`: Masuk mode pertempuran.

`exit`: Keluar dan tutup koneksi.

5. main()
```c
int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    
    srand(time(NULL));
    
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    
    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    
    printf("Dungeon server running on port %d\n", PORT);
    
    while(1) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            continue;
        }
        
        printf("New connection from %s:%d\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));
        handle_client(new_socket);
    }
    
    return 0;
}
```
Fungsi utama server.

Membuka socket server di port 8080.

Mengatur socket agar bisa reuse.

Menerima koneksi dan memanggil handle_client() untuk setiap koneksi.

## shop.c
```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

void print_menu() {
    printf("\n==== MAIN MENU ====\n");
    printf("1. Show Player Stats\n");
    printf("2. Shop (Buy Weapons)\n");
    printf("3. View Inventory & Equip Weapons\n");
    printf("4. Battle Mode\n");
    printf("5. Exit Game\n");
    printf("Choose an option: ");
}

void battle_mode(int sock) {
    char buffer[BUFFER_SIZE] = {0};
    
    // Enter battle mode
    send(sock, "battle", 6, 0);
    
    // Get initial battle message
    int bytes_read = read(sock, buffer, BUFFER_SIZE-1);
    if (bytes_read <= 0) return;
    buffer[bytes_read] = '\0';
    printf("%s", buffer);
    
    while(1) {
        printf("> ");
        fflush(stdout);
        
        if (fgets(buffer, BUFFER_SIZE, stdin) == NULL) break;
        buffer[strcspn(buffer, "\n")] = 0;
        
        if(strcmp(buffer, "attack") == 0 || strcmp(buffer, "exit") == 0) {
            send(sock, buffer, strlen(buffer), 0);
            
            // Get server response
            memset(buffer, 0, BUFFER_SIZE);
            bytes_read = read(sock, buffer, BUFFER_SIZE-1);
            if (bytes_read <= 0) break;
            buffer[bytes_read] = '\0';
            
            printf("%s", buffer);
            
            if(strcmp(buffer, "Exiting battle...\n") == 0) {
                break;
            }
        } else {
            printf("Invalid command. Type 'attack' or 'exit'\n");
        }
    }
}

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};
    
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    
    if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }
    
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }
    
    printf("Connected to dungeon server\n");
    
    int choice;
    while(1) {
        print_menu();
        if (scanf("%d", &choice) != 1) {
            while (getchar() != '\n');
            printf("Invalid input. Please enter a number.\n");
            continue;
        }
        getchar();
        
        switch(choice) {
            case 1:
                send(sock, "stats", 5, 0);
                memset(buffer, 0, BUFFER_SIZE);
                read(sock, buffer, BUFFER_SIZE);
                printf("\n=== PLAYER STATS ===\n%s\n", buffer);
                break;
                
            case 2:
                send(sock, "shop", 4, 0);
                memset(buffer, 0, BUFFER_SIZE);
                read(sock, buffer, BUFFER_SIZE);
                printf("\n%s", buffer);
                
                printf("Enter weapon number to buy (0 to cancel): ");
                fgets(buffer, BUFFER_SIZE, stdin);
                buffer[strcspn(buffer, "\n")] = 0;
                send(sock, buffer, strlen(buffer), 0);
                
                memset(buffer, 0, BUFFER_SIZE);
                read(sock, buffer, BUFFER_SIZE);
                printf("\n%s\n", buffer);
                break;
                
            case 3:
                send(sock, "inventory", 9, 0);
                memset(buffer, 0, BUFFER_SIZE);
                read(sock, buffer, BUFFER_SIZE);
                printf("\n%s", buffer);
                
                printf("Enter weapon number to equip (0 to cancel): ");
                fgets(buffer, BUFFER_SIZE, stdin);
                buffer[strcspn(buffer, "\n")] = 0;
                send(sock, buffer, strlen(buffer), 0);
                
                memset(buffer, 0, BUFFER_SIZE);
                read(sock, buffer, BUFFER_SIZE);
                printf("\n%s\n", buffer);
                break;
                
            case 4:
                battle_mode(sock);
                break;
                
            case 5:
                send(sock, "exit", 4, 0);
                close(sock);
                return 0;
                
            default:
                printf("Invalid option. Please try again.\n");
        }
    }
    
    close(sock);
    return 0;
}

```

file header
```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
```


`stdio.h` & `stdlib.h`: I/O standar dan fungsi umum.

`string.h`: Untuk manipulasi string.

`unistd.h`: Untuk fungsi sistem seperti read, write, close.

`arpa/inet.h`: Untuk pengaturan koneksi socket TCP/IP.


define
```c
#define PORT 8080
#define BUFFER_SIZE 1024
```
PORT: Port server yang akan dihubungkan.

BUFFER_SIZE: Ukuran buffer untuk komunikasi teks antara client-server.

fungsi-fungsi utama

1.Fungsi: print_menu()
```c
void print_menu() {
    printf("\n==== MAIN MENU ====\n");
    printf("1. Show Player Stats\n");
    printf("2. Shop (Buy Weapons)\n");
    printf("3. View Inventory & Equip Weapons\n");
    printf("4. Battle Mode\n");
    printf("5. Exit Game\n");
    printf("Choose an option: ");
}
```
Menampilkan menu utama:

1. Show Player Stats
2. Shop (Buy Weapons)
3. View Inventory & Equip Weapons
4. Battle Mode
5. Exit Game


2. Fungsi: battle_mode(int sock)
```c
void battle_mode(int sock) {
    char buffer[BUFFER_SIZE] = {0};
    
    send(sock, "battle", 6, 0);
    
    int bytes_read = read(sock, buffer, BUFFER_SIZE-1);
    if (bytes_read <= 0) return;
    buffer[bytes_read] = '\0';
    printf("%s", buffer);
    
    while(1) {
        printf("> ");
        fflush(stdout);
        
        if (fgets(buffer, BUFFER_SIZE, stdin) == NULL) break;
        buffer[strcspn(buffer, "\n")] = 0;
        
        if(strcmp(buffer, "attack") == 0 || strcmp(buffer, "exit") == 0) {
            send(sock, buffer, strlen(buffer), 0);
            
            memset(buffer, 0, BUFFER_SIZE);
            bytes_read = read(sock, buffer, BUFFER_SIZE-1);
            if (bytes_read <= 0) break;
            buffer[bytes_read] = '\0';
            
            printf("%s", buffer);
            
            if(strcmp(buffer, "Exiting battle...\n") == 0) {
                break;
            }
        } else {
            printf("Invalid command. Type 'attack' or 'exit'\n");
        }
    }
}
```
Masuk ke mode pertarungan.

Alurnya:

1. Mengirim `battle` ke server.


2. Menerima prompt awal pertarungan dari server.


3. Memasuki loop input attack atau exit:

Jika attack, kirim ke server, tunggu respons (damage, HP musuh).

Jika exit, kirim perintah dan keluar dari loop jika server merespons `Exiting battle...`.


3.Fungsi: main()
```c
int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};
    
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    
    if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }
    
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }
    
    printf("Connected to dungeon server\n");
    
    int choice;
    while(1) {
        print_menu();
        if (scanf("%d", &choice) != 1) {
            while (getchar() != '\n');
            printf("Invalid input. Please enter a number.\n");
            continue;
        }
        getchar();
        
        switch(choice) {
            case 1:
                send(sock, "stats", 5, 0);
                memset(buffer, 0, BUFFER_SIZE);
                read(sock, buffer, BUFFER_SIZE);
                printf("\n=== PLAYER STATS ===\n%s\n", buffer);
                break;
                
            case 2:
                send(sock, "shop", 4, 0);
                memset(buffer, 0, BUFFER_SIZE);
                read(sock, buffer, BUFFER_SIZE);
                printf("\n%s", buffer);
                
                printf("Enter weapon number to buy (0 to cancel): ");
                fgets(buffer, BUFFER_SIZE, stdin);
                buffer[strcspn(buffer, "\n")] = 0;
                send(sock, buffer, strlen(buffer), 0);
                
                memset(buffer, 0, BUFFER_SIZE);
                read(sock, buffer, BUFFER_SIZE);
                printf("\n%s\n", buffer);
                break;
                
            case 3:
                send(sock, "inventory", 9, 0);
                memset(buffer, 0, BUFFER_SIZE);
                read(sock, buffer, BUFFER_SIZE);
                printf("\n%s", buffer);
                
                printf("Enter weapon number to equip (0 to cancel): ");
                fgets(buffer, BUFFER_SIZE, stdin);
                buffer[strcspn(buffer, "\n")] = 0;
                send(sock, buffer, strlen(buffer), 0);
                
                memset(buffer, 0, BUFFER_SIZE);
                read(sock, buffer, BUFFER_SIZE);
                printf("\n%s\n", buffer);
                break;
                
            case 4:
                battle_mode(sock);
                break;
                
            case 5:
                send(sock, "exit", 4, 0);
                close(sock);
                return 0;
                
            default:
                printf("Invalid option. Please try again.\n");
        }
    }
    
    close(sock);
    return 0;
}
```
Fungsi utama program player.

1. Setup Socket Client

socket(AF_INET, SOCK_STREAM, 0)

Membuat socket TCP.

Menghubungkan ke 127.0.0.1:8080.


2. Loop Menu Utama

Setelah terhubung, tampilkan menu dan proses pilihan pemain:

Case 1: Show Player Stats

Kirim `stats` ke server.

Terima dan tampilkan statistik.


Case 2: Shop

Kirim `shop` ke server.

Tampilkan daftar senjata.

Minta input ID senjata yang ingin dibeli.

Kirim ID ke server.

Terima dan tampilkan hasil pembelian.


Case 3: Inventory

Kirim `inventory` ke server.

Tampilkan daftar senjata.

Minta input ID senjata untuk equip.

Kirim ID ke server.

Tampilkan hasil equip.


Case 4: Battle Mode

Memanggil fungsi `battle_mode(sock)`.


Case 5: Exit

Kirim `exit` ke server.

Tutup koneksi dan keluar.

##shop.c
```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_WEAPONS 5
#define MAX_INVENTORY 10

typedef struct {
    int gold;
    int base_damage;
    char equipped_weapon[50];
    int kills;
    int inventory[10];
    int inventory_size;
} Player;

typedef struct {
    char name[50];
    int price;
    int damage;
    char passive[100];
} Weapon;

Weapon shop_items[MAX_WEAPONS];

void init_shop() {
    strcpy(shop_items[0].name, "Terra Blade");
    shop_items[0].price = 50;
    shop_items[0].damage = 10;
    strcpy(shop_items[0].passive, "None");
    
    strcpy(shop_items[1].name, "Flint & Steel");
    shop_items[1].price = 150;
    shop_items[1].damage = 25;
    strcpy(shop_items[1].passive, "None");
    
    strcpy(shop_items[2].name, "Kitchen Knife");
    shop_items[2].price = 200;
    shop_items[2].damage = 35;
    strcpy(shop_items[2].passive, "None");
    
    strcpy(shop_items[3].name, "Staff of Light");
    shop_items[3].price = 120;
    shop_items[3].damage = 20;
    strcpy(shop_items[3].passive, "10% Insta-Kill Chance");
    
    strcpy(shop_items[4].name, "Dragon Claws");
    shop_items[4].price = 300;
    shop_items[4].damage = 50;
    strcpy(shop_items[4].passive, "30% Crit Chance");
}

char* get_shop_menu() {
    static char menu[1024];
    snprintf(menu, sizeof(menu),
        "\n=== WEAPON SHOP ===\n"
        "[1] %s - Price: %d gold, Damage: %d\n"
        "[2] %s - Price: %d gold, Damage: %d\n"
        "[3] %s - Price: %d gold, Damage: %d\n"
        "[4] %s - Price: %d gold, Damage: %d (Passive: %s)\n"
        "[5] %s - Price: %d gold, Damage: %d (Passive: %s)\n"
        "Enter weapon number to buy (0 to cancel): ",
        shop_items[0].name, shop_items[0].price, shop_items[0].damage,
        shop_items[1].name, shop_items[1].price, shop_items[1].damage,
        shop_items[2].name, shop_items[2].price, shop_items[2].damage,
        shop_items[3].name, shop_items[3].price, shop_items[3].damage, shop_items[3].passive,
        shop_items[4].name, shop_items[4].price, shop_items[4].damage, shop_items[4].passive);
    return menu;
}

void buy_weapon(int weapon_id, Player* player, char* result) {
    if (weapon_id < 1 || weapon_id > MAX_WEAPONS) {
        snprintf(result, 1024, "Invalid weapon choice!");
        return;
    }
    
    weapon_id--;
    
    if (player->gold < shop_items[weapon_id].price) {
        snprintf(result, 1024, "Not enough gold!");
        return;
    }
    
    if (player->inventory_size >= MAX_INVENTORY) {
        snprintf(result, 1024, "Inventory full!");
        return;
    }
    
    player->inventory[player->inventory_size] = weapon_id;
    player->inventory_size++;
    player->gold -= shop_items[weapon_id].price;
    
    snprintf(result, 1024, "Purchase successful!");
}

void get_inventory(Player* player, char* inventory) {
    char temp[1024] = "\n=== YOUR INVENTORY ===\n";
    
    for (int i = 0; i < player->inventory_size; i++) {
        int weapon_id = player->inventory[i];
        char item[256];
        if (strcmp(shop_items[weapon_id].passive, "None") == 0) {
            snprintf(item, sizeof(item), "[%d] %s\n", i+1, shop_items[weapon_id].name);
        } else {
            snprintf(item, sizeof(item), "[%d] %s (Passive: %s)\n", i+1, 
                    shop_items[weapon_id].name, shop_items[weapon_id].passive);
        }
        strcat(temp, item);
    }
    
    strcat(temp, "Enter weapon number to equip (0 to cancel): ");
    strcpy(inventory, temp);
}

void equip_weapon(int weapon_id, Player* player, char* result) {
    if (weapon_id < 1 || weapon_id > player->inventory_size) {
        snprintf(result, 1024, "Invalid weapon choice!");
        return;
    }
    
    weapon_id--;
    int actual_weapon_id = player->inventory[weapon_id];
    
    strcpy(player->equipped_weapon, shop_items[actual_weapon_id].name);
    player->base_damage = shop_items[actual_weapon_id].damage;
    
    snprintf(result, 1024, "Equipped %s!", shop_items[actual_weapon_id].name);
}
```

file header dan Define
```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
```
stdio.h: untuk printf, snprintf, dll.

stdlib.h: untuk fungsi umum seperti malloc, atoi, dll.

string.h: untuk operasi string seperti strcpy, strcmp, strcat.

```c
#define MAX_WEAPONS 5
#define MAX_INVENTORY 10
```
MAX_WEAPONS: Jumlah total senjata di toko.

MAX_INVENTORY: Kapasitas maksimal senjata yang bisa disimpan pemain.


Struktur Data

Player
```c
typedef struct {
    int gold;
    int base_damage;
    char equipped_weapon[50];
    int kills;
    int inventory[10];             
    int inventory_size;
} Player;
```
Mewakili data pemain.

Weapon
```
typedef struct {
    char name[50];
    int price;
    int damage;
    char passive[100];
} Weapon;
```
Mewakili senjata di toko:

name: Nama senjata.

price: Harga senjata.

damage: Damage saat digunakan.

passive: Efek pasif tambahan.


```c
Weapon shop_items[MAX_WEAPONS];
```
Array global yang menyimpan semua senjata yang tersedia di toko.

fungsi-fungsi utama

1. Fungsi init_shop()
```c
void init_shop() {
    strcpy(shop_items[0].name, "Terra Blade");
    shop_items[0].price = 50;
    shop_items[0].damage = 10;
    strcpy(shop_items[0].passive, "None");
    
    strcpy(shop_items[1].name, "Flint & Steel");
    shop_items[1].price = 150;
    shop_items[1].damage = 25;
    strcpy(shop_items[1].passive, "None");
    
    strcpy(shop_items[2].name, "Kitchen Knife");
    shop_items[2].price = 200;
    shop_items[2].damage = 35;
    strcpy(shop_items[2].passive, "None");
    
    strcpy(shop_items[3].name, "Staff of Light");
    shop_items[3].price = 120;
    shop_items[3].damage = 20;
    strcpy(shop_items[3].passive, "10% Insta-Kill Chance");
    
    strcpy(shop_items[4].name, "Dragon Claws");
    shop_items[4].price = 300;
    shop_items[4].damage = 50;
    strcpy(shop_items[4].passive, "30% Crit Chance");
}

```
Inisialisasi data senjata:

Mengisi nama, harga, damage, dan passive efek dari setiap senjata.

Fungsi ini harus dipanggil satu kali di awal server, agar data senjata siap digunakan.


Contoh senjata:

"Dragon Claws" - 300 gold, 50 damage, 30% crit chance


---

2. Fungsi get_shop_menu()
```
char* get_shop_menu() {
    static char menu[1024];
    snprintf(menu, sizeof(menu),
        "\n=== WEAPON SHOP ===\n"
        "[1] %s - Price: %d gold, Damage: %d\n"
        "[2] %s - Price: %d gold, Damage: %d\n"
        "[3] %s - Price: %d gold, Damage: %d\n"
        "[4] %s - Price: %d gold, Damage: %d (Passive: %s)\n"
        "[5] %s - Price: %d gold, Damage: %d (Passive: %s)\n"
        "Enter weapon number to buy (0 to cancel): ",
        shop_items[0].name, shop_items[0].price, shop_items[0].damage,
        shop_items[1].name, shop_items[1].price, shop_items[1].damage,
        shop_items[2].name, shop_items[2].price, shop_items[2].damage,
        shop_items[3].name, shop_items[3].price, shop_items[3].damage, shop_items[3].passive,
        shop_items[4].name, shop_items[4].price, shop_items[4].damage, shop_items[4].passive);
    return menu;
}
```
Mengembalikan menu senjata dalam format teks.

Menggunakan snprintf untuk menyusun string senjata dengan harga, damage, dan passive (jika ada).

Dikembalikan ke klien saat pemain memilih "shop".


Penting: static char menu[1024] dipakai agar buffer tetap tersedia setelah fungsi keluar.


3. Fungsi buy_weapon(int weapon_id, Player* player, char* result)
```c
void buy_weapon(int weapon_id, Player* player, char* result) {
    if (weapon_id < 1 || weapon_id > MAX_WEAPONS) {
        snprintf(result, 1024, "Invalid weapon choice!");
        return;
    }
    
    weapon_id--;
    
    if (player->gold < shop_items[weapon_id].price) {
        snprintf(result, 1024, "Not enough gold!");
        return;
    }
    
    if (player->inventory_size >= MAX_INVENTORY) {
        snprintf(result, 1024, "Inventory full!");
        return;
    }
    
    player->inventory[player->inventory_size] = weapon_id;
    player->inventory_size++;
    player->gold -= shop_items[weapon_id].price;
    
    snprintf(result, 1024, "Purchase successful!");
}
```
Membeli senjata berdasarkan input dari pemain.

Alur logika:

1. Cek validitas ID senjata (1-5).


2. Kurangi 1 untuk konversi ke indeks array (0-4).


3. Cek apakah pemain punya cukup gold.


4. Cek apakah inventaris sudah penuh.


5. Jika valid:

Tambah senjata ke inventory.

Kurangi gold pemain.

Kirim pesan sukses.



6. Jika tidak valid:

Kirim pesan kesalahan ("Inventory full!", "Not enough gold!", dll.)



4. Fungsi get_inventory(Player* player, char* inventory)
```c
void get_inventory(Player* player, char* inventory) {
    char temp[1024] = "\n=== YOUR INVENTORY ===\n";
    
    for (int i = 0; i < player->inventory_size; i++) {
        int weapon_id = player->inventory[i];
        char item[256];
        if (strcmp(shop_items[weapon_id].passive, "None") == 0) {
            snprintf(item, sizeof(item), "[%d] %s\n", i+1, shop_items[weapon_id].name);
        } else {
            snprintf(item, sizeof(item), "[%d] %s (Passive: %s)\n", i+1, 
                    shop_items[weapon_id].name, shop_items[weapon_id].passive);
        }
        strcat(temp, item);
    }
    
    strcat(temp, "Enter weapon number to equip (0 to cancel): ");
    strcpy(inventory, temp);
}
```
Menyusun dan mengirim string daftar senjata yang dimiliki pemain.

Alur:

1. Loop semua senjata yang dimiliki.


2. Ambil nama dan efek passive.


3. Tambahkan ke string temp dengan format:

[1] Terra Blade
[2] Staff of Light (Passive: 10% Insta-Kill Chance)


4. Tambahkan prompt:

Enter weapon number to equip (0 to cancel):


5. Salin hasil akhir ke inventory.


5. Fungsi equip_weapon(int weapon_id, Player* player, char* result)
```c
void equip_weapon(int weapon_id, Player* player, char* result) {
    if (weapon_id < 1 || weapon_id > player->inventory_size) {
        snprintf(result, 1024, "Invalid weapon choice!");
        return;
    }
    
    weapon_id--;
    int actual_weapon_id = player->inventory[weapon_id];
    
    strcpy(player->equipped_weapon, shop_items[actual_weapon_id].name);
    player->base_damage = shop_items[actual_weapon_id].damage;
    
    snprintf(result, 1024, "Equipped %s!", shop_items[actual_weapon_id].name);
}
```
Memilih dan menggunakan senjata dari inventory.

Alur:

1. Validasi ID input.


2. Konversi ID ke indeks inventory.


3. Ambil actual_weapon_id dari array inventory.


4. Set equipped_weapon dan base_damage berdasarkan senjata yang dipilih.


5. Kirim pesan sukses: "Equipped Terra Blade!"

## Error Handling

Peringatan untuk input tidak valid di menu.
Contoh:

Invalid option. Please try again.
Kekurangan & Perbaikan yang Diperlukan
Multi-Client Support

Server saat ini hanya menangani 1 client secara bergantian.

Solusi: Implementasi threading di dungeon.c untuk handle multiple clients.

Efek Pasif Senjata Belum Aktif

Critical Hit menggunakan fixed 10% (tidak sesuai senjata).

Solusi: Modifikasi logika critical di handle_battle() untuk membaca efek senjata.

Insta-Kill untuk Staff of Light

Efek 10% Insta-Kill belum diimplementasikan.

Solusi: Tambahkan pengecekan passive di handle_battle().

Race Condition

Tidak ada mutex untuk shared data (misal: gold pemain).

Solusi: Gunakan mutex/lock saat mengakses data pemain.

## Cara Menjalankan Program
Kompilasi:

bash
gcc dungeon.c shop.c -o dungeon
gcc player.c -o player
Jalankan Server:
![Screenshot 2025-05-02 133612](https://github.com/user-attachments/assets/30718094-2fe8-4bce-b655-95cd92cbbdca)

bash
./dungeon
Jalankan Client (di terminal lain):
![Screenshot 2025-05-02 133709](https://github.com/user-attachments/assets/730062b9-f641-4dc5-b3ad-0af4dd6e969e)


bash
./player
jalankan:
![Screenshot 2025-05-03 153126](https://github.com/user-attachments/assets/810bce96-57ca-4331-b091-981cb7b031ea)

# Soal 4

Dikerjakan oleh I Gede Bagus Saka Sinatrya/5027241088

### Penjelasan system.c dan hunter.c

#### Struktur Awal System.c
```bash
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <time.h>

#define MAX_HUNTERS 50
#define MAX_DUNGEONS 50
#define MAX_NAME 50

typedef struct {
    char username[MAX_NAME];
    int level, exp, atk, hp, def;
    int banned;
    int notification_on;
    key_t shm_key;
} Hunter;

typedef struct {
    char name[MAX_NAME];
    int min_level, atk, hp, def, exp;
    key_t shm_key;
} Dungeon;

typedef struct {
    Hunter hunters[MAX_HUNTERS];
    int num_hunters;
    Dungeon dungeons[MAX_DUNGEONS];
    int num_dungeons;
    sem_t hunter_sem;
    sem_t dungeon_sem;
    int system_active;
    pid_t system_pid;
} SystemData;

SystemData *sys_data = NULL;
int shm_id = -1;

key_t get_system_key() {
    return ftok("/tmp", 'S');
}
```

Fungsi untuk membuat shared memory
```bash
void init_shared_memory() {
    key_t key = get_system_key(); //mendapatkan kunci unik dari file
    shm_id = shmget(key, sizeof(SystemData), IPC_CREAT | 0666); //mendapatkan id shared memory untuk ukuran system data
    if (shm_id == -1) { //jika gagal akan menampilkan pesan error
        perror("[!] shmget failed");
        exit(EXIT_FAILURE);
    }

    sys_data = (SystemData*)shmat(shm_id, NULL, 0); //attach shared memory ke proses ini
    if (sys_data == (void*)-1) { //mengecek, jika gagal akan menampilkan pesan error
        perror("[!] shmat failed");
        exit(EXIT_FAILURE);
    }

    //mencari info shared memory untuk cek proses pertama
    struct shmid_ds buf;
    shmctl(shm_id, IPC_STAT, &buf);
    if (buf.shm_nattch == 1) {
        memset(sys_data, 0, sizeof(SystemData)); //mengkosongkan semua field
	 sys_data->system_pid = getpid(); //simpan pid proses system
        sys_data->system_active = 1; //menandai system active 
        sem_init(&sys_data->hunter_sem, 1, 1); //init untuk semaphore hunter
        sem_init(&sys_data->dungeon_sem, 1, 1); // init untuk semaphore dungeon
        sys_data->system_active = 1;
        printf("[System] Initialized shared memory\n");
    }
}
```

Fungsi untuk menampilkan list hunter
```bash
void list_hunters() {
    sem_wait(&sys_data->hunter_sem); //mengunci akses ke data hunter saat ini agar hanya ada 1 proses yang memodifikasi
    printf("\n=== HUNTER INFO ===\n");
    for (int i = 0; i < sys_data->num_hunters; i++) { //looping untuk sebanyak jumlah hunter
        Hunter *h = &sys_data->hunters[i]; //ambil pointer hunter ke i agar lebih mudah untuk selanjutnya
        printf("%d. %s | Lv:%d ATK:%d HP:%d DEF:%d EXP:%d [%s]\n",
               i + 1, h->username, h->level, h->atk, h->hp, h->def, h->exp,
               h->banned ? "BANNED" : "ACTIVE");
    }
    sem_post(&sys_data->hunter_sem);//buka akses ke data hunter setelah selesai
}
```

Fungsi untuk menampilkan list dungeon
```bash
void list_dungeons() {
    sem_wait(&sys_data->dungeon_sem); 
    printf("\n=== DUNGEON INFO ===\n");
    for (int i = 0; i < sys_data->num_dungeons; i++) { //looping sesuai sebanyak jumlah dungeon
        Dungeon *d = &sys_data->dungeons[i];
        printf("%d. %s | Lv %d+ | ATK:%d HP:%d DEF:%d EXP:%d\n",
               i + 1, d->name, d->min_level, d->atk, d->hp, d->def, d->exp);
    }
    sem_post(&sys_data->dungeon_sem);
}
```

Fungsi untuk membuat dungeon
```bash
void generate_dungeon() {
    const char *names[] = {
        "Double Dungeon", "Demon Castle", "Pyramid Dungeon",
        "Red Gate Dungeon", "Hunters Guild Dungeon", "Busan A-Rank Dungeon",
        "Insects Dungeon", "Goblins Dungeon", "D-Rank Dungeon",
        "Gwanak Mountain Dungeon", "Hapjeong Subway Station Dungeon"
    };

    sem_wait(&sys_data->dungeon_sem);

    if (sys_data->num_dungeons >= MAX_DUNGEONS) { //jika jumlah dungeon sudah maks
        printf("[!] Maximum dungeons reached\n");
        sem_post(&sys_data->dungeon_sem);
        return;
    }

    Dungeon *d = &sys_data->dungeons[sys_data->num_dungeons]; // mengambil pointer ke dungeon berikutnya yang akan di isi
    int index = rand() % 11; // memilih secara acak
    strncpy(d->name, names[index], MAX_NAME - 1); 
    d->min_level = rand() % 5 + 1;
    d->atk = rand() % 51 + 100; 
    d->hp = rand() % 51 + 50;
    d->def = rand() % 26 + 25;
    d->exp = rand() % 151 + 150;
    printf("[+] Generated: %s (Lv %d+)\n", d->name, d->min_level);
    sys_data->num_dungeons++;

    sem_post(&sys_data->dungeon_sem);
}
```

Fungsi untuk ban hunter
```bash
void ban_hunter() {
    char name[MAX_NAME];
    printf("Enter hunter username to ban: ");
    scanf("%s", name);

    sem_wait(&sys_data->hunter_sem);
    for (int i = 0; i < sys_data->num_hunters; i++) {  // looping untuk semua hunter
        if (strcmp(sys_data->hunters[i].username, name) == 0) {  // mencari hunter yang username sama dengan nama
            sys_data->hunters[i].banned = 1;  // set banned menjadi 1 yaitu true
            printf("[*] %s has been banned\n", name);
            sem_post(&sys_data->hunter_sem);
            return;
        }
    }
    printf("Hunter not found\n"); // jika nama hunter tidak ada
    sem_post(&sys_data->hunter_sem);
}
```

Fungsi untuk unban hunter
```bash
void unban_hunter() {
    char name[MAX_NAME];
    printf("Enter hunter username to unban: ");
    scanf("%s", name);

    sem_wait(&sys_data->hunter_sem);
    for (int i = 0; i < sys_data->num_hunters; i++) {  // looping kesemua daftar hunter
        if (strcmp(sys_data->hunters[i].username, name) == 0) {
            sys_data->hunters[i].banned = 0;  // set banned menjadi 0 (unban)
            printf("%s has been unbanned\n", name);
            sem_post(&sys_data->hunter_sem);
            return;
        }
    }
    printf("Hunter not found\n");
    sem_post(&sys_data->hunter_sem);
}
```

Fungsi untuk reset hunter
```bash
void reset_hunter() {
    char name[MAX_NAME];
    printf("Enter hunter username to reset: ");
    scanf("%s", name);

    sem_wait(&sys_data->hunter_sem);
    for (int i = 0; i < sys_data->num_hunters; i++) {  // looping pada semua hunter
        if (strcmp(sys_data->hunters[i].username, name) == 0) {  // Mengecek apakah username hunter pada indeks i sama dengan input name
            Hunter *h = &sys_data->hunters[i];
            h->level = 1; h->exp = 0; h->atk = 10; h->hp = 100; h->def = 5;  // mengatur ulang stat
            h->banned = 0; // unban kemabli hunter jika sebelumnya di ban
            printf("[*] %s has been reset\n", name);
            sem_post(&sys_data->hunter_sem);
            return;
        }
    }
    printf("Hunter not found\n");
    sem_post(&sys_data->hunter_sem);
}
```

Fungsi untuk menghapus shared memory
```bash
void cleanup(int sig) {
    printf("\nShutting down and cleaning up shared memory\n");
    sem_destroy(&sys_data->hunter_sem); // menghapus semaphore 
    sem_destroy(&sys_data->dungeon_sem);
    shmdt(sys_data); // lepas pointer sys_data dari segment shared memory
    shmctl(shm_id, IPC_RMID, NULL); // menghapus shared memory dari system
    printf("Cleanup complete. Shared memory removed.\n");
    exit(0);
}
```

Fungsi untuk tampilan system
```bash
void system_menu() {
    int choice;
    do {
        printf("\n=== SYSTEM MENU ===\n");
        printf("1. Hunter Info\n");
	printf("2. Dungeon Info\n");
	printf("3. Generate Dungeon\n");
        printf("4. Ban Hunter\n");
	printf("5. Unban Hunter\n");
	printf("6. Reset Hunter\n");
	printf("7. Exit\n");
        printf("Choice: ");
        scanf("%d", &choice);

        switch (choice) {
            case 1: list_hunters(); break;
            case 2: list_dungeons(); break;
            case 3: generate_dungeon(); break;
            case 4: ban_hunter(); break;
            case 5: unban_hunter(); break;
	    case 6: reset_hunter(); break;
            case 7: cleanup(SIGINT); break;
            default: printf("Invalid choice\n");
        }
    } while (1);
}
```

Untuk int main()
```bash

int main() {
    srand(time(NULL)); // seed untuk angka acak dan menghasilkan waktu sekarang sebagai seed agar hasil random
    signal(SIGINT, cleanup); // sinyal untuk SIGINT dan langsung clean up jika terpanggil
    init_shared_memory();
    system("clear");  // membersihkan layar saat dijalankan
    system_menu(); 
    return 0;
}
```

#### Struktur Awal Hunter.c
```bash
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <time.h>
#include <errno.h>

#define MAX_HUNTERS 50
#define MAX_DUNGEONS 50
#define MAX_NAME 50
#define EXP_TO_LEVEL_UP 500
#define HEARTBEAT_INTERVAL 2

typedef struct {
    char username[MAX_NAME];
    int level, exp, atk, hp, def;
    int banned;
    int notification_on;
    key_t shm_key;
} Hunter;

typedef struct {
    char name[MAX_NAME];
    int min_level, atk, hp, def, exp;
    key_t shm_key;
} Dungeon;

typedef struct {
    Hunter hunters[MAX_HUNTERS];
    int num_hunters;
    Dungeon dungeons[MAX_DUNGEONS];
    int num_dungeons;
    sem_t hunter_sem;
    sem_t dungeon_sem;
    int system_active;
    pid_t system_pid;
} SystemData;

SystemData *sys_data = NULL;
int shm_id = -1;
volatile sig_atomic_t shutdown_flag = 0;
pthread_t notif_thread, heartbeat_thread;

key_t get_system_key() {
    return ftok("/tmp", 'S');
}
```
Mengecek system.c apakah masih aktif atau tidak
```bash
void *heartbeat_checker(void *arg) {
    while (1) {
        sleep(HEARTBEAT_INTERVAL);

        if (sys_data == NULL || sys_data == (void *)-1) {  // mengecek apakah pointer ke shared memory tidak valid
            fprintf(stderr, "\n[!] ERROR: Shared memory is turned off. Turn off the hunter!\n");
            exit(1); // langsung exit dengan tanda error
        }

        if (sys_data->system_active != 1) { // mengecek apakah flag system_active bernilai 1
            fprintf(stderr, "\n[!] System.c is shutting down. Turn of the hunter...\n");
            exit(1);  
        }

        if (kill(sys_data->system_pid, 0) != 0 && errno == ESRCH) { // Mengecek apakah proses dengan PID system_pid masih hidup.
            fprintf(stderr, "\n[!] System.c is turned off. Shutdown Hunter...\n");
            exit(1);  
        }
    }
    return NULL;
}
```
Membersihkan resource
```bash
void cleanup() {
    if (sys_data != NULL && sys_data != (void *)-1) {
        shmdt(sys_data);
    }
    exit(0);
}
```
Menghandle sinyal
```bash
void handle_signal(int sig) {
    shutdown_flag = 1;
    cleanup();
}
```


Fungsi untuk login
```bash
int login(const char *username) {
    sem_wait(&sys_data->hunter_sem);
    for (int i = 0; i < sys_data->num_hunters; i++) {
        if (strcmp(sys_data->hunters[i].username, username) == 0) {
            if (sys_data->hunters[i].banned) { // mengecek apakah hunter di ban
                printf("[!] You are banned.\n");
                sem_post(&sys_data->hunter_sem);
                return -1;
            }
            sem_post(&sys_data->hunter_sem); 
            printf("[+] Welcome back, %s!\n", username);
            return i;
        }
    }
    sem_post(&sys_data->hunter_sem);
    printf("[!] Username not found.\n");
    return -1;
}
```

Fungsi untuk melihat dungeon yang tersedia pada setiap akun hunter
```bash
void show_dungeons(int level) {
    sem_wait(&sys_data->dungeon_sem);
    printf("\n=== AVAILABLE DUNGEONS ===\n");
    int count = 0;
    for (int i = 0; i < sys_data->num_dungeons; i++) {  // looping keseluruh dungeon
        Dungeon *d = &sys_data->dungeons[i];  // pointer yang menujuk dungeon saat ini 
        if (d->min_level <= level) { 
            printf("%d. %s (Lv %d+) ATK+%d HP+%d DEF+%d EXP+%d\n", 
                   ++count, d->name, d->min_level, d->atk, d->hp, d->def, d->exp);
        }
    }
    if (count == 0) printf("No dungeons available.\n");
    sem_post(&sys_data->dungeon_sem);
}
```
Fungsi untuk raid
```bash
void raid(int idx) {
    if (shutdown_flag) return; // cek flag shutdown

    Hunter *h = &sys_data->hunters[idx]; // ambil data hunter berdasarkan index
    show_dungeons(h->level); // tampilkan dungeon yang tersedia sesuai level
    printf("Choose dungeon number to raid: ");
    int input, count = 0, selected = -1;
    scanf("%d", &input);

    sem_wait(&sys_data->dungeon_sem);
    for (int i = 0; i < sys_data->num_dungeons; i++) {
        if (sys_data->dungeons[i].min_level <= h->level) { // menghitung dungeon sesuai syarat level 
            count++;
            if (count == input) {
                selected = i;
                break;
            }
        }
    }
    if (selected == -1) {
        printf("[!] Invalid selection.\n");
        sem_post(&sys_data->dungeon_sem);
        return;
    }

    Dungeon d = sys_data->dungeons[selected];
    h->atk += d.atk;
    h->hp += d.hp;
    h->def += d.def;
    h->exp += d.exp;

    if (h->exp >= EXP_TO_LEVEL_UP) {
        h->exp = 0;
        h->level++;
        printf("[+] LEVEL UP! Now Level %d\n", h->level);
    }

    for (int i = selected; i < sys_data->num_dungeons - 1; i++){  // menghapus dungeon yang sudah berhasil di raid
        sys_data->dungeons[i] = sys_data->dungeons[i + 1];
    }
    sys_data->num_dungeons--;

    printf("Raid success!\n");
    printf("Gained: ATK+%d HP+%d DEF+%d EXP+%d\n", d.atk, d.hp, d.def, d.exp);
    sem_post(&sys_data->dungeon_sem);
    press_enter();
}
```
Fungsi untuk battle
```bash
void battle(int idx) {
    if (shutdown_flag) return;

    Hunter *me = &sys_data->hunters[idx];
    int list[MAX_HUNTERS], count = 0;
    printf("\n=== PVP LIST ===\n");
    sem_wait(&sys_data->hunter_sem);
    for (int i = 0; i < sys_data->num_hunters; i++) { // looping unuk mencari hunter yang tersedia
        if (i != idx && !sys_data->hunters[i].banned) {
            Hunter *h = &sys_data->hunters[i];
            printf("%s - Total Power: %d\n", h->username, h->atk + h->hp + h->def);
            list[count++] = i;
        }
    }
    sem_post(&sys_data->hunter_sem);
    if (count == 0) { 
        printf("No opponents available.\n");
        press_enter();
        return;
    }

    char target[MAX_NAME];
    printf("Target: ");
    scanf("%s", target);

    int opp = -1;
    for (int i = 0; i < sys_data->num_hunters; i++) { // mengecek validasi target
        if (i != idx && strcmp(sys_data->hunters[i].username, target) == 0 && !sys_data->hunters[i].banned) {
            opp = i;
            break;
        }
    }

    if (opp == -1) {
        printf("[!] Invalid target.\n");
        press_enter();
        return;
    }

    Hunter *enemy = &sys_data->hunters[opp]; // pointer hunter target
    int p1 = me->atk + me->hp + me->def; // total power diri sendiri
    int p2 = enemy->atk + enemy->hp + enemy->def; // total power lawan

    printf("\nYou chose to battle %s\n", enemy->username);
    printf("Your Power: %d\nOpponent's Power: %d\n", p1, p2);

    if (p1 >= p2) { // transfer stat lawan
        me->atk += enemy->atk;
        me->hp += enemy->hp;
        me->def += enemy->def;
        printf("Deleting defender's shared memory (shmid: %d)\n", enemy->shm_key);

        for (int i = opp; i < sys_data->num_hunters - 1; i++) // hapus lawan ari system
            sys_data->hunters[i] = sys_data->hunters[i + 1];
        sys_data->num_hunters--;

        printf("Battle won! You acquired %s's stats\n", enemy->username);
    } else { // jika lawan menang
        enemy->atk += me->atk;
        enemy->hp += me->hp;
        enemy->def += me->def;

        for (int i = idx; i < sys_data->num_hunters - 1; i++) // hapus diri sendiri dari system
            sys_data->hunters[i] = sys_data->hunters[i + 1];
        sys_data->num_hunters--;

        printf("You lost. You were removed from the system.\n");
        press_enter();
        exit(0);
    }
    press_enter();
}
```

Fungsi untuk notifikasi dan tampilannya
```bash
void toggle_notify(int idx) {
    if (shutdown_flag) return;

    Hunter *h = &sys_data->hunters[idx];
    h->notification_on = !h->notification_on;  // status untuk notifikasi
    printf("[*] Notifications %s\n", h->notification_on ? "enabled" : "disabled");
    if (h->notification_on) { // mengecek jika notif aktif
        int *arg = malloc(sizeof(int));
        *arg = idx;
        pthread_create(&notif_thread, NULL, notify_thread, arg); // membuat thread baru jika notif aktif
    } else { // jika tidak aktif
        pthread_cancel(notif_thread);
    }
    press_enter();
}
```

```bash
void *notify_thread(void *arg) {
    int idx = *(int *)arg;
    free(arg); // membebaskan memory yang dialokasikan

    while (!shutdown_flag) { 
        if (sys_data->hunters[idx].notification_on) { //
            sem_wait(&sys_data->dungeon_sem);
            int found = 0; 
            for (int i = 0; i < sys_data->num_dungeons; i++) { 
                if (sys_data->dungeons[i].min_level <= sys_data->hunters[idx].level) { // jika dungeon sesuai dengan level hunter
                    Dungeon *d = &sys_data->dungeons[i];
                    printf("\n=== HUNTER SYSTEM ===\n%s for minimum level %d opened!\n",
                           d->name, d->min_level);
                    found = 1;
                    break;
                }
            }
            if (!found) { // jika tidak ada dungeon yang sesuai
                printf("\nNo suitable dungeons available.\n");
            }
            sem_post(&sys_data->dungeon_sem);
        }
        sleep(3);
    }
    return NULL;
}
```
Fungsi untuk tampilan setiap akun hunter
```bash
void dashboard(int idx) {
    int ch;
    do{
        if(sys_data == NULL || sys_data->system_active != 1) { // mengecek apakah system masih aktif
            exit(1);
	}

        Hunter *h = &sys_data->hunters[idx];
        printf("\n=== %s's MENU ===\n", h->username);
        printf("1. List Dungeon\n");
	printf("2. Raid\n");
	printf("3. Battle\n");
	printf("4. Toggle Notification\n");
	printf("5. Exit\n");
	printf("Choice: ");
        scanf("%d", &ch);
        switch (ch) {
            case 1: show_dungeons(h->level); press_enter(); break;
            case 2: raid(idx); break;
            case 3: battle(idx); break;
            case 4: toggle_notify(idx); break;
            case 5: return;
            default: printf("Invalid choice\n");
        }
    } while (1);
}
```
Untuk int main()
```bash
int main() {
    // menangani sinyal
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    // mengecek apakah system sudah dijalankan terlebih dahulu
    key_t key = get_system_key();
    shm_id = shmget(key, sizeof(SystemData), 0666);
    if (shm_id == -1) { 
        printf("[!] Run ./system first.\n");
        return 1;
    }

    // attach shared memory
    sys_data = (SystemData *)shmat(shm_id, NULL, 0);
    if (sys_data == (void *)-1) {
        perror("shmat failed");
        return 1;
    }

    // validasi status system
    if (sys_data->system_active != 1) {
        printf("[!] System is inactive. Please start the system first.\n");
        shmdt(sys_data);
        return 1;
    }

    pthread_create(&heartbeat_thread, NULL, heartbeat_checker, NULL); // mengecek status system secara berkala

    system("clear"); 
    int ch;
    char name[MAX_NAME];
    do {
        if(sys_data == NULL || sys_data->system_active != 1) {
            exit(1);
        }

        printf("\n=== HUNTER MENU ===\n");
	printf("1. Register\n");
	printf("2. Login\n");
	printf("3. Exit\nChoice: ");
        scanf("%d", &ch);
        switch (ch) {
            case 1: {
                printf("Username: ");
                scanf("%s", name);
                sem_wait(&sys_data->hunter_sem);
                int exists = 0;
                for (int i = 0; i < sys_data->num_hunters; i++) { // mengecek userbame apakah sudah digunakan
                    if (strcmp(sys_data->hunters[i].username, name) == 0) exists = 1;
                }
                if (exists) printf("[!] Username already taken\n");
                else if (sys_data->num_hunters >= MAX_HUNTERS) printf("[!] Hunter list full\n"); // jika list hunter penuh
                else {
                    Hunter *h = &sys_data->hunters[sys_data->num_hunters++];
                    strcpy(h->username, name); // set stat awal hunter
                    h->level = 1; h->exp = 0; h->atk = 10; h->hp = 100; h->def = 5;
                    h->banned = 0; h->notification_on = 0;
                    h->shm_key = ftok("/tmp", name[0]);
                    printf("Registration success!\n");
                }
                sem_post(&sys_data->hunter_sem);
                press_enter();
                break;
            }
            case 2: {
                printf("Username: ");
                scanf("%s", name);
		// memverifikasi login dan status banned
                int id = login(name);
                if (id != -1) dashboard(id);
                break;
            }
            case 3: break;
            default: printf("Invalid choice\n");
        }
    } while (ch != 3 && !shutdown_flag);

    cleanup();
    return 0;
}
```
### Beberapa masalah yang terjadi

![Image](https://github.com/user-attachments/assets/fc8c919e-b798-465b-a66f-48f9ab314840)

![Image](https://github.com/user-attachments/assets/623be93d-16d9-4e44-8a42-662c95f4e32b)

Hunter.c masih bisa berjalan tanpa menjalankan system.c terlebih dahulu.

Shared memory tidak terhapus begitu keluar dari system.
### Revisi

1. Hunter.c masih bisa berjalan tanpa menggunakan system.c, solusinya tambahkan fungsi dan beberapa syarat berikut.
```bash
void *heartbeat_checker(void *arg) {
    while (1) {
        sleep(HEARTBEAT_INTERVAL);

        if (sys_data == NULL || sys_data == (void *)-1) {
            fprintf(stderr, "\n[!] ERROR: Shared memory is turned off. Turn off the hunter!\n");
            exit(1); 
        }

        if (sys_data->system_active != 1) {
            fprintf(stderr, "\n[!] System.c is shutting down. Turn of the hunter...\n");
            exit(1);  
        }

        if (kill(sys_data->system_pid, 0) != 0 && errno == ESRCH) {
            fprintf(stderr, "\n[!] System.c is turned off. Shutdown Hunter...\n");
            exit(1);  
        }
    }
    return NULL;
}
```
Pada int main kita tambahkan pengecekan
```bash
 key_t key = get_system_key();
    shm_id = shmget(key, sizeof(SystemData), 0666);
    if (shm_id == -1) {
        printf("[!] Run ./system first.\n");
        return 1;
    }

    sys_data = (SystemData *)shmat(shm_id, NULL, 0);
    if (sys_data == (void *)-1) {
        perror("shmat failed");
        return 1;
    }

    if (sys_data->system_active != 1) {
        printf("[!] System is inactive. Please start the system first.\n");
        shmdt(sys_data);
        return 1;
    }

    pthread_create(&heartbeat_thread, NULL, heartbeat_checker, NULL);
```


2. Shared memory belum terhapus saat keluar dari system, solusinya menambahkan fungsi untuk menghapus shared memory
```bash
void cleanup(int sig) {
    printf("\nShutting down and cleaning up shared memory\n");
    sem_destroy(&sys_data->hunter_sem);
    sem_destroy(&sys_data->dungeon_sem);
    shmdt(sys_data);
    shmctl(shm_id, IPC_RMID, NULL);
    printf("Cleanup complete. Shared memory removed.\n");
    exit(0);
}
```






