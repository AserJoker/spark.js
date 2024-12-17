#pragma once
#include "engine/entity/JSObjectEntity.hpp"
#include "engine/runtime/JSStore.hpp"
#include <vector>
namespace spark::engine {
class JSPromiseEntity : public JSObjectEntity {
public:
  enum class Status {
    PENDING,
    FULFILLED,
    REJECTED,
  };

private:
  Status _status;
  JSStore *_value;
  std::vector<JSStore *> _fulfilledCallbacks;
  std::vector<JSStore *> _rejectedCallbacks;
  std::vector<JSStore *> _finallyCallbacks;

public:
  JSPromiseEntity(JSStore *prototype);
  JSStore *getValue();
  void setValue(JSStore *value);
  Status &getStatus();
  std::vector<JSStore *> &getFulfilledCallbacks();
  std::vector<JSStore *> &getRejectedCallbacks();
  std::vector<JSStore *> &getFinallyCallbacks();
};
} // namespace spark::engine