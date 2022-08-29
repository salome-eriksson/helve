#ifndef BDDFILE_H
#define BDDFILE_H

#include "../global_funcs.h"
#include "../task.h"

#include <unordered_map>
#include <vector>
#include "cuddObj.hh"

class BDDUtil;

class BDDFile {
private:
    static std::unordered_map<std::vector<int>, BDDUtil, IntVectorHasher> utils;
    static std::vector<int> compose;
    BDDUtil *util;
    FILE *fp;
    std::unordered_map<int, DdNode *> ddnodes;
public:
    BDDFile() {}
    BDDFile(Task &task, std::string filename);
    // expects caller to take ownership and call deref!
    DdNode *get_ddnode(int index);
    BDDUtil *get_util();
};


#endif // BDDFILE_H
