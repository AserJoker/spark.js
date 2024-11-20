#pragma once
#include "common/AutoPtr.hpp"
#include "engine/base/JSValueType.hpp"
#include <any>
#include <optional>
#include <string>
#include <vector>

namespace spark::engine {
class JSContext;
class JSValue;
class JSEntity {
private:
  std::vector<JSEntity *> _parents;

  std::vector<JSEntity *> _children;

  std::any _opaque;

protected:
  JSValueType _type;

public:
  JSEntity(const JSValueType &type = JSValueType::JS_INTERNAL);

  virtual ~JSEntity();

  void appendChild(JSEntity *entity);

  void removeChild(JSEntity *entity);

  const JSValueType &getType() const;

  std::vector<JSEntity *> &getParent();

  std::vector<JSEntity *> &getChildren();

  std::any &getOpaque();
  const std::any &getOpaque() const;
  void setOpaque(const std::any &value);

  virtual std::wstring toString(common::AutoPtr<JSContext> ctx) const;

  virtual std::optional<double> toNumber(common::AutoPtr<JSContext> ctx) const;

  virtual bool toBoolean(common::AutoPtr<JSContext> ctx) const;
};

} // namespace spark::engine