#pragma once
#include "common/AutoPtr.hpp"
#include "common/Map.hpp"
#include "engine/base/JSValueType.hpp"
#include "engine/entity/JSEntity.hpp"
#include "engine/runtime/JSValue.hpp"
#include <string>

namespace spark::engine {
class JSScope : public common::Object {
private:
  JSEntity *_root;
  JSScope *_parent;

  std::vector<common::AutoPtr<JSScope>> _children;

  common::Map<std::wstring, common::AutoPtr<JSValue>> _values;

  std::vector<common::AutoPtr<JSValue>> _anonymousValues;

private:
  bool isEntityAlived(JSEntity *entity, common::Map<JSEntity *, bool> &cache);

public:
  JSScope(const common::AutoPtr<JSScope>& parent);

  virtual ~JSScope();

  JSEntity *getRoot();

  common::AutoPtr<JSScope> getRootScope();

  common::AutoPtr<JSScope> getParent();
  
  void removeChild(const common::AutoPtr<JSScope>& child);

  common::AutoPtr<JSValue> createValue(JSEntity *entity,
                                       const std::wstring &name = L"");

  const common::Map<std::wstring, common::AutoPtr<JSValue>> &getValues() const;

  common::AutoPtr<JSValue> getValue(const std::wstring &name) const;
};
}; // namespace spark::engine