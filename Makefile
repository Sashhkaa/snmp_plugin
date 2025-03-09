CC = cc

CFLAGS = -I/usr/local/include -g

SRC = agent.c

OBJ = agent.o

TARGET = agent

LIBS = -lsnmp

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) -o $(TARGET) $(OBJ) $(LIBS)

$(OBJ): $(SRC)
	$(CC) $(CFLAGS) -c $(SRC) -o $(OBJ)

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean