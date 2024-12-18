#include "include/exam_room.h"
#include <stdio.h>
#include <sqlite3.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

#define DATABASE_PATH "database/exam_system.db"

void create_exam_room(int client_socket, const char *name, int num_questions, int time_limit, const char *category, const char *difficulty) {
    sqlite3 *db;

    // Mở cơ sở dữ liệu
    int rc = sqlite3_open("database/exam_system.db", &db);
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

    // Lấy câu hỏi cho phòng thi từ cơ sở dữ liệu
    sqlite3_stmt *question_stmt;
    const char *sql_select = "SELECT id, question_text, option_1, option_2, option_3, option_4 FROM questions WHERE category = ? AND difficulty = ? ORDER BY RANDOM() LIMIT ?;";

    rc = sqlite3_prepare_v2(db, sql_select, -1, &question_stmt, 0);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        send(client_socket, "Failed to get questions", strlen("Failed to get questions"), 0);
        return;
    }

    sqlite3_bind_text(question_stmt, 1, category, -1, SQLITE_STATIC);
    sqlite3_bind_text(question_stmt, 2, difficulty, -1, SQLITE_STATIC);
    sqlite3_bind_int(question_stmt, 3, num_questions);

    // Trả về câu hỏi cho người dùng
    char response[1024];
    int question_count = 0;
    while ((rc = sqlite3_step(question_stmt)) == SQLITE_ROW) {
        int question_id = sqlite3_column_int(question_stmt, 0);
        const char *question_text = (const char *)sqlite3_column_text(question_stmt, 1);
        const char *option_1 = (const char *)sqlite3_column_text(question_stmt, 2);
        const char *option_2 = (const char *)sqlite3_column_text(question_stmt, 3);
        const char *option_3 = (const char *)sqlite3_column_text(question_stmt, 4);
        const char *option_4 = (const char *)sqlite3_column_text(question_stmt, 5);

        snprintf(response, sizeof(response), "Question %d: %s\n1. %s\n2. %s\n3. %s\n4. %s\n",
                 question_id, question_text, option_1, option_2, option_3, option_4);
        send(client_socket, response, strlen(response), 0);
        question_count++;

        printf("Returned question %d\n", question_count);

        if (question_count >= num_questions) {
            break;
        }
    }

    if (question_count < num_questions) {
        send(client_socket, "Not enough questions available.", strlen("Not enough questions available."), 0);
    }

    sqlite3_finalize(question_stmt);
    sqlite3_finalize(stmt);
    sqlite3_close(db);
}

void list_exam_rooms(int client_socket) {
    sqlite3 *db;

    // Mở cơ sở dữ liệu
    int rc = sqlite3_open("database/exam_system.db", &db);
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

void join_exam_room(int client_socket, int room_id) {
    sqlite3 *db;
    int rc = sqlite3_open(DATABASE_PATH, &db);
    if (rc != SQLITE_OK) {
        send(client_socket, "Database connection failed", strlen("Database connection failed"), 0);
        return;
    }

    // Kiểm tra phòng thi có tồn tại không
    const char *sql_select = "SELECT room_id, num_clients, status, max_clients FROM exam_rooms WHERE room_id = ?";
    sqlite3_stmt *stmt;
    rc = sqlite3_prepare_v2(db, sql_select, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        send(client_socket, "Failed to prepare statement", strlen("Failed to prepare statement"), 0);
        sqlite3_close(db);
        return;
    }

    sqlite3_bind_int(stmt, 1, room_id);
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_ROW) {
        send(client_socket, "Room not found", strlen("Room not found"), 0);
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return;
    }

    // Lấy thông tin về số lượng người tham gia và trạng thái phòng thi
    int num_clients = sqlite3_column_int(stmt, 1);
    const char *status = (const char *)sqlite3_column_text(stmt, 2);
    int max_clients = sqlite3_column_int(stmt, 3);

    // Kiểm tra trạng thái phòng thi và số lượng người tham gia
    if (strcmp(status, "not_started") != 0) {
        send(client_socket, "Room has already started or finished", strlen("Room has already started or finished"), 0);
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return;
    }

    if (num_clients >= max_clients) {
        send(client_socket, "Room is full", strlen("Room is full"), 0);
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return;
    }

    // Cập nhật số lượng người tham gia
    const char *sql_update = "UPDATE exam_rooms SET num_clients = num_clients + 1 WHERE room_id = ?";
    rc = sqlite3_prepare_v2(db, sql_update, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        send(client_socket, "Failed to update room", strlen("Failed to update room"), 0);
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return;
    }

    sqlite3_bind_int(stmt, 1, room_id);
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        send(client_socket, "Failed to update room", strlen("Failed to update room"), 0);
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return;
    }

    // Lấy thông tin chi tiết phòng thi
    const char *sql_details = "SELECT name, num_questions, time_limit, status FROM exam_rooms WHERE room_id = ?";
    rc = sqlite3_prepare_v2(db, sql_details, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        send(client_socket, "Failed to fetch room details", strlen("Failed to fetch room details"), 0);
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return;
    }

    sqlite3_bind_int(stmt, 1, room_id);
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_ROW) {
        send(client_socket, "Room details not found", strlen("Room details not found"), 0);
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return;
    }

    // Lấy và gửi thông tin phòng thi
    const char *name = (const char *)sqlite3_column_text(stmt, 0);
    int num_questions = sqlite3_column_int(stmt, 1);
    int time_limit = sqlite3_column_int(stmt, 2);
    const char *room_status = (const char *)sqlite3_column_text(stmt, 3);

    char response[1024];
    snprintf(response, sizeof(response), "Success: Room %d (%s), Questions: %d, Time Limit: %d, Status: %s\n",
             room_id, name, num_questions, time_limit, room_status);
    send(client_socket, response, strlen(response), 0);

    sqlite3_finalize(stmt);
    sqlite3_close(db);
}
