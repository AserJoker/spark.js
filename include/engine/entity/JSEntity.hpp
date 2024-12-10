#pragma once
#include "common/AutoPtr.hpp"
#include "common/Object.hpp"
#include "engine/base/JSValueType.hpp"
#include <optional>
#include <string>
#include <type_traits>
#include <typeinfo>

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

  Opaque *_opaque;

protected:
  JSValueType _type;

public:
  JSEntity(const JSValueType &type = JSValueType::JS_INTERNAL);

  virtual ~JSEntity();

  const JSValueType &getType() const;

  template <class T> void setOpaque(T &&value) {
    if (_opaque) {
      delete _opaque;
    }
    auto opaque =
        new OpaqueImpl<std::remove_cv_t<std::remove_reference_t<T>>>();
    opaque->_value = value;
    _opaque = opaque;
  }
  template <class T> T &getOpaque() {
    auto impl = dynamic_cast<OpaqueImpl<T> *>(_opaque);
    if (!impl) {
      throw std::bad_cast();
    }
    return impl->_value;
  }

  template <class T> const bool hasOpaque() const {
    auto impl = dynamic_cast<OpaqueImpl<T>*>((Opaque *)_opaque);
    if (!impl) {
      return false;
    }
    return true;
  }

  virtual std::wstring toString(common::AutoPtr<JSContext> ctx) const;

  virtual std::optional<double> toNumber(common::AutoPtr<JSContext> ctx) const;

  virtual bool toBoolean(common::AutoPtr<JSContext> ctx) const;
};

} // namespace spark::engine