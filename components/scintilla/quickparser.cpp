#include "AnsiParser.h"
#include "AnsiList.h"
#include <strings.h>
#include "quickparser.h"

std::map<unsigned int, ClassContainer *> ClassList;

unsigned int murmur_hash(const void *key, long len) {
    if (!key)
        return 0;
    const unsigned int  m     = 0x5bd1e995;
    const int           r     = 24;
    unsigned int        seed  = 0x7870AAFF;
    const unsigned char *data = (const unsigned char *)key;
    //int len = strlen((const char *)data);
    if (!len)
        return 0;

    unsigned int h = seed ^ len;

    while (len >= 4) {
        unsigned int k = *(unsigned int *)data;

        k *= m;
        k ^= k >> r;
        k *= m;

        h *= m;
        h ^= k;

        data += 4;
        len  -= 4;
    }

    switch (len) {
        case 3:
            h ^= data[2] << 16;

        case 2:
            h ^= data[1] << 8;

        case 1:
            h ^= data[0];
            h *= m;
    }

    h ^= h >> 13;
    h *= m;
    h ^= h >> 15;
    return h;
}

int ReadTo(AnsiParser *P, char *terminator) {
    AnsiString atom;

    do {
        P->NextAtom(atom);
        if (atom == terminator)
            return 1;
        else
        if (atom == (char *)"{")
            ReadTo(P, "}");
        else
        if (atom == (char *)"(")
            ReadTo(P, ")");
        else
        if (atom == (char *)"[")
            ReadTo(P, "]");
    } while (atom != (char *)"");
    return 0;
}

void AddSortedMember(ClassContainer *CC, ClassMember *CM) {
    /*int cnt = CC->Members.Count();
       int i;

       for (i = 0; i < cnt; i++) {
        ClassMember *CM_O = (ClassMember *)CC->Members.Item(i);
        if (strcasecmp(CM->Name.c_str(), CM_O->Name.c_str()) < 0)
            break;
       }
       CC->Members.Insert(CM, i, DATA_MEMBER);*/
    unsigned int hash = murmur_hash(CM->Name.c_str(), CM->Name.Length());

    if (CC->Members[hash])
        delete CC->Members[hash];
    CC->Members[hash] = CM;
}

int FindClass(std::map<unsigned int, ClassContainer *> *ClassList, AnsiString& item) {
    /*int count = ClassList->Count();

       for (int i = 0; i < count; i++) {
        ClassContainer *CC = (ClassContainer *)ClassList->Item(i);
        if (CC->Name == item)
            return(i);
       }*/
    unsigned int   hash = murmur_hash(item.c_str(), item.Length());
    ClassContainer *CM  = (*ClassList)[hash];

    if (CM) {
        int hash2 = hash;
        if (hash2 < 0)
            hash2 *= -1;
        return hash2;
    }

    if (item == (char *)"[]")
        return V_ARRAY;
    return V_NUMBER;
}

int ReadFunctionParams(AnsiParser *P, char *terminator, AnsiString *data, ClassMember *CM, std::map<unsigned int, ClassContainer *> *ClassList) {
    AnsiString atom;
    char       prec_is_var = 0;
    bool       first       = true;
    char       *ptr;
    AnsiString prec;

    do {
        P->NextAtom(atom);
        ptr = atom.c_str();
        if (atom == terminator)
            return 1;
        else
        if (atom == (char *)"var")
            prec_is_var = 1;
        else {
            if (prec_is_var) {
                prec_is_var = 0;
                *data      += "var ";
            }

            if ((!first) && (prec)) {
                int id = FindClass(ClassList, prec);

                if (id < 0) {
                    id = V_NUMBER;

                    if (prec == (char *)"string")
                        id = V_STRING;
                    else
                    if (prec == (char *)"array")
                        id = V_ARRAY;
                    else
                    if (prec == (char *)"delegate")
                        id = V_DELEGATE;
                }
                Variable *var = new Variable;
                var->Name = atom;
                var->Type = id;
                unsigned int hash = murmur_hash(atom.c_str(), atom.Length());
                if (CM->Variables[hash])
                    delete CM->Variables[hash];
                CM->Variables[hash] = var;
                prec = (char *)"";
            }

            if (*ptr == ',') {
                first = true;
                prec  = (char *)"";
            } else {
                if (!first)
                    *data += " ";
                else {
                    first = false;
                    prec  = atom;
                }
            }

            *data += atom;
        }
    } while (atom != (char *)"");
    return 0;
}

