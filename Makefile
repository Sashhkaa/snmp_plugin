CC = gcc  # или g++ если вам действительно нужен C++
CFLAGS = -Wall -Wextra -I$(UTHASH_PATH)
CFLAGS += -I./src
CFLAGS += -Iuthash/src
CFLAGS += -I/usr/include/net-snmp
LDFLAGS = -lnetsnmp 

TARGET = my_program
SRCDIR = ./src
SRC = $(SRCDIR)/agent.c \
      $(SRCDIR)/main.c

CLI_SRCS = src/api.c
CLI_OBJS = $(CLI_SRCS:.c=.o)
OBJS = $(SRC:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(CLI_OBJS) $(TARGET)

install:
	install -m 755 $(TARGET) /usr/local/bin/
	cp service/my_agent.service /etc/systemd/system/
	systemctl daemon-reload
	systemctl enable my_agent
	systemctl start my_agent

uninstall:
	systemctl stop my_agent
	systemctl disable my_agent
	rm -f /etc/systemd/system/my_agent.service
	systemctl daemon-reload

.PHONY: all clean install uninstall