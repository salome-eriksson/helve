#include "ssvconstant.h"

#include <fstream>
#include <iostream>

#include "ssfhorn.h"
#include "global_funcs.h"

SSVConstant::SSVConstant(std::stringstream &input, Task &task)
    : task(task) {
    std::string constant_type;
    input >> constant_type;
    if(constant_type.compare("e") == 0) {
        constanttype = ConstantType::EMPTY;
    } else if(constant_type.compare("i") == 0) {
        constanttype = ConstantType::INIT;
    } else if(constant_type.compare("g") == 0) {
        constanttype = ConstantType::GOAL;
    } else {
        std::cerr << "unknown constant type " << constant_type << std::endl;
        exit_with(ExitCode::CRITICAL_ERROR);
    }
}

ConstantType SSVConstant::get_constant_type() const {
    return constanttype;
}

StateSetBuilder<SSVConstant> constant_builder("c");
