#This Makefile expects the path to the cudd package to be set in the environment variable CUDD_DIR

CXX = g++
CXXFLAGS = -g -O3 -std=c++11 -D_FILE_OFFSET_BITS=64
CUDD_CXXFLAGS = -I$(CUDD_DIR)/include
CUDD_LDFLAGS = -static -L$(CUDD_DIR)/lib -lcudd
CXXFLAGS += $(CUDD_CXXFLAGS)

HEADERS = \
	  actionset.h \
     	  global_funcs.h \
          knowledge.h \
	  proofchecker.h \
	  ssvconstant.h \
          stateset.h \
	  statesetcompositions.h \
          task.h \
	  timer.h \
	  bdd/ssfbdd.h \
	  horn/ssfhorn.h \
	  mods/modsutil.h \
	  mods/ssfmods.h \

SOURCES = helve.cc $(HEADERS:%.h=%.cc)
OBJ = $(SOURCES:%.cc=%.o)


all: helve

$(OBJ): %.o: %.cc
	$(CXX) $(CXXFLAGS)  -c $< -o $@

helve: $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(CUDD_LDFLAGS)

clean: clean-obj
	rm -f helve

clean-obj:
	rm -f *.o *~
	rm -f */*.o *~


.PHONY : cleam clean-obj
