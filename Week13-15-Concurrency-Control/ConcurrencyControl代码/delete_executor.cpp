//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// delete_executor.cpp
//
// Identification: src/execution/delete_executor.cpp
//
// Copyright (c) 2015-19, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//
#include <memory>

#include "execution/executors/delete_executor.h"

namespace bustub {

DeleteExecutor::DeleteExecutor(ExecutorContext *exec_ctx, const DeletePlanNode *plan,
                               std::unique_ptr<AbstractExecutor> &&child_executor)
    : AbstractExecutor(exec_ctx), plan_(plan), child_executor_(std::move(child_executor)) {}

void DeleteExecutor::Init() {
  Catalog *catalog = exec_ctx_->GetCatalog();
  table_oid_t table_oid = plan_->TableOid();
  table_info_ = catalog->GetTable(table_oid);
  table_indexes_ = catalog->GetTableIndexes(table_info_->name_);
  child_executor_->Init();
}

void DeleteExecutor::LockExclusive(RID &rid){
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

bool DeleteExecutor::Next([[maybe_unused]] Tuple *tuple, RID *rid) { 
  if(child_executor_->Next(tuple,rid)){
    LockExclusive(*rid);
    table_info_->table_->MarkDelete(*rid,exec_ctx_->GetTransaction());
    for(auto &index_info:table_indexes_){
      IndexWriteRecord index_record{*rid,
                                    plan_->TableOid(),
                                    WType::DELETE,
                                    *tuple,
                                    index_info->index_oid_,
                                    GetExecutorContext()->GetCatalog()};
      GetExecutorContext()->GetTransaction()->AppendTableWriteRecord(index_record);
      auto key_tuple = tuple->KeyFromTuple(table_info_->schema_,index_info->key_schema_,index_info->index_->GetKeyAttrs());
      index_info->index_->DeleteEntry(key_tuple,*rid,exec_ctx_->GetTransaction());
    }
    return true;
  }
  return false;
}

}  // namespace bustub
