LIBS =
CFLAGS = -g -std=c++11 -Wall
INCLUDE =

OS := $(shell uname)
ifeq ($(OS),Darwin)
CC = clang++
LIBS += 
else
CC = g++
LIBS += 
endif

SRC = src/rand.cc src/net.cc src/timer.cc src/packetSender.cc src/packetReciever.cc src/main.cc
OBJ = $(patsubst src/%.cc, obj/%.o, $(SRC))
TARGET = net



all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS) $(INCLUDE)

obj/%.o: src/%.cc src/%.h
	$(CC) $(CFLAGS) -c $< -o $@ $(INCLUDE)

obj/%.o: src/%.cc
	$(CC) $(CFLAGS) -c $< -o $@ $(INCLUDE)

clean:
	rm -f obj/*.o $(TARGET)
