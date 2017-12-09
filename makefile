CC = gcc

CFLAGS = -pthread -D_REENTRANT -Wall

TARGET = server

$(TARGET): $(TARGET).c 

		$(CC) $(CFLAGS) -o $(TARGET) $(TARGET).c


clean:
		$(RM) $(TARGET)
