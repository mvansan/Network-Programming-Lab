# Biến
CC = gcc
CFLAGS = -Wall
PKG_CONFIG_FLAGS = `pkg-config --libs --cflags gtk+-3.0`
LDFLAGS = -luuid
OUTPUT = app
PROG = app
SOURCES = $(PROG).c app_function.c utlis.c

# Mục tiêu mặc định
all: $(OUTPUT)
	./$(OUTPUT)

# Quy tắc biên dịch và liên kết tệp để tạo myapp trong thư mục client
$(OUTPUT): $(SOURCES)
	$(CC) $(SOURCES) -o $(OUTPUT) $(PKG_CONFIG_FLAGS)

# Quy tắc làm sạch
clean:
	rm -f $(OUTPUT)