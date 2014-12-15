CC = cc
CFLAGS = -O2 $(DBG) -I/usr/local/include -I../src -D_REENTRANT -D_BSD_SOURCE

LDFLAGS = -L/usr/local/lib -lm -lpthread -lssl -lcrypto

all: test echo1 echo2 echod httpd lindad

test: test.o ../src/yxtrang.a
	$(CC) -o test test.o ../src/yxtrang.a $(LDFLAGS)

echo1: echo1.o ../src/yxtrang.a
	$(CC) -o echo1 echo1.o ../src/yxtrang.a $(LDFLAGS)

echo2: echo2.o ../src/yxtrang.a
	$(CC) -o echo2 echo2.o ../src/yxtrang.a $(LDFLAGS)

echod: echod.o ../src/yxtrang.a
	$(CC) -o echod echod.o ../src/yxtrang.a $(LDFLAGS)

httpd: httpd.o ../src/yxtrang.a
	$(CC) -o httpd httpd.o ../src/yxtrang.a $(LDFLAGS)

lindad: lindad.o ../src/yxtrang.a
	$(CC) -o httpd httpd.o ../src/yxtrang.a $(LDFLAGS)

clean:
	rm -f *.o test echo1 echo2 echod httpd lindad

# DO NOT DELETE

test.o: ../src/json.h ../src/base64.h ../src/skiplist.h ../src/tree.h
test.o: ../src/uuid.h ../src/store.h ../src/network.h ../src/scriptlet.h
test.o: ../src/linda.h ../src/jsonq.h ../src/thread.h
echo1.o: ../src/network.h
echo2.o: ../src/network.h
echod.o: ../src/network.h
httpd.o: ../src/network.h ../src/httpserver.h
lindad.o: ../src/network.h ../src/httpserver.h ../src/lindatp.h