/*int ReadFunctionParams(AnsiParser *P, char *terminator, AnsiString *data) {
 *  AnsiString atom;
 *  char prec_is_var=0;
 *  do {
 *      atom=P->NextAtom();
 *      if (atom==terminator)
 *          return 1;
 *      else
 *      if (atom==(char*)"var") {
 *          prec_is_var=1;
 *      } else {
 *          if (prec_is_var) {
 *              prec_is_var=0;
 * *data+="var ";
 *          }
 * *data+=atom;
 *      }
 *  } while (atom!=(char*)"");
 *  return 0;
 * }*/

Variable *FindVar(std::map<unsigned int, Variable *> *Variables, AnsiString& item) {
    return (*Variables)[murmur_hash(item.c_str(), item.Length())];
}

ClassMember *FindMember(std::map<unsigned int, ClassMember *> *Members, AnsiString& item) {
    return (*Members)[murmur_hash(item.c_str(), item.Length())];
}

int RemoveMember(std::map<unsigned int, ClassMember *> *Members, AnsiString& item) {
    unsigned int hash = murmur_hash(item.c_str(), item.Length());
    ClassMember  *CM  = (*Members)[hash];

    if (CM) {
        Members->erase(hash);
        delete CM;
    }
    return 0;
}

int ReadFunction(AnsiParser *P, ClassMember *CM, std::map<unsigned int, ClassContainer *> *ClassList, ClassContainer *CC) {
    AnsiString atom;
    char       prec_is_var   = 0;
    char       is_assignment = 0;
    AnsiString prec;
    AnsiString asg_var;
    int        level = 0;

    do {
        prec = atom;
        P->NextAtom(atom);
        if ((prec_is_var) && (atom == (char *)"[]"))
            prec_is_var = 2;
        else
        if (atom == (char *)"{")
            level++;
        else
        if (atom == (char *)"}") {
            if (!level)
                return 1;
            else
                level--;
        } else
        if (atom == (char *)"var")
            prec_is_var = 1;
        else
        if ((atom == (char *)"=") || (atom == (char *)"=&")) {
            is_assignment = 1;
            asg_var       = prec;
        } else
        if (is_assignment) {
            if (atom == (char *)"new") {
                AnsiString next;
                P->NextAtom(next);
                int      id   = FindClass(ClassList, next);
                Variable *var = FindVar(&CM->Variables, asg_var);
                if (var)
                    var->Type = id;
                else {
                    ClassMember *var_m = FindMember(&CC->Members, asg_var);
                    if (var_m)
                        var_m->Type = id;
                    // cauta in clasa membru ...
                }
            } else {
                Variable *var     = FindVar(&CM->Variables, atom);
                int      *id      = 0;
                int      def_type = V_NUMBER;

                if (var)
                    id = &var->Type;
                else {
                    ClassMember *var_m = FindMember(&CC->Members, atom);
                    if (var_m) {
                        // verific daca este apel de functie sau delegat
                        AnsiString next;
                        P->NextAtom(next);
                        if ((var_m->fun_type == 1) && (next != (char *)"(")) {
                            def_type = V_DELEGATE;
                            id       = &def_type;
                        } else
                            id = &var_m->Type;
                    }

                    if (!id)
                        if ((atom.Length()) && ((atom[0] == '"') || (atom[0] == '\''))) {
                            def_type = V_STRING;
                            id       = &def_type;
                        }
                }

                if (id) {
                    int      *id2  = 0;
                    Variable *var2 = FindVar(&CM->Variables, asg_var);
                    if (var2)
                        id2 = &var2->Type;                       //var2->Type=var->Type;
                    else {
                        ClassMember *var_m = FindMember(&CC->Members, asg_var);
                        if (var_m)
                            id2 = &var_m->Type;
                    }

                    if (id2)
                        *id2 = *id;
                }
            }
            is_assignment = 0;
        } else
        if (atom == (char *)"return") {
            AnsiString tmp;
            P->NextAtom(tmp);
            if (tmp == (char *)"new") {
                AnsiString next;
                P->NextAtom(next);
                CM->Type = FindClass(ClassList, next);
            } else {
                Variable *var = FindVar(&CM->Variables, tmp);
                if (var)
                    CM->Type = var->Type;
                else {
                    if ((tmp.Length()) && ((tmp[0] == '"') || (tmp[0] == '\'')))
                        CM->Type = V_STRING;

                    ClassMember *CM1 = FindMember(&CC->Members, tmp);
                    if (CM1) {
                        // este functie ... oare este delegat ?
                        AnsiString next;
                        P->NextAtom(next);
                        if ((CM1->fun_type == 1) && (next != (char *)"("))
                            CM->Type = V_DELEGATE;
                        else
                            CM->Type = CM1->Type;
                    } else
                    if (tmp == (char *)"this")
                        CM->Type = FindClass(ClassList, CC->Name);
                }
            }
            //std::cout << "    Function returns : " << "something" << "\n";
        } else
        if (prec_is_var) {
            //std::cout << "    Variable : " << atom << "\n";
            Variable *var = new Variable;
            var->Name = atom;
            if (prec_is_var == 1)
                var->Type = V_NUMBER;
            else
                var->Type = V_ARRAY;
            //CM->Variables.Add(var, DATA_VARIABLE);
            unsigned int hash = murmur_hash(atom.c_str(), atom.Length());
            if (CM->Variables[hash])
                delete CM->Variables[hash];
            CM->Variables[hash] = var;

            prec_is_var = 0;
        }
    } while (atom != (char *)"");
    return 0;
}

