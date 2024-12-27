#include "include/exam_room.h"
#include <stdio.h>
#include <sqlite3.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>

#define DATABASE_PATH "database/exam_system.db"

void take_exam(int client_socket, int room_id) {
    sqlite3 *db;
    int rc = sqlite3_open(DATABASE_PATH, &db);
    if (rc != SQLITE_OK) {
        const char *err_msg = sqlite3_errmsg(db);
        send(client_socket, err_msg, strlen(err_msg), 0);
        return;
    }

    // Start a transaction
    sqlite3_exec(db, "BEGIN TRANSACTION;", 0, 0, 0);

    const char *sql_questions = "SELECT q.id, q.question_text, q.option_1, q.option_2, q.option_3, q.option_4, q.correct_answer "
                                "FROM questions q "
                                "JOIN exam_questions eq ON q.id = eq.question_id "
                                "WHERE eq.exam_room_id = ?";
    sqlite3_stmt *question_stmt;

    rc = sqlite3_prepare_v2(db, sql_questions, -1, &question_stmt, 0);
    if (rc != SQLITE_OK) {
        const char *err_msg = sqlite3_errmsg(db);
        send(client_socket, err_msg, strlen(err_msg), 0);
        sqlite3_exec(db, "ROLLBACK;", 0, 0, 0);  // Rollback transaction in case of error
        sqlite3_close(db);
        return;
    }

    sqlite3_bind_int(question_stmt, 1, room_id);
    rc = sqlite3_step(question_stmt);

    int score = 0;
    int question_count = 1;

    while (rc == SQLITE_ROW) {
        const char *question_text = (const char *)sqlite3_column_text(question_stmt, 1);
        const char *option_1 = (const char *)sqlite3_column_text(question_stmt, 2);
        const char *option_2 = (const char *)sqlite3_column_text(question_stmt, 3);
        const char *option_3 = (const char *)sqlite3_column_text(question_stmt, 4);
        const char *option_4 = (const char *)sqlite3_column_text(question_stmt, 5);
        const char *correct_answer = (const char *)sqlite3_column_text(question_stmt, 6);

        char question_str[1024];
        snprintf(question_str, sizeof(question_str), "%d. %s\n1. %s\n2. %s\n3. %s\n4. %s\n\n",
                 question_count++, question_text, option_1, option_2, option_3, option_4);
        send(client_socket, question_str, strlen(question_str), 0);

        char answer[2];
        int bytes_received = recv(client_socket, answer, sizeof(answer), 0);
        if (bytes_received <= 0) {
            send(client_socket, "Error receiving answer", strlen("Error receiving answer"), 0);
            sqlite3_exec(db, "ROLLBACK;", 0, 0, 0);  // Rollback transaction in case of error
            sqlite3_finalize(question_stmt);
            sqlite3_close(db);
            return;
        }

        char user_answer[10];
        snprintf(user_answer, sizeof(user_answer), "option_%c", answer[0]);

        if (strcmp(user_answer, correct_answer) == 0) {
            score++;
        } else if (answer[0] < '1' || answer[0] > '4') {
            send(client_socket, "Invalid answer", strlen("Invalid answer"), 0);
            sqlite3_exec(db, "ROLLBACK;", 0, 0, 0);  // Rollback transaction in case of invalid answer
            sqlite3_finalize(question_stmt);
            sqlite3_close(db);
            return;
        }

        rc = sqlite3_step(question_stmt);
    }

    char result[256];
    snprintf(result, sizeof(result), "Exam finished. Your score is: %d\n", score);
    send(client_socket, result, strlen(result), 0);

    sqlite3_finalize(question_stmt);
    sqlite3_exec(db, "COMMIT;", 0, 0, 0);  // Commit transaction after all operations
    sqlite3_close(db);
}
