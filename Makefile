
SRCS=main.cpp lexer.cpp parser.cpp
OBJS=$(SRCS:.cpp=.o)

all: $(OBJS)
	gcc $(OBJS) -o bscript -lstdc++

clean:
	rm -f $(OBJS) bscript

.cpp.o:
	cc -c -Wall -Werror $<

