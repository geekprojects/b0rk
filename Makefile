
SRCS=main.cpp lexer.cpp parser.cpp runtime.cpp context.cpp class.cpp function.cpp object.cpp string.cpp system.cpp value.cpp
OBJS=$(SRCS:.cpp=.o)

all: $(OBJS)
	gcc $(OBJS) -o bscript -lstdc++

clean:
	rm -f $(OBJS) bscript

.cpp.o:
	cc -c -Wall -Werror $<