int DoFunction(AnsiParser *P, ClassContainer *CC, std::map<unsigned int, ClassContainer *> *ClassList, char acc, AnsiString *prec = NULL) {
    AnsiString atom;
    AnsiString params;

    if (prec)
        atom = *prec;
    else
        P->NextAtom(atom);
    //std::cout << "  Function name : " << atom;
    ClassMember *CM = new ClassMember;
    CM->def_start = P->LastLine();
    //CC->Members.Add(CM,DATA_MEMBER);
    CM->Name       = atom;
    CM->Type       = V_NUMBER;
    CM->fun_type   = 1;
    CM->access     = acc;
    CM->in_include = CC->in_include;
    AddSortedMember(CC, CM);

    P->NextAtom(atom);
    if (atom == (char *)"(") {
        if (!ReadFunctionParams(P, ")", &params, CM, ClassList)) {
            CM->def_end = P->LastLine();
            return 0;
        }
    } else {
        CM->def_end = P->LastLine();
        return 0;
    }
    CM->Hint = params;
    //std::cout << ", hint (" << params << ")\n";
    P->NextAtom(atom);

    if (atom == (char *)";") {
        CM->def_end = P->LastLine();
        return 1;
    }
    if (atom != (char *)"{") {
        CM->def_end = P->LastLine();
        return 0;
    }

    int res = ReadFunction(P, CM, ClassList, CC);
    CM->def_end = P->LastLine();

    return res;
}

