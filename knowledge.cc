#include "knowledge.h"
#include "stateset.h"
#include "actionset.h"

Knowledge::~Knowledge() {}

template<class T>
SubsetKnowledge<T>::SubsetKnowledge(int left_id, int right_id)
    : left_id(left_id), right_id(right_id) {

}

template<class T>
int SubsetKnowledge<T>::get_left_id() {
    return left_id;
}

template<class T>
int SubsetKnowledge<T>::get_right_id() {
    return right_id;
}

DeadKnowledge::DeadKnowledge(int set_id)
    : set_id(set_id) {

}

int DeadKnowledge::get_set_id() {
    return set_id;
}

template class SubsetKnowledge<StateSet>;
template class SubsetKnowledge<ActionSet>;
