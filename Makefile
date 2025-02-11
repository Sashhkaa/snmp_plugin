CC = gcc

CFLAGS = -g -I/usr/local/include

LDFLAGS = -lsnmp

SRCDIR = .
SOURCES = $(SRCDIR)/agent.c \
 		  $(SRCDIR)/alogitm.c

OBJECTS = $(SOURCES:.c=.o)

TARGET = snmp_plugin

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

rebuild: clean all
