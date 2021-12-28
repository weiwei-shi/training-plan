//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// seq_scan_executor.cpp
//
// Identification: src/execution/seq_scan_executor.cpp
//
// Copyright (c) 2015-19, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//
#include "execution/executors/seq_scan_executor.h"

namespace bustub {

SeqScanExecutor::SeqScanExecutor(ExecutorContext *exec_ctx, const SeqScanPlanNode *plan) : AbstractExecutor(exec_ctx), plan_(plan) {}

void SeqScanExecutor::Init() {
  table_oid_t table_oid = plan_->GetTableOid();
  TableMetadata *table_metadata_ = exec_ctx_->GetCatalog()->GetTable(table_oid);
  table_heap_ = table_metadata_->table_.get();
  table_iterator_ = table_heap_->Begin(exec_ctx_->GetTransaction());
}

void SeqScanExecutor::LockShared(RID &rid){
  Transaction *txn = exec_ctx_->GetTransaction();
  if(txn->GetIsolationLevel() == IsolationLevel::READ_UNCOMMITTED){
    return;
  }
  if(txn->GetExclusiveLockSet()->find(rid) != txn->GetExclusiveLockSet()->end() || txn->GetSharedLockSet()->find(rid) != txn->GetSharedLockSet()->end()){
    return;
  }
  exec_ctx_->GetLockManager()->LockShared(txn,rid);
}

void SeqScanExecutor::UnlockShared(RID &rid){
  Transaction *txn = exec_ctx_->GetTransaction();
  //为啥需要判断是否还有写锁？
  if(txn->GetIsolationLevel() == IsolationLevel::READ_COMMITTED && txn->GetExclusiveLockSet()->find(rid) == txn->GetExclusiveLockSet()->end()){
    exec_ctx_->GetLockManager()->Unlock(txn,rid);
  }
}

bool SeqScanExecutor::Next(Tuple *tuple, RID *rid) { 
  while(table_iterator_ != table_heap_->End()){
    *tuple = *table_iterator_;
    *rid = tuple->GetRid();
    //得到rid之后才能进行加锁
    LockShared(*rid);
    bool res = table_heap_->GetTuple(*rid,tuple,exec_ctx_->GetTransaction());
    UnlockShared(*rid);
    table_iterator_++;
    if(!res){
      continue;
    }
    if(plan_->GetPredicate() == nullptr || plan_->GetPredicate()->Evaluate(tuple,GetOutputSchema()).GetAs<bool>()){
      std::vector<Value> res_values;
      for(const auto &col:GetOutputSchema()->GetColumns()){
          res_values.push_back(tuple->GetValue(GetOutputSchema(),GetOutputSchema()->GetColIdx(col.GetName())));
      }
      *tuple = {res_values, GetOutputSchema()};
      return true;
    }
  }
  return false;
}

}  // namespace bustub
