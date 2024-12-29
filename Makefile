CC = gcc
CFLAGS = -Wall -Wextra -g -I.
# Các thư mục
SRC_DIR = src
INCLUDE_DIR = include
BUILD_DIR = build

# Các file nguồn
SRC = $(SRC_DIR)/auth.c $(SRC_DIR)/exam_room.c $(SRC_DIR)/init_db.c $(SRC_DIR)/take_exam.c import_questions.c

# Các file đối tượng (object files)
OBJ = $(SRC:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

# Tên file thực thi (binary)
SERVER_OUT = $(BUILD_DIR)/server_app
CLIENT_OUT = $(BUILD_DIR)/client

# Cài đặt rule cho Makefile
all: $(SERVER_OUT) $(CLIENT_OUT)

# Tạo file thực thi cho server từ các file đối tượng
$(SERVER_OUT): $(OBJ) $(BUILD_DIR)/server.o
	$(CC) $(CFLAGS) -o $(SERVER_OUT) $(OBJ) $(BUILD_DIR)/server.o -lsqlite3

# Tạo file thực thi cho client từ các file đối tượng
$(CLIENT_OUT): $(OBJ) $(BUILD_DIR)/client.o
	$(CC) $(CFLAGS) -o $(CLIENT_OUT) $(OBJ) $(BUILD_DIR)/client.o -lsqlite3

# Tạo các file đối tượng từ mã nguồn
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) -c -o $@ $<

$(BUILD_DIR)/server.o: $(SRC_DIR)/server.c
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) -c -o $(BUILD_DIR)/server.o $(SRC_DIR)/server.c

$(BUILD_DIR)/client.o: $(SRC_DIR)/client.c
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) -c -o $(BUILD_DIR)/client.o $(SRC_DIR)/client.c

# Rule để clean các file biên dịch
clean:
	rm -f $(BUILD_DIR)/*.o $(SERVER_OUT) $(CLIENT_OUT)

# Rule để chạy server
run_server: $(SERVER_OUT)
	./$(SERVER_OUT)

# Rule để chạy client
run_client: $(CLIENT_OUT)
	./$(CLIENT_OUT)

# Rule để import câu hỏi
import_questions:
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) -o $(BUILD_DIR)/import_questions import_questions.c

# Rule để tạo thư mục build nếu chưa có
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)
