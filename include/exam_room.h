#ifndef EXAM_ROOM_H
#define EXAM_ROOM_H

void create_exam_room(int client_socket, const char *name, int num_questions, int time_limit, const char *category, const char *difficulty);
void list_exam_rooms(int client_socket);
void join_exam_room(int client_socket, int room_id);

#endif