int DoProperty(AnsiParser *P, ClassContainer *CC, char acc) {
    AnsiString atom;

    P->NextAtom(atom);
    //std::cout << "  Property name : " << atom << "\n";
    ClassMember *CM = new ClassMember();
    CM->def_start  = P->LastLine();
    CM->fun_type   = 2;
    CM->Name       = atom;
    CM->Type       = V_NUMBER;
    CM->in_include = CC->in_include;
    CM->access     = acc;
    //CC->Members.Add(CM,DATA_MEMBER);
    AddSortedMember(CC, CM);

    P->NextAtom(atom);

    if (atom == (char *)";")
        return 1;
    if (atom != (char *)"{")
        return 0;
    do {
        P->NextAtom(atom);
        if (atom == (char *)"get") {
            P->NextAtom(atom);
            CM->linked_member = atom;
        } else
        if (atom == (char *)"}")
            break;
    } while (atom != (char *)"");
    //int res=ReadTo(P,"}");
    CM->def_end = P->LastLine();
    //return res;
    return 1;
}

int ExtendClass(ClassContainer *CC_TARGET, AnsiString& ClassName, std::map<unsigned int, ClassContainer *> *ClassList) {
    /*int        count = ClassList->Count();
       AnsiString result;

       for (int i = 0; i < count; i++) {
        ClassContainer *CC = (ClassContainer *)ClassList->Item(i);
        if (CC->Name == ClassName) {
            int count1 = CC->Members.Count();
            for (int j = 0; j < count1; j++) {
                ClassMember *CM    = (ClassMember *)CC->Members.Item(j);
                ClassMember *newCM = new ClassMember;
                newCM->Name       = CM->Name;
                newCM->Hint       = CM->Hint;
                newCM->def_start  = CM->def_start;
                newCM->def_end    = CM->def_end;
                newCM->Type       = CM->Type;
                newCM->fun_type   = CM->fun_type;
                newCM->in_include = CM->in_include;
                newCM->access     = CM->access;

                int count3 = CM->Variables.Count();
                for (int k = 0; k < count3; k++) {
                    Variable *var    = (Variable *)CM->Variables.Item(k);
                    Variable *newvar = new Variable;
                    newvar->Name = var->Name;
                    newvar->Name = var->Type;
                    newCM->Variables.Add(newvar, DATA_VARIABLE);
                }

                //CC_TARGET->Members.Add(newCM, DATA_MEMBER);
                AddSortedMember(CC_TARGET, newCM);
            }
            return(1);
        }
       }*/
    unsigned int   hash = murmur_hash(ClassName.c_str(), ClassName.Length());
    ClassContainer *CC  = (*ClassList)[hash];

    if (CC) {
        for (std::map<unsigned int, ClassMember *>::iterator it2 = CC->Members.begin(); it2 != CC->Members.end(); ++it2) {
            ClassMember *CM = it2->second;
            if (CM) {
                ClassMember *newCM = new ClassMember;
                newCM->Name       = CM->Name;
                newCM->Hint       = CM->Hint;
                newCM->def_start  = CM->def_start;
                newCM->def_end    = CM->def_end;
                newCM->Type       = CM->Type;
                newCM->fun_type   = CM->fun_type;
                newCM->in_include = CM->in_include;
                newCM->access     = CM->access;

                for (std::map<unsigned int, Variable *>::iterator it = CM->Variables.begin(); it != CM->Variables.end(); ++it) {
                    Variable *var = it->second;
                    if (var) {
                        Variable *newvar = new Variable;
                        newvar->Name = var->Name;
                        newvar->Name = var->Type;
                        unsigned int hash2 = murmur_hash(var->Name.c_str(), var->Name.Length());
                        newCM->Variables[hash2] = newvar;
                    }
                }

                AddSortedMember(CC_TARGET, newCM);
            }
        }
        return 1;
    }
    return 0;
}

int GetLinked(std::map<unsigned int, ClassMember *> *Members, char *name, int start_from) {
    /*int cnt = Members->Count();

       for (int i = start_from; i < cnt; i++) {
        ClassMember *CM = (ClassMember *)Members->Item(i);
        if (CM->Name == name)
            return(CM->Type);
       }*/
    ClassMember *CM = (*Members)[murmur_hash(name, strlen(name))];

    if (CM)
        return CM->Type;

    return V_NUMBER;
}

