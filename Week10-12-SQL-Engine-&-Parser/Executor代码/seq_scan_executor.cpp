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
    // page_id_t first_page_id = table_heap->GetFirstPageId();
    // BufferPoolManager *bpm = exec_ctx_->GetBufferPoolManager();
    // Page *page = bpm->FetchPage(first_page_id);
    // TablePage *table_page = reinterpret_cast<TablePage *>(page->GetData());
    // RID rid{};
    // table_page->GetFirstTupleRid(&rid);
    // bpm->UnpinPage(first_page_id,false);
    // table_iterator = TableIterator(table_heap,rid,exec_ctx_->GetTransaction());
    table_iterator_ = table_heap_->Begin(exec_ctx_->GetTransaction());
}

bool SeqScanExecutor::Next(Tuple *tuple, RID *rid) { 
    while(table_iterator_ != table_heap_->End()){
        *tuple = *table_iterator_;
        *rid = tuple->GetRid();
        table_iterator_++;
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
