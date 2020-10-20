#ifndef KNOWLEDGE_H
#define KNOWLEDGE_H

class Knowledge
{
public:
    virtual ~Knowledge() = 0;
};

template<class T>
class SubsetKnowledge : public Knowledge
{
private:
    int left_id;
    int right_id;
public:
    SubsetKnowledge(int left_id, int right_id);
    virtual ~SubsetKnowledge() {}

    int get_left_id();
    int get_right_id();
};

// no template needed - only state sets can be dead (so far...)
class DeadKnowledge : public Knowledge
{
private:
    int set_id;
public:
    DeadKnowledge(int set_id);
    virtual ~DeadKnowledge() {}

    int get_set_id();
};

class UnsolvableKnowledge : public Knowledge
{
public:
    virtual ~UnsolvableKnowledge() {}
};

#endif // KNOWLEDGE_H