int DoClass(AnsiParser *P, std::map<unsigned int, ClassContainer *> *ClassList, char in_include) {
    ClassContainer *CC = new ClassContainer;

    CC->def_start = P->LastLine();
    //ClassList->Add(CC, DATA_CLASS);

    AnsiString atom;
    P->NextAtom(atom);
    //std::cout << "Class name : " << atom << "\n";
    CC->Name       = atom;
    CC->in_include = in_include;
    char has_prop = 0;

    unsigned int hash = murmur_hash(atom.c_str(), atom.Length());
    if (ClassList->count(hash))
        delete (*ClassList)[hash];

    (*ClassList)[hash] = CC;
    do {
        P->NextAtom(atom);
        if (atom == (char *)"extends") {
            P->NextAtom(atom);
            ExtendClass(CC, atom, ClassList);
        } else
        if (atom == (char *)"{")
            break;
        else
            return 0;
    } while (atom != (char *)"");

    char acces     = 0;
    char is_static = 0;

    do {
        P->NextAtom(atom);
        if (atom == (char *)"public")
            acces = 0;
        else
        if (atom == (char *)"private")
            acces = 1;
        else
        if (atom == (char *)"protected")
            acces = 2;
        else
        if (atom == (char *)"static")
            is_static = 1;
        else
        if (atom == (char *)"function") {
            DoFunction(P, CC, ClassList, acces);
            acces     = 0;
            is_static = 0;
        } else
        if (atom == (char *)"property") {
            DoProperty(P, CC, acces);
            has_prop  = 1;
            acces     = 0;
            is_static = 0;
        } else
        if (atom == (char *)"var") {
            is_static = 0;
            P->NextAtom(atom);
            ClassMember *CM = new ClassMember();
            CM->def_start = P->LastLine();
            CM->fun_type  = 0;
            CM->Name      = atom;
            if (atom == (char *)"[]") {
                P->NextAtom(CM->Name);
                CM->Type = V_ARRAY;
            } else
                CM->Type = V_NUMBER;
            CM->def_end    = CM->def_start;
            CM->in_include = CC->in_include;
            CM->access     = acces;
            //CC->Members.Add(CM,DATA_MEMBER);
            AddSortedMember(CC, CM);
            do {
                P->NextAtom(atom);
            } while ((atom.Length()) && (atom != (char *)";"));
            //std::cout << "  Member variable name : " << atom << "\n";
            acces = 0;
        } else
        if (atom == (char *)"event") {
            P->NextAtom(atom);
            //std::cout << "  Event name : " << atom << "\n";

            ClassMember *CM = new ClassMember();
            CM->def_start  = P->LastLine();
            CM->fun_type   = 3;
            CM->Name       = atom;
            CM->Type       = V_NUMBER;
            CM->in_include = CC->in_include;
            CM->def_end    = CM->def_start;
            CM->access     = acces;
            //CC->Members.Add(CM,DATA_MEMBER);
            AddSortedMember(CC, CM);

            acces     = 0;
            is_static = 0;
            P->NextAtom(atom);
            if (atom != (char *)"triggers")
                continue;
            else {
                AnsiString dummy;
                P->NextAtom(dummy);
            }
            CM->def_end = P->LastLine();
        } else
        if (atom == (char *)"override") {
            acces     = 0;
            is_static = 0;
            P->NextAtom(atom);
            RemoveMember(&CC->Members, atom);
            //std::cout << "  Override member : " << atom << "\n";
        } else
        if (atom.Length()) {
            char c = atom[0];
            if (Contains2(FIRSTCHAR, c)) {
                DoFunction(P, CC, ClassList, acces, &atom);
                acces     = 0;
                is_static = 0;
            }
        }

        if (atom == (char *)"") {
            CC->def_end = P->LastLine();
            return 0;
        }
    } while (atom != (char *)"}");
    CC->def_end = P->LastLine();
    if (has_prop) {
        for (std::map<unsigned int, ClassMember *>::iterator it2 = CC->Members.begin(); it2 != CC->Members.end(); ++it2) {
            ClassMember *CM1 = it2->second;
            if ((CM1) && (CM1->linked_member.Length()))
                CM1->Type = GetLinked(&CC->Members, CM1->linked_member.c_str(), 0);
        }
    }
    return 1;
}

