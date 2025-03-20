
Installing and running helve with CUDD
======================================
(In what follows <path-to-cudd> is the path where you want CUDD to be
installed to)
1. Download CUDD 3.0.0  as a zip from here: https://github.com/ivmai/cudd/archive/refs/heads/release.zip
 (unofficial
mirror https://github.com/ivmai/cudd)
2. Unpack the archive.
3. In the folder cudd-release call the following steps to get the 64-bit
library with dddmp and c++-wrapper:

        ./configure --prefix=\<path-to-cudd\> --enable-shared --enable-dddmp --enable-obj --enable-static "CFLAGS=-D_FILE_OFFSET_BITS=64" "CXXFLAGS=-D_FILE_OFFSET_BITS=64"
        && make
        && make install

4. Move the following two header files config.h and util/util.h to \<path-to-cudd\>/include:

        cp config.h \<path-to-cudd\>/include
        && cp util/util.h \<path-to-cudd\>/include

  (I don't know why this is necessary, but else the dddmp library complains...)

5. Set the environment variable CUDD_DIR to \<path-to-cudd\> (or change the
Makefile, adding the path in place of the variable).

6. Navigate to \<path-to-helve>\.
7. Compile the verifier with make.
8. For verifying a certificate, you need
   - a task file (see below)
   - a certificate file (see below)
   - optionally files for describing BDDs

   The verifier is called with

        verify <task file> <certificate file>"
   A successful verification ends with "Exiting: certificate is valid".
9. Test your setup by navigating into helve/test and run:

        ./../helve fail-task.txt fail-certificate.txt
   the expected output is:

        Critical error when checking line 9 (k 110 d 0 ed 1): Premise list is not empty.
        Exiting: unexplained critical error

   next run

        ./../helve success-task.txt success-certificate.txt
   the expected output is:

        Verify total time: 0.01
        Verify memory: 32624KB
        unsolvability proven
        Exiting: certificate is valid

File Formats
============

task file
---------

The task description (currently limited to STRIPS) has the following format:

    begin_atoms:<#atoms>
    <atom 1>
    <atom 2>
    ... (list all atoms (by name), one each row)
    end_atoms
    begin_init
    <initital state atom index 1>
    <initital state atom index 2>
    ... (lists atoms (by index) that are true in initial state, one each row)
    end_init
    begin_goal
    <goal atom index 1>
    <goal atom index 2>
    ... (lists atoms (by index) that are true in goal, one each row)
    end_goal
    begin_actions:<#actions>
    begin_action
    <action_name>
    cost: <action_cost>
    PRE:<precondition atom index 1>
    ... (more PRE)
    ADD:<added atom index 1>
    ... (more ADD)
    DEL:<deleted atom index 1>
    ... (more DEL)
    end_action
    ... (lists all actions)
    end_actions



proof
-----

In the proof, each line describes either a set expression, an action expression
or a new piece of knowledge. In what follows, # denotes an ID of a set
expression, action expression, or knowledge.


set expressions:

Set expressions can either be constant sets, "basic" sets (which are defined
in a concrete language), or "compound" sets (negation, intersection, union,
progression, regression). Each set expression has a unique identifier.
The following tree represents the delcaration syntax for set expressions:

                 /- e                                         (constant empty set)
            /- c -- i                                         (constant initstate set)
           /     \- g                                         (constant goal set)
          /--- b <bdd_filename> <bdd_index> ;                 (BDD set)
         /---- t <description in DIMACS> ;                    (2CNF set)
        /----- e <list of STRIPS states encoded in hex> ;     (explicit set)
    e # ------ n #                                            (negation)
        \----- i # #                                          (intersection)
         \---- u # #                                          (union)
          \--- p # #                                          (progression)
           \-- r # #                                          (regression)

(the different parts of the description are separated by space characters)

The first number is the unique identifier of the new set expression.
For compound sets, the numbers at the end are the identifiers of the respective
subsets (for example "e 10 u 3 5" means "set expression #10 is the union of
set expression #3 and #5).

The description for BDDs must be stored in a different file called
\<bdd_filename\> (created by the dddmp library). It stores an array of bdds,
thus the \<bdd_index\> refers to the concrete BDD.


Horn and 2CNF formulas are described in the DIMACS format (without comments
or using newline characters). The structure is as follows:

    p cnf <#var> <#clauses> <clause> 0 ... 0 <clause>"
, where each clause is a list of positive
or negative integers separated by space characters. A positive integer
x refers to variable (x-1) (DIMACS starts counting at 1), and a negative
integer -y to the negation of variable (y-1).
Example: "p cnf 3 2 2 -1 0 3" represents the formula ((b \lor \lnot a) \land c)

Explicit sets are described by a list of STRIPS states, separated by
comma. The state is encoded in hex, meaning that 4 STRIPS variables are
combined to one hex digit. When the number of variables is not divisible by
4 the last hex digit assumes 0 for the missing bits.
Example: for a task with 9 variables, "5e8" represents state 010111101 (5 =
0101, e = 1110, 8 = 1000


action expression:

Action sets are either a list of action ids, the union of two action sets or
the constant set containing all actions.

        /- b <amount> <list of action ids>    (basic enumeration of <amount> action ids)
    a # -- u # #                              (union of two action sets)
        \- a                                  (the constant all actions sets)

knowledge:

Knowledge comes in 3 forms:
 - a set expression is dead
 - a set expression is a subset of another set expression
 - the task is unsolvable

The following tree represents the declaration syntax for knowledge:


                              /- ed
                             /-- ud # #
                            /--- sd # #
                        d # ---- pg # # #
                       /    \--- pi # # #
                      /      \-- rg # # #
                     /        \- ri # # #
                    /
                   /      /- ci #
                  / --- u -- cg #
                 /
                /           /- tc
               /           /-- ec
              / ------- b # <bound> --- sc # #
             /             \-- uc # #
            /               \- pc # # (# #)+
           /
          / ----------- o <bound> bi #
         /
        /
       /                              /- ur
      /                              /-- ul
     /                              /--- ir
    k                              /---- il
     \                            /----- di
      \                          /------ su # #
       \                        /------- si # #
        \                      /-------- st # #
         \                    /--------- at # #
          ------------- s # # ---------- au # #
                              \--------- pt # #
                               \-------- pu # #
                                \------- pr #
                                 \------ rp #
                                  \----- b1
                                   \---- b2
                                    \--- b3
                                     \-- b4
                                      \- b5

(the different parts of the description are separated by space characters)

The first number is the unique identifier for the knowledge.
The the number behind d (dead) as well as the two numbers behind s (subset)
are set expression identifiers.
(Example: "s 3 5") means set expression #3 is a subset of set expression #5,
and "d 6" means set expression #6 is dead)

Note for bound rule pc, the number of relevant kowledge id's is variable, but
always an even number and at least 4.

For more information about the different derivation rules related to
unsolvability, as well as basic statements B1-B5 consult the documentation in
Proof_System_Overview.pdf. For derivation rules related to optimality ("b" and
"o" rules), consult the following paper:
Esther Mugdan, Remo Christen and Salomé Eriksson.
Optimality Certificates for Classical Planning.
In Proceedings of the 33rd International Conference on Automated Planning and Scheduling (ICAPS 2023), pp. 286-294. 2023.
