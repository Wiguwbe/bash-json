
# this may be different on your system
INCBASH = /usr/include/bash


CFLAGS = -I$(INCBASH) -I$(INCBASH)/include -I$(INCBASH)/builtins
CFLAGS+= -g -O2 -DHAVE_CONFIG_H -DSHELL -fPIC
CFLAGS+= -Wno-discarded-qualifiers
# for debug output (there's lots of it)
#CFLAGS+= -DDEBUG

LDFLAGS = -lrt -lc -shared -Wl,-soname,bash-json

OBJS = jprint.o jload.o jhandler.o jnew.o jtype.o jget.o jset.o jdel.o jlen.o jcmp.o jkeys.o jvalues.o jhaskey.o jhasval.o
OBJS += json.o json-parser.o shmalloc.o common.o

bash-json.so: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

common.o: common.c

jprint.o: jprint.c
jload.o: jload.c
jhandler.o: jhandler.c
jnew.o: jnew.c
jtype.o: jtype.c
jget.o: jget.c
jset.o: jset.c
jdel.o: jdel.c
jlen.o: jlen.c
jcmp.o: jcmp.c
jkeys.o: jkeys.c
jvalues.o: jvalues.c
jhaskey.o: jhaskey.c
jhasval.o: jhasval.c

json.o: json.c
json-parser.o: json-parser.c
shmalloc.o: shmalloc.c

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -rf bash-json.so *.o