int Parse(void *owner, AnsiString *text, INCLUDE_REQUEST cback, char in_include, char clean) {
    std::map<unsigned int, ClassContainer *> *TargetList = &ClassList;
    int index = 0;

    if (clean) {
        for (std::map<unsigned int, ClassContainer *>::iterator it = TargetList->begin(); it != TargetList->end(); ++it) {
            ClassContainer *CC = it->second;
            //if ((CC) && ((!CC->in_include) || (clean))) {
            if (CC) {
                it->second = NULL;
                delete CC;
            }
        }
        TargetList->clear();
    }

    AnsiList ConstantsList;
    //AnsiList ClassList;
    AnsiParser P(text, &ConstantsList);
    AnsiString atom;

    do {
        P.NextAtom(atom);

        if (atom == (char *)"class")
            DoClass(&P, TargetList, in_include);
        else
        if (atom == (char *)"pragma")
            P.GetConstant();
        else
        if (atom == (char *)"define")
            P.GetConstant();
        else
        if (atom == (char *)"import")
            P.GetConstant();
        else
        if (atom == (char *)"include") {
            atom = P.GetConstant();
            if (cback)
                //int start=IncludeClassList.Count();
                cback(owner, Strip(atom).c_str());
            //AddToMain(&IncludeClassList, &ClassList, start);
        }
    } while (atom != (char *)"");
    //PrintData(&ClassList);
    return 0;
}

int PreParse(void *owner, AnsiString *text, INCLUDE_REQUEST cback) {
    //AnsiString text;
    //text.LoadFile("test.con");
    AnsiList ConstantsList;
    //AnsiList ClassList;
    AnsiParser P(text, &ConstantsList);
    AnsiString atom;

    do {
        P.NextAtom(atom);

        if (atom == (char *)"include") {
            atom = P.GetConstant();
            if (cback)
                cback(owner, Strip(atom).c_str());
        }
    } while (atom != (char *)"");
    //PrintData(&ClassList);
    return 0;
}

int Sort(AnsiList *lst, char *pelem) {
    int count = lst->Count();

    for (int i = 0; i < count; i++)
        if (strcasecmp(((AnsiString *)lst->Item(i))->c_str(), pelem) > 0)
            return i;
    return count;
}

AnsiString GetSciList(AnsiString& ClassName) {
    //int        count = ClassList.Count();
    AnsiString result;

    /*for (int i = 0; i < count; i++) {
        ClassContainer *CC = (ClassContainer *)ClassList.Item(i);
        if (CC->Name == ClassName) {*/
    unsigned int hash = murmur_hash(ClassName.c_str(), ClassName.Length());

    if (!ClassList.count(hash))
        return result;

    ClassContainer *CC = ClassList[hash];
    if (CC) {
        int      count2 = 0;
        AnsiList temp_list_public;
        AnsiList temp_list_not_public;

        AnsiString element;
        for (std::map<unsigned int, ClassMember *>::iterator it2 = CC->Members.begin(); it2 != CC->Members.end(); ++it2) {
            ClassMember *CM = it2->second;
            // the list isn't an "autoclean" list
            if (CM) {
                count2++;
                AnsiString *element = new AnsiString(CM->Name.c_str());
                *element += "?";
                *element += AnsiString((long)(CM->access ? (CM->fun_type + 11) : (CM->fun_type + 1)));

                if (CM->access == 0)
                    temp_list_public.Insert(element, Sort(&temp_list_public, CM->Name.c_str()), DATA_STRING);
                else
                    temp_list_not_public.Insert(element, Sort(&temp_list_not_public, CM->Name.c_str()), DATA_STRING);
            }

            /*if (j)
             *  result+=" ";
             * result+=CM->Name;*/
        }
        int cnt = temp_list_public.Count();
        for (int k = 0; k < count2; k++) {
            if (k)
                result += (char *)" ";
            if (k < cnt)
                result += *(AnsiString *)temp_list_public.Item(k);
            else
                result += *(AnsiString *)temp_list_not_public.Item(k - cnt);
        }
    }
    //break;

    /*    }
       }*/
    return result;
}

