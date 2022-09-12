#ifndef BASIC_STATEMENT_FUNCTIONS_H
#define BASIC_STATEMENT_FUNCTIONS_H

#include "../../statesetformalism.h"

#include <vector>

const StateSetFormalism *get_formalism(
        std::vector<std::vector<const StateSetVariable *>> &sets);

#endif // BASIC_STATEMENT_FUNCTIONS_H
