

#include "redbase.h"
#include <string.h>

#ifndef COMP2_H
#define COMP2_H

bool default_comp(void * value1, void * value2, AttrType attrtype, int attrLength);

bool equal(void * value1, void * value2, AttrType attrtype, int attrLength);

bool less_than(void * value1, void * value2, AttrType attrtype, int attrLength);

bool greater_than(void * value1, void * value2, AttrType attrtype, int attrLength);

bool less_than_or_eq_to(void * value1, void * value2, AttrType attrtype, int attrLength);

bool greater_than_or_eq_to(void * value1, void * value2, AttrType attrtype, int attrLength);

bool not_equal(void * value1, void * value2, AttrType attrtype, int attrLength);

#endif