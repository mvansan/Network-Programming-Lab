#include "include/exam_room.h"
#include <stdio.h>
#include <sqlite3.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <take_exam.h>
#include <stdlib.h>

#define DATABASE_PATH "database/exam_system.db"

void create_exam_room(int client_socket, const char *name, int num_easy_questions, int num_medium_questions, int num_hard_questions, int time_limit, const char *category, const char *privacy, int max_people) {
    sqlite3 *db;
    char *err_msg = 0;
    int rc;

    // Mở cơ sở dữ liệu
    rc = sqlite3_open(DATABASE_PATH, &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        send(client_socket, "Database connection failed", strlen("Database connection failed"), 0);
        sqlite3_close(db);
        return;
    }

    // Bắt đầu giao dịch
    rc = sqlite3_exec(db, "BEGIN TRANSACTION;", 0, 0, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot begin transaction: %s\n", err_msg);
        sqlite3_free(err_msg);
        send(client_socket, "Room creation failed", strlen("Room creation failed"), 0);
        sqlite3_close(db);
        return;
    }

    // Thiết lập chế độ WAL
    rc = sqlite3_exec(db, "PRAGMA journal_mode=WAL;", 0, 0, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to set journal mode: %s\n", err_msg);
        sqlite3_free(err_msg);
    }

    const char *sql_insert = "INSERT INTO exam_rooms (name, num_easy_questions, num_medium_questions, num_hard_questions, time_limit, category, privacy, max_people, status) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?);";
    sqlite3_stmt *stmt;

    // Chuẩn bị câu lệnh SQL
    rc = sqlite3_prepare_v2(db, sql_insert, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        send(client_socket, "Room creation failed", strlen("Room creation failed"), 0);
        sqlite3_exec(db, "ROLLBACK;", 0, 0, 0);
        sqlite3_close(db);
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
        send(client_socket, "Room creation failed", strlen("Room creation failed"), 0);
        sqlite3_exec(db, "ROLLBACK;", 0, 0, 0);
        sqlite3_close(db);
        return;
    }

    // Lấy ID của phòng thi vừa tạo
    int room_id = sqlite3_last_insert_rowid(db);

    // Lấy câu hỏi cho phòng thi từ cơ sở dữ liệu và insert vào bảng exam_questions
    sqlite3_stmt *question_stmt;
    const char *sql_select = "SELECT id FROM questions WHERE category = ? AND difficulty = ? ORDER BY RANDOM() LIMIT ?;";

    rc = sqlite3_prepare_v2(db, sql_select, -1, &question_stmt, 0);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        send(client_socket, "Failed to get questions", strlen("Failed to get questions"), 0);
        sqlite3_exec(db, "ROLLBACK;", 0, 0, 0);
        sqlite3_close(db);
        return;
    }

    sqlite3_bind_text(question_stmt, 1, category, -1, SQLITE_STATIC);

    // Xử lý từng độ khó (easy, medium, hard) và chèn câu hỏi vào bảng exam_questions
    const char *sql_insert_question = "INSERT INTO exam_questions (exam_room_id, question_id) VALUES (?, ?);";
    sqlite3_stmt *insert_question_stmt;
    rc = sqlite3_prepare_v2(db, sql_insert_question, -1, &insert_question_stmt, 0);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement for inserting questions: %s\n", sqlite3_errmsg(db));
        send(client_socket, "Failed to prepare insert question statement", strlen("Failed to prepare insert question statement"), 0);
        sqlite3_exec(db, "ROLLBACK;", 0, 0, 0);
        sqlite3_close(db);
        return;
    }

    // Xử lý câu hỏi dễ
    sqlite3_bind_text(question_stmt, 2, "easy", -1, SQLITE_STATIC);
    sqlite3_bind_int(question_stmt, 3, num_easy_questions);
    while ((rc = sqlite3_step(question_stmt)) == SQLITE_ROW) {
        int question_id = sqlite3_column_int(question_stmt, 0);
        sqlite3_bind_int(insert_question_stmt, 1, room_id);
        sqlite3_bind_int(insert_question_stmt, 2, question_id);
        rc = sqlite3_step(insert_question_stmt);
        if (rc != SQLITE_DONE) {
            fprintf(stderr, "Failed to insert question into exam_questions: %s\n", sqlite3_errmsg(db));
        }
        sqlite3_reset(insert_question_stmt);
    }

    // Xử lý câu hỏi trung bình
    sqlite3_bind_text(question_stmt, 2, "medium", -1, SQLITE_STATIC);
    sqlite3_bind_int(question_stmt, 3, num_medium_questions);
    while ((rc = sqlite3_step(question_stmt)) == SQLITE_ROW) {
        int question_id = sqlite3_column_int(question_stmt, 0);
        sqlite3_bind_int(insert_question_stmt, 1, room_id);
        sqlite3_bind_int(insert_question_stmt, 2, question_id);
        rc = sqlite3_step(insert_question_stmt);
        if (rc != SQLITE_DONE) {
            fprintf(stderr, "Failed to insert question into exam_questions: %s\n", sqlite3_errmsg(db));
        }
        sqlite3_reset(insert_question_stmt);
    }

    // Xử lý câu hỏi khó
    sqlite3_bind_text(question_stmt, 2, "hard", -1, SQLITE_STATIC);
    sqlite3_bind_int(question_stmt, 3, num_hard_questions);
    while ((rc = sqlite3_step(question_stmt)) == SQLITE_ROW) {
        int question_id = sqlite3_column_int(question_stmt, 0);
        sqlite3_bind_int(insert_question_stmt, 1, room_id);
        sqlite3_bind_int(insert_question_stmt, 2, question_id);
        rc = sqlite3_step(insert_question_stmt);
        if (rc != SQLITE_DONE) {
            fprintf(stderr, "Failed to insert question into exam_questions: %s\n", sqlite3_errmsg(db));
        }
        sqlite3_reset(insert_question_stmt);
    }

    if (rc != SQLITE_DONE && rc != SQLITE_ROW) {
        fprintf(stderr, "Failed to fetch or insert questions: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(question_stmt);
        sqlite3_finalize(insert_question_stmt);
        sqlite3_finalize(stmt);
        send(client_socket, "Failed to retrieve or insert questions", strlen("Failed to retrieve or insert questions"), 0);
        sqlite3_exec(db, "ROLLBACK;", 0, 0, 0);
        sqlite3_close(db);
        return;
    }

    sqlite3_finalize(question_stmt);
    sqlite3_finalize(insert_question_stmt);
    sqlite3_finalize(stmt);

    // Commit giao dịch
    rc = sqlite3_exec(db, "COMMIT;", 0, 0, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot commit transaction: %s\n", err_msg);
        sqlite3_free(err_msg);
        send(client_socket, "Room creation failed", strlen("Room creation failed"), 0);
        sqlite3_exec(db, "ROLLBACK;", 0, 0, 0);
        sqlite3_close(db);
        return;
    }

    sqlite3_close(db);

    // Gửi thông báo phòng thi đã được tạo và các câu hỏi đã được thêm vào
    send(client_socket, "Exam room created successfully with questions.", strlen("Exam room created successfully with questions."), 0);
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
    const char *sql_select = "SELECT room_id, name, num_easy_questions, num_medium_questions, num_hard_questions, time_limit, category, privacy, max_people, status, num_clients FROM exam_rooms;";
    sqlite3_stmt *stmt;

    rc = sqlite3_prepare_v2(db, sql_select, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        send(client_socket, "Failed to fetch room list", strlen("Failed to fetch room list"), 0);
        return;
    }

    // Duyệt qua kết quả và gửi danh sách về client
    char response[4096] = "\nExam Room List:\n";
    strncat(response, "-----------------------------------------------------------------------------------------------------------------------------------------------\n", sizeof(response) - strlen(response) - 1);
    strncat(response, "| Room ID | Name              | Easy | Medium | Hard | Time Limit | Category  | Privacy | Max People | Status     | Clients |\n", sizeof(response) - strlen(response) - 1);
    strncat(response, "-----------------------------------------------------------------------------------------------------------------------------------------------\n", sizeof(response) - strlen(response) - 1);

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
        int num_clients = sqlite3_column_int(stmt, 10);

        char room_info[512];
        snprintf(room_info, sizeof(room_info), "| %-7d | %-17s | %-4d | %-6d | %-4d | %-10d | %-9s | %-7s | %-10d | %-10s | %-7d |\n",
                 room_id, name, num_easy_questions, num_medium_questions, num_hard_questions, time_limit, category, privacy, max_people, status, num_clients);
        strncat(response, room_info, sizeof(response) - strlen(response) - 1);
    }

    if (strlen(response) == strlen("\nExam Room List:\n")) {
        strncat(response, "No exam rooms found.\n", sizeof(response) - strlen(response) - 1);
    }

    strncat(response, "-----------------------------------------------------------------------------------------------------------------------------------------------\n", sizeof(response) - strlen(response) - 1);

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

    // Thiết lập chế độ WAL
    rc = sqlite3_exec(db, "PRAGMA journal_mode=WAL;", 0, 0, 0);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to set journal mode: %s\n", sqlite3_errmsg(db));
    }

    sqlite3_exec(db, "BEGIN TRANSACTION;", 0, 0, 0);

    const char *sql_select = "SELECT room_id, num_clients, status, max_people FROM exam_rooms WHERE room_id = ?";
    sqlite3_stmt *stmt;
    rc = sqlite3_prepare_v2(db, sql_select, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        const char *err_msg = sqlite3_errmsg(db);
        send(client_socket, err_msg, strlen(err_msg), 0);
        sqlite3_finalize(stmt);
        sqlite3_exec(db, "ROLLBACK;", 0, 0, 0);
        sqlite3_close(db);
        return;
    }

    sqlite3_bind_int(stmt, 1, room_id);
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_ROW) {
        send(client_socket, "Room not found", strlen("Room not found"), 0);
        sqlite3_finalize(stmt);
        sqlite3_exec(db, "ROLLBACK;", 0, 0, 0);
        sqlite3_close(db);
        return;
    }

    int num_clients = sqlite3_column_int(stmt, 1);
    const char *status = (const char *)sqlite3_column_text(stmt, 2);
    int max_people = sqlite3_column_int(stmt, 3);

    if (strcmp(status, "not_started") != 0) {
        send(client_socket, "Room has already started or finished", strlen("Room has already started or finished"), 0);
        sqlite3_finalize(stmt);
        sqlite3_exec(db, "ROLLBACK;", 0, 0, 0);
        sqlite3_close(db);
        return;
    }

    if (num_clients >= max_people) {
        send(client_socket, "Room is full", strlen("Room is full"), 0);
        sqlite3_finalize(stmt);
        sqlite3_exec(db, "ROLLBACK;", 0, 0, 0);
        sqlite3_close(db);
        return;
    }

    const char *sql_update = "UPDATE exam_rooms SET num_clients = num_clients + 1 WHERE room_id = ?";
    rc = sqlite3_prepare_v2(db, sql_update, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        const char *err_msg = sqlite3_errmsg(db);
        send(client_socket, err_msg, strlen(err_msg), 0);
        sqlite3_finalize(stmt);
        sqlite3_exec(db, "ROLLBACK;", 0, 0, 0);
        sqlite3_close(db);
        return;
    }

    sqlite3_bind_int(stmt, 1, room_id);
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        const char *err_msg = sqlite3_errmsg(db);
        printf("SQL error: %s\n", err_msg);
        send(client_socket, "Failed to update room", strlen("Failed to update room"), 0);
        sqlite3_finalize(stmt);
        sqlite3_exec(db, "ROLLBACK;", 0, 0, 0);
        sqlite3_close(db);
        return;
    }

    sqlite3_finalize(stmt);
    sqlite3_exec(db, "COMMIT;", 0, 0, 0);  // Commit transaction

    sqlite3_close(db);

    take_exam(client_socket, room_id);
}
