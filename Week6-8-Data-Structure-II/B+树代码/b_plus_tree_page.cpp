//===----------------------------------------------------------------------===//
//
//                         CMU-DB Project (15-445/645)
//                         ***DO NO SHARE PUBLICLY***
//
// Identification: src/page/b_plus_tree_page.cpp
//
// Copyright (c) 2018, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "storage/page/b_plus_tree_page.h"

namespace bustub {

/*
 * Helper methods to get/set page type
 * Page type enum class is defined in b_plus_tree_page.h
 */
bool BPlusTreePage::IsLeafPage() const { 
    if(page_type_== IndexPageType::LEAF_PAGE)
        return true;
    else
        return false; 
}
bool BPlusTreePage::IsRootPage() const { 
    if(parent_page_id_ == INVALID_PAGE_ID)
        return true;
    else
        return false; 
}
void BPlusTreePage::SetPageType(IndexPageType page_type) {
    page_type_ = page_type;
}

/*
 * Helper methods to get/set size (number of key/value pairs stored in that
 * page)
 */
int BPlusTreePage::GetSize() const { return size_; }
void BPlusTreePage::SetSize(int size) { size_ = size; }
void BPlusTreePage::IncreaseSize(int amount) { size_ += amount; }

/*
 * Helper methods to get/set max size (capacity) of the page
 */
int BPlusTreePage::GetMaxSize() const { return max_size_; }
void BPlusTreePage::SetMaxSize(int size) { max_size_ = size; }

/*
 * Helper method to get min page size
 * Generally, min page size == max page size / 2
 */
int BPlusTreePage::GetMinSize() const { 
    //根节点需判断是否是叶子节点
    //是叶子节点的话不需要存指针，所以最小值为1；
    //如果不是叶子节点，则需要一个数组存放空key+指针，而至少得有一个key存在，所以最小值为2.
    if(IsRootPage()){
        return IsLeafPage() ? 1 : 2;
    }
    //其他节点
    return max_size_/2;
    //叶子节点
//     if(IsLeafPage()){
//         return max_size_/2;
//     }
//     //内部节点
//     else{
//         return (max_size_ - 1) / 2 + 1; 
//     } 
}

/*
 * Helper methods to get/set parent page id
 */
page_id_t BPlusTreePage::GetParentPageId() const { return parent_page_id_; }
void BPlusTreePage::SetParentPageId(page_id_t parent_page_id) { parent_page_id_ = parent_page_id; }

/*
 * Helper methods to get/set self page id
 */
page_id_t BPlusTreePage::GetPageId() const { return page_id_; }
void BPlusTreePage::SetPageId(page_id_t page_id) { page_id_ = page_id; }

/*
 * Helper methods to set lsn
 */
void BPlusTreePage::SetLSN(lsn_t lsn) { lsn_ = lsn; }

}  // namespace bustub
