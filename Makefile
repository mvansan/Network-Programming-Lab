# Biến
CC = gcc
CFLAGS = -Wall
PKG_CONFIG_FLAGS = `pkg-config --libs --cflags gtk+-3.0 json-glib-1.0`
OUTPUT = app
SOURCES = test.c

# Mục tiêu mặc định
all: $(OUTPUT)
	./$(OUTPUT)

# Quy tắc biên dịch và liên kết tệp để tạo app
$(OUTPUT): $(SOURCES)
	$(CC) $(SOURCES) -o $(OUTPUT) $(PKG_CONFIG_FLAGS)

# Quy tắc làm sạch
clean:
	rm -f $(OUTPUT)
