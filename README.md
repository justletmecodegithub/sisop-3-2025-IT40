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


# Soal Nomor 3
dikerjakan oleh Ahsani Rakhman

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

---

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

---

### Fitur yang Diimplementasikan
#### a. Server-Client via Socket (RPC-like)
- **dungeon.c** membuat socket server dan menerima koneksi dari **player.c**.  
- Setiap pemain memiliki ID unik berdasarkan socket.  
- **Kekurangan**: Server tidak multi-threaded (hanya handle 1 client secara bergantian).  

#### b. Main Menu Interaktif
```plaintext
==== MAIN MENU ====
1. Show Player Stats
2. Shop (Buy Weapons)
3. View Inventory & Equip Weapons
4. Battle Mode
5. Exit Game
Pemain memilih opsi via input angka.

c. Player Stats
```c
Menampilkan:

Gold

Senjata yang dipakai

Base Damage

Jumlah kill
Contoh:
```

Gold: 500 | Equipped Weapon: Fists | Base Damage: 5 | Kills: 0
d. Weapon Shop

5 senjata dengan harga, damage, dan efek pasif:

Terra Blade (50G, 10 DMG)

Flint & Steel (150G, 25 DMG)

Kitchen Knife (200G, 35 DMG)

Staff of Light (120G, 20 DMG, 10% Insta-Kill)

Dragon Claws (300G, 50 DMG, 30% Critical Chance)

e. Inventory Management
```
Menampilkan senjata yang dimiliki beserta efek pasif.

Pemain bisa equip senjata untuk meningkatkan damage.
Contoh:

[1] Dragon Claws (Passive: 30% Crit Chance) (EQUIPPED)
``
f. Battle Mode
```
Musuh memiliki HP acak (50-200).

Health bar visual: [==== ] 80/100 HP.

Damage dihitung: Base Damage + random(0, Base Damage).

Critical Hit (10% chance default) → Damage 2x.

Reward gold acak setelah mengalahkan musuh.
```

g. Error Handling
```
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

Cara Menjalankan Program
Kompilasi:

bash
gcc dungeon.c shop.c -o dungeon -lpthread
gcc player.c -o player
Jalankan Server:

bash
./dungeon
Jalankan Client (di terminal lain):

bash
./player
Contoh Interaksi:

Pilih opsi 2 (Shop) → Beli senjata.

Pilih opsi 3 (Inventory) → Equip senjata.

Pilih opsi 4 (Battle) → Serang musuh dengan perintah attack.
```

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






