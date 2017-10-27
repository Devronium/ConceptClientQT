#include "AnsiList.h"
#include "ConceptClient.h"
#include "GTKControl.h"
#include "ToWriteText.h"
#include <stdlib.h>

AnsiList::AnsiList(int _ac) {
    First        = NULL;
    AutoClean    = _ac;
    count        = 0;
    LastIterator = 0;
    LastPos      = 0;
}

void AnsiList::Add(void *data, char data_is_vector, char no_clean) {
    LastPos = 0;
    Node *Cursor  = First;
    Node *newNode = new Node;

    newNode->_DATA     = data;
    newNode->_NextNode = 0;
    newNode->data_type = data_is_vector;
    newNode->no_clean  = no_clean;

    if (Cursor) {
        while (Cursor->_NextNode)
            Cursor = (Node *)Cursor->_NextNode;
        Cursor->_NextNode = (void *)newNode;
    } else
        First = newNode;
    count++;
}

void AnsiList::Clear() {
    LastPos = 0;
    Node *Cursor = First;
    Node *Next;

    if (!Cursor)
        return;

    while (Cursor) {
        Next = (Node *)Cursor->_NextNode;
        void *DATA = Cursor->_DATA;
        if ((AutoClean) && (!Cursor->no_clean)) {
            switch (Cursor->data_type) {
                case DATA_MESSAGE:
                    delete (TParameters *)DATA;
                    DATA = 0;
                    break;

                case DATA_CONTROL:
                    delete (GTKControl *)DATA;
                    DATA = 0;
                    break;

                case DATA_WRITETEXT:
                    delete (ToWriteText *)DATA;
                    DATA = 0;
                    break;

                case DATA_STRING:
                    delete (AnsiString *)DATA;
                    DATA = 0;
                    break;

                case DATA_CHAR:
                    delete[] (char *)DATA;
                    DATA = 0;
                    break;
            }
        }
        delete Cursor;
        Cursor = Next;
    }
    First = NULL;
    count = 0;
}

void *AnsiList::Delete(int i) {
    LastPos = 0;
    Node *Cursor = First;
    Node *Prev   = NULL;
    void *DATA   = NULL;
    if (i >= count)
        return DATA;

    while (Cursor) {
        if (!i) {
            DATA = Cursor->_DATA;
            count--;
            if (Prev)
                Prev->_NextNode = Cursor->_NextNode;
            else
                First = (Node *)Cursor->_NextNode;

            if ((AutoClean) && (!Cursor->no_clean)) {
                switch (Cursor->data_type) {
                    case DATA_MESSAGE:
                        delete (TParameters *)DATA;
                        DATA = 0;
                        break;

                    case DATA_CONTROL:
                        delete (GTKControl *)DATA;
                        DATA = 0;
                        break;

                    case DATA_WRITETEXT:
                        delete (ToWriteText *)DATA;
                        DATA = 0;
                        break;

                    case DATA_STRING:
                        delete (AnsiString *)DATA;
                        DATA = 0;
                        break;

                    case DATA_CHAR:
                        delete[] (char *)DATA;
                        DATA = 0;
                        break;
                }
            } else
                DATA = Cursor->_DATA;
            delete Cursor;
            return DATA;
        }
        i--;
        Prev   = Cursor;
        Cursor = (Node *)Cursor->_NextNode;
    }
    return DATA;
}

// la fel ca Delete, numai ca nu tzine cont de autoclean (ac)
void *AnsiList::Remove(int i) {
    LastPos = 0;
    Node *Cursor = First;
    Node *Prev   = NULL;
    void *DATA   = NULL;
    if (i >= count)
        return DATA;

    while (Cursor) {
        if (!i) {
            count--;
            if (Prev)
                Prev->_NextNode = Cursor->_NextNode;
            else
                First = (Node *)Cursor->_NextNode;

            DATA = Cursor->_DATA;
            delete Cursor;
            return DATA;
        }
        i--;
        Prev   = Cursor;
        Cursor = (Node *)Cursor->_NextNode;
    }
    return DATA;
}

long AnsiList::Count() {
    return count;
}

void *AnsiList::Item(int i) {
    Node *Cursor   = First;
    int  lastindex = i;
    void *DATA     = NULL;

    if ((i >= count) || (i < 0))
        return DATA;

    if ((LastPos) && (i >= LastPos)) {
        i     -= LastPos;
        Cursor = LastIterator;
    }

    while (Cursor) {
        if (!i) {
            LastPos      = lastindex;
            LastIterator = Cursor;
            DATA         = Cursor->_DATA;
            return DATA;
        }
        i--;
        Cursor = (Node *)Cursor->_NextNode;
    }
    return DATA;
}

void *AnsiList::operator[](int i) {
    return Item(i);
}

void AnsiList::Insert(void *data, int i, char data_type, char no_clean) {
    LastPos = 0;
    Node *Cursor = First;
    Node *Prec   = NULL;
    Node *NewNode;
    if (i < 0)
        return;

    if (i >= count)
        Add(data, data_type, no_clean);
    else
        while (Cursor) {
            if (!i) {
                NewNode            = new Node;
                NewNode->data_type = data_type;
                NewNode->no_clean  = no_clean;
                NewNode->_DATA     = data;
                if (!Prec) {
                    NewNode->_NextNode = (void *)Cursor;
                    First = NewNode;
                } else {
                    NewNode->_NextNode = Prec->_NextNode;
                    Prec->_NextNode    = (void *)NewNode;
                }
                count++;
                return;
            }
            i--;
            Prec   = Cursor;
            Cursor = (Node *)Cursor->_NextNode;
        }
}
