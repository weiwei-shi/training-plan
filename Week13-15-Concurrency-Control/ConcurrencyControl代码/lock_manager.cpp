//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// lock_manager.cpp
//
// Identification: src/concurrency/lock_manager.cpp
//
// Copyright (c) 2015-2019, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "concurrency/lock_manager.h"
#include "concurrency/transaction_manager.h"

#include <utility>
#include <vector>

namespace bustub {

std::list<LockManager::LockRequest>::iterator LockManager::GetIterator(std::list<LockRequest> &request_queue, txn_id_t txn_id){
  for (auto iter = request_queue.begin(); iter != request_queue.end(); ++iter){
    if (iter->txn_id_ == txn_id){
      return iter;
    }
  }
  return request_queue.end();
}

bool LockManager::LockShared(Transaction *txn, const RID &rid) {
  std::unique_lock<std::mutex> lock(latch_);
  //如果是读未提交不需要申请读锁
  if(txn->GetIsolationLevel() == IsolationLevel::READ_UNCOMMITTED){
    txn->SetState(TransactionState::ABORTED);
    throw TransactionAbortException(txn->GetTransactionId(),AbortReason::LOCKSHARED_ON_READ_UNCOMMITTED);
    return false;
  }
  // 如果是收缩阶段
  if(txn->GetState() == TransactionState::SHRINKING){
    txn->SetState(TransactionState::ABORTED);
    throw TransactionAbortException(txn->GetTransactionId(),AbortReason::LOCK_ON_SHRINKING);
    return false;
  }
  // 如果是增长阶段
  if(lock_table_.find(rid) == lock_table_.end()){
    lock_table_.emplace(std::piecewise_construct,std::forward_as_tuple(rid),std::forward_as_tuple());
  }
  LockRequestQueue *request_queue = &lock_table_[rid];
  request_queue->request_queue_.emplace_back(txn->GetTransactionId(),LockMode::SHARED);
  if(request_queue->upgrading_||request_queue->hasExclusiveLock()){
    request_queue->cv_.wait(lock);
  }
  //检测aborted，这里主要是由于死锁导致的回滚
  if(txn->GetState() == TransactionState::ABORTED){
    auto iter = GetIterator(request_queue->request_queue_,txn->GetTransactionId());
    request_queue->request_queue_.erase(iter);
    throw TransactionAbortException(txn->GetTransactionId(),AbortReason::DEADLOCK);
  }
  txn->GetSharedLockSet()->emplace(rid);
  auto iter = GetIterator(request_queue->request_queue_,txn->GetTransactionId());
  iter->granted_ = true;
  return true;
}

bool LockManager::LockExclusive(Transaction *txn, const RID &rid) {
  std::unique_lock<std::mutex> lock(latch_);
  // 如果是收缩阶段
  if(txn->GetState() == TransactionState::SHRINKING){
    txn->SetState(TransactionState::ABORTED);
    throw TransactionAbortException(txn->GetTransactionId(),AbortReason::LOCK_ON_SHRINKING);
    return false;
  }
  // 如果是增长阶段
  if(lock_table_.find(rid) == lock_table_.end()){
    lock_table_.emplace(std::piecewise_construct,std::forward_as_tuple(rid),std::forward_as_tuple());
  }
  LockRequestQueue *request_queue = &lock_table_[rid];
  request_queue->request_queue_.emplace_back(txn->GetTransactionId(),LockMode::EXCLUSIVE);
  if(request_queue->upgrading_||request_queue->hasExclusiveLock()||request_queue->hasSharedLock()){
    request_queue->cv_.wait(lock);
  }
  //检测aborted，这里主要是由于死锁导致的回滚
  if(txn->GetState() == TransactionState::ABORTED){
    auto iter = GetIterator(request_queue->request_queue_,txn->GetTransactionId());
    request_queue->request_queue_.erase(iter);
    throw TransactionAbortException(txn->GetTransactionId(),AbortReason::DEADLOCK);
  }
  txn->GetExclusiveLockSet()->emplace(rid);
  auto iter = GetIterator(request_queue->request_queue_,txn->GetTransactionId());
  iter->granted_ = true;
  return true;
}

bool LockManager::LockUpgrade(Transaction *txn, const RID &rid) {
  std::unique_lock<std::mutex> lock(latch_);
  // 如果是收缩阶段
  if(txn->GetState() == TransactionState::SHRINKING){
    txn->SetState(TransactionState::ABORTED);
    throw TransactionAbortException(txn->GetTransactionId(),AbortReason::LOCK_ON_SHRINKING);
    return false;
  }
  // 如果是增长阶段
  LockRequestQueue *request_queue = &lock_table_[rid];
  //更新锁冲突
  if (request_queue->upgrading_){
    txn->SetState(TransactionState::ABORTED);
    throw TransactionAbortException(txn->GetTransactionId(), AbortReason::UPGRADE_CONFLICT);
    return false;
  }
  request_queue->upgrading_ = true;
  txn->GetSharedLockSet()->erase(rid);
  auto iter = GetIterator(request_queue->request_queue_,txn->GetTransactionId());
  iter->lock_mode_ = LockMode::EXCLUSIVE;
  iter->granted_ = false;
  if(request_queue->hasSharedLock()||request_queue->hasExclusiveLock()){
    request_queue->cv_.wait(lock);
  }
  //检测aborted，这里主要是由于死锁导致的回滚
  if(txn->GetState() == TransactionState::ABORTED){
    request_queue->request_queue_.erase(iter);
    throw TransactionAbortException(txn->GetTransactionId(),AbortReason::DEADLOCK);
  }
  txn->GetExclusiveLockSet()->emplace(rid);
  request_queue->upgrading_ = false;
  iter->granted_ = true;
  return true;
}

bool LockManager::Unlock(Transaction *txn, const RID &rid) {
  std::unique_lock<std::mutex> lock(latch_);
  LockRequestQueue *request_queue = &lock_table_[rid];
  txn->GetSharedLockSet()->erase(rid);
  txn->GetExclusiveLockSet()->erase(rid);
  auto iter = GetIterator(request_queue->request_queue_, txn->GetTransactionId());
  LockMode mode = iter->lock_mode_;
  request_queue->request_queue_.erase(iter);
  if(txn->GetState() == TransactionState::GROWING && !(txn->GetIsolationLevel() == IsolationLevel::READ_COMMITTED && mode == LockMode::SHARED)){
    txn->SetState(TransactionState::SHRINKING);
  }
  if(mode == LockMode::SHARED){
    if(!request_queue->hasSharedLock()){
      request_queue->cv_.notify_all();
    }
  }
  else{
    request_queue->cv_.notify_all();
  }
  return true;
}

void LockManager::AddEdge(txn_id_t t1, txn_id_t t2) {
  for (auto iter = waits_for_[t1].begin();iter != waits_for_[t1].end(); ++iter){
    if(*iter == t2) {
      return;
    }
  }
  waits_for_[t1].push_back(t2);
}

void LockManager::RemoveEdge(txn_id_t t1, txn_id_t t2) {
  for (auto iter = waits_for_[t1].begin();iter != waits_for_[t1].end(); ++iter){
    if (*iter == t2){
      waits_for_[t1].erase(iter);
      return;
    }
  }
}

bool LockManager::DFS(txn_id_t txn_id){
  visited.insert(txn_id);
  std::vector<txn_id_t> &next_node_vector = waits_for_[txn_id];
  std::sort(next_node_vector.begin(),next_node_vector.end());
  for(txn_id_t next_node : next_node_vector){
    if(visited.find(next_node)!=visited.end()){
      cycle_point_set.insert(next_node);
      cycle_point_set.insert(txn_id);
      id = next_node;
      return true;
    }
    if(DFS(next_node)){
      while(txn_id != id){
        cycle_point_set.insert(txn_id);
        return true;
      }
      return false;
    }
  }
  return false;
}

bool LockManager::HasCycle(txn_id_t *txn_id) {
  std::vector<txn_id_t> txn_set;
  for(auto &pair : waits_for_){
    auto t1 = pair.first;
    if(std::find(txn_set.begin(), txn_set.end(), t1) == txn_set.end()){
      txn_set.push_back(t1);
    }
  }
  std::sort(txn_set.begin(),txn_set.end());
  for(txn_id_t start_txn_id : txn_set){
    visited.clear();
    id = -1;
    DFS(start_txn_id);
    if(cycle_point_set.size()>0){
      *txn_id = *cycle_point_set.begin();
      for(auto &cycle_point_id : cycle_point_set){
        *txn_id = std::max(*txn_id,cycle_point_id);
      }
      cycle_point_set.clear();
      return true;
    }
  }
  return false;
}

std::vector<std::pair<txn_id_t, txn_id_t>> LockManager::GetEdgeList() {
  std::vector<std::pair<txn_id_t, txn_id_t>> edge;
  for(auto &pair : waits_for_){
    auto t1 = pair.first;
    for(auto &t2 : pair.second){
      edge.emplace_back(t1,t2);
    }
  }
  return edge;
}

void LockManager::RunCycleDetection() {
  while (enable_cycle_detection_) {
    std::this_thread::sleep_for(cycle_detection_interval);
    {
      std::unique_lock<std::mutex> l(latch_);
      // TODO(student): remove the continue and add your cycle detection and abort code here
      //构建等待图
      for(auto &pair : lock_table_){
        for(auto &request : pair.second.request_queue_){
          if(request.granted_) { continue; }
          txn_tuple[request.txn_id_] = pair.first;
          for(auto &request_granted : pair.second.request_queue_){
            if(!request_granted.granted_){ continue; }
            AddEdge(request.txn_id_,request_granted.txn_id_);
          }
        }
      }
      //打破环
      txn_id_t txn_id;
      while(HasCycle(&txn_id)){
        Transaction *txn = TransactionManager::GetTransaction(txn_id);
        txn->SetState(TransactionState::ABORTED);
        //删除点及与其相关的边
        waits_for_.erase(txn_id);//删除从txn_id出发的边
        for(auto &pair : waits_for_){
          auto t1 = pair.first;
          for(auto &t2 : pair.second){
            if(t2 == txn_id){
              RemoveEdge(t1,t2);
            }
          }
        }
        //重新唤醒等待中的事务
        lock_table_[txn_tuple[txn_id]].cv_.notify_all();
      }
      //清除数据
      waits_for_.clear();
    }
  }
}

}  // namespace bustub
