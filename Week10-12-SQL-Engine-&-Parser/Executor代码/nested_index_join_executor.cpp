//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// nested_index_join_executor.cpp
//
// Identification: src/execution/nested_index_join_executor.cpp
//
// Copyright (c) 2015-19, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "execution/executors/nested_index_join_executor.h"

namespace bustub {

NestIndexJoinExecutor::NestIndexJoinExecutor(ExecutorContext *exec_ctx, const NestedIndexJoinPlanNode *plan,
                                             std::unique_ptr<AbstractExecutor> &&child_executor)
    : AbstractExecutor(exec_ctx), plan_(plan), child_executor_(std::move(child_executor)) {}

void NestIndexJoinExecutor::Init() {
    child_executor_->Init();
    Catalog *catalog = exec_ctx_->GetCatalog();
    TableMetadata *inner_table_metadata_ = catalog->GetTable(plan_->GetInnerTableOid());
    IndexInfo *inner_index_info_ = catalog->GetIndex(plan_->GetIndexName(),inner_table_metadata_->name_);
    inner_table_ = inner_table_metadata_->table_.get();
    //bplustree_index = reinterpret_cast<BPlusTreeIndex<GenericKey<8>, RID, GenericComparator<8>> *>(index_info_->index_.get());
    inner_index = inner_index_info_->index_.get();
}

bool NestIndexJoinExecutor::Next(Tuple *tuple, RID *rid) { 
    Tuple outer_tuple;
    Tuple inner_tuple;
    std::vector<bustub::RID> res_rid;
    while(child_executor_->Next(&outer_tuple,rid)){
        Tuple key_tuple = outer_tuple.KeyFromTuple(*child_executor_->GetOutputSchema(),*inner_index->GetKeySchema(),inner_index->GetKeyAttrs());
        inner_index->ScanKey(key_tuple,&res_rid,exec_ctx_->GetTransaction());
        if(!res_rid.empty()){
            RID common_rid = res_rid.back();
            res_rid.pop_back();
            inner_table_->GetTuple(common_rid,&inner_tuple,exec_ctx_->GetTransaction());
            std::vector<Value> res_values;
            for(const auto &col:GetOutputSchema()->GetColumns()){
                res_values.push_back(col.GetExpr()->EvaluateJoin(&outer_tuple,plan_->OuterTableSchema(),&inner_tuple,plan_->InnerTableSchema()));
            }
            *tuple = {res_values,GetOutputSchema()};
            return true;
        }    
    }
    return false; 
}

}  // namespace bustub
