#include "include/exam_room.h"
#include <stdio.h>
#include <sqlite3.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

#define DATABASE_PATH "database/exam_system.db"

void create_exam_room(int client_socket, const char *name, int num_easy_questions, int num_medium_questions, int num_hard_questions, int time_limit, const char *category, const char *privacy, int max_people) {
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
    const char *sql_insert = "INSERT INTO exam_rooms (name, num_easy_questions, num_medium_questions, num_hard_questions, time_limit, category, privacy, max_people, status) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?);";
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
    sqlite3_bind_int(stmt, 2, num_easy_questions);
    sqlite3_bind_int(stmt, 3, num_medium_questions);
    sqlite3_bind_int(stmt, 4, num_hard_questions);
    sqlite3_bind_int(stmt, 5, time_limit);
    sqlite3_bind_text(stmt, 6, category, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 7, privacy, -1, SQLITE_STATIC);
    if (strcmp(privacy, "public") == 0) {
        sqlite3_bind_int(stmt, 8, max_people);
    } else {
        sqlite3_bind_null(stmt, 8);
    }
    sqlite3_bind_text(stmt, 9, "not_started", -1, SQLITE_STATIC);


    // Thực thi câu lệnh
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        fprintf(stderr, "Failed to insert data: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        send(client_socket, "Room creation failed", strlen("Room creation failed"), 0);
        return;
    }
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
    const char *sql_select = "SELECT room_id, name, num_easy_questions, num_medium_questions, num_hard_questions, time_limit, category, privacy, max_people, status FROM exam_rooms;";
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
        int num_easy_questions = sqlite3_column_int(stmt, 2);
        int num_medium_questions = sqlite3_column_int(stmt, 3);
        int num_hard_questions = sqlite3_column_int(stmt, 4);
        int time_limit = sqlite3_column_int(stmt, 5);
        const char *category = (const char *)sqlite3_column_text(stmt, 6);
        const char *privacy = (const char *)sqlite3_column_text(stmt, 7);
        int max_people = sqlite3_column_int(stmt, 8);
        const char *status = (const char *)sqlite3_column_text(stmt, 9);

        char room_info[256];
        snprintf(room_info, sizeof(room_info), "Room ID: %d, Name: %s, Easy Questions: %d, Medium Questions: %d, Hard Questions: %d, Time Limit: %d, Category: %s, Privacy: %s, Max People: %d, Status: %s\n",
                 room_id, name, num_easy_questions, num_medium_questions, num_hard_questions, time_limit, category, privacy, max_people, status);
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

