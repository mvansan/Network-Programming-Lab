#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sqlite3.h>
#include "init_db.h"
#include "auth.h"

#define PORT 8080
#define BUFFER_SIZE 1024

void create_exam_room(int client_socket, const char *name, int num_questions, int time_limit) {
    sqlite3 *db;
    char *err_msg = 0;

    // Mở cơ sở dữ liệu
    int rc = sqlite3_open("exam_system.db", &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        send(client_socket, "Database connection failed", strlen("Database connection failed"), 0);
        return;
    }

    // Tạo phòng thi mới
    const char *sql_insert = "INSERT INTO exam_rooms (name, num_questions, time_limit, status) VALUES (?, ?, ?, ?);";
    sqlite3_stmt *stmt;

    // Chuẩn bị câu lệnh SQL
    rc = sqlite3_prepare_v2(db, sql_insert, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        send(client_socket, "Room creation failed", strlen("Room creation failed"), 0);
        return;
    }

    // Bind các tham số
    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, num_questions);
    sqlite3_bind_int(stmt, 3, time_limit);
    sqlite3_bind_text(stmt, 4, "not_started", -1, SQLITE_STATIC);

    // Thực thi câu lệnh
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        fprintf(stderr, "Failed to insert data: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        send(client_socket, "Room creation failed", strlen("Room creation failed"), 0);
        return;
    }

    // Lấy room_id tự động tăng vừa tạo
    int last_room_id = sqlite3_last_insert_rowid(db);

    // Trả về thông tin phòng thi đã tạo
    char response[256];
    snprintf(response, sizeof(response), "Room created successfully with ROOM_ID: %d (status: not_started)", last_room_id);
    send(client_socket, response, strlen(response), 0);

    // Dọn dẹp
    sqlite3_finalize(stmt);
    sqlite3_close(db);
}


void list_exam_rooms(int client_socket) {
    sqlite3 *db;
    char *err_msg = 0;

    // Mở cơ sở dữ liệu
    int rc = sqlite3_open("exam_system.db", &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        send(client_socket, "Database connection failed", strlen("Database connection failed"), 0);
        return;
    }

    // Truy vấn danh sách phòng thi
    const char *sql_select = "SELECT room_id, name, num_questions, time_limit, status FROM exam_rooms;";
    sqlite3_stmt *stmt;

    rc = sqlite3_prepare_v2(db, sql_select, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        send(client_socket, "Failed to fetch room list", strlen("Failed to fetch room list"), 0);
        return;
    }

    // Duyệt qua kết quả và gửi danh sách về client
    char response[1024] = "Exam Room List:\n";
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        int room_id = sqlite3_column_int(stmt, 0);
        const char *name = (const char *)sqlite3_column_text(stmt, 1);
        int num_questions = sqlite3_column_int(stmt, 2);
        int time_limit = sqlite3_column_int(stmt, 3);
        const char *status = (const char *)sqlite3_column_text(stmt, 4);

        char room_info[256];
        snprintf(room_info, sizeof(room_info), "Room ID: %d, Name: %s, Questions: %d, Time Limit: %d, Status: %s\n",
                 room_id, name, num_questions, time_limit, status);
        strncat(response, room_info, sizeof(response) - strlen(response) - 1);
    }

    if (strlen(response) == strlen("Exam Room List:\n")) {
        strncat(response, "No exam rooms found.\n", sizeof(response) - strlen(response) - 1);
    }

    // Gửi kết quả về client
    send(client_socket, response, strlen(response), 0);

    // Dọn dẹp
    sqlite3_finalize(stmt);
    sqlite3_close(db);
}


void handle_client_request(int client_socket) {
    char buffer[BUFFER_SIZE] = {0};
    read(client_socket, buffer, BUFFER_SIZE);

    char command[50], username[50], password[50], name[50];
    int num_questions, time_limit;

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
    } else if (buffer[0] == 0x01) {
        n = sscanf(buffer + 1, "%s %d %d", name, &num_questions, &time_limit);
        if (n != 3) {
            send(client_socket, "Invalid CREATE_ROOM command format", strlen("Invalid CREATE_ROOM command format"), 0);
            return;
        }

        printf("Creating exam room: %s with %d questions and %d minutes time limit\n", name, num_questions, time_limit);
        create_exam_room(client_socket, name, num_questions, time_limit);
    } else if (buffer[0] == 0x02) {
        list_exam_rooms(client_socket);
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
