#pragma once
#include "common/AutoPtr.hpp"
#include "common/Object.hpp"
#include "engine/base/JSValueType.hpp"
#include <any>
#include <optional>
#include <string>

namespace spark::engine {
class JSContext;
class JSValue;
class JSEntity : public common::Object {
private:
  struct Opaque {
    virtual ~Opaque() = default;
  };

  template <class T> struct OpaqueImpl : public Opaque {
    T _value;
  };

  std::any _opaque;

protected:
  JSValueType _type;

public:
  JSEntity(const JSValueType &type = JSValueType::JS_INTERNAL);

  virtual ~JSEntity();

  const JSValueType &getType() const;

  template <class T> void setOpaque(T &&value) { _opaque = value; }

  template <class T> T &getOpaque() { return std::any_cast<T &>(_opaque); }

  template <class T> const bool hasOpaque() const {
    return _opaque.type() != typeid(nullptr);
  }

  virtual std::wstring toString(common::AutoPtr<JSContext> ctx) const;

  virtual std::optional<double> toNumber(common::AutoPtr<JSContext> ctx) const;

  virtual bool toBoolean(common::AutoPtr<JSContext> ctx) const;
};

} // namespace spark::engine