
SRCS=main.cpp lexer.cpp parser.cpp runtime.cpp context.cpp class.cpp function.cpp scriptfunction.cpp object.cpp string.cpp system.cpp value.cpp assembler.cpp expression.cpp executor.cpp
OBJS=$(SRCS:.cpp=.o)

all: $(OBJS)
	gcc $(OBJS) -o bscript -lstdc++

clean:
	rm -f $(OBJS) bscript

.cpp.o:
	cc -c -ggdb -Wall -Werror $<

