#ifndef EXAM_ROOM_H
#define EXAM_ROOM_H

#include <sqlite3.h>

void create_exam_room(int client_socket, const char *name, int num_easy_questions, int num_medium_questions, int num_hard_questions, int time_limit, const char *category, const char *privacy, int max_people, int userID, int room_limit);
void list_exam_rooms(int client_socket, int userID);
void join_exam_room(int client_socket, int id);
void insert_questions(const char *category, const char *difficulty, int num_questions, int room_id, sqlite3 *db, sqlite3_stmt *question_stmt, int client_socket);
void start_exam_room(int client_socket, int room_id);

#endif
