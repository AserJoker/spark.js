#pragma once
#include "common/AutoPtr.hpp"
#include "common/Map.hpp"
#include "engine/base/JSValueType.hpp"
#include "engine/entity/JSEntity.hpp"
#include <functional>
#include <string>
namespace spark::engine {

struct JSNativeFunction {};

class JSNativeFunctionEntity : public JSEntity {
private:
  std::wstring _name;
  std::function<JSFunction> _callee;
  JSEntity *_bind;
  common::Map<std::wstring, JSEntity *> _closure;

public:
  JSNativeFunctionEntity(const std::wstring &name,
                         const std::function<JSFunction> &callee,
                         const common::Map<std::wstring, JSEntity *> &closure);

  void bind(common::AutoPtr<JSContext> ctx, JSEntity *self);

  const std::function<JSFunction> &getCallee() const;

  const common::Map<std::wstring, JSEntity *> &getClosure() const;

  const JSEntity *getBind(common::AutoPtr<JSContext> ctx) const;

  const std::wstring &getFunctionName() const;

  std::wstring toString(common::AutoPtr<JSContext> ctx) const override;

  bool toBoolean(common::AutoPtr<JSContext> ctx) const override;
};
}; // namespace spark::engine