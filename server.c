#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "init_db.c"
#include "auth.h"

#define PORT 8080
#define BUFFER_SIZE 1024

void handle_client_request(int client_socket) {
    char buffer[BUFFER_SIZE] = {0};
    read(client_socket, buffer, BUFFER_SIZE);

    char command[10], username[50], password[50];
    sscanf(buffer, "%s %s %s", command, username, password);

    if (strcmp(command, "REGISTER") == 0) {
        printf("Registering user: %s\n", username);
        int result = register_user(username, password);
        if (result == 0) {
            send(client_socket, "Registration successful", strlen("Registration successful"), 0);
        } else {
            send(client_socket, "Registration failed", strlen("Registration failed"), 0);
        }
    } else if (strcmp(command, "LOGIN") == 0) {
        printf("Logging in user: %s\n", username);
        int result = login_user(username, password);
        if (result == 1) {
            send(client_socket, "Login successful", strlen("Login successful"), 0);
        } else {
            send(client_socket, "Login failed", strlen("Login failed"), 0);
        }
    } else {
        send(client_socket, "Unknown command", strlen("Unknown command"), 0);
    }
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    init_database();

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    #ifdef SO_REUSEPORT
        if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
    #else
        if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
    #endif
        perror("setsockopt");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server is listening on port %d\n", PORT);

    while (1) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("Accept failed");
            close(server_fd);
            exit(EXIT_FAILURE);
        }

        printf("Accepted a new client connection\n");

        handle_client_request(new_socket);

        close(new_socket);
    }

    close(server_fd);
    return 0;
}
