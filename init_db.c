#include <sqlite3.h>
#include <stdio.h>

void init_database() {
    sqlite3 *db;
    char *err_msg = 0;
    
    int rc = sqlite3_open("exam_system.db", &db);

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

    printf("Database initialized successfully\n");

    sqlite3_close(db);
}
