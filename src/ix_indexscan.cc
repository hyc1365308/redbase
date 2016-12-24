#include "ix.h"
#include "ix_internal.h"

//
// comparator functions
//
bool ix_default_comp(void * value1, void * value2, AttrType attrtype, int attrLength){
  return true;
}

bool ix_equal(void * value1, void * value2, AttrType attrtype, int attrLength){
  switch(attrtype){
    case FLOAT: return (*(float *)value1 == *(float*)value2);
    case INT: return (*(int *)value1 == *(int *)value2) ;
    default:
      return (strncmp((char *) value1, (char *) value2, attrLength) == 0); 
  }
}

bool ix_less_than(void * value1, void * value2, AttrType attrtype, int attrLength){
  switch(attrtype){
    case FLOAT: return (*(float *)value1 < *(float*)value2);
    case INT: return (*(int *)value1 < *(int *)value2) ;
    default: 
      return (strncmp((char *) value1, (char *) value2, attrLength) < 0);
  }
}

bool ix_greater_than(void * value1, void * value2, AttrType attrtype, int attrLength){
  switch(attrtype){
    case FLOAT: return (*(float *)value1 > *(float*)value2);
    case INT: return (*(int *)value1 > *(int *)value2) ;
    default: 
      return (strncmp((char *) value1, (char *) value2, attrLength) > 0);
  }
}

bool ix_less_than_or_eq_to(void * value1, void * value2, AttrType attrtype, int attrLength){
  switch(attrtype){
    case FLOAT: return (*(float *)value1 <= *(float*)value2);
    case INT: return (*(int *)value1 <= *(int *)value2) ;
    default: 
      return (strncmp((char *) value1, (char *) value2, attrLength) <= 0);
  }
}

bool ix_greater_than_or_eq_to(void * value1, void * value2, AttrType attrtype, int attrLength){
  switch(attrtype){
    case FLOAT: return (*(float *)value1 >= *(float*)value2);
    case INT: return (*(int *)value1 >= *(int *)value2) ;
    default: 
      return (strncmp((char *) value1, (char *) value2, attrLength) >= 0);
  }
}

bool ix_not_equal(void * value1, void * value2, AttrType attrtype, int attrLength){
  switch(attrtype){
    case FLOAT: return (*(float *)value1 != *(float*)value2);
    case INT: return (*(int *)value1 != *(int *)value2) ;
    default: 
      return (strncmp((char *) value1, (char *) value2, attrLength) != 0);
  }
}

IX_IndexScan::IX_IndexScan() {
	openScan = false;
}
	
IX_IndexScan::~IX_IndexScan() {

}
	
RC IX_IndexScan::OpenScan(const IX_IndexHandle &indexHandle,
					 CompOp		compOp,
					 void		*value,
					 ClientHint	pinHint) {
	//check if the input is Invalid
	//check fileHandle
	if (!indexHandle.isValidIndexHeader())
		return IX_INVALIDINDEXHANDLE;
	//check attrType & attrLength
	switch (attrType){
		case INT  :	
		case FLOAT:	if (attrLength != 4)
						return IX_INVALIDATTRLENGTH;//Invalid value;
					break;

		case STRING:if ((attrLength < 1) || (attrLength > MAXSTRINGLEN))
						return IX_INVALIDATTRLENGTH;//Invalid value
					break;

		default :	return IX_INVALIDATTRTYPE;//Invalid AttrType
	}
	//-------------------------------------------
	//check compOp
	switch (compOp){
		case NO_OP : this->comparator = &ix_default_comp; break;
		case EQ_OP : this->comparator = &ix_equal; break;
		case NE_OP : this->comparator = &ix_not_equal; break;
		case LT_OP : this->comparator = &ix_less_than; break;
		case GT_OP : this->comparator = &ix_greater_than; break;
		case LE_OP : this->comparator = &ix_less_than_or_eq_to; break;
		case GE_OP : this->comparator = &ix_greater_than_or_eq_to; break;
		default:	 return IX_INVALIDCOMPOP;//Invalid AttrType
	}
	//check value
	if (value == NULL){
		if (compOp != NO_OP) return IX_NULLVALUEPOINTER;
	}else{
		this->value = (void *) malloc(attrLength);
		memcpy(this->value, value, attrLength);
	}

	//check pinHint
	switch (pinHint){
		case NO_HINT : this->pinHint = pinHint;break;
		default		 : return IX_INVALIDPINHINT;//Invalid ClientHint
	}
	this->ixh = &indexHandle;
	this->pfh = &(indexHandle.pfh);

	//init scan paramsv
	IX_NodeHeader_L *pData;
	if ((rc = indexHandle.GetFirstLeafPage(ph, currentPage)) ||
		(rc = ph.GetData((char *)pData)))
		return (rc);

	currentSlot = pData->firstSlotIndex;
	openScan = true;

	return 0;
}

RC IX_IndexScan::GetNextEntry(RID &rid) {
	if (!openScan) 
		return (IX_SCANNOTOPENED);
	//first get the return result;
	char *pData;
	if ((rc = ph.GetData((char *)pData)))
		return (rc);
	Node_Entry *entry = pData + sizeof(IX_IndexHeader) + currentSlot * sizeof(Node_Entry);
	if (entry->slot == NO_MORE_SLOTS){
		return IX_EOF;
	}
	rid = RID(entry->page, entry->slot);

	//find next slot in all index
	currentSlot = entry -> nextSlot;
	if (currentSlot == NO_MORE_SLOTS){
		IX_NodeHeader_L *nodeHeader = (IX_NodeHeader_L *)pData;
        currentPage = nodeHeader -> nextPage;
		if (currentPage != NO_MORE_PAGES){
			if ((rc = pfh->GetThisPage(currentPage, ph)) ||
				(rc = ph.GetData((char *)pData))){
				return (rc);
			}
			nodeHeader = (IX_NodeHeader_L *)pData;
			currentSlot = nodeHeader->firstSlotIndex;
		}
	}

    return 0;
}
	
RC IX_IndexScan::CloseScan() {
	openScan = false;
    return 0;
}