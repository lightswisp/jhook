CC=gcc
INCLUDES=-I/usr/lib/jvm/java-23-openjdk/include/linux -I/usr/lib/jvm/java-23-openjdk/include
LIBS=-lpthread -ldl 
CFLAGS=-Wall -shared -ggdb -O0
SOURCES=main.c
OUT=lib2inject.so
OBJS=lib2inject.o
EXEC=@bash -c

all: $(OUT)

$(OUT): $(OBJS)
	$(CC) -o $@ $(INCLUDES) $(CFLAGS) $^ 

$(OBJS): $(SOURCES)
	$(CC) -fPIC -ggdb $(INCLUDES) -c $< -o $@

inject:	all 
	$(EXEC) "sudo ruby inject"
	

clean:
	rm *.o 
	rm $(OUT)

.PHONY: inject
