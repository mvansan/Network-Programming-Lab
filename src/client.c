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
    message[0] = JOIN_EXAM_ROOM;
    snprintf((char *)message + 1, sizeof(message) - 1, "%d", room_id); // Send room_id

    send_request(sock, (char *)message); // Send JOIN_EXAM_ROOM request to server

    char buffer[BUFFER_SIZE] = {0};
    int bytes_received;

    // Receive questions from the server and send answers
    while ((bytes_received = recv(sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytes_received] = '\0';
        printf("Received exam questions:\n%s\n", buffer);

        // Client processes each question and sends an answer
        char answer[2];
        printf("Enter your answer (1-4): ");
        scanf("%s", answer);  // User enters answer

        send(sock, answer, strlen(answer), 0); // Send the answer to the server
    }

    if (bytes_received == 0) {
        printf("Exam finished.\n");
    } else {
        printf("Error during exam.\n");
    }
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

        printf("3. Create Exam Room\n4. View Exam Rooms\n5. Start Exam\n6. Logout\nChoose an option: ");
        scanf("%d", &choice);

        if (choice == 3) {
            char name[50];
            int num_easy_questions, num_medium_questions, num_hard_questions, time_limit, max_people = 0;
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

            time_limit *= 60;

            unsigned char message[BUFFER_SIZE];
            message[0] = CREATE_EXAM_ROOM;  // Sử dụng mã 0x01 cho tạo phòng thi

            snprintf((char *)message + 1, sizeof(message) - 1, "%s %d %d %d %d %s %s %d", name, num_easy_questions, num_medium_questions, num_hard_questions, time_limit, category, privacy, max_people);

            send_request(sock, (char *)message);
        } else if (choice == 4) {
            unsigned char message[BUFFER_SIZE];
            message[0] = VIEW_EXAM_ROOMS;  // Sử dụng mã 0x02 cho xem danh sách phòng thi

            send_request(sock, (char *)message);
        } else if (choice == 5) {
            int room_id;
            printf("Enter the room ID to start the exam: ");
            scanf("%d", &room_id);
            start_exam(sock, room_id);  // Gọi hàm start_exam để bắt đầu thi
        } else if (choice == 6) {
            unsigned char message[BUFFER_SIZE];
            message[0] = LOGOUT;  // Sử dụng mã 0x04 cho logout

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
