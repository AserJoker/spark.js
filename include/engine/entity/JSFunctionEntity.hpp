#pragma once
#include "common/AutoPtr.hpp"
#include "common/Map.hpp"
#include "engine/base/JSValueType.hpp"
#include "engine/entity/JSEntity.hpp"
#include "engine/entity/JSObjectEntity.hpp"
#include <functional>
#include <string>
namespace spark::engine {

struct JSNativeFunction {};

class JSFunctionEntity : public JSObjectEntity {
private:
  std::wstring _name;
  std::function<JSFunction> _callee;
  JSEntity *_prototype;
  JSEntity *_bind;
  common::Map<std::wstring, JSEntity *> _closure;

public:
  JSFunctionEntity(JSEntity *funcProto, const std::wstring &name,
                   const std::function<JSFunction> &callee,
                   const common::Map<std::wstring, JSEntity *> &closure);

  void bind(JSEntity *self);

  const JSEntity *getBind(common::AutoPtr<JSContext> ctx) const;

  void setPrototype(JSEntity *prototype);

  const JSEntity *getPrototype() const;

  const std::function<JSFunction> &getCallee() const;

  const common::Map<std::wstring, JSEntity *> &getClosure() const;

  const std::wstring &getFunctionName() const;

  std::wstring toString(common::AutoPtr<JSContext> ctx) const override;

  bool toBoolean(common::AutoPtr<JSContext> ctx) const override;
};
}; // namespace spark::engine