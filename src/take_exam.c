#include "include/exam_room.h"
#include <stdio.h>
#include <sqlite3.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <time.h>
#include <auth.h>
#include <auth.h>

#define DATABASE_PATH "database/exam_system.db"

void take_exam(int client_socket, int room_id, int userID) {
    sqlite3 *db;
    int rc = sqlite3_open(DATABASE_PATH, &db);
    if (rc != SQLITE_OK) {
        const char *err_msg = sqlite3_errmsg(db);
        send(client_socket, err_msg, strlen(err_msg), 0);
        return;
    }

    // Thiết lập chế độ WAL
    sqlite3_exec(db, "PRAGMA journal_mode=WAL;", 0, 0, 0);

    // Lấy thời gian giới hạn từ cơ sở dữ liệu
    const char *sql_time_limit = "SELECT time_limit FROM exam_rooms WHERE room_id = ?";
    sqlite3_stmt *time_stmt;
    rc = sqlite3_prepare_v2(db, sql_time_limit, -1, &time_stmt, 0);
    if (rc != SQLITE_OK) {
        const char *err_msg = sqlite3_errmsg(db);
        send(client_socket, err_msg, strlen(err_msg), 0);
        sqlite3_close(db);
        return;
    }

    sqlite3_bind_int(time_stmt, 1, room_id);
    rc = sqlite3_step(time_stmt);
    if (rc != SQLITE_ROW) {
        send(client_socket, "Room not found", strlen("Room not found"), 0);
        sqlite3_finalize(time_stmt);
        sqlite3_close(db);
        return;
    }

    int time_limit = sqlite3_column_int(time_stmt, 0);
    sqlite3_finalize(time_stmt);

    const char *sql_questions = "SELECT q.id, q.question_text, q.option_1, q.option_2, q.option_3, q.option_4, q.correct_answer, q.difficulty "
                                "FROM questions q "
                                "JOIN exam_questions eq ON q.id = eq.question_id "
                                "WHERE eq.exam_room_id = ?";
    sqlite3_stmt *question_stmt;

    rc = sqlite3_prepare_v2(db, sql_questions, -1, &question_stmt, 0);
    if (rc != SQLITE_OK) {
        const char *err_msg = sqlite3_errmsg(db);
        send(client_socket, err_msg, strlen(err_msg), 0);
        sqlite3_close(db);
        return;
    }

    sqlite3_bind_int(question_stmt, 1, room_id);
    rc = sqlite3_step(question_stmt);

    int score = 0;
    int question_count = 1;
    time_t start_time = time(NULL);

    time_limit *= 60;  // Chuyển đổi thời gian từ phút sang giây

    while (rc == SQLITE_ROW) {
        // Kiểm tra thời gian còn lại
        time_t current_time = time(NULL);
        double elapsed_time = difftime(current_time, start_time);
        if (elapsed_time >= time_limit) {
            send(client_socket, "Time's up! Exam finished.\n", strlen("Time's up! Exam finished.\n"), 0);
            break;
        }

        // Calculate remaining time
        int remaining_time = (int)(time_limit - elapsed_time);
        int minutes = remaining_time / 60;
        int seconds = remaining_time % 60;

        // Send remaining time to client
        char time_str[256];
        snprintf(time_str, sizeof(time_str), "Time remaining: %02d:%02d\n", minutes, seconds);
        send(client_socket, time_str, strlen(time_str), 0);

        const char *question_text = (const char *)sqlite3_column_text(question_stmt, 1);
        const char *option_1 = (const char *)sqlite3_column_text(question_stmt, 2);
        const char *option_2 = (const char *)sqlite3_column_text(question_stmt, 3);
        const char *option_3 = (const char *)sqlite3_column_text(question_stmt, 4);
        const char *option_4 = (const char *)sqlite3_column_text(question_stmt, 5);
        const char *correct_answer = (const char *)sqlite3_column_text(question_stmt, 6);
        const char *difficulty = (const char *)sqlite3_column_text(question_stmt, 7);

        char question_str[1024];
        snprintf(question_str, sizeof(question_str), "%d. %s\n1. %s\n2. %s\n3. %s\n4. %s\n",
                 question_count++, question_text, option_1, option_2, option_3, option_4);
        send(client_socket, question_str, strlen(question_str), 0);

        // Prompt for answer after sending the question
        send(client_socket, "Enter your answer (1-4): ", strlen("Enter your answer (1-4): "), 0);

        char answer[2];
        int bytes_received = recv(client_socket, answer, sizeof(answer), 0);
        if (bytes_received <= 0) {
            send(client_socket, "Error receiving answer", strlen("Error receiving answer"), 0);
            sqlite3_finalize(question_stmt);
            sqlite3_close(db);
            return;
        }

        char user_answer[10];
        snprintf(user_answer, sizeof(user_answer), "option_%c", answer[0]);

        // Bắt đầu giao dịch cho việc cập nhật điểm
        sqlite3_exec(db, "BEGIN TRANSACTION;", 0, 0, 0);

        if (strcmp(user_answer, correct_answer) == 0) {
            if (strcmp(difficulty, "easy") == 0) {
                score += 1;
            } else if (strcmp(difficulty, "medium") == 0) {
                score += 2;
            } else if (strcmp(difficulty, "hard") == 0) {
                score += 3;
            }
        } else if (answer[0] < '1' || answer[0] > '4') {
            send(client_socket, "Invalid answer\n", strlen("Invalid answer"), 0);
            sqlite3_exec(db, "ROLLBACK;", 0, 0, 0);  // Rollback transaction in case of invalid answer
            continue;
        }

        // Commit transaction sau khi cập nhật điểm
        sqlite3_exec(db, "COMMIT;", 0, 0, 0);

        rc = sqlite3_step(question_stmt);
    }

    char result[256];
    snprintf(result, sizeof(result), "Exam finished. Your score is: %d\n", score);
    send(client_socket, result, strlen(result), 0);

    const char *sql_insert_result = "INSERT OR REPLACE INTO exam_results (room_id, userID, score) VALUES (?, ?, ?);";
    sqlite3_stmt *insert_result_stmt;
    rc = sqlite3_prepare_v2(db, sql_insert_result, -1, &insert_result_stmt, 0);
    if (rc != SQLITE_OK) {
        const char *err_msg = sqlite3_errmsg(db);
        send(client_socket, err_msg, strlen(err_msg), 0);
        sqlite3_close(db);
        return;
    }

    sqlite3_bind_int(insert_result_stmt, 1, room_id);
    sqlite3_bind_int(insert_result_stmt, 2, userID);
    sqlite3_bind_int(insert_result_stmt, 3, score);
    sqlite3_step(insert_result_stmt);
    sqlite3_finalize(insert_result_stmt);

    sqlite3_finalize(question_stmt);
    sqlite3_close(db);
}
