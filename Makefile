#This Makefile expects the path to the cudd package to be set in the environment variable CUDD_DIR

CXX = g++
CXXFLAGS = -g -O3 -std=c++14 -D_FILE_OFFSET_BITS=64
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
	  bdd/bddfile.h \
	  bdd/bddutil.h \
	  bdd/ssfbdd.h \
	  horn/cnfformula.h \
	  horn/hornstateset.h \
	  horn/horntypedisjunction.h \
	  horn/horntypeformula.h \
	  horn/taskinformation.h \
	  mods/modsutil.h \
	  mods/ssfmods.h \
	  rules/rules.h \
	  rules/basic_statements/basic_statement_functions.h \

SOURCES = $(HEADERS:%.h=%.cc) \
	  helve.cc \
	  rules/basic_statements/basic_statement_1.cc \
	  rules/basic_statements/basic_statement_2.cc \
	  rules/basic_statements/basic_statement_3.cc \
	  rules/basic_statements/basic_statement_4.cc \
	  rules/basic_statements/basic_statement_5.cc \
	  rules/deadness_rules/emptyset_dead.cc \
	  rules/deadness_rules/union_dead.cc \
	  rules/deadness_rules/subset_dead.cc \
	  rules/deadness_rules/progression_goal.cc \
	  rules/deadness_rules/progression_initial.cc \
	  rules/deadness_rules/regression_goal.cc \
	  rules/deadness_rules/regression_initial.cc \
	  rules/prog_reg_rules/action_transitivity.cc \
	  rules/prog_reg_rules/action_union.cc \
	  rules/prog_reg_rules/progression_transitivity.cc \
	  rules/prog_reg_rules/progression_union.cc \
	  rules/prog_reg_rules/progression_to_regression.cc \
	  rules/prog_reg_rules/regression_to_progression.cc \
	  rules/set_theory_rules/union_right.cc \
	  rules/set_theory_rules/union_left.cc \
	  rules/set_theory_rules/intersection_right.cc \
	  rules/set_theory_rules/intersection_left.cc \
	  rules/set_theory_rules/distributivity.cc \
	  rules/set_theory_rules/subset_union.cc \
	  rules/set_theory_rules/subset_intersection.cc \
	  rules/set_theory_rules/subset_transitivity.cc \
	  rules/unsolvable_rules/conclusion_initial.cc \
	  rules/unsolvable_rules/conclusion_goal.cc \

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
	rm -f */*/*.o *~

.PHONY : cleam clean-obj
