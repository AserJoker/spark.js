#pragma once
#include "JSStore.hpp"
#include "common/AutoPtr.hpp"
#include "common/Map.hpp"
#include "engine/base/JSValueType.hpp"
#include "engine/entity/JSEntity.hpp"
#include "engine/runtime/JSValue.hpp"
#include <string>

namespace spark::engine {
class JSContext;
class JSScope : public common::Object {
private:
  JSStore *_root;
  JSScope *_parent;
  JSStore *_gcRoot;

  std::vector<common::AutoPtr<JSScope>> _children;

  common::Map<std::wstring, common::AutoPtr<JSValue>> _values;

  std::vector<common::AutoPtr<JSValue>> _anonymousValues;

private:
  bool isEntityAlived(JSStore *store, common::Map<JSStore *, bool> &cache);

public:
  JSScope(JSStore *gcRoot, const common::AutoPtr<JSScope> &parent = nullptr);

  virtual ~JSScope();

  JSStore *getRoot();

  common::AutoPtr<JSScope> getRootScope();

  common::AutoPtr<JSScope> getParent();

  void removeChild(const common::AutoPtr<JSScope> &child);

  common::AutoPtr<JSValue> createValue(JSStore *store,
                                       const std::wstring &name = L"");

  const common::Map<std::wstring, common::AutoPtr<JSValue>> &getValues() const;

  common::AutoPtr<JSValue> getValue(const std::wstring &name) const;
};
}; // namespace spark::engine