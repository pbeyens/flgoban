CXX      = g++
DEBUG    = -g
CXXFLAGS = $(shell fltk-config --cxxflags ) -Werror -Wall -I. -Isgf -Igoban -I/Users/Pieter/argp-standalone-1.3/
LDFLAGS  = $(shell fltk-config --ldstaticflags ) -L/Users/Pieter/argp-standalone-1.3/ -largp
LINK     = $(CXX)

TARGET = flgoban
OBJS = flgoban.o main.o sgf/sgf.o goban/goban.o
SRCS = flgoban.cxx main.cxx sgf/sgf.c goban/goban.c

.SUFFIXES: .o .cxx

all: $(OBJS)
	$(LINK) $(LDFLAGS) $(OBJS) -o $(TARGET)

%.o: %.cxx
	$(CXX) $(CXXFLAGS) $(DEBUG) -c $< -o $@

%.o: %.c
	$(CXX) $(CXXFLAGS) $(DEBUG) -c $< -o $@

install: ${TARGET}
	sudo cp ${TARGET} /usr/local/bin	

clean:
	rm -f ${OBJS} 2> /dev/null
	rm -f $(TARGET) 2> /dev/null
