#pragma once
#include "common/AutoPtr.hpp"
#include "engine/base/JSValueType.hpp"
#include <optional>
#include <string>
#include <type_traits>
#include <typeinfo>
#include <vector>

namespace spark::engine {
class JSContext;
class JSValue;
class JSEntity {
private:
  std::vector<JSEntity *> _parents;

  std::vector<JSEntity *> _children;

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

  void appendChild(JSEntity *entity);

  void removeChild(JSEntity *entity);

  const JSValueType &getType() const;

  std::vector<JSEntity *> &getParent();

  std::vector<JSEntity *> &getChildren();

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

  template <class T> const T &getOpaque() const {
    auto impl = dynamic_cast<OpaqueImpl<T>>(_opaque);
    if (!impl) {
      throw std::bad_cast();
    }
    return impl->_value;
  }

  virtual std::wstring toString(common::AutoPtr<JSContext> ctx) const;

  virtual std::optional<double> toNumber(common::AutoPtr<JSContext> ctx) const;

  virtual bool toBoolean(common::AutoPtr<JSContext> ctx) const;
};

} // namespace spark::engine