
#include "ql.h"
#include "redbase.h"
#include "comparators2.h"

using namespace std;

QL_NODE::QL_NODE(const Condition condition, AttrType attrType) {

    attrOffset1 = -1; attrOffset2 = -1;
    attrLength1 = -1; attrLength2 = -1;
    attrIndex1  = -1;
    recLength1  = -1;
    lhsAttr = condition.lhsAttr;
    op = condition.op;
    bRhsIsAttr = condition.bRhsIsAttr;
    rhsAttr = condition.rhsAttr;
    rhsValue = condition.rhsValue;
    this->attrType = attrType;
    switch (condition.op){
        case NO_OP : this->comparator = &default_comp; break;
        case EQ_OP : this->comparator = &equal; break;
        case NE_OP : this->comparator = &not_equal; break;
        case LT_OP : this->comparator = &less_than; break;
        case GT_OP : this->comparator = &greater_than; break;
        case LE_OP : this->comparator = &less_than_or_eq_to; break;
        case GE_OP : this->comparator = &greater_than_or_eq_to; break;
    }
    if ((bRhsIsAttr == FALSE) && (rhsValue.data == NULL)){
        if (condition.op == EQ_OP){
            compareToNull = 1;
        }
        else if (condition.op == NE_OP){
            compareToNull = 2;
        }
        else compareToNull = -1;
    }
    else compareToNull = 0;
    compareType = 0;
}

RC QL_NODE::setParams(int attrOffset, int attrLength, int attrIndex, int recLength) {
    RC rc = 0;
    if (compareToNull == -1)
        return QL_INVALIDCONDITIONS;
    if (compareType != 0)
        return QL_NODEINITED;
    this->attrOffset1 = attrOffset;
    this->attrIndex1 = attrIndex;
    this->attrLength1 = attrLength;
    this->recLength1 = recLength;
    compareType = 1;
    return rc;
}

RC QL_NODE::setParams(int attrOffset1, int attrLength1, int attrIndex1, int recLength1,
                      int attrOffset2, int attrLength2) {
    RC rc = 0;
    if (compareToNull == -1)
        return QL_INVALIDCONDITIONS;
    if (compareType != 0)
        return QL_NODEINITED;
    this->attrOffset1 = attrOffset1;
    this->attrIndex1 = attrIndex1;
    this->attrLength1 = attrLength1;
    this->recLength1 = recLength1;
    this->attrOffset2 = attrOffset2;
    this->attrLength2 = attrLength2;
    compareType = 2;
    return rc;
}

bool QL_NODE::compare(const char *record){
    if (compareToNull == 1){ // == NULL
        string nullString = "_n";
        return comparator((void *)(record + attrOffset1), nullString.c_str(), attrType, attrLength1);
    }
    else if (compareToNull == 2){// != NULL       
        string nullString = "_n";
        return comparator((void *)(record + attrOffset1), nullString.c_str(), attrType, attrLength1);
    }
    else if (compareToNull == 0){
        if (compareType == 0)
            return QL_NODENOTINITED;
        else if (compareType == 1){//one param
            return comparator((void *)(record + attrOffset1), rhsValue.data, attrType, attrLength1);
        }
        else if (compareType == 2){
            return QL_NODEFUNCSELECTERROR;
        }
        else return QL_NODEERROR;
    }
    else return QL_NODEERROR;
}

bool QL_NODE::compare(const char *record1,const char *record2){
    if (compareToNull == 1){ // == NULL
        return true;
    }
    else if (compareToNull == 2){// != NULL
        return true;
    }
    else if (compareToNull == 0){
        if (compareType == 0)
            return QL_NODENOTINITED;
        else if (compareType == 1){//one param
            return QL_NODEFUNCSELECTERROR;
        }
        else if (compareType == 2){
            return comparator((void *)(record1 + attrOffset1), (void *)(record2 + attrOffset2), attrType, attrLength1);
        }
        else return QL_NODEERROR;
    }
    else return QL_NODEERROR;
}

void QL_NODE::print(){
    cout<<"compareType   :"<<compareType<<endl;
    cout<<"compareToNull :"<<compareToNull<<endl;
    if (attrType == INT)    
        cout<<"attrType      :INT   "<<endl;
    else if (attrType == STRING) 
        cout<<"attrType      :STRING"<<endl;
    else if (attrType == FLOAT)  
        cout<<"attrType      :FLOAT "<<endl;
    else cout<<"attrType      :OTHERS "<<endl;
    cout<<"comparator    :";
    switch (op){
        case NO_OP : cout<<"default_comp"<<endl; break;
        case EQ_OP : cout<<"equal"<<endl; break;
        case NE_OP : cout<<"not_equal"<<endl; break;
        case LT_OP : cout<<"less_than"<<endl; break;
        case GT_OP : cout<<"greater_than"<<endl; break;
        case LE_OP : cout<<"less_than_or_eq_to"<<endl; break;
        case GE_OP : cout<<"greater_than_or_eq_to"<<endl; break;
        default    : cout<<"ERROR"<<endl;
    }
    cout<<"attrOffset1   :"<<attrOffset1<<endl;
    cout<<"attrLength1   :"<<attrLength1<<endl;
    cout<<"attrIndex1    :"<<attrIndex1<<endl;
    cout<<"recLength1    :"<<recLength1<<endl;
    cout<<"attrOffset2   :"<<attrOffset2<<endl;
    cout<<"attrLength2   :"<<attrLength2<<endl;
    cout<<"nextNode      :"<<(nextNode == NULL? "NULL":"NOT NULL")<<endl<<endl;
}