#include "rm.h"

RID::RID(PageNum pageNum,SlotNum slotNum){
	this->page = pageNum;
	this->slot = slotNum;
}

RID::RID(){
	this->page = this->INVALID_PAGE;
	this->slot = this->INVALID_SLOT;
}

RID& RID::operator=(const RID &rid){
	this->page = rid.page;
	this->slot = rid.slot;
	return (*this);
}

bool RID::operator==(const RID &rid) const{
	if (this->page == rid.page && this->slot == rid.slot)
		return true;
	else 
		return false;
}

RC RID::GetPageNum(PageNum &pageNum) const{
	pageNum = this->page;
	return 0;
}

RC RID::GetSlotNum(SlotNum &slotNum) const{
	slotNum = this->slot;
	return 0;
}

RC RID::isValidRID() const{
	if (this->page >= 0 && this->slot >= 0) return 0;
	else return RM_INVALIDRID;//RM_isValid
}
