#pragma once
#include <memory>
#include "./sqlConnPool.h"

template <typename ConnType>
class connRAII {
 public:
  connRAII(std::shared_ptr<ConnType>& con,
           std::shared_ptr<connPool<ConnType>> connPool);
  ~connRAII();

 private:
  std::shared_ptr<ConnType> mCon;
  std::shared_ptr<connPool<ConnType>> mPool;
};

template <typename ConnType>
connRAII<ConnType>::connRAII(std::shared_ptr<ConnType>& con,
                             std::shared_ptr<connPool<ConnType>> pool) {
  mPool = pool;
  mCon = mPool->GetConnection();
  con = mCon;
}

template <typename ConnType>
connRAII<ConnType>::~connRAII() {
  mPool->ReleaseConnection(mCon);
}