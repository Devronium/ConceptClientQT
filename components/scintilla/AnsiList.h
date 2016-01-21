#ifndef __ANSILIST_H
#define __ANSILIST_H

#define DATA_STRING      0x01
#define DATA_CLASS       100
#define DATA_MEMBER      101
#define DATA_VARIABLE    102
#define DATA_INCLUDED    103

struct Node {
    void *_DATA;
    void *_NextNode;
    char data_type;
    char no_clean;
};

class AnsiList
{
private:
    Node *First;
    Node *Last;
    int  AutoClean;
    long count;

    Node *LastIterator;
    long LastPos;
    char LastType;
public:
    AnsiList(int _ac = 1);
    void Add(void *data, char data_is_vector /*=0*/, char no_clean = 0);

    void *operator[](int i);
    void *Item(int i);
    void *Delete(int i);
    void *Remove(int i);
    void Clear();
    void Insert(void *data, int i, char data_type, char no_clean = 0);
    long Count();
    char GetLastType();

    ~AnsiList(void);
};
#endif
