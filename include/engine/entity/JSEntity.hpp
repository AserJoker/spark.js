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

  JSEntity *_meta;

  JSValueType _type;

public:
  JSEntity(const JSValueType &type = JSValueType::JS_INTERNAL);

  virtual ~JSEntity();

  void appendChild(JSEntity *entity);

  void removeChild(JSEntity *entity);

  void setMetatable(JSEntity *meta);

  JSEntity *getMetatable();

  const JSValueType &getType() const;

  common::Array<JSEntity *> &getParent();

  common::Array<JSEntity *> &getChildren();

  virtual std::wstring toString(common::AutoPtr<JSContext> ctx) const;

  virtual std::optional<double> toNumber(common::AutoPtr<JSContext> ctx) const;

  virtual bool toBoolean(common::AutoPtr<JSContext> ctx) const;

  virtual std::wstring getTypeName(common::AutoPtr<JSContext> ctx) const;
};

template <class T> class JSBaseEntity : public JSEntity {

private:
  T _data;

public:
  JSBaseEntity(const JSValueType &type) : JSEntity(type){};

  JSBaseEntity(const JSValueType &type, T &&value)
      : JSEntity(type), _data(std::forward<T>(value)) {}

  JSBaseEntity(const JSValueType &type, const T &value)
      : JSEntity(type), _data(value) {}

protected:
  T &getData() { return _data; }

  const T &getData() const { return _data; }
};
} // namespace spark::engine