AnsiString GetHint(AnsiString& ClassName, AnsiString& Member) {
    //int        count = ClassList.Count();
    AnsiString result;

    //for (int i = 0; i < count; i++) {
    //    ClassContainer *CC = (ClassContainer *)ClassList.Item(i);
    //    if (CC->Name == ClassName) {
    unsigned int hash  = murmur_hash(ClassName.c_str(), ClassName.Length());
    unsigned int hash2 = murmur_hash(Member.c_str(), Member.Length());

    ClassContainer *CC = ClassList[hash];

    if ((CC) && (CC->Members.count(hash2))) {
        //int count2 = CC->Members.size();
        //for (int j = 0; j < count2; j++) {
        ClassMember *CM = CC->Members[hash2];
        if (CM) {
            if (CM->fun_type != 1) {
                result = "Warning: This is not a member function";
                return result;
            }
            switch (CM->Type) {
                case V_DELEGATE:
                    result = "delegate ";
                    break;

                case V_ARRAY:
                    result = "[] ";
                    break;

                case V_STRING:
                    result = "string ";
                    break;

                case V_NUMBER:
                    result = "number ";
                    break;

                default:
                    {
                        ClassContainer *CC1 = (ClassContainer *)ClassList[CM->Type];
                        if ((!CC1) && (CM->Type < 0)) {
                            unsigned int hash2 = -CM->Type;
                            CC1 = ClassList[hash2];
                        }
                        if ((!CC1) && (CM->Type < 0)) {
                            unsigned int hash2 = -CM->Type;
                            CC1 = ClassList[hash2];
                        }
                        if (CC1)
                            result = CC1->Name;
                        result += (char *)" ";
                    }
            }
            AnsiString hint = (char *)"(";
            hint   += CM->Hint;
            hint   += (char *)")";
            result += ClassName;
            result += (char *)".";
            result += Member;
            result += hint;
            result  = (hint + "\n\nDeclared as:\n") + result;
        }
    }
    //break;
    //}
    //}
    return result;
}

//#include <iostream>
AnsiString GetElementType(AnsiString& element, int line) {
    AnsiString result;

    for (std::map<unsigned int, ClassContainer *>::iterator it = ClassList.begin(); it != ClassList.end(); ++it) {
        ClassContainer *CC = it->second;
        if ((CC) && (!CC->in_include) && (line >= CC->def_start) && (line <= CC->def_end)) {
            if (element == (char *)"this") {
                result = CC->Name;
                break;
            }
            for (std::map<unsigned int, ClassMember *>::iterator it2 = CC->Members.begin(); it2 != CC->Members.end(); ++it2) {
                ClassMember *CM = it2->second;
                if ((CM) && (!CM->in_include) && (line >= CM->def_start) && (line <= CM->def_end)) {
                    /*int count3 = CM->Variables.Count();
                       for (int k = 0; k < count3; k++) {
                        Variable *var = (Variable *)CM->Variables.Item(k);
                        if (var->Name == element) {
                            if (var->Type >= 0) {
                                ClassContainer *CC1 = (ClassContainer *)ClassList.Item(var->Type);
                                if (CC1)
                                    result = CC1->Name;
                            }
                            break;
                        }
                       }*/
                    unsigned int hash = murmur_hash(element.c_str(), element.Length());
                    Variable     *var = CM->Variables[hash];
                    if (var) {
                        unsigned int hash2;
                        if (var->Type >= 0)
                            hash2 = var->Type;
                        else
                            hash2 = -var->Type;

                        ClassContainer *CC1 = ClassList[hash2];
                        if (CC1)
                            result = CC1->Name;
                    }
                    break;
                }
            }
            break;
        }
    }
    return result;
}

