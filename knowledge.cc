#include "knowledge.h"

#include "actionset.h"
#include "stateset.h"

Knowledge::~Knowledge() {}

template<class T>
SubsetKnowledge<T>::SubsetKnowledge(SetID left_id, SetID right_id)
    : left_id(left_id), right_id(right_id) {

}

template<class T>
SetID SubsetKnowledge<T>::get_left_id() const {
    return left_id;
}

template<class T>
SetID SubsetKnowledge<T>::get_right_id() const {
    return right_id;
}

DeadKnowledge::DeadKnowledge(SetID set_id)
    : set_id(set_id) {

}

SetID DeadKnowledge::get_set_id() const {
    return set_id;
}

BoundKnowledge::BoundKnowledge(SetID set_id, unsigned bound)
    : set_id(set_id), bound(bound) {

}

SetID BoundKnowledge::get_set_id() const {
    return set_id;
}

unsigned BoundKnowledge::get_bound() const {
    return bound;
}

OptimalCostKnowledge::OptimalCostKnowledge(unsigned lower_bound)
    : lower_bound(lower_bound) {

}

unsigned OptimalCostKnowledge::get_lower_bound() const {
    return lower_bound;
}

template class SubsetKnowledge<StateSet>;
template class SubsetKnowledge<ActionSet>;
