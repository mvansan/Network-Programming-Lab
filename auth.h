#ifndef AUTH_H
#define AUTH_H

int register_user(const char *username, const char *password);
int login_user(const char *username, const char *password);

#endif
