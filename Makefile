#This Makefile expects the path to the cudd package to be set in the environment variable CUDD_DIR

CXX = g++
CXXFLAGS = -g -O3 -std=c++11 -D_FILE_OFFSET_BITS=64
CUDD_CXXFLAGS = -I$(CUDD_DIR)/include
CUDD_LDFLAGS = -static -L$(CUDD_DIR)/lib -lcudd

CXXFLAGS += $(CUDD_CXXFLAGS)

DEPEND = $(CXX) -MM

HEADERS = \
	  actionset.h \
     	  global_funcs.h \
          knowledge.h \
	  proofchecker.h \
	  ssfbdd.h \
	  ssfexplicit.h \
	  ssfhorn.h \
	  ssvconstant.h \
          stateset.h \
	  statesetcompositions.h \
          task.h \
	  timer.h \

SOURCES = helve.cc $(HEADERS:%.h=%.cc)
OBJ = $(SOURCES:%.cc=%.o)

all: helve

$(OBJ): %.o: %.cc
	$(CXX) $(CXXFLAGS)  -c $< -o $@

helve: $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(CUDD_LDFLAGS)

clean-obj:
	rm -f *.o *~

clean:
	rm -f *.o *~ 
	rm -f helve
	rm -f test
	rm -f Makefile.depend

Makefile.depend: $(SOURCES) $(HEADERS)
	rm -f Makefile.depend
	for source in $(SOURCES) ; do \
	    $(DEPEND) $(CXXFLAGS) $$source > Makefile.temp0; \
	    objfile=$${source%%.cc}.o; \
	    sed -i -e "s@^[^:]*:@$$objfile:@" Makefile.temp0; \
	    cat Makefile.temp0 >> Makefile.depend; \
	done
	rm -f Makefile.temp0 #Makefile.depend
#	sed -e "s@\(.*\)\.o:\(.*\)@.obj/\1$(OBJECT_SUFFIX_RELEASE).o:\2@" Makefile.temp >> Makefile.depend
#	sed -e "s@\(.*\)\.o:\(.*\)@.obj/\1$(OBJECT_SUFFIX_DEBUG).o:\2@" Makefile.temp >> Makefile.depend
#	sed -e "s@\(.*\)\.o:\(.*\)@.obj/\1$(OBJECT_SUFFIX_PROFILE).o:\2@" Makefile.temp >> Makefile.depend
#	rm -f Makefile.temp

ifneq ($(MAKECMDGOALS),clean)
    ifneq ($(MAKECMDGOALS),distclean)
        include Makefile.depend
    endif
endif
