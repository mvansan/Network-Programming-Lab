#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>

#define MAX_LINE 1024

// Hàm đọc file CSV và nhập dữ liệu vào cơ sở dữ liệu
void import_questions(const char *csv_file, const char *db_file) {
    sqlite3 *db;
    sqlite3_stmt *stmt;
    FILE *file = fopen(csv_file, "r");
    char line[MAX_LINE];

    if (!file) {
        fprintf(stderr, "Không thể mở file CSV: %s\n", csv_file);
        return;
    }

    if (sqlite3_open(db_file, &db) != SQLITE_OK) {
        fprintf(stderr, "Không thể kết nối đến database: %s\n", sqlite3_errmsg(db));
        fclose(file);
        return;
    }

    // Câu lệnh SQL để chèn dữ liệu
    const char *sql = "INSERT INTO questions (question_text, category, difficulty, correct_answer, option_1, option_2, option_3, option_4) "
                      "VALUES (?, ?, ?, ?, ?, ?, ?, ?)";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK) {
        fprintf(stderr, "Không thể chuẩn bị statement: %s\n", sqlite3_errmsg(db));
        fclose(file);
        sqlite3_close(db);
        return;
    }

    // Bỏ qua dòng đầu tiên (header)
    fgets(line, MAX_LINE, file);

    // Đọc từng dòng của file CSV
    while (fgets(line, MAX_LINE, file)) {
        char *question_text, *category, *difficulty, *correct_answer;
        char *option_1, *option_2, *option_3, *option_4;

        question_text = strtok(line, ",");
        category = strtok(NULL, ",");
        difficulty = strtok(NULL, ",");
        correct_answer = strtok(NULL, ",");
        option_1 = strtok(NULL, ",");
        option_2 = strtok(NULL, ",");
        option_3 = strtok(NULL, ",");
        option_4 = strtok(NULL, ",");

        // Gán giá trị vào statement
        sqlite3_bind_text(stmt, 1, question_text, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, category, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, difficulty, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 4, correct_answer, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 5, option_1, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 6, option_2, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 7, option_3, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 8, option_4, -1, SQLITE_STATIC);

        if (sqlite3_step(stmt) != SQLITE_DONE) {
            fprintf(stderr, "Không thể chèn dữ liệu: %s\n", sqlite3_errmsg(db));
        }

        // Reset statement để tái sử dụng
        sqlite3_reset(stmt);
    }

    printf("Import dữ liệu thành công!\n");

    fclose(file);
    sqlite3_finalize(stmt);
    sqlite3_close(db);
}

int main() {
    const char *csv_file = "questions.csv";
    const char *db_file = "exam_system.db";

    import_questions(csv_file, db_file);
    return 0;
}
