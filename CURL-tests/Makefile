# Variables
CC = gcc
CFLAGS = -g -Wall
LIBS = -lcurl

# Targets
all: proxy

proxy: proxy_parse.o temp.o
	@$(CC) $(CFLAGS) -o proxy proxy_parse.o temp.o $(LIBS)

proxy_parse.o: proxy_parse.c
	@$(CC) $(CFLAGS) -o proxy_parse.o -c proxy_parse.c
temp.o: temp.c
	@$(CC) $(CFLAGS) -o temp.o -c temp.c

clean:
	@rm -f proxy *.o

tar:
	@tar -cvzf ass1.tgz temp.c README Makefile proxy_parse.c proxy_parse.h
