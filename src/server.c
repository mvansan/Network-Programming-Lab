#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sqlite3.h>
#include <sys/select.h>
#include <sys/time.h>
#include <fcntl.h>
#include <pthread.h>

#include "include/init_db.h"
#include "include/auth.h"
#include "include/exam_room.h"

#define PORT 8080
#define BUFFER_SIZE 8000
#define MAX_CLIENTS 30

#define CREATE_ROOM 0x01
#define LIST_ROOMS 0x02
#define JOIN_ROOM 0x03
#define LOGOUT 0x04
#define START_EXAM 0x05
#define LIST_USER_ROOMS 0x06

typedef struct {
    int client_socket;
} client_args;

typedef struct {
    int client_socket;
    int userID;
} client_session;

client_session sessions[MAX_CLIENTS];
pthread_mutex_t session_mutex = PTHREAD_MUTEX_INITIALIZER;

void init_sessions() {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        sessions[i].client_socket = -1;
        sessions[i].userID = -1;
    }
}

void store_user_session(int client_socket, int userID) {
    pthread_mutex_lock(&session_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (sessions[i].client_socket == -1) {
            sessions[i].client_socket = client_socket;
            sessions[i].userID = userID;
            break;
        }
    }
    pthread_mutex_unlock(&session_mutex);
}

int get_user_session(int client_socket) {
    pthread_mutex_lock(&session_mutex);
    int userID = -1;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (sessions[i].client_socket == client_socket) {
            userID = sessions[i].userID;
            break;
        }
    }
    pthread_mutex_unlock(&session_mutex);
    return userID;
}

void remove_user_session(int client_socket) {
    pthread_mutex_lock(&session_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (sessions[i].client_socket == client_socket) {
            sessions[i].client_socket = -1;
            sessions[i].userID = -1;
            break;
        }
    }
    pthread_mutex_unlock(&session_mutex);
}

void handle_login(int client_socket, const char *username, const char *password) {
    int userID = login_user(username, password);
    if (userID > 0) { // Đăng nhập thành công nếu userID > 0
        store_user_session(client_socket, userID); // Lưu userID vào phiên đăng nhập
        char response[BUFFER_SIZE];
        snprintf(response, sizeof(response), "Login successful ID: %d", userID);
        send(client_socket, response, strlen(response), 0);
    } else {
        send(client_socket, "Login failed", strlen("Login failed"), 0);
    }
}

void handle_create_exam_room(int client_socket, const char *request) {
    char name[50], category[20], privacy[10];
    int num_easy_questions, num_medium_questions, num_hard_questions, time_limit, max_people;

    sscanf(request, "%s %d %d %d %d %s %s %d", name, &num_easy_questions, &num_medium_questions, &num_hard_questions, &time_limit, category, privacy, &max_people);

    int userID = get_user_session(client_socket); // Lấy userID từ phiên đăng nhập

    if (userID > 0) {
        create_exam_room(client_socket, name, num_easy_questions, num_medium_questions, num_hard_questions, time_limit, category, privacy, max_people, userID);
    } else {
        send(client_socket, "User not logged in", strlen("User not logged in"), 0);
    }
}

void *handle_client_request(void *args) {
    client_args *client = (client_args *)args;
    int client_socket = client->client_socket;
    free(client);

    char buffer[BUFFER_SIZE] = {0};
    read(client_socket, buffer, BUFFER_SIZE);

    char command[50], username[50], password[50], name[50], category[50], privacy[10];
    int num_easy_questions, num_medium_questions, num_hard_questions, time_limit, max_people, room_id;

    int n = sscanf(buffer, "%s", command);
    if (n < 1) {
        send(client_socket, "Invalid command", strlen("Invalid command"), 0);
        close(client_socket);
        return NULL;
    }

    if (strcmp(command, "REGISTER") == 0) {
        n = sscanf(buffer, "%s %s %s", command, username, password);
        if (n != 3) {
            send(client_socket, "Invalid REGISTER command format", strlen("Invalid REGISTER command format"), 0);
            close(client_socket);
            return NULL;
        }

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
            close(client_socket);
            return NULL;
        }

        handle_login(client_socket, username, password); // Gọi hàm xử lý đăng nhập
    } else if (buffer[0] == CREATE_ROOM) {
        n = sscanf(buffer + 1, "%s %d %d %d %d %s %s %d", name, &num_easy_questions, &num_medium_questions, &num_hard_questions, &time_limit, category, privacy, &max_people);
        if (n != 8) {
            send(client_socket, "Invalid CREATE_ROOM command format", strlen("Invalid CREATE_ROOM command format"), 0);
            close(client_socket);
            return NULL;
        }

        handle_create_exam_room(client_socket, buffer + 1); // Gọi hàm xử lý tạo phòng thi
    } else if (buffer[0] == LIST_ROOMS) {
        list_exam_rooms(client_socket);
    } else if (buffer[0] == JOIN_ROOM) {
        n = sscanf(buffer + 1, "%d", &room_id);
        if (n != 1) {
            send(client_socket, "Invalid JOIN_ROOM command format", strlen("Invalid JOIN_ROOM command format"), 0);
            close(client_socket);
            return NULL;
        }

        join_exam_room(client_socket, room_id);
    } else if (buffer[0] == START_EXAM) {
        n = sscanf(buffer + 1, "%d", &room_id);
        if (n != 1) {
            send(client_socket, "Invalid START_EXAM command format", strlen("Invalid START_EXAM command format"), 0);
            close(client_socket);
            return NULL;
        }

        int userID = get_user_session(client_socket);
        start_exam_room(client_socket, room_id, userID);
    } else if (buffer[0] == LIST_USER_ROOMS) {
        int userID = get_user_session(client_socket);
        list_user_exam_rooms(client_socket, userID);
    } else {
        send(client_socket, "Unknown command", strlen("Unknown command"), 0);
    }
    close(client_socket);
    return NULL;
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    int client_sockets[MAX_CLIENTS];
    fd_set readfds;
    int max_sd;

    for (int i = 0; i < MAX_CLIENTS; i++) {
        client_sockets[i] = 0;
    }

    init_sessions(); // Khởi tạo mảng sessions
    init_database();

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
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

    if (listen(server_fd, 10) < 0) {
        perror("Listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server is listening on port %d\n", PORT);

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        max_sd = server_fd;

        for (int i = 0; i < MAX_CLIENTS; i++) {
            int sd = client_sockets[i];
            if (sd > 0) {
                FD_SET(sd, &readfds);
            }
            if (sd > max_sd) {
                max_sd = sd;
            }
        }

        int activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

        if (activity < 0) {
            perror("select error");
            continue;
        }

        if (FD_ISSET(server_fd, &readfds)) {
            if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
                perror("Accept failed");
                continue;
            }
            printf("New connection established, socket fd: %d\n", new_socket);

            pthread_t thread_id;
            client_args *args = malloc(sizeof(client_args));
            args->client_socket = new_socket;
            pthread_create(&thread_id, NULL, handle_client_request, (void*)args);
            pthread_detach(thread_id);
        }
    }

    close(server_fd);
    return 0;
}
