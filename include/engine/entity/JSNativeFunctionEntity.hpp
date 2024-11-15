#pragma once
#include "common/AutoPtr.hpp"
#include "common/Map.hpp"
#include "engine/base/JSValueType.hpp"
#include "engine/entity/JSEntity.hpp"
#include <functional>
#include <string>
namespace spark::engine {

struct JSNativeFunctionData {
  std::wstring name;
  std::function<JSFunction> callee;
  JSEntity *bind;
  common::Map<std::wstring, JSEntity *> closure;
};

class JSNativeFunctionEntity : public JSBaseEntity<JSNativeFunctionData> {
public:
  JSNativeFunctionEntity(const std::wstring &name,
                         const std::function<JSFunction> &callee,
                         const common::Map<std::wstring, JSEntity *> &closure);

  common::AutoPtr<JSValue> apply(common::AutoPtr<JSContext> ctx,
                                 common::AutoPtr<JSValue> self,
                                 common::Array<common::AutoPtr<JSValue>> args);

  void bind(common::AutoPtr<JSValue> self);

  const std::function<JSFunction> &getCallee() const;

  const common::Map<std::wstring, JSEntity *> &getClosure() const;

  const JSEntity *getBind(common::AutoPtr<JSContext> ctx) const;

  const std::wstring &getFunctionName() const;

  std::wstring toString(common::AutoPtr<JSContext> ctx) const override;

  bool toBoolean(common::AutoPtr<JSContext> ctx) const override;

  std::wstring getTypeName(common::AutoPtr<JSContext> ctx) const override;
};
}; // namespace spark::engine