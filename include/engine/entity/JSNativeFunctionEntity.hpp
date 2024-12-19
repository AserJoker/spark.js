#pragma once
#include "common/Map.hpp"
#include "engine/base/JSValueType.hpp"
#include "engine/entity/JSObjectEntity.hpp"
#include <functional>
#include <string>
namespace spark::engine {

class JSNativeFunctionEntity : public JSObjectEntity {
private:
  std::wstring _name;
  std::function<JSFunction> _callee;
  JSStore *_bind;
  common::Map<std::wstring, JSStore *> _closure;

public:
  JSNativeFunctionEntity(JSStore *funcProto, const std::wstring &name,
                         const std::function<JSFunction> &callee,
                         const common::Map<std::wstring, JSStore *> &closure);

  void bind(JSStore *self);

  const JSStore *getBind() const;

  JSStore *getBind();

  const std::function<JSFunction> &getCallee() const;

  const common::Map<std::wstring, JSStore *> &getClosure() const;

  common::Map<std::wstring, JSStore *> &getClosure();

  const std::wstring &getFunctionName() const;
};
}; // namespace spark::engine