CC=gcc
INCLUDES=-I/usr/lib/jvm/default-runtime/include/linux -I/usr/lib/jvm/default-runtime/include
LIBS=-lpthread -ldl 
CFLAGS=-Wall -shared -ggdb -O0
SOURCES=main.c hooks.c dump.c helpers.c logger.c sigscan.c mappings.c
OUT=lib2inject.so
OBJS=$(SOURCES:.c=.o)
EXEC=@bash -c

all: $(OUT)

$(OUT): $(OBJS)
	$(CC) -o $@ $(INCLUDES) $(CFLAGS) $^ 
	rm *.o

%.o: %.c 
	$(CC) -fPIC -ggdb $(INCLUDES) -c $< -o $@

inject:	all 
	$(EXEC) "sudo ruby inject"
	

clean:
	rm *.o 
	rm $(OUT)

.PHONY: inject
