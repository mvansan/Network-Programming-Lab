#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <string.h>
#include "include/auth.h"

int register_user(const char *username, const char *password) {
    sqlite3 *db;
    char *err_msg = 0;
    int rc = sqlite3_open("database/exam_system.db", &db);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return -1;
    }

    char sql[256];
    snprintf(sql, sizeof(sql), "INSERT INTO users (username, password) VALUES ('%s', '%s');", username, password);

    rc = sqlite3_exec(db, sql, 0, 0, &err_msg);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return -1;
    }

    sqlite3_close(db);
    return 0;
}

int login_user(const char *username, const char *password) {
    sqlite3 *db;
    sqlite3_stmt *stmt;
    int rc = sqlite3_open("exam_system.db", &db);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return -1;
    }

    char sql[256];
    snprintf(sql, sizeof(sql), "SELECT * FROM users WHERE username='%s' AND password='%s';", username, password);

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to execute statement: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return -1;
    }

    int step = sqlite3_step(stmt);
    int result = (step == SQLITE_ROW) ? 1 : 0;

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    return result;
}
