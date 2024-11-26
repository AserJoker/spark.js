#pragma once
#include "common/AutoPtr.hpp"
#include "common/Map.hpp"
#include "engine/base/JSValueType.hpp"
#include "engine/entity/JSEntity.hpp"
#include "engine/runtime/JSValue.hpp"
#include <string>

namespace spark::engine {
class JSScope {
private:
  JSEntity *_root;
  JSScope *_parent;

  std::vector<JSScope *> _children;

  common::Map<std::wstring, common::AutoPtr<JSValue>> _values;

  std::vector<common::AutoPtr<JSValue>> _anonymousValues;

private:
  bool isEntityAlived(JSEntity *entity, common::Map<JSEntity *, bool> &cache);

public:
  JSScope(JSScope *parent);

  virtual ~JSScope();

  JSEntity *getRoot();

  JSScope *getRootScope();

  JSScope *getParent();

  common::AutoPtr<JSValue> createValue(JSEntity *entity,
                                       const std::wstring &name = L"");

  const common::Map<std::wstring, common::AutoPtr<JSValue>> &getValues() const;

  common::AutoPtr<JSValue> getValue(const std::wstring &name) const;
};
}; // namespace spark::engine