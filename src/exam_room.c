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

void insert_questions(const char *category, const char *difficulty, int num_questions, int room_id, sqlite3 *db, sqlite3_stmt *question_stmt, int client_socket) {
    int rc;

    sqlite3_bind_text(question_stmt, 1, category, -1, SQLITE_STATIC);
    sqlite3_bind_text(question_stmt, 2, difficulty, -1, SQLITE_STATIC);
    sqlite3_bind_int(question_stmt, 3, num_questions);

    while ((rc = sqlite3_step(question_stmt)) == SQLITE_ROW) {
        int question_id = sqlite3_column_int(question_stmt, 0);
        sqlite3_stmt *insert_question_stmt;
        const char *sql_insert_question = "INSERT INTO exam_questions (exam_room_id, question_id) VALUES (?, ?);";
        rc = sqlite3_prepare_v2(db, sql_insert_question, -1, &insert_question_stmt, 0);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "Failed to prepare statement for inserting questions: %s\n", sqlite3_errmsg(db));
            sqlite3_finalize(question_stmt);
            sqlite3_close(db);
            send(client_socket, "Failed to prepare insert question statement", strlen("Failed to prepare insert question statement"), 0);
            return;
        }
        sqlite3_bind_int(insert_question_stmt, 1, room_id);
        sqlite3_bind_int(insert_question_stmt, 2, question_id);
        rc = sqlite3_step(insert_question_stmt);
        if (rc != SQLITE_DONE) {
            fprintf(stderr, "Failed to insert question into exam_questions: %s\n", sqlite3_errmsg(db));
        }
        sqlite3_finalize(insert_question_stmt);
    }
    sqlite3_reset(question_stmt);
}
void create_exam_room(int client_socket, const char *name, int num_easy_questions, int num_medium_questions, int num_hard_questions, int time_limit, const char *category, const char *privacy, int max_people, int userID) {
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

    const char *sql_insert = "INSERT INTO exam_rooms (name, num_easy_questions, num_medium_questions, num_hard_questions, time_limit, category, privacy, max_people, status, userID) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";
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
    sqlite3_bind_text(stmt, 9, "pending", -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 10, userID);

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
    sqlite3_finalize(stmt);

    // Insert câu hỏi (như cũ)
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

    // Insert easy questions
    insert_questions(category, "easy", num_easy_questions, room_id, db, question_stmt, client_socket);

    // Insert medium questions
    insert_questions(category, "medium", num_medium_questions, room_id, db, question_stmt, client_socket);

    // Insert hard questions
    insert_questions(category, "hard", num_hard_questions, room_id, db, question_stmt, client_socket);

    sqlite3_finalize(question_stmt);

    // Commit transaction
    rc = sqlite3_exec(db, "COMMIT;", 0, 0, 0);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to commit transaction: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        send(client_socket, "Room creation failed", strlen("Room creation failed"), 0);
        return;
    }

    sqlite3_close(db);
    send(client_socket, "Room created successfully", strlen("Room created successfully"), 0);
}

