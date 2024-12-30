#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 8000

// Define message types
#define CREATE_EXAM_ROOM 0x01
#define VIEW_EXAM_ROOMS  0x02
#define JOIN_EXAM_ROOM   0x03
#define LOGOUT           0x04
#define START_EXAM       0x05
#define LIST_USER_ROOMS  0x06
#define VIEW_HISTORY     0x07

void send_request(int sock, const char *message) {
    send(sock, message, strlen(message), 0);
}

int connect_to_server(struct sockaddr_in *serv_addr) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        printf("Socket creation error\n");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)serv_addr, sizeof(*serv_addr)) < 0) {
        printf("Connection Failed\n");
        close(sock);
        return -1;
    }

    return sock;
}

void start_exam(int sock, int room_id) {
    unsigned char message[BUFFER_SIZE];
    message[0] = START_EXAM;  // Sử dụng mã 0x05 cho bắt đầu thi
    snprintf((char *)message + 1, sizeof(message) - 1, "%d", room_id);

    send_request(sock, (char *)message);  // Gửi yêu cầu START_EXAM

    char buffer[BUFFER_SIZE] = {0};
    int bytes_received;

    // Nhận câu hỏi từ server và gửi câu trả lời
    while ((bytes_received = recv(sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytes_received] = '\0';

        if (strstr(buffer, "Exam finished") != NULL) {
            printf("%s\n", buffer);
            break;
        }

        printf("%s\n", buffer);

        // Client xử lý từng câu hỏi và gửi câu trả lời
        if (strstr(buffer, "Enter your answer (1-4): ") != NULL) {
            char answer[2];
            printf("Enter your answer (1-4): ");
            scanf("%s", answer);  // User nhập câu trả lời

            send(sock, answer, strlen(answer), 0); // Gửi câu trả lời đến server
        }
    }

    if (bytes_received <= 0) {
        printf("Error during exam.\n");
    }
}

void join_room(int sock, int room_id) {
    unsigned char message[BUFFER_SIZE];
    message[0] = JOIN_EXAM_ROOM;  // Sử dụng mã 0x03 cho tham gia phòng
    snprintf((char *)message + 1, sizeof(message) - 1, "%d", room_id);

    send_request(sock, (char *)message);  // Gửi yêu cầu JOIN_EXAM_ROOM

    char buffer[BUFFER_SIZE] = {0};
    int bytes_received;

    while (1) {
        // Lắng nghe trạng thái từ server
        bytes_received = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received <= 0) {
            printf("Disconnected from server.\n");
            break;
        }

        buffer[bytes_received] = '\0';

        if (strstr(buffer, "Waiting for the exam to start...") != NULL) {
            printf("%s\n", buffer);
        } else if (strstr(buffer, "Exam started") != NULL) {
            printf("Exam started! Answer the questions below.\n");
            break; // Chuyển sang chế độ thi
        } else {
            printf("Unexpected server response: %s\n", buffer);
            break;
        }
    }

    // Nhận câu hỏi và gửi câu trả lời
    while ((bytes_received = recv(sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytes_received] = '\0';

        if (strstr(buffer, "Exam finished") != NULL) {
            printf("%s\n", buffer);
            break;
        }

        printf("%s\n", buffer);

        if (strstr(buffer, "Enter your answer (1-4): ") != NULL) {
            char answer[2];
            scanf("%s", answer);
            send(sock, answer, strlen(answer), 0);  // Gửi câu trả lời
        }
    }

    if (bytes_received <= 0) {
        printf("Error or disconnected during the exam.\n");
    }
}

void view_history(int sock) {
    unsigned char message[BUFFER_SIZE];
    message[0] = VIEW_HISTORY;  // Ensure this matches the server's command handling

    send_request(sock, (char *)message);

    char buffer[BUFFER_SIZE] = {0};
    int bytes_received = recv(sock, buffer, sizeof(buffer) - 1, 0);
    buffer[bytes_received] = '\0';
    printf("Received: %s\n", buffer);
}

int main() {
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("Invalid address\n");
        return -1;
    }

    int choice;
    char username[50], password[50], message[BUFFER_SIZE];
    int authenticated = 0;

    while (!authenticated) {
        int sock = connect_to_server(&serv_addr);
        if (sock < 0) {
            return -1;
        }

        printf("1. Register\n2. Login\nChoose an option: ");
        scanf("%d", &choice);

        if (choice == 1) {
            printf("Register - Enter username: ");
            scanf("%s", username);
            printf("Enter password: ");
            scanf("%s", password);
            snprintf(message, sizeof(message), "REGISTER %s %s", username, password);
            send_request(sock, message);
        } else if (choice == 2) {
            printf("Login - Enter username: ");
            scanf("%s", username);
            printf("Enter password: ");
            scanf("%s", password);
            snprintf(message, sizeof(message), "LOGIN %s %s", username, password);
            send_request(sock, message);
        } else {
            printf("Invalid choice\n");
            close(sock);
            continue;
        }

        int bytes_received = recv(sock, buffer, sizeof(buffer) - 1, 0);
        buffer[bytes_received] = '\0';
        printf("Received: %s\n", buffer);

        if (strstr(buffer, "successful")) {
            authenticated = 1;
        } else {
            printf("Authentication failed. Please try again.\n");
        }

        close(sock);
    }

    while (1) {
        int sock = connect_to_server(&serv_addr);
        if (sock < 0) {
            return -1;
        }

        printf("3. Create Exam Room\n4. View Exam Rooms\n5. Join Room\n6. Start Exam\n7. View Exam History\n8. Logouut\nChoose an option: ");
        scanf("%d", &choice);

        if (choice == 3) {
            char name[50];
            int num_easy_questions, num_medium_questions, num_hard_questions, time_limit, max_people = 0, room_limit =0;
            char category[20], privacy[10];

            printf("Enter room name: ");
            scanf("%s", name);
            printf("Select exam category (IT, Science, Blockchain): ");
            scanf("%s", category);
            printf("Enter number of easy questions: ");
            scanf("%d", &num_easy_questions);
            printf("Enter number of medium questions: ");
            scanf("%d", &num_medium_questions);
            printf("Enter number of hard questions: ");
            scanf("%d", &num_hard_questions);
            printf("Enter time limit (minutes): ");
            scanf("%d", &time_limit);
            printf("Enter privacy (public, private): ");
            scanf("%s", privacy);

            if (strcmp(privacy, "public") == 0) {
                printf("Enter maximum number of people: ");
                scanf("%d", &max_people);
            }
         if (strcmp(privacy, "private") == 0) {
                printf("Enter limit: ");
                scanf("%d", &room_limit);
            }

            unsigned char message[BUFFER_SIZE];
            message[0] = CREATE_EXAM_ROOM;
            snprintf((char *)message + 1, sizeof(message) - 1, "%s %d %d %d %d %s %s %d %d", name, num_easy_questions, num_medium_questions, num_hard_questions, time_limit, category, privacy, max_people, room_limit);

            send_request(sock, (char *)message);
        } else if (choice == 4) {
            unsigned char message[BUFFER_SIZE];
            message[0] = VIEW_EXAM_ROOMS;

            send_request(sock, (char *)message);
        } else if (choice == 5) {
            int room_id;
            printf("Enter the room ID to join: ");
            scanf("%d", &room_id);
            join_room(sock, room_id);  // Xử lý tham gia phòng
        } else if (choice == 6) {
            int room_id;
            unsigned char message[BUFFER_SIZE];
            message[0] = LIST_USER_ROOMS;
            
            send_request(sock, (char *)message);
            
            int bytes_received = recv(sock, buffer, sizeof(buffer) - 1, 0);
            buffer[bytes_received] = '\0';
            printf("Received: %s\n", buffer);

            printf("Enter the room ID to start the exam: ");
            scanf("%d", &room_id);
            start_exam(sock, room_id);  // Xử lý bắt đầu thi
        } else if (choice == 7) {
            view_history(sock);
        } else if (choice == 8) {
            unsigned char message[BUFFER_SIZE];
            message[0] = LOGOUT;

            send_request(sock, (char *)message);
            printf("Logged out successfully.\n");
            close(sock);
            break;
        } else {
            printf("Invalid choice\n");
            close(sock);
            continue;
        }

        int bytes_received = recv(sock, buffer, sizeof(buffer) - 1, 0);
        buffer[bytes_received] = '\0';
        printf("Received: %s\n", buffer);

        close(sock);
    }

    return 0;
}
