//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// index_scan_executor.cpp
//
// Identification: src/execution/index_scan_executor.cpp
//
// Copyright (c) 2015-19, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//
#include "execution/executors/index_scan_executor.h"

namespace bustub {
IndexScanExecutor::IndexScanExecutor(ExecutorContext *exec_ctx, const IndexScanPlanNode *plan)
    : AbstractExecutor(exec_ctx), plan_(plan) {}

void IndexScanExecutor::Init() {
    index_oid_t index_oid = plan_->GetIndexOid();
    Catalog *catalog = exec_ctx_->GetCatalog();
    IndexInfo *index_info = catalog->GetIndex(index_oid);
    Index *index = index_info->index_.get();
    BPlusTreeIndex<GenericKey<8>, RID, GenericComparator<8>> *bplustree_index = reinterpret_cast<BPlusTreeIndex<GenericKey<8>, RID, GenericComparator<8>> *>(index);
    TableMetadata *table_metadata_ = catalog->GetTable(index_info->table_name_);
    table_heap_ = table_metadata_->table_.get();
    index_iterator_ = bplustree_index->GetBeginIterator();
    end_iterator_ = bplustree_index->GetEndIterator();
}

bool IndexScanExecutor::Next(Tuple *tuple, RID *rid) { 
    while(index_iterator_ != end_iterator_){
        *rid = (*index_iterator_).second;
        ++index_iterator_;
        table_heap_->GetTuple(*rid,tuple,exec_ctx_->GetTransaction());
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
