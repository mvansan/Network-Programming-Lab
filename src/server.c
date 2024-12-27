#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sqlite3.h>
#include "include/init_db.h"
#include "include/auth.h"
#include "include/exam_room.h"
#include "include/import_questions.h"

#define PORT 8080
#define BUFFER_SIZE 8000

#define CREATE_ROOM 0x01
#define LIST_ROOMS 0x02
#define JOIN_ROOM 0x03
#define LOGOUT 0x04

#include "exam_room.h"  // Đảm bảo đã include header file chứa khai báo join_exam_room

void handle_client_request(int client_socket) {
    char buffer[BUFFER_SIZE] = {0};
    read(client_socket, buffer, BUFFER_SIZE);

    char command[50], username[50], password[50], name[50], category[50], difficulty[50];
    int num_questions, time_limit, room_id;  // Khai báo room_id

    // In ra nội dung buffer để debug
    printf("Received buffer: %s\n", buffer);

    // Đọc lệnh đầu tiên từ client
    int n = sscanf(buffer, "%s", command);
    if (n < 1) {
        send(client_socket, "Invalid command", strlen("Invalid command"), 0);
        return;
    }

    if (strcmp(command, "REGISTER") == 0) {
        n = sscanf(buffer, "%s %s %s", command, username, password);
        if (n != 3) {
            send(client_socket, "Invalid REGISTER command format", strlen("Invalid REGISTER command format"), 0);
            return;
        }

        printf("Registering user: %s\n", username);
        int result = register_user(username, password);
        if (result == 0) {
            send(client_socket, "Registration successful", strlen("Registration successful"), 0);
        } else {
            send(client_socket, "Registration failed", strlen("Registration failed"), 0);
        }
    } else if (strcmp(command, "LOGIN") == 0) {
        n = sscanf(buffer, "%s %s %s", command, username, password);
        if (n != 3) {
            send(client_socket, "Invalid LOGIN command format", strlen("Invalid LOGIN command format"), 0);
            return;
        }

        printf("Logging in user: %s\n", username);
        int result = login_user(username, password);
        if (result == 1) {
            send(client_socket, "Login successful", strlen("Login successful"), 0);
        } else {
            send(client_socket, "Login failed", strlen("Login failed"), 0);
        }
    } else if (buffer[0] == CREATE_ROOM) {
        // Xử lý tạo phòng thi
        n = sscanf(buffer + 1, "%s %d %d %s %s", name, &num_questions, &time_limit, category, difficulty);
        if (n != 5) {
            send(client_socket, "Invalid CREATE_ROOM command format", strlen("Invalid CREATE_ROOM command format"), 0);
            return;
        }

        printf("Creating exam room: %s with %d questions and %d minutes time limit, Category: %s, Difficulty: %s\n",
               name, num_questions, time_limit, category, difficulty);
        create_exam_room(client_socket, name, num_questions, time_limit, category, difficulty);
    } else if (buffer[0] == LIST_ROOMS) {
        // Xử lý liệt kê các phòng thi
        list_exam_rooms(client_socket);
    } else if (buffer[0] == JOIN_ROOM) {
        // Xử lý tham gia phòng thi
        n = sscanf(buffer + 1, "%d", &room_id);  // Đọc ID phòng thi từ buffer
        if (n != 1) {
            send(client_socket, "Invalid JOIN_ROOM command format", strlen("Invalid JOIN_ROOM command format"), 0);
            return;
        }

        printf("Client requested to join exam room with ID: %d\n", room_id);
        join_exam_room(client_socket, room_id);  // Gọi hàm join_exam_room
    } else if (buffer[0] == LOGOUT) {
        // Xử lý logout
        printf("Client logged out\n");
        send(client_socket, "Logout successful", strlen("Logout successful"), 0);
        close(client_socket);
        return;
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
