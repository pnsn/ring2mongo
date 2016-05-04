# HTML = webswave.html

#WEBPAGE = -D_HTMLVAR=$(subst .,_,$(HTML))_gz -D_HTMLVARLEN=$(subst .,_,$(HTML))_gz_len

# CFLAGS = -DUSE_WEBSOCKET -D_REENTRANT $(WEBPAGE) -W -Wall -I.. -pthread -g $(GLOBALFLAGS)


LIBS = $(MT_FLAG) -lpthread -lm -ldl



B = $(EW_HOME)/$(EW_VERSION)/bin
L = $(EW_HOME)/$(EW_VERSION)/lib


EWBIN = $L/site.o $L/fleng.o $L/ws_clientII.o \
	$L/socket_ew_common.o $L/socket_ew.o $L/swap.o $L/rw_mag.o \
	$L/rdpickcoda.o $L/libew_mt.a
	

all:
	make -f makefile clean
	make -f makefile ring2mongo

MONGO=$(shell pkg-config --cflags --libs libmongoc-1.0)

ring2mongo: ring2mongo.o $(EWBIN)
	$(CC) $(CFLAGS) -o $B/ring2mongo ring2mongo.o \
	$(EWBIN) $(LIBS) $(MONGO)

gdb: ring2mongo.o $(EWBIN)
	$(CC) $(CFLAGS) -o $B/ring2mongo ring2mongo.o \
	$(EWBIN) $(LIBS) $(MONGO) -g3 
	
# webswave: webswave.o mongoose.o $(EWBIN)
# 	$(CC) $(CFLAGS) -o $B/webswave webswave.o mongoose.o \
# 	$(EWBIN) $(LIBS)

lint:
	lint ring2mongo.c $(GLOBALFLAGS)

# Clean-up rules
clean:
	rm -f a.out core *.o *.obj *% *~

clean_bin:
	rm -f $B/ring2mongo*