AnsiString GetMemberType(AnsiString& ClassName, AnsiString& element) {
    /*int        count = ClassList.Count();
       AnsiString result;

       for (int i = 0; i < count; i++) {
        ClassContainer *CC = (ClassContainer *)ClassList.Item(i);
        if (CC->Name == ClassName) {
            int count2 = CC->Members.Count();
            for (int j = 0; j < count2; j++) {
                ClassMember *CM = (ClassMember *)CC->Members.Item(j);
                if (CM->Name == element) {
                    if (CM->Type >= 0) {
                        ClassContainer *CC1 = (ClassContainer *)ClassList.Item(CM->Type);
                        if (CC1)
                            result = CC1->Name;
                    }
                    break;
                }
            }
            break;
        }
       }*/
    AnsiString     result;
    unsigned int   hash = murmur_hash(ClassName.c_str(), ClassName.Length());
    ClassContainer *CC  = ClassList[hash];

    if (CC) {
        hash = murmur_hash(element.c_str(), element.Length());
        if (CC->Members.count(hash)) {
            ClassMember *CM = CC->Members[hash];
            if (CM) {
                unsigned int hash2;
                if (CM->Type >= 0)
                    hash2 = CM->Type;
                else
                    hash2 = -CM->Type;

                ClassContainer *CC1 = ClassList[hash2];
                if (CC1)
                    result = CC1->Name;
            }
        }
    }
    return result;
}

AnsiString DoType(int Type) {
    unsigned int hash = Type;

    if (Type < 0)
        hash = -Type;

    switch (Type) {
        case V_DELEGATE:
            return "delegate";
            break;

        case V_ARRAY:
            return "[]";
            break;

        case V_STRING:
            return "string";
            break;

        case V_NUMBER:
            return "number";
            break;

        default:
            {
                ClassContainer *CC1 = (ClassContainer *)ClassList[hash];
                if (CC1)
                    return CC1->Name;
            }
    }
    return "number";
}

AnsiString LastProjectList;

AnsiString GetProjectList() {
    AnsiString result;
    char       first_class = 1;


    for (std::map<unsigned int, ClassContainer *>::iterator it = ClassList.begin(); it != ClassList.end(); ++it) {
        ClassContainer *CC = it->second;//ClassList.at(i);
        if ((CC) && (!CC->in_include)) {
            int  count2 = CC->Members.size();
            char first;
            if (count2) {
                if (!first_class)
                    result += (char *)"|";
                else
                    first_class = 0;

                result += CC->Name;

                result += (char *)":";

                first = 1;
                for (std::map<unsigned int, ClassMember *>::iterator it2 = CC->Members.begin(); it2 != CC->Members.end(); ++it2) {
                    ClassMember *CM = it2->second;
                    if ((CM) && (!CM->in_include)) {
                        if (!first)
                            result += (char *)"%";
                        else
                            first = 0;

                        result += AnsiString((long)CM->access);
                        result += (char *)";";
                        result += AnsiString((long)CM->fun_type);
                        result += (char *)";";
                        result += DoType(CM->Type);
                        result += (char *)";";
                        result += CM->Name;
                        result += (char *)";";
                        result += CM->Hint;
                        result += (char *)";";
                        result += AnsiString((long)CM->def_start);
                    }
                }
            }
        }
    }
    return result;
}

char ProjectListChanged() {
    AnsiString temp = GetProjectList();
    int        res  = LastProjectList != temp;

    if (res)
        LastProjectList = temp;
    return res;
}

AnsiString GetCheckedProjectList() {
    return LastProjectList;
}
