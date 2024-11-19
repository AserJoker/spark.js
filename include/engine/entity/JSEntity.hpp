#pragma once
#include "common/Array.hpp"
#include "common/AutoPtr.hpp"
#include "engine/base/JSValueType.hpp"
#include <optional>
#include <string>

namespace spark::engine {
class JSContext;
class JSValue;
class JSEntity {
private:
  common::Array<JSEntity *> _parents;

  common::Array<JSEntity *> _children;

  JSValueType _type;

public:
  JSEntity(const JSValueType &type = JSValueType::JS_INTERNAL);

  virtual ~JSEntity();

  void appendChild(JSEntity *entity);

  void removeChild(JSEntity *entity);

  const JSValueType &getType() const;

  common::Array<JSEntity *> &getParent();

  common::Array<JSEntity *> &getChildren();

  virtual std::wstring toString(common::AutoPtr<JSContext> ctx) const;

  virtual std::optional<double> toNumber(common::AutoPtr<JSContext> ctx) const;

  virtual bool toBoolean(common::AutoPtr<JSContext> ctx) const;
};

} // namespace spark::engine