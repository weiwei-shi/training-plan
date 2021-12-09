//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// update_executor.cpp
//
// Identification: src/execution/update_executor.cpp
//
// Copyright (c) 2015-20, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//
#include <memory>

#include "execution/executors/update_executor.h"

namespace bustub {

UpdateExecutor::UpdateExecutor(ExecutorContext *exec_ctx, const UpdatePlanNode *plan,
                               std::unique_ptr<AbstractExecutor> &&child_executor)
    : AbstractExecutor(exec_ctx), plan_(plan), child_executor_(std::move(child_executor)) {}

void UpdateExecutor::Init() {
    Catalog *catalog = exec_ctx_->GetCatalog();
    table_oid_t table_oid = plan_->TableOid();
    table_info_ = catalog->GetTable(table_oid);
    table_indexes_ = catalog->GetTableIndexes(table_info_->name_); 
    child_executor_->Init();
}

bool UpdateExecutor::Next([[maybe_unused]] Tuple *tuple, RID *rid) { 
    if(child_executor_->Next(tuple,rid)){
        Tuple update_tuple = GenerateUpdatedTuple(*tuple);
        table_info_->table_->UpdateTuple(update_tuple,*rid,exec_ctx_->GetTransaction());
        for(auto &index_info:table_indexes_){
            auto key_tuple = tuple->KeyFromTuple(table_info_->schema_,index_info->key_schema_,index_info->index_->GetKeyAttrs());
            index_info->index_->DeleteEntry(key_tuple,*rid,exec_ctx_->GetTransaction());
            auto key_update_tuple = update_tuple.KeyFromTuple(table_info_->schema_,index_info->key_schema_,index_info->index_->GetKeyAttrs());
            index_info->index_->InsertEntry(key_update_tuple,*rid,exec_ctx_->GetTransaction());
        }
        return true;
    }
    return false; 
}

}  // namespace bustub
