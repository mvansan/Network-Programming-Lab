#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

void send_request(int sock, const char *message) {
    send(sock, message, strlen(message), 0);
}

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};

    char name[50];
    int num_questions, time_limit;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Socket creation error\n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("Invalid address\n");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("Connection Failed\n");
        return -1;
    }

    int choice;
    printf("1. Register\n2. Login\n3. Create Exam Room\n4. View Exam Rooms\nChoose an option: ");
    scanf("%d", &choice);

    char username[50], password[50], message[BUFFER_SIZE];
    if (choice == 1) {
        printf("Register - Enter username: ");
        scanf("%s", username);
        printf("Enter password: ");
        scanf("%s", password);
        snprintf(message, sizeof(message), "REGISTER %s %s", username, password);
    } else if (choice == 2) {
        printf("Login - Enter username: ");
        scanf("%s", username);
        printf("Enter password: ");
        scanf("%s", password);
        snprintf(message, sizeof(message), "LOGIN %s %s", username, password);
    } else if (choice == 3) {
        printf("Enter room name: ");
        scanf("%s", name);
        printf("Enter number of questions: ");
        scanf("%d", &num_questions);
        printf("Enter time limit (minutes): ");
        scanf("%d", &time_limit);

        time_limit *= 60;

        unsigned char message[BUFFER_SIZE];
        message[0] = 0x01;

        snprintf((char *)message + 1, sizeof(message) - 1, "%s %d %d", name, num_questions, time_limit);

        send_request(sock, (char *)message);
    } else if (choice == 4) {
        unsigned char message[BUFFER_SIZE];
        message[0] = 0x02;

        send_request(sock, (char *)message);
    } else {
        printf("Invalid choice\n");
        close(sock);
        return 0;
    }

    // Nhận phản hồi từ server
    read(sock, buffer, BUFFER_SIZE);
    printf("Server response: %s\n", buffer);

    close(sock);
    return 0;
}