void list_exam_rooms(int client_socket) {
    sqlite3 *db;

    // Mở cơ sở dữ liệu
    int rc = sqlite3_open(DATABASE_PATH, &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        send(client_socket, "Database connection failed", strlen("Database connection failed"), 0);
        return;
    }

    // Truy vấn danh sách phòng thi
    const char *sql_select = "SELECT room_id, name, num_easy_questions, num_medium_questions, num_hard_questions, time_limit, category, privacy, max_people, status, num_clients, userID FROM exam_rooms;";
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
    strncat(response, "---------------------------------------------------------------------------------------------------------------------------------------------------------\n", sizeof(response) - strlen(response) - 1);
    strncat(response, "| Room ID | Name              | Easy | Medium | Hard | Time Limit | Category  | Privacy | Max People | Status     | Clients | User ID |\n", sizeof(response) - strlen(response) - 1);
    strncat(response, "---------------------------------------------------------------------------------------------------------------------------------------------------------\n", sizeof(response) - strlen(response) - 1);

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
        int userID = sqlite3_column_int(stmt, 11);

        char room_info[512];
        snprintf(room_info, sizeof(room_info), "| %-7d | %-17s | %-4d | %-6d | %-4d | %-10d | %-9s | %-7s | %-10d | %-10s | %-7d | %-7d |\n",
                 room_id, name, num_easy_questions, num_medium_questions, num_hard_questions, time_limit, category, privacy, max_people, status, num_clients, userID);
        strncat(response, room_info, sizeof(response) - strlen(response) - 1);
    }

    if (strlen(response) == strlen("\nExam Room List:\n")) {
        strncat(response, "No exam rooms found.\n", sizeof(response) - strlen(response) - 1);
    }

    strncat(response, "---------------------------------------------------------------------------------------------------------------------------------------------------------\n", sizeof(response) - strlen(response) - 1);

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

    const char *sql_select = "SELECT num_clients, max_people, status, waiting_clients FROM exam_rooms WHERE room_id = ?;";
    sqlite3_stmt *stmt;

    rc = sqlite3_prepare_v2(db, sql_select, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        send(client_socket, "Failed to prepare select statement", strlen("Failed to prepare select statement"), 0);
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

    int num_clients = sqlite3_column_int(stmt, 0);
    int max_people = sqlite3_column_int(stmt, 1);
    const char *status = (const char *)sqlite3_column_text(stmt, 2);
    const char *waiting_clients = (const char *)sqlite3_column_text(stmt, 3);
    if (!waiting_clients) waiting_clients = "";  // Giá trị mặc định nếu null

    if (strcmp(status, "pending") != 0) {
        send(client_socket, "Room is not available for joining", strlen("Room is not available for joining"), 0);
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return;
    }

    if (num_clients >= max_people) {
        send(client_socket, "Room is full", strlen("Room is full"), 0);
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return;
    }

    sqlite3_finalize(stmt);  // Giải phóng trước khi dùng lại truy vấn

    // Cập nhật danh sách chờ
    char updated_waiting_clients[1024];
    snprintf(updated_waiting_clients, sizeof(updated_waiting_clients), "%s,%d", waiting_clients, client_socket);

    const char *sql_update = "UPDATE exam_rooms SET num_clients = num_clients + 1, waiting_clients = ? WHERE room_id = ?;";
    rc = sqlite3_prepare_v2(db, sql_update, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        send(client_socket, "Failed to prepare update statement", strlen("Failed to prepare update statement"), 0);
        sqlite3_close(db);
        return;
    }

    sqlite3_bind_text(stmt, 1, updated_waiting_clients, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, room_id);
    rc = sqlite3_step(stmt);

    if (rc != SQLITE_DONE) {
        send(client_socket, "Failed to update room information", strlen("Failed to update room information"), 0);
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return;
    }
    sqlite3_finalize(stmt);

    // Thông báo client đợi bắt đầu thi
    send(client_socket, "Waiting for the exam to start...", strlen("Waiting for the exam to start..."), 0);

    // Kiểm tra trạng thái phòng và giữ kết nối
    while (1) {
        sleep(1);  // Chờ một giây
        rc = sqlite3_prepare_v2(db, sql_select, -1, &stmt, 0);
        if (rc != SQLITE_OK) {
            send(client_socket, "Database error during waiting", strlen("Database error during waiting"), 0);
            sqlite3_close(db);
            return;
        }

        sqlite3_bind_int(stmt, 1, room_id);
        rc = sqlite3_step(stmt);

        if (rc == SQLITE_ROW) {
            const char *current_status = (const char *)sqlite3_column_text(stmt, 2);
            if (strcmp(current_status, "in_progress") == 0) {
                send(client_socket, "Exam started! Answer the questions below.", strlen("Exam started! Answer the questions below."), 0);
                sqlite3_finalize(stmt);
                sqlite3_close(db);
                return;  // Bắt đầu thi
            }
        }

        sqlite3_finalize(stmt);
    }
}


void start_exam_room(int client_socket, int room_id) {
    sqlite3 *db;
    int rc = sqlite3_open(DATABASE_PATH, &db);
    if (rc != SQLITE_OK) {
        send(client_socket, "Database connection failed", strlen("Database connection failed"), 0);
        return;
    }

    const char *sql_select = "SELECT status, waiting_clients FROM exam_rooms WHERE room_id = ?;";
    sqlite3_stmt *stmt;
    rc = sqlite3_prepare_v2(db, sql_select, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        send(client_socket, "Failed to prepare select statement", strlen("Failed to prepare select statement"), 0);
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

    const char *status = (const char *)sqlite3_column_text(stmt, 0);
    const char *waiting_clients = (const char *)sqlite3_column_text(stmt, 1);

    if (strcmp(status, "pending") != 0) {
        send(client_socket, "Room is not in pending state", strlen("Room is not in pending state"), 0);
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return;
    }

    // Cập nhật trạng thái phòng thi sang "in_progress"
    sqlite3_finalize(stmt);
    const char *sql_update = "UPDATE exam_rooms SET status = 'in_progress' WHERE room_id = ?;";
    rc = sqlite3_prepare_v2(db, sql_update, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        send(client_socket, "Failed to update room status", strlen("Failed to update room status"), 0);
        sqlite3_close(db);
        return;
    }
    sqlite3_bind_int(stmt, 1, room_id);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        send(client_socket, "Failed to update room status", strlen("Failed to update room status"), 0);
        sqlite3_close(db);
        return;
    }

    // Gửi thông báo đến các client đang chờ
    char *client_tokens = strtok((char *)waiting_clients, ",");
    while (client_tokens != NULL) {
        int client_fd = atoi(client_tokens);
        send(client_fd, "Exam started! Answer the questions below.", strlen("Exam started! Answer the questions below."), 0);
        take_exam(client_fd, room_id); // Gọi hàm take_exam
        client_tokens = strtok(NULL, ",");
    }

    send(client_socket, "Exam room started successfully", strlen("Exam room started successfully"), 0);
    sqlite3_close(db);
}
