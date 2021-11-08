/**
 * index_iterator.cpp
 */
#include <cassert>

#include "storage/index/index_iterator.h"

namespace bustub {

/*
 * NOTE: you can change the destructor/constructor method here
 * set your own input parameters
 */
INDEX_TEMPLATE_ARGUMENTS
INDEXITERATOR_TYPE::IndexIterator()
    :page_id_(INVALID_PAGE_ID), index_(-1), page_(nullptr), leaf_(nullptr),
       buffer_pool_manager_(nullptr){};

INDEX_TEMPLATE_ARGUMENTS
INDEXITERATOR_TYPE::IndexIterator(Page* page, int index, BufferPoolManager *buffer_pool_manager)
    : index_(index),page_(page),leaf_(nullptr), buffer_pool_manager_(buffer_pool_manager) {
    if (page_ == nullptr){
        page_id_ = INVALID_PAGE_ID;
        leaf_ = nullptr;
    }
    else{
        page_id_ = page_->GetPageId();
        leaf_ = reinterpret_cast<B_PLUS_TREE_LEAF_PAGE_TYPE*>(page_->GetData());
        // if (index == leaf_->GetSize()){
        //     operator++();
        // }
    }
};

INDEX_TEMPLATE_ARGUMENTS
INDEXITERATOR_TYPE::~IndexIterator(){
    if(page_id_ != INVALID_PAGE_ID){
        page_->RUnlatch();
        buffer_pool_manager_->UnpinPage(page_id_,false);
    }
}

INDEX_TEMPLATE_ARGUMENTS
bool INDEXITERATOR_TYPE::isEnd() { 
    return page_id_ == INVALID_PAGE_ID;
}

INDEX_TEMPLATE_ARGUMENTS
const MappingType &INDEXITERATOR_TYPE::operator*() { return leaf_->GetItem(index_); }

INDEX_TEMPLATE_ARGUMENTS
INDEXITERATOR_TYPE &INDEXITERATOR_TYPE::operator++() { 
    if(index_ >= leaf_->GetSize() - 1 && leaf_->GetNextPageId() != INVALID_PAGE_ID){
        page_id_ = leaf_->GetNextPageId();
        page_->RUnlatch();
        buffer_pool_manager_->UnpinPage(leaf_->GetPageId(),false);
        page_ = buffer_pool_manager_->FetchPage(page_id_);
        leaf_ = reinterpret_cast<B_PLUS_TREE_LEAF_PAGE_TYPE *>(page_->GetData());
        if(leaf_->GetNextPageId() != INVALID_PAGE_ID){
            page_->RLatch();
        }
        index_ = 0;
    }
    else{
        index_++;
    }
    return *this;
}

template class IndexIterator<GenericKey<4>, RID, GenericComparator<4>>;

template class IndexIterator<GenericKey<8>, RID, GenericComparator<8>>;

template class IndexIterator<GenericKey<16>, RID, GenericComparator<16>>;

template class IndexIterator<GenericKey<32>, RID, GenericComparator<32>>;

template class IndexIterator<GenericKey<64>, RID, GenericComparator<64>>;

}  // namespace bustub
