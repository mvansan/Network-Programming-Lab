#ifndef EXAM_ROOM_H
#define EXAM_ROOM_H

void create_exam_room(int client_socket, const char *name, int num_easy_questions, int num_medium_questions, int num_hard_questions, int time_limit, const char *category, const char *privacy, int max_people);
void list_exam_rooms(int client_socket);
void join_exam_room(int client_socket, int id);

#endif
