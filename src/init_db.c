#include <sqlite3.h>
#include <stdio.h>
#include <import_questions.h>
#include <unistd.h>
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

    rc = sqlite3_exec(db, "PRAGMA journal_mode=WAL;", 0, 0, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to set journal mode: %s\n", err_msg);
        sqlite3_free(err_msg);
    }

    const char *users = "CREATE TABLE IF NOT EXISTS users ("
                              "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                              "username TEXT NOT NULL UNIQUE, "
                              "password TEXT NOT NULL);";

    const char *exam_rooms = "CREATE TABLE IF NOT EXISTS exam_rooms ("
                             "room_id INTEGER PRIMARY KEY AUTOINCREMENT, "
                             "name TEXT NOT NULL, "
                             "num_easy_questions INTEGER NOT NULL, "
                             "num_medium_questions INTEGER NOT NULL, "
                             "num_hard_questions INTEGER NOT NULL, "
                             "time_limit INTEGER NOT NULL, "
                             "category TEXT CHECK(category IN ('IT','Blockchain','Science')) NOT NULL, "
                             "privacy TEXT CHECK(privacy IN ('public','private')) NOT NULL, "
                             "num_clients INTEGER NOT NULL DEFAULT 0, "
                             "max_people INTEGER, "
                             "status TEXT CHECK(status IN ('pending', 'in_progress', 'finished')) NOT NULL DEFAULT 'pending', "
                             "waiting_clients TEXT DEFAULT '', "
                             "userID INTEGER NOT NULL, "
                             "room_limit INTEGER DEFAULT NULL, "
                             "FOREIGN KEY (userID) REFERENCES users(id) ON DELETE CASCADE"
                             ");";

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

    const char *exam_questions = "CREATE TABLE IF NOT EXISTS exam_questions ("
                                       "exam_room_id INTEGER, "
                                       "question_id INTEGER, "
                                       "PRIMARY KEY (exam_room_id, question_id), "
                                       "FOREIGN KEY (exam_room_id) REFERENCES exam_rooms(room_id) ON DELETE CASCADE, "
                                       "FOREIGN KEY (question_id) REFERENCES questions(id) ON DELETE CASCADE);";

    const char *exam_results = "CREATE TABLE IF NOT EXISTS exam_results ("
                                   "room_id INTEGER, "
                                   "userID INTEGER, "
                                   "score INTEGER, "
                                   "PRIMARY KEY (room_id, userID), "
                                   "FOREIGN KEY (room_id) REFERENCES exam_rooms(room_id) ON DELETE CASCADE, "
                                   "FOREIGN KEY (userID) REFERENCES users(id) ON DELETE CASCADE); ";

    rc = sqlite3_exec(db, users, 0, 0, &err_msg);
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

    rc = sqlite3_exec(db, exam_questions, 0, 0, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return;
    }

    rc = sqlite3_exec(db, exam_results, 0, 0, &err_msg);
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
