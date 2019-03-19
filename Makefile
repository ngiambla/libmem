# Makefile for QVMI;
# =-----------------=

EXE = ut
CXX = gcc

_OBJS = unittests.o libgnumem.o libbitmem.o libbudmem2.o memutils.o
OBJS = $(patsubst %,%,$(_OBJS))


all: $(EXE)

%.o: %.c 
	$(CXX) -c $(INC) -o $@ $< $(CFLAGS) 

$(EXE): $(OBJS)
	$(CXX) -o $(EXE) $^ memutils.h $(CFLAGS)



.PHONY: clean

clean:
	rm -f $(EXE)
	rm *.o