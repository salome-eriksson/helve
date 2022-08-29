#include "bddfile.h"

#include "bddutil.h"

#include <cassert>
#include <iostream>
#include <sstream>
#include <string>
#include "dddmp.h"

BDDFile::BDDFile(Task &task, std::string filename) {
    if (compose.empty()) {
        /*
         * The dumped BDDs only contain the original variables.
         * Since we need also primed variables for checking
         * statements B4 and B5 (pro/regression), we move the
         * variables in such a way that a primed variable always
         * occurs directly after its unprimed version
         * (Example: BDD dump with vars "a b c": "a a' b b' c c'")
         */
        compose.resize(task.get_number_of_facts());
        for(int i = 0; i < task.get_number_of_facts(); ++i) {
            compose[i] = 2*i;
        }

    }
    // we need to read in the file as FILE* since dddmp uses this
    fp = fopen(filename.c_str(), "r");
    if(!fp) {
        std::cerr << "could not open bdd file " << filename << std::endl;
    }

    // the first line contains the variable order, separated by space
    std::vector<int> varorder;
    varorder.reserve(task.get_number_of_facts());
    // read in line char by char into a stringstream
    std::stringstream ss;
    char c;
    while((c = fgetc (fp)) != '\n') {
        ss << c;
    }
    // read out the same line from the string stream int by int
    // TODO: what happens if it cannot interpret the next word as int?
    // TODO: can we do this more directly? Ie. get the ints while reading the first line.
    int n;
    while (ss >> n){
        varorder.push_back(n);
    }
    assert(varorder.size() == task.get_number_of_facts());
    auto it = utils.find(varorder);
    if (it == utils.end()) {
        it = utils.emplace(std::piecewise_construct,
                      std::forward_as_tuple(varorder),
                      std::forward_as_tuple(task,varorder)).first;
    }
    util = &(it->second);

}

DdNode *BDDFile::get_ddnode(int index) {
    auto it = ddnodes.find(index);
    if (it != ddnodes.end()) {

    }

    bool found = false;
    if (it != ddnodes.end()) {
        found = true;
    }
    while(!found) {
        std::vector<int> indices;
        // next line contains indices
        std::stringstream ss;
        char c;
        while((c = fgetc (fp)) != '\n') {
            ss << c;
        }
        // read out the same line from the string stream int by int
        // TODO: what happens if it cannot interpret the next word as int?
        // TODO: can we do this more directly? Ie. get the ints while reading the first line.
        int n;
        while (ss >> n){
            indices.push_back(n);
        }
        DdNode **tmp_array;
        /* read in the BDDs into an array of DdNodes. The parameters are as follows:
         *  - manager
         *  - how to match roots: we want them to be matched by id
         *  - root names: only needed when you want to match roots by name
         *  - how to match variables: since we want to permute the BDDs in order to
         *    allow primed variables in between the original one, we take COMPOSEIDS
         *  - varnames: needed if you want to match vars according to names
         *  - varmatchauxids: needed if you want to match vars according to auxidc
         *  - varcomposeids: the variable permutation if you want to permute the BDDs
         *  - mode: if the file was dumped in text or in binary mode
         *  - filename: needed if you don't directly pass the FILE *
         *  - FILE*
         *  - Pointer to array where the DdNodes should be saved to
         */
        int nRoots = Dddmp_cuddBddArrayLoad(manager.getManager(),DDDMP_ROOT_MATCHLIST,NULL,
            DDDMP_VAR_COMPOSEIDS,NULL,NULL,compose.data(),DDDMP_MODE_TEXT,NULL,fp,&tmp_array);

        assert(indices.size() == nRoots);
        for(int i = 0; i < nRoots; ++i) {
            if (indices[i] == index) {
                found = true;
                it = ddnodes.insert(std::make_pair(indices[i],tmp_array[i])).first;
            } else {
                ddnodes.insert(std::make_pair(indices[i],tmp_array[i]));
            }
        }
        // we do not need to free the memory for tmp_array, memcheck detected no leaks
    }
    DdNode *ret = it->second;
    //ddnodes.erase(it);
    return ret;
}


BDDUtil *BDDFile::get_util() {
    return util;
}
