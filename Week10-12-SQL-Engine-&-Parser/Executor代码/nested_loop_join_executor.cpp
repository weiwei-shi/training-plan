//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// nested_loop_join_executor.cpp
//
// Identification: src/execution/nested_loop_join_executor.cpp
//
// Copyright (c) 2015-19, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "execution/executors/nested_loop_join_executor.h"

namespace bustub {

NestedLoopJoinExecutor::NestedLoopJoinExecutor(ExecutorContext *exec_ctx, const NestedLoopJoinPlanNode *plan,
                                               std::unique_ptr<AbstractExecutor> &&left_executor,
                                               std::unique_ptr<AbstractExecutor> &&right_executor)
    : AbstractExecutor(exec_ctx), plan_(plan), left_executor_(std::move(left_executor)), right_executor_(std::move(right_executor)) {}

void NestedLoopJoinExecutor::Init() {
    left_executor_->Init();
    right_executor_->Init();
}

bool NestedLoopJoinExecutor::Next(Tuple *tuple, RID *rid) { 
    Tuple left_tuple;
    Tuple right_tuple;
    while(left_executor_->Next(&left_tuple,rid)){
        while(right_executor_->Next(&right_tuple,rid)){
            if(plan_->Predicate()->EvaluateJoin(&left_tuple,left_executor_->GetOutputSchema(),&right_tuple,right_executor_->GetOutputSchema()).GetAs<bool>()){
                std::vector<Value> res_values;
                for(const auto &col:GetOutputSchema()->GetColumns()){
                    res_values.push_back(col.GetExpr()->EvaluateJoin(&left_tuple,left_executor_->GetOutputSchema(),&right_tuple,right_executor_->GetOutputSchema()));
                }
                *tuple = {res_values,GetOutputSchema()};
                return true;
            }
        }
    }
    return false; 
}

}  // namespace bustub
