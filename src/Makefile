CC = cc
CFLAGS = -O3 $(DBG) -I/usr/local/include -Wall -D_REENTRANT -D_BSD_SOURCE
OBJECTS = base64.o daemon.o json.o jsonq.o network.o scriptlet.o skiplist.o store.o thread.o tree.o uuid.o

all: $(OBJECTS) yxtrang.a

yxtrang.a: $(OBJECTS)
	ar -r yxtrang.a $(OBJECTS)
	ranlib yxtrang.a

clean:
	rm -f *.o *.a

# DO NOT DELETE

base64.o: base64.h
daemon.o: daemon.h
json.o: json.h
jsonq.o: jsonq.h
network.o: network.h skiplist.h thread.h
scriptlet.o: scriptlet.h skiplist.h
skiplist.o: skiplist.h
store.o: store.h uuid.h tree.h
thread.o: thread.h
tree.o: tree.h uuid.h
uuid.o: uuid.h
