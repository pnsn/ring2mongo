LINUX_FLAGS=-lm -lpthread
SOLARIS_FLAGS=-lm -lposix4
SPECIFIC_FLAGS = $($(PLATFORM)_FLAGS)

CFLAGS = -g  ${GLOBALFLAGS}



B = $(EW_HOME)/$(EW_VERSION)/bin
L = $(EW_HOME)/$(EW_VERSION)/lib
LIB_MS = $L/libmseed.a


O = ring2mongo.o $L/libew.a $L/swap.o
MONGO=$(shell pkg-config --cflags --libs libmongoc-1.0)

ring2mongo2: $O
	$(CC) $(CFLAGS) -o $B/ring2mongo $O $(SPECIFIC_FLAGS) $(LIB_MS) $(MONGO)

lint:
	lint ring2mongo.c $(GLOBALFLAGS)



# Clean-up rules
clean:
	rm -f a.out core *.o *.obj *% *~

clean_bin:
	rm -f $B/ring2mongo*
