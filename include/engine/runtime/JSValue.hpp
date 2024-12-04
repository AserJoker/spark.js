#pragma once
#include "common/AutoPtr.hpp"
#include "common/BigInt.hpp"
#include "common/Object.hpp"
#include "engine/base/JSLocation.hpp"
#include "engine/base/JSValueType.hpp"
#include "engine/entity/JSEntity.hpp"
#include "engine/entity/JSObjectEntity.hpp"
#include <optional>
#include <string>

namespace spark::engine {
class JSContext;
class JSScope;

class JSValue : public common::Object {
private:
  JSEntity *_entity;

  JSScope *_scope;

public:
  JSValue(JSScope *scope, JSEntity *entity);

  ~JSValue() override;

  const JSValueType &getType() const;

  std::wstring getName() const;

  template <class T = JSEntity> T *getEntity() { return (T *)_entity; }

  template <class T = JSEntity> const T *getEntity() const {
    return (T *)_entity;
  }

  common::AutoPtr<JSScope> getScope();

  void setEntity(JSEntity *entity);

  std::optional<double> getNumber() const;

  std::optional<std::wstring> getString() const;

  std::optional<bool> getBoolean() const;

  std::optional<common::BigInt<>> getBigInt() const;

  template <class T> T getOpaque() {
    return std::any_cast<T>(_entity->getOpaque());
  }

  template <class T> T getOpaque() const {
    return std::any_cast<T>(_entity->getOpaque());
  }

  template <class T> void setOpaque(T &&value) const {
    _entity->setOpaque(std::forward<T>(value));
  }

  bool isUndefined() const;

  bool isNull() const;

  bool isInfinity() const;

  bool isNaN() const;

  void setNumber(double value);

  void setString(const std::wstring &value);

  void setBoolean(bool value);

  void setUndefined();

  void setNull();

  void setInfinity();

  void setNaN();

  std::wstring getTypeName();

  common::AutoPtr<JSValue>
  apply(common::AutoPtr<JSContext> ctx, common::AutoPtr<JSValue> self,
        std::vector<common::AutoPtr<JSValue>> args = {},
        const JSLocation &location = {});

  std::wstring convertToString(common::AutoPtr<JSContext> ctx);

  common::AutoPtr<JSValue> toPrimitive(common::AutoPtr<JSContext> ctx,
                                       const std::wstring &hint = L"default");

  common::AutoPtr<JSValue> pack(common::AutoPtr<JSContext> ctx);

  std::optional<double> convertToNumber(common::AutoPtr<JSContext> ctx);

  bool convertToBoolean(common::AutoPtr<JSContext> ctx);

  common::AutoPtr<JSValue> getPrototype(common::AutoPtr<JSContext> ctx);

  JSObjectEntity::JSField *
  getOwnPropertyDescriptor(common::AutoPtr<JSContext> ctx,
                           const std::wstring &name);

  JSObjectEntity::JSField *getPropertyDescriptor(common::AutoPtr<JSContext> ctx,
                                                 const std::wstring &name);

  common::AutoPtr<JSValue>
  setPropertyDescriptor(common::AutoPtr<JSContext> ctx,
                        const std::wstring &name,
                        const JSObjectEntity::JSField &descriptor);

  common::AutoPtr<JSValue> getProperty(common::AutoPtr<JSContext> ctx,
                                       const std::wstring &name);

  common::AutoPtr<JSValue> setProperty(common::AutoPtr<JSContext> ctx,
                                       const std::wstring &name,
                                       const common::AutoPtr<JSValue> &field);

  common::AutoPtr<JSValue> getIndex(common::AutoPtr<JSContext> ctx,
                                    const uint32_t &name);

  common::AutoPtr<JSValue> setIndex(common::AutoPtr<JSContext> ctx,
                                    const uint32_t &name,
                                    const common::AutoPtr<JSValue> &field);

  common::AutoPtr<JSValue> removeProperty(common::AutoPtr<JSContext> ctx,
                                          const std::wstring &name);

  JSObjectEntity::JSField *
  getOwnPropertyDescriptor(common::AutoPtr<JSContext> ctx,
                           common::AutoPtr<JSValue> name);

  JSObjectEntity::JSField *getPropertyDescriptor(common::AutoPtr<JSContext> ctx,
                                                 common::AutoPtr<JSValue> name);

  common::AutoPtr<JSValue>
  setPropertyDescriptor(common::AutoPtr<JSContext> ctx,
                        common::AutoPtr<JSValue> name,
                        const JSObjectEntity::JSField &descriptor);

  common::AutoPtr<JSValue> getProperty(common::AutoPtr<JSContext> ctx,
                                       common::AutoPtr<JSValue> name);

  common::AutoPtr<JSValue> setProperty(common::AutoPtr<JSContext> ctx,
                                       common::AutoPtr<JSValue> name,
                                       const common::AutoPtr<JSValue> &field);

  common::AutoPtr<JSValue> removeProperty(common::AutoPtr<JSContext> ctx,
                                          common::AutoPtr<JSValue> name);

  common::AutoPtr<JSValue> unaryPlus(common::AutoPtr<JSContext> ctx); // +a

  common::AutoPtr<JSValue> unaryNetation(common::AutoPtr<JSContext> ctx); // -a

  common::AutoPtr<JSValue> increment(common::AutoPtr<JSContext> ctx); // ++a

  common::AutoPtr<JSValue> decrement(common::AutoPtr<JSContext> ctx); // --a

  common::AutoPtr<JSValue> logicalNot(common::AutoPtr<JSContext> ctx); // !a

  common::AutoPtr<JSValue> bitwiseNot(common::AutoPtr<JSContext> ctx); // ~a

  common::AutoPtr<JSValue> add(common::AutoPtr<JSContext> ctx,
                               common::AutoPtr<JSValue> another); // a+b

  common::AutoPtr<JSValue> sub(common::AutoPtr<JSContext> ctx,
                               common::AutoPtr<JSValue> another); // a-b

  common::AutoPtr<JSValue> mul(common::AutoPtr<JSContext> ctx,
                               common::AutoPtr<JSValue> another); // a*b

  common::AutoPtr<JSValue> div(common::AutoPtr<JSContext> ctx,
                               common::AutoPtr<JSValue> another); // a/b

  common::AutoPtr<JSValue> mod(common::AutoPtr<JSContext> ctx,
                               common::AutoPtr<JSValue> another); // a%b

  common::AutoPtr<JSValue> equal(common::AutoPtr<JSContext> ctx,
                                 common::AutoPtr<JSValue> another); // a==b

  common::AutoPtr<JSValue> notEqual(common::AutoPtr<JSContext> ctx,
                                    common::AutoPtr<JSValue> another); // a!=b

  common::AutoPtr<JSValue>
  strictEqual(common::AutoPtr<JSContext> ctx,
              common::AutoPtr<JSValue> another); // a===b

  common::AutoPtr<JSValue>
  notStrictEqual(common::AutoPtr<JSContext> ctx,
                 common::AutoPtr<JSValue> another); // a!==b
};
}; // namespace spark::engine