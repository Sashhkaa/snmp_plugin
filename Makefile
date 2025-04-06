CC = gcc
CFLAGS = -Wall -Wextra
CFLAGS += -I./src
CFLAGS += -Iuthash/src
CFLAGS += -I/usr/include/net-snmp
LDFLAGS = -lnetsnmp -lm

MAIN_TARGET = my_program
MAIN_SRCS = src/agent.c src/main.c src/algo.c
MAIN_OBJS = $(MAIN_SRCS:.c=.o)

API_TARGET = api_program
API_SRCS = src/api.c src/agent.c src/algo.c
API_OBJS = $(API_SRCS:.c=.o)

COMMON_SRCS = src/algo.c
COMMON_OBJS = $(COMMON_SRCS:.c=.o)

.PHONY: all clean install uninstall

all: $(MAIN_TARGET) $(API_TARGET)

$(MAIN_TARGET): $(MAIN_OBJS)
	$(CC) $(CFLAGS) -o $@ $(MAIN_OBJS) $(LDFLAGS)

$(API_TARGET): $(API_OBJS)
	$(CC) $(CFLAGS) -o $@ $(API_OBJS) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(MAIN_OBJS) $(API_OBJS) $(MAIN_TARGET) $(API_TARGET)

install: all
	install -m 755 $(MAIN_TARGET) /usr/local/bin/
	install -m 755 $(API_TARGET) /usr/local/bin/
	cp service/my_agent.service /etc/systemd/system/
	systemctl daemon-reload
	systemctl enable my_agent
	systemctl start my_agent

uninstall:
	systemctl stop my_agent
	systemctl disable my_agent
	rm -f /etc/systemd/system/my_agent.service
	systemctl daemon-reload
	rm -f /usr/local/bin/$(MAIN_TARGET)
	rm -f /usr/local/bin/$(API_TARGET)