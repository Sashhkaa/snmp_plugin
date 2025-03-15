# make rpm package for agent.
CXX = g++

CXXFLAGS = -Wall -Wextra -std=c++17

TARGET = my_program

SRCDIR = ./src

SRC = $(SRCDIR)/agent.c \
      $(SRCDIR)/main.c

CLI_SRCS = src/api.c

CLI_OBJS = $(CLI_SRCS:.cpp=.o)

OBJS = $(SRC:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(CLI_TARGET): $(CLI_OBJS)
	$(CC) $(CFLAGS) -o $(CLI_TARGET) $(CLI_OBJS)

clean:
	rm -f $(OBJS) $(TARGET)

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

.PHONY: all clean
