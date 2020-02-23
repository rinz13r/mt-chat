ds_src = $(wildcard src/ds/*.c)
serv_src = $(wildcard src/core/server.c src/main.c)
client_core_src = $(wildcard src/core/client.c)
ds_obj = $(ds_src:.c=.o)
serv_obj = $(serv_src:.c=.o)
client_core_obj = $(client_core_src:.c=.o)
client_obj = src/ui/client.o
spam_client_obj = test/spam_client.o
test_client_obj = test/test_client.o
test_obj = test/tests.o

objs = $(ds_obj) $(serv_obj) $(client_core_obj) $(client_obj) $(spam_client_obj) $(test_client_obj) $(test_obj)

LDFLAGS = -lpthread
CC = cc -I include/

all: server client test

server: $(serv_obj) $(ds_obj)
	$(CC) -o bin/$@ $^ $(LDFLAGS)
client_core: $(ds_obj) $(client_core_obj)
	$(CC) $^ $(LDFLAGS)

client: $(client_obj) $(ds_obj) $(client_core_obj)
	$(CC) -o bin/$@ $^ $(LDFLAGS)
spam_client: $(client_core_obj) $(ds_obj) $(spam_client_obj)
	$(CC) -o bin/$@ $^ $(LDFLAGS)
test_client: $(client_core_obj) $(ds_obj) $(test_client_obj)
	$(CC) -o bin/$@  $^ $(LDFLAGS)
test: $(test_obj) spam_client test_client
	g++ test/tests.cpp -o bin/test

.PHONY: clean
clean:
	rm $(objs) bin/*
