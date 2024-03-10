CC = gcc
TARGET = Version_Compare
OBJS = prompt.c skin molt skin.c molt.c

$(TARGET) : $(OBJS)
	$(CC) -o $@ prompt.c

skin : skin.c
	$(CC) -o $@ $^

molt : molt.c
	$(CC) -o $@ $^

clean :
	rm -f skin
	rm -f molt
	rm -f $(TARGET)