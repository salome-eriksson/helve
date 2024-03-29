#ifndef SSVCONSTANT_H
#define SSVCONSTANT_H

#include "stateset.h"

#include <sstream>

enum class ConstantType {
    EMPTY,
    GOAL,
    INIT
};

class SSVConstant : public StateSetVariable
{
private:
    ConstantType constanttype;
    Task &task;
public:
    SSVConstant(std::stringstream &input, Task &task);
    ConstantType get_constant_type() const;
};

#endif // SSVCONSTANT_H
