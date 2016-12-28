
#include "ql.h"

QL_NODE::QL_NODE(const Condition condition) {
    this->lhsAttr = condition.lhsAttr;
    this->op = condition.op;
    this->bRhsIsAttr = condition.bRhsIsAttr;
    this->rhsAttr = condition.rhsAttr;
    this->rhsValue = condition.rhsValue;
}

RC QL_NODE::BuildNode();

    QL_NODE *nextNode;

private:
    RelAttr  lhsAttr;
    CompOp   op;
    int      bRhsIsAttr;
    RelAttr  rhsAttr;
    Value    rhsValue;
}