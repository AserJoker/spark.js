#pragma once
#include "common/AutoPtr.hpp"
#include "engine/entity/JSEntity.hpp"
#include <vector>
namespace spark::engine {
class JSStore {

private:
  std::vector<JSStore *> _parents;

  std::vector<JSStore *> _children;

  common::AutoPtr<JSEntity> _entity;

public:
  JSStore(const common::AutoPtr<JSEntity> &entity = nullptr);

  virtual ~JSStore();

  void appendChild(JSStore *store);

  void removeChild(JSStore *store);

  common::AutoPtr<JSEntity> getEntity();

  const common::AutoPtr<JSEntity> &getEntity() const;

  const std::vector<JSStore *> &getParent() const;

  std::vector<JSStore *> &getParent();

  const std::vector<JSStore *> &getChildren() const;

  std::vector<JSStore *> &getChildren();

  void setEntity(common::AutoPtr<JSEntity> entity);
};
}; // namespace spark::engine