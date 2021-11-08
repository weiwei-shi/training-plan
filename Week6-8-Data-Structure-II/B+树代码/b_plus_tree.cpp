//===----------------------------------------------------------------------===//
//
//                         CMU-DB Project (15-445/645)
//                         ***DO NO SHARE PUBLICLY***
//
// Identification: src/index/b_plus_tree.cpp
//
// Copyright (c) 2018, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include <string>

#include "common/exception.h"
#include "common/rid.h"
#include "storage/index/b_plus_tree.h"
#include "storage/page/header_page.h"

namespace bustub {
INDEX_TEMPLATE_ARGUMENTS
BPLUSTREE_TYPE::BPlusTree(std::string name, BufferPoolManager *buffer_pool_manager, const KeyComparator &comparator,
                          int leaf_max_size, int internal_max_size)
    : index_name_(std::move(name)),
      root_page_id_(INVALID_PAGE_ID),
      buffer_pool_manager_(buffer_pool_manager),
      comparator_(comparator),
      leaf_max_size_(leaf_max_size),
      internal_max_size_(internal_max_size) {}

/*
 * Helper function to decide whether current b+tree is empty
 */
INDEX_TEMPLATE_ARGUMENTS
bool BPLUSTREE_TYPE::IsEmpty() const { return root_page_id_ == INVALID_PAGE_ID; }
/*****************************************************************************
 * SEARCH
 *****************************************************************************/
/*
 * Return the only value that associated with input key
 * This method is used for point query
 * @return : true means key exists
 */
INDEX_TEMPLATE_ARGUMENTS
bool BPLUSTREE_TYPE::GetValue(const KeyType &key, std::vector<ValueType> *result, Transaction *transaction) {
  root_id_latch_.RLock();
  Page *page = FindLeafPage(key,false,OpType::READ,transaction);
  LeafPage *leafPage = reinterpret_cast<LeafPage *>(page->GetData());
  // ValueType *value=nullptr;
  // auto isExist = leafPage->Lookup(key,value,comparator_);
  // result->push_back(*value);
  RID rid;
  bool isExist = leafPage->Lookup(key,&rid,comparator_);
  //buffer_pool_manager_->UnpinPage(leafPage->GetPageId(),false);
  if(transaction != nullptr){
    ReleaseLatchQueue(OpType::READ, transaction);
  }
  else{
    page->RUnlatch();
    buffer_pool_manager_->UnpinPage(page->GetPageId(),false);
  }
  if(!isExist){ return false; }
  result->push_back(rid);
  return true;
}

/*****************************************************************************
 * INSERTION
 *****************************************************************************/
/*
 * Insert constant key & value pair into b+ tree
 * if current tree is empty, start new tree, update root page id and insert
 * entry, otherwise insert into leaf page.
 * @return: since we only support unique key, if user try to insert duplicate
 * keys return false, otherwise return true.
 */
INDEX_TEMPLATE_ARGUMENTS
bool BPLUSTREE_TYPE::Insert(const KeyType &key, const ValueType &value, Transaction *transaction) { 
  root_id_latch_.WLock();
  //bool ret;
  if(IsEmpty()){
    StartNewTree(key,value);
    root_id_latch_.WUnlock();
    return true;
    //ret = true;
  }
  return InsertIntoLeaf(key,value,transaction);
}
/*
 * Insert constant key & value pair into an empty tree
 * User needs to first ask for new page from buffer pool manager(NOTICE: throw
 * an "out of memory" exception if returned value is nullptr), then update b+
 * tree's root page id and insert entry directly into leaf page.
 */
INDEX_TEMPLATE_ARGUMENTS
void BPLUSTREE_TYPE::StartNewTree(const KeyType &key, const ValueType &value) {
  page_id_t rootPageId;
  Page *newPage = buffer_pool_manager_->NewPage(&rootPageId);
  if(newPage == nullptr){
    throw "out of memory";
  }
  LeafPage *rootPage = reinterpret_cast<LeafPage *>(newPage->GetData());
  rootPage->Init(rootPageId,INVALID_PAGE_ID,leaf_max_size_);
  root_page_id_ = rootPageId;
  UpdateRootPageId(1);
  rootPage->Insert(key,value,comparator_);
  buffer_pool_manager_->UnpinPage(rootPageId,true);
}

/*
 * Insert constant key & value pair into leaf page
 * User needs to first find the right leaf page as insertion target, then look
 * through leaf page to see whether insert key exist or not. If exist, return
 * immdiately, otherwise insert entry. Remember to deal with split if necessary.
 * @return: since we only support unique key, if user try to insert duplicate
 * keys return false, otherwise return true.
 */
INDEX_TEMPLATE_ARGUMENTS
bool BPLUSTREE_TYPE::InsertIntoLeaf(const KeyType &key, const ValueType &value, Transaction *transaction) {
  Page *page = FindLeafPage(key,false,OpType::INSERT,transaction);
  if (page == nullptr) return false;
  LeafPage  *leafPage = reinterpret_cast<LeafPage *>(page->GetData());
  //如果key已经存在
  RID rid;
  if(leafPage->Lookup(key,&rid,comparator_)){
    //buffer_pool_manager_->UnpinPage(leafPage->GetPageId(),false);
    ReleaseLatchQueue(OpType::INSERT, transaction);
    return false;
  }
  leafPage->Insert(key,value,comparator_);
  //讨论分裂的情况
  if(leafPage->GetSize() == leafPage->GetMaxSize()){
    LeafPage *newLeafPage = Split<LeafPage>(leafPage);
    InsertIntoParent(leafPage,newLeafPage->KeyAt(0),newLeafPage,transaction);
    buffer_pool_manager_->UnpinPage(newLeafPage->GetPageId(),true);
  }
  //buffer_pool_manager_->UnpinPage(leafPage->GetPageId(),true);
  ReleaseLatchQueue(OpType::INSERT, transaction);
  return true;
}

/*
 * Split input page and return newly created page.
 * Using template N to represent either internal page or leaf page.
 * User needs to first ask for new page from buffer pool manager(NOTICE: throw
 * an "out of memory" exception if returned value is nullptr), then move half
 * of key & value pairs from input page to newly created page
 */
INDEX_TEMPLATE_ARGUMENTS
template <typename N>
N *BPLUSTREE_TYPE::Split(N *node) {
  BPlusTreePage *page = reinterpret_cast<BPlusTreePage *>(node);
  page_id_t newPageId;
  Page *newPage = buffer_pool_manager_->NewPage(&newPageId);
  if(newPage == nullptr){
    throw "out of memory";
  }
  N *newNode;
  if(page->IsLeafPage()){
    LeafPage *leafNode = reinterpret_cast<LeafPage *>(node);
    LeafPage *newLeafNode = reinterpret_cast<LeafPage *>(newPage->GetData());
    newLeafNode->Init(newPageId,leafNode->GetParentPageId(),leaf_max_size_);
    leafNode->MoveHalfTo(newLeafNode);
    //更新兄弟节点
    newLeafNode->SetNextPageId(leafNode->GetNextPageId());
    leafNode->SetNextPageId(newPageId);
    newNode = reinterpret_cast<N *>(newLeafNode);
  }
  else{
    InternalPage *internalNode = reinterpret_cast<InternalPage *>(node);
    InternalPage *newInternalNode = reinterpret_cast<InternalPage *>(newPage->GetData());
    newInternalNode->Init(newPageId,internalNode->GetParentPageId(),internal_max_size_);
    internalNode->MoveHalfTo(newInternalNode,buffer_pool_manager_);
    newNode = reinterpret_cast<N *>(newInternalNode);
  }
  return newNode;
}

/*
 * Insert key & value pair into internal page after split
 * @param   old_node      input page from split() method
 * @param   key
 * @param   new_node      returned page from split() method
 * User needs to first find the parent page of old_node, parent node must be
 * adjusted to take info of new_node into account. Remember to deal with split
 * recursively if necessary.
 */
INDEX_TEMPLATE_ARGUMENTS
void BPLUSTREE_TYPE::InsertIntoParent(BPlusTreePage *old_node, const KeyType &key, BPlusTreePage *new_node,
                                      Transaction *transaction) {
  if(old_node->IsRootPage()){
    page_id_t newRootPageId;
    Page *page = buffer_pool_manager_->NewPage(&newRootPageId);
    InternalPage *newRootPage = reinterpret_cast<InternalPage *>(page->GetData());
    newRootPage->Init(newRootPageId,INVALID_PAGE_ID,internal_max_size_);
    root_page_id_ = newRootPageId;
    UpdateRootPageId(0);
    newRootPage->PopulateNewRoot(old_node->GetPageId(),key,new_node->GetPageId());
    old_node->SetParentPageId(newRootPageId);
    new_node->SetParentPageId(newRootPageId);
    buffer_pool_manager_->UnpinPage(newRootPageId,true);
  }
  else{
    page_id_t parentPageId = old_node->GetParentPageId();
    Page *page = buffer_pool_manager_->FetchPage(parentPageId);
    InternalPage *parentPage = reinterpret_cast<InternalPage *>(page->GetData());
    parentPage->InsertNodeAfter(old_node->GetPageId(),key,new_node->GetPageId());
    if(parentPage->GetSize() == parentPage->GetMaxSize()){
      InternalPage *newParentPage = Split(parentPage);
      InsertIntoParent(parentPage,newParentPage->KeyAt(0),newParentPage,transaction);
      buffer_pool_manager_->UnpinPage(newParentPage->GetPageId(),true);
    }
    buffer_pool_manager_->UnpinPage(parentPageId,true);
  }
}

/*****************************************************************************
 * REMOVE
 *****************************************************************************/
/*
 * Delete key & value pair associated with input key
 * If current tree is empty, return immdiately.
 * If not, User needs to first find the right leaf page as deletion target, then
 * delete entry from leaf page. Remember to deal with redistribute or merge if
 * necessary.
 */
INDEX_TEMPLATE_ARGUMENTS
void BPLUSTREE_TYPE::Remove(const KeyType &key, Transaction *transaction) {
  root_id_latch_.WLock();
  if(IsEmpty()){
    ReleaseLatchQueue(OpType::DELETE,transaction);
    return;
  }
  Page *page = FindLeafPage(key,false,OpType::DELETE,transaction);
  LeafPage *leafPage = reinterpret_cast<LeafPage *>(page->GetData());
  int size = leafPage->RemoveAndDeleteRecord(key,comparator_);
  if(size < leafPage->GetMinSize()){
    CoalesceOrRedistribute(leafPage,transaction);
  }
  //buffer_pool_manager_->UnpinPage(leafPage->GetPageId(),true);
  ReleaseLatchQueue(OpType::DELETE, transaction);
  DeletePages(transaction);
}

/*
 * User needs to first find the sibling of input page. If sibling's size + input
 * page's size > page's max size, then redistribute. Otherwise, merge.
 * Using template N to represent either internal page or leaf page.
 * @return: true means target leaf page should be deleted, false means no
 * deletion happens
 */
INDEX_TEMPLATE_ARGUMENTS
template <typename N>
bool BPLUSTREE_TYPE::CoalesceOrRedistribute(N *node, Transaction *transaction) {
  if(node->IsRootPage()){
    bool delRoot = AdjustRoot(node);
    if(delRoot){
      transaction->AddIntoDeletedPageSet(node->GetPageId());
    }
    return delRoot;
  }
  page_id_t prePageId;
  Page *prePage;
  N *preNode;
  page_id_t nextPageId;
  Page *nextPage;
  N *nextNode;
  Page *parentPage = buffer_pool_manager_->FetchPage(node->GetParentPageId());
  InternalPage *parentNode = reinterpret_cast<InternalPage *>(parentPage->GetData());
  int nodeIndex = parentNode->ValueIndex(node->GetPageId());
  if(nodeIndex>0){
    prePageId = parentNode->ValueAt(nodeIndex - 1);
    prePage = buffer_pool_manager_->FetchPage(prePageId);
    preNode = reinterpret_cast<N *>(prePage->GetData());
    if(preNode->GetSize() + node->GetSize() > node->GetMaxSize()){
      Redistribute(preNode,node,1);
      buffer_pool_manager_->UnpinPage(prePageId,true);
      buffer_pool_manager_->UnpinPage(parentNode->GetPageId(),true);
      return false;
    }
  }
  if(nodeIndex < parentNode->GetSize() - 1){
    nextPageId = parentNode->ValueAt(nodeIndex + 1);
    nextPage = buffer_pool_manager_->FetchPage(nextPageId);
    nextNode = reinterpret_cast<N *>(nextPage->GetData());
    if(nextNode->GetSize() + node->GetSize() > node->GetMaxSize()){
      Redistribute(nextNode,node,0);
      buffer_pool_manager_->UnpinPage(nextPageId,true);
      buffer_pool_manager_->UnpinPage(parentNode->GetPageId(),true);
      if(nodeIndex>0){
        buffer_pool_manager_->UnpinPage(prePageId,false);
      }
      return false;
    }
  }
  if(nodeIndex>0){
    Coalesce(&preNode,&node,&parentNode,nodeIndex,transaction);
    buffer_pool_manager_->UnpinPage(parentNode->GetPageId(),true);
    transaction->AddIntoDeletedPageSet(node->GetPageId());
    buffer_pool_manager_->UnpinPage(prePageId,true);
    if(nodeIndex < parentNode->GetSize() - 1){
      buffer_pool_manager_->UnpinPage(nextPageId,false);
    }
    return true;
  }
  Coalesce(&node,&nextNode,&parentNode,nodeIndex+1,transaction);
  buffer_pool_manager_->UnpinPage(nextPageId,true);
  transaction->AddIntoDeletedPageSet(nextNode->GetPageId());
  buffer_pool_manager_->UnpinPage(parentNode->GetPageId(),true);
  return true;
}

/*
 * Move all the key & value pairs from one page to its sibling page, and notify
 * buffer pool manager to delete this page. Parent page must be adjusted to
 * take info of deletion into account. Remember to deal with coalesce or
 * redistribute recursively if necessary.
 * Using template N to represent either internal page or leaf page.
 * @param   neighbor_node      sibling page of input "node"
 * @param   node               input from method coalesceOrRedistribute()
 * @param   parent             parent page of input "node"
 * @return  true means parent node should be deleted, false means no deletion
 * happend
 */
INDEX_TEMPLATE_ARGUMENTS
template <typename N>
bool BPLUSTREE_TYPE::Coalesce(N **neighbor_node, N **node,
                              BPlusTreeInternalPage<KeyType, page_id_t, KeyComparator> **parent, int index,
                              Transaction *transaction) {
  if((*node)->IsLeafPage()){
    LeafPage *leafNode = reinterpret_cast<LeafPage *>(*node);
    LeafPage *leafNeighborNode = reinterpret_cast<LeafPage *>(*neighbor_node);
    leafNode->MoveAllTo(leafNeighborNode);
  }
  else{
    InternalPage *internalNode = reinterpret_cast<InternalPage *>(*node);
    InternalPage *internalNeighborNode = reinterpret_cast<InternalPage *>(*neighbor_node);
    KeyType middleKey = (*parent)->KeyAt(index);
    internalNode->MoveAllTo(internalNeighborNode,middleKey,buffer_pool_manager_);
  }
  //buffer_pool_manager_->UnpinPage((*node)->GetPageId(),true);
  //buffer_pool_manager_->DeletePage((*node)->GetPageId());
  (*parent)->Remove(index);
  if((*parent)->GetSize()<(*parent)->GetMinSize()){
    return CoalesceOrRedistribute((*parent),transaction);
  }
  return false;
}

/*
 * Redistribute key & value pairs from one page to its sibling page. If index ==
 * 0, move sibling page's first key & value pair into end of input "node",
 * otherwise move sibling page's last key & value pair into head of input
 * "node".
 * Using template N to represent either internal page or leaf page.
 * @param   neighbor_node      sibling page of input "node"
 * @param   node               input from method coalesceOrRedistribute()
 */
INDEX_TEMPLATE_ARGUMENTS
template <typename N>
void BPLUSTREE_TYPE::Redistribute(N *neighbor_node, N *node, int index) {
  Page *page = buffer_pool_manager_->FetchPage(node->GetParentPageId());
  InternalPage *parentPage =reinterpret_cast<InternalPage *>(page->GetData());
  if(node->IsLeafPage()){
    LeafPage *leafNode = reinterpret_cast<LeafPage *>(node);
    LeafPage *leafNeighborNode = reinterpret_cast<LeafPage *>(neighbor_node);
    if(index == 0){
      leafNeighborNode->MoveFirstToEndOf(leafNode);
      int nodeIndex = parentPage->ValueIndex(leafNeighborNode->GetPageId());
      parentPage->KeyAt(nodeIndex) = leafNeighborNode->KeyAt(0);
    }
    else{
      leafNeighborNode->MoveLastToFrontOf(leafNode);
      int nodeIndex = parentPage->ValueIndex(leafNode->GetPageId());
      parentPage->KeyAt(nodeIndex) = leafNode->KeyAt(0);
    }
  }
  else{
    InternalPage *internalNode = reinterpret_cast<InternalPage *>(node);
    InternalPage *internalNeighborNode = reinterpret_cast<InternalPage *>(neighbor_node);
    if(index == 0){
      int nodeIndex = parentPage->ValueIndex(internalNeighborNode->GetPageId());
      KeyType middleKey = internalNeighborNode->KeyAt(1);
      internalNeighborNode->MoveFirstToEndOf(internalNode,parentPage->KeyAt(nodeIndex),buffer_pool_manager_);
      parentPage->KeyAt(nodeIndex) = middleKey;
    }
    else{
      int nodeIndex = parentPage->ValueIndex(internalNode->GetPageId());
      KeyType middleKey = internalNeighborNode->KeyAt(internalNeighborNode->GetSize()-1);
      internalNeighborNode->MoveLastToFrontOf(internalNode,parentPage->KeyAt(nodeIndex),buffer_pool_manager_);
      parentPage->KeyAt(nodeIndex) = middleKey;
    }
  }
  buffer_pool_manager_->UnpinPage(parentPage->GetPageId(),true);
}
/*
 * Update root page if necessary
 * NOTE: size of root page can be less than min size and this method is only
 * called within coalesceOrRedistribute() method
 * case 1: when you delete the last element in root page, but root page still
 * has one last child
 * case 2: when you delete the last element in whole b+ tree
 * @return : true means root page should be deleted, false means no deletion
 * happend
 */
INDEX_TEMPLATE_ARGUMENTS
bool BPLUSTREE_TYPE::AdjustRoot(BPlusTreePage *old_root_node) { 
  //case 1
  if (!old_root_node->IsLeafPage() && old_root_node->GetSize() == 1){
    InternalPage *old_root_page = reinterpret_cast<InternalPage *>(old_root_node);
    page_id_t new_root_id = old_root_page->RemoveAndReturnOnlyChild();
    Page *new_root_page = buffer_pool_manager_->FetchPage(new_root_id);
    BPlusTreePage *new_root = reinterpret_cast<BPlusTreePage *>(new_root_page->GetData());
    new_root->SetParentPageId(INVALID_PAGE_ID);
    buffer_pool_manager_->UnpinPage(new_root_id,true);
    //buffer_pool_manager_->UnpinPage(old_root_page->GetPageId(),true);
    //buffer_pool_manager_->DeletePage(old_root_page->GetPageId());
    root_page_id_ = new_root_id;
    UpdateRootPageId(0);
    return true;
  }
  //case 2
  if (old_root_node->IsLeafPage() && old_root_node->GetSize() == 0){
    //buffer_pool_manager_->UnpinPage(old_root_node->GetPageId(),true);
    //buffer_pool_manager_->DeletePage(old_root_node->GetPageId());
    root_page_id_ = INVALID_PAGE_ID;
    UpdateRootPageId(0);
    return true;
  }
  return false;
}

/*****************************************************************************
 * INDEX ITERATOR
 *****************************************************************************/
/*
 * Input parameter is void, find the leaftmost leaf page first, then construct
 * index iterator
 * @return : index iterator
 */
INDEX_TEMPLATE_ARGUMENTS
INDEXITERATOR_TYPE BPLUSTREE_TYPE::begin() { 
  root_id_latch_.RLock();
  Page *leftmostPage = FindLeafPage(KeyType(),true);
  //buffer_pool_manager_->UnpinPage(leftmostPage->GetPageId(),false);
  return INDEXITERATOR_TYPE(leftmostPage,0,buffer_pool_manager_); 
}

/*
 * Input parameter is low key, find the leaf page that contains the input key
 * first, then construct index iterator
 * @return : index iterator
 */
INDEX_TEMPLATE_ARGUMENTS
INDEXITERATOR_TYPE BPLUSTREE_TYPE::Begin(const KeyType &key) { 
  root_id_latch_.RLock();
  Page *beginPage = FindLeafPage(key,false);
  LeafPage *leafPage = reinterpret_cast<LeafPage *>(beginPage->GetData());
  int index = leafPage->KeyIndex(key,comparator_);
  //buffer_pool_manager_->UnpinPage(leafPage->GetPageId(),false);
  return INDEXITERATOR_TYPE(beginPage,index,buffer_pool_manager_); 
}

/*
 * Input parameter is void, construct an index iterator representing the end
 * of the key/value pair in the leaf node
 * @return : index iterator
 */
INDEX_TEMPLATE_ARGUMENTS
INDEXITERATOR_TYPE BPLUSTREE_TYPE::end() { 
  root_id_latch_.RLock();
  Page *leftmostPage = FindLeafPage(KeyType(),true);
  LeafPage *leafPage = reinterpret_cast<LeafPage *>(leftmostPage->GetData());
  while(leafPage->GetNextPageId()!=INVALID_PAGE_ID){
    Page *nextPage = buffer_pool_manager_->FetchPage(leafPage->GetNextPageId());
    nextPage->RLatch();
    buffer_pool_manager_->UnpinPage(leafPage->GetPageId(),false);
    leftmostPage->RUnlatch();
    leftmostPage = nextPage;
    leafPage = reinterpret_cast<LeafPage *>(nextPage->GetData());  
  }
  //leftmostPage->RUnlatch();
  //Page *rightmostPage = reinterpret_cast<Page *>(leafPage);
  //return INDEXITERATOR_TYPE(rightmostPage,leafPage->GetSize(),buffer_pool_manager_);
  return INDEXITERATOR_TYPE(leftmostPage,leafPage->GetSize(),buffer_pool_manager_); 
}

/*****************************************************************************
 * UTILITIES AND DEBUG
 *****************************************************************************/
/*
 * Find leaf page containing particular key, if leftMost flag == true, find
 * the left most leaf page
 */
INDEX_TEMPLATE_ARGUMENTS
Page *BPLUSTREE_TYPE::FindLeafPage(const KeyType &key, bool leftMost, OpType op, Transaction *transaction) {
  if (IsEmpty()) {
    ReleaseLatchQueue(op,transaction);
    return nullptr;
  }
  Page *page = buffer_pool_manager_->FetchPage(root_page_id_);
  BPlusTreePage *treePage = reinterpret_cast<BPlusTreePage *>(page->GetData());
  if(transaction == nullptr){
    page->RLatch();
    root_id_latch_.RUnlock();
  }
  else if(op == OpType::READ) {
    page->RLatch();
    transaction->AddIntoPageSet(page);//将该页加入已加锁页的集合
  }
  else {
    page->WLatch();
    transaction->AddIntoPageSet(page);//将该页加入已加锁页的集合
  }
  while(!treePage->IsLeafPage()){
    InternalPage *internalPage = reinterpret_cast<InternalPage *>(treePage);
    page_id_t childPageId;
    if(leftMost){
      childPageId = internalPage->ValueAt(0);
    }
    else{
      childPageId = internalPage->Lookup(key,comparator_);
    }
    Page *childPage = buffer_pool_manager_->FetchPage(childPageId);
    BPlusTreePage *childTreePage = reinterpret_cast<BPlusTreePage *>(childPage->GetData());
    //buffer_pool_manager_->UnpinPage(treePage->GetPageId(),false);
    if(transaction == nullptr){
      childPage->RLatch();
      page->RUnlatch();
      buffer_pool_manager_->UnpinPage(treePage->GetPageId(),false);
    }
    page = childPage;
    treePage = childTreePage;
    if(transaction != nullptr){
      if(op == OpType::READ) {
        page->RLatch();
        ReleaseLatchQueue(op, transaction);
      }
      else {
        page->WLatch();
        if (IsSafe(treePage, op)) {
          ReleaseLatchQueue(op, transaction);
        }
      }
      transaction->AddIntoPageSet(page);
    }
  }
  return page;
}

INDEX_TEMPLATE_ARGUMENTS
bool BPLUSTREE_TYPE::IsSafe(BPlusTreePage* page, OpType op) {
  // insert
  if (op == OpType::INSERT) {
    return page->GetSize() < page->GetMaxSize() - 1;
  }
  // remove
  //if(op == OpType::DELETE) {
  if (page->IsRootPage()) {
    if (page->IsLeafPage()) {
      return true;
    }
    return page->GetSize() > 2;
  }
  return page->GetSize() > page->GetMinSize();
}

INDEX_TEMPLATE_ARGUMENTS
void BPLUSTREE_TYPE::ReleaseLatchQueue(OpType op, Transaction *transaction) {
  if (transaction == nullptr) {
    return;
  }
  auto pages = transaction->GetPageSet();
  for (auto page : *pages) {
    page_id_t page_id = page->GetPageId();
    if (op == OpType::READ) {
      page->RUnlatch();
      buffer_pool_manager_->UnpinPage(page_id, false);
    } else {
      page->WUnlatch();
      buffer_pool_manager_->UnpinPage(page_id, true);
    }
  }
  pages->clear();
  if(op == OpType::READ) {
    root_id_latch_.RUnlock();
  }
  else {
    root_id_latch_.WUnlock();
  }
}

INDEX_TEMPLATE_ARGUMENTS
void BPLUSTREE_TYPE::DeletePages(Transaction *transaction) {
  auto pages = transaction->GetDeletedPageSet();
  for (auto &page_id : *pages){
    buffer_pool_manager_->DeletePage(page_id);
  }
  pages->clear();
}

/*
 * Update/Insert root page id in header page(where page_id = 0, header_page is
 * defined under include/page/header_page.h)
 * Call this method everytime root page id is changed.
 * @parameter: insert_record      defualt value is false. When set to true,
 * insert a record <index_name, root_page_id> into header page instead of
 * updating it.
 */
INDEX_TEMPLATE_ARGUMENTS
void BPLUSTREE_TYPE::UpdateRootPageId(int insert_record) {
  HeaderPage *header_page = static_cast<HeaderPage *>(buffer_pool_manager_->FetchPage(HEADER_PAGE_ID));
  if (insert_record != 0) {
    // create a new record<index_name + root_page_id> in header_page
    header_page->InsertRecord(index_name_, root_page_id_);
  } else {
    // update root_page_id in header_page
    header_page->UpdateRecord(index_name_, root_page_id_);
  }
  buffer_pool_manager_->UnpinPage(HEADER_PAGE_ID, true);
}

/*
 * This method is used for test only
 * Read data from file and insert one by one
 */
INDEX_TEMPLATE_ARGUMENTS
void BPLUSTREE_TYPE::InsertFromFile(const std::string &file_name, Transaction *transaction) {
  int64_t key;
  std::ifstream input(file_name);
  while (input) {
    input >> key;

    KeyType index_key;
    index_key.SetFromInteger(key);
    RID rid(key);
    Insert(index_key, rid, transaction);
  }
}
/*
 * This method is used for test only
 * Read data from file and remove one by one
 */
INDEX_TEMPLATE_ARGUMENTS
void BPLUSTREE_TYPE::RemoveFromFile(const std::string &file_name, Transaction *transaction) {
  int64_t key;
  std::ifstream input(file_name);
  while (input) {
    input >> key;
    KeyType index_key;
    index_key.SetFromInteger(key);
    Remove(index_key, transaction);
  }
}

/**
 * This method is used for debug only, You don't  need to modify
 * @tparam KeyType
 * @tparam ValueType
 * @tparam KeyComparator
 * @param page
 * @param bpm
 * @param out
 */
INDEX_TEMPLATE_ARGUMENTS
void BPLUSTREE_TYPE::ToGraph(BPlusTreePage *page, BufferPoolManager *bpm, std::ofstream &out) const {
  std::string leaf_prefix("LEAF_");
  std::string internal_prefix("INT_");
  if (page->IsLeafPage()) {
    LeafPage *leaf = reinterpret_cast<LeafPage *>(page);
    // Print node name
    out << leaf_prefix << leaf->GetPageId();
    // Print node properties
    out << "[shape=plain color=green ";
    // Print data of the node
    out << "label=<<TABLE BORDER=\"0\" CELLBORDER=\"1\" CELLSPACING=\"0\" CELLPADDING=\"4\">\n";
    // Print data
    out << "<TR><TD COLSPAN=\"" << leaf->GetSize() << "\">P=" << leaf->GetPageId() << "</TD></TR>\n";
    out << "<TR><TD COLSPAN=\"" << leaf->GetSize() << "\">"
        << "max_size=" << leaf->GetMaxSize() << ",min_size=" << leaf->GetMinSize() << "</TD></TR>\n";
    out << "<TR>";
    for (int i = 0; i < leaf->GetSize(); i++) {
      out << "<TD>" << leaf->KeyAt(i) << "</TD>\n";
    }
    out << "</TR>";
    // Print table end
    out << "</TABLE>>];\n";
    // Print Leaf node link if there is a next page
    if (leaf->GetNextPageId() != INVALID_PAGE_ID) {
      out << leaf_prefix << leaf->GetPageId() << " -> " << leaf_prefix << leaf->GetNextPageId() << ";\n";
      out << "{rank=same " << leaf_prefix << leaf->GetPageId() << " " << leaf_prefix << leaf->GetNextPageId() << "};\n";
    }

    // Print parent links if there is a parent
    if (leaf->GetParentPageId() != INVALID_PAGE_ID) {
      out << internal_prefix << leaf->GetParentPageId() << ":p" << leaf->GetPageId() << " -> " << leaf_prefix
          << leaf->GetPageId() << ";\n";
    }
  } else {
    InternalPage *inner = reinterpret_cast<InternalPage *>(page);
    // Print node name
    out << internal_prefix << inner->GetPageId();
    // Print node properties
    out << "[shape=plain color=pink ";  // why not?
    // Print data of the node
    out << "label=<<TABLE BORDER=\"0\" CELLBORDER=\"1\" CELLSPACING=\"0\" CELLPADDING=\"4\">\n";
    // Print data
    out << "<TR><TD COLSPAN=\"" << inner->GetSize() << "\">P=" << inner->GetPageId() << "</TD></TR>\n";
    out << "<TR><TD COLSPAN=\"" << inner->GetSize() << "\">"
        << "max_size=" << inner->GetMaxSize() << ",min_size=" << inner->GetMinSize() << "</TD></TR>\n";
    out << "<TR>";
    for (int i = 0; i < inner->GetSize(); i++) {
      out << "<TD PORT=\"p" << inner->ValueAt(i) << "\">";
      if (i > 0) {
        out << inner->KeyAt(i);
      } else {
        out << " ";
      }
      out << "</TD>\n";
    }
    out << "</TR>";
    // Print table end
    out << "</TABLE>>];\n";
    // Print Parent link
    if (inner->GetParentPageId() != INVALID_PAGE_ID) {
      out << internal_prefix << inner->GetParentPageId() << ":p" << inner->GetPageId() << " -> " << internal_prefix
          << inner->GetPageId() << ";\n";
    }
    // Print leaves
    for (int i = 0; i < inner->GetSize(); i++) {
      auto child_page = reinterpret_cast<BPlusTreePage *>(bpm->FetchPage(inner->ValueAt(i))->GetData());
      ToGraph(child_page, bpm, out);
      if (i > 0) {
        auto sibling_page = reinterpret_cast<BPlusTreePage *>(bpm->FetchPage(inner->ValueAt(i - 1))->GetData());
        if (!sibling_page->IsLeafPage() && !child_page->IsLeafPage()) {
          out << "{rank=same " << internal_prefix << sibling_page->GetPageId() << " " << internal_prefix
              << child_page->GetPageId() << "};\n";
        }
        bpm->UnpinPage(sibling_page->GetPageId(), false);
      }
    }
  }
  bpm->UnpinPage(page->GetPageId(), false);
}

/**
 * This function is for debug only, you don't need to modify
 * @tparam KeyType
 * @tparam ValueType
 * @tparam KeyComparator
 * @param page
 * @param bpm
 */
INDEX_TEMPLATE_ARGUMENTS
void BPLUSTREE_TYPE::ToString(BPlusTreePage *page, BufferPoolManager *bpm) const {
  if (page->IsLeafPage()) {
    LeafPage *leaf = reinterpret_cast<LeafPage *>(page);
    std::cout << "Leaf Page: " << leaf->GetPageId() << " parent: " << leaf->GetParentPageId()
              << " next: " << leaf->GetNextPageId() << std::endl;
    for (int i = 0; i < leaf->GetSize(); i++) {
      std::cout << leaf->KeyAt(i) << ",";
    }
    std::cout << std::endl;
    std::cout << std::endl;
  } else {
    InternalPage *internal = reinterpret_cast<InternalPage *>(page);
    std::cout << "Internal Page: " << internal->GetPageId() << " parent: " << internal->GetParentPageId() << std::endl;
    for (int i = 0; i < internal->GetSize(); i++) {
      std::cout << internal->KeyAt(i) << ": " << internal->ValueAt(i) << ",";
    }
    std::cout << std::endl;
    std::cout << std::endl;
    for (int i = 0; i < internal->GetSize(); i++) {
      ToString(reinterpret_cast<BPlusTreePage *>(bpm->FetchPage(internal->ValueAt(i))->GetData()), bpm);
    }
  }
  bpm->UnpinPage(page->GetPageId(), false);
}

template class BPlusTree<GenericKey<4>, RID, GenericComparator<4>>;
template class BPlusTree<GenericKey<8>, RID, GenericComparator<8>>;
template class BPlusTree<GenericKey<16>, RID, GenericComparator<16>>;
template class BPlusTree<GenericKey<32>, RID, GenericComparator<32>>;
template class BPlusTree<GenericKey<64>, RID, GenericComparator<64>>;

}  // namespace bustub
