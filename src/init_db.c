#include <sqlite3.h>
#include <stdio.h>
#include "import_questions.h"  

void init_database() {
    sqlite3 *db;
    char *err_msg = 0;
    
    int rc = sqlite3_open("database/exam_system.db", &db);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return;
    }

    const char *sql = "CREATE TABLE IF NOT EXISTS users ("
                      "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                      "username TEXT NOT NULL UNIQUE, "
                      "password TEXT NOT NULL);";

    const char *exam_rooms = "CREATE TABLE IF NOT EXISTS exam_rooms ("
                            "room_id INTEGER PRIMARY KEY AUTOINCREMENT, "
                            "name TEXT NOT NULL, "
                            "num_questions INTEGER NOT NULL, "
                            "time_limit INTEGER NOT NULL, "
                            "status TEXT NOT NULL DEFAULT 'not_started');";

    const char *questions = "CREATE TABLE IF NOT EXISTS questions ("
                            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                            "question_text TEXT NOT NULL, "
                            "category TEXT NOT NULL, "
                            "difficulty TEXT CHECK(difficulty IN ('easy','medium','hard')) NOT NULL, "
                            "correct_answer TEXT CHECK(correct_answer IN ('option_1','option_2','option_3','option_4')) NOT NULL, "
                            "option_1 TEXT NOT NULL, "
                            "option_2 TEXT NOT NULL, "
                            "option_3 TEXT NOT NULL, "
                            "option_4 TEXT NOT NULL);";

    rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return;
    }

    rc = sqlite3_exec(db, exam_rooms, 0, 0, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return;
    }

    rc = sqlite3_exec(db, questions, 0, 0, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return;
    }

    import_questions("database/questions.csv", "database/exam_system.db");

    printf("Database initialized successfully\n");

    sqlite3_close(db);
}
