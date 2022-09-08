#include "basic_statement_functions.h"

const StateSetFormalism *get_formalism(
        std::vector<std::vector<const StateSetVariable *>> &sets) {
    const StateSetFormalism *ret = nullptr;
    for (auto set : sets) {
        for (const StateSetVariable *var : set) {
            ret = dynamic_cast<const StateSetFormalism *>(var);
            if(ret) {
                return ret;
            }
        }
    }
    return nullptr;
}
