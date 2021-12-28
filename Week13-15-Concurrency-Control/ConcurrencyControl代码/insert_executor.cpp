//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// insert_executor.cpp
//
// Identification: src/execution/insert_executor.cpp
//
// Copyright (c) 2015-19, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//
#include <memory>

#include "execution/executors/insert_executor.h"

namespace bustub {

InsertExecutor::InsertExecutor(ExecutorContext *exec_ctx, const InsertPlanNode *plan,
                               std::unique_ptr<AbstractExecutor> &&child_executor)
    : AbstractExecutor(exec_ctx), plan_(plan), child_executor_(std::move(child_executor)) {}

void InsertExecutor::Init() {
    Catalog *catalog = exec_ctx_->GetCatalog();
    table_oid_t table_oid = plan_->TableOid();
    table_metadata_ = catalog->GetTable(table_oid);
    table_indexes_ = catalog->GetTableIndexes(table_metadata_->name_); 
    if (plan_->IsRawInsert()){
        iterator_ = plan_->RawValues().begin();
    }
    else{
        child_executor_->Init();
    }
}

void InsertExecutor::LockExclusive(RID &rid){
  Transaction *txn = exec_ctx_->GetTransaction();
  if(txn->GetSharedLockSet()->find(rid) != txn->GetSharedLockSet()->end()){
    exec_ctx_->GetLockManager()->LockUpgrade(txn,rid);
    return;
  }
  if(txn->GetExclusiveLockSet()->find(rid) != txn->GetExclusiveLockSet()->end()){
    return;
  }
  exec_ctx_->GetLockManager()->LockExclusive(txn,rid);
}

void InsertExecutor::InsertTuple(Tuple &insert_tuple, RID *rid) {
    table_metadata_->table_->InsertTuple(insert_tuple, rid, exec_ctx_->GetTransaction());
    for(auto &index_info:table_indexes_){
        auto key_insert_tuple = insert_tuple.KeyFromTuple(table_metadata_->schema_,index_info->key_schema_,index_info->index_->GetKeyAttrs());
        index_info->index_->InsertEntry(key_insert_tuple,*rid,exec_ctx_->GetTransaction());
    }
}

bool InsertExecutor::Next([[maybe_unused]] Tuple *tuple, RID *rid) { 
    if(plan_->IsRawInsert()){
        while(iterator_ != plan_->RawValues().end()){
            Tuple insert_tuple(*iterator_, &table_metadata_->schema_);
            InsertTuple(insert_tuple,rid);
            LockExclusive(*rid);
            iterator_++;
            //return true;
        }
        return false;
    }
    else{
        while(child_executor_->Next(tuple,rid)){
            InsertTuple(*tuple,rid);
            LockExclusive(*rid);
            //return true;
        }
        return false;
    }
}

}  // namespace bustub
