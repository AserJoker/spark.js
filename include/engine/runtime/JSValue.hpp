#pragma once
#include "JSStore.hpp"
#include "common/AutoPtr.hpp"
#include "common/BigInt.hpp"
#include "common/Object.hpp"
#include "engine/base/JSLocation.hpp"
#include "engine/base/JSValueType.hpp"
#include "engine/entity/JSEntity.hpp"
#include "engine/entity/JSObjectEntity.hpp"
#include <optional>
#include <string>
#include <vector>

namespace spark::engine {
class JSContext;
class JSScope;

class JSValue : public common::Object {
private:
  JSStore *_store;

  JSScope *_scope;

public:
  JSValue(JSScope *scope, JSStore *store);

  ~JSValue() override;

  const JSValueType &getType() const;

  std::wstring getName() const;

  template <class T = JSEntity> common::AutoPtr<T> getEntity() {
    return _store->getEntity().cast<T>();
  }

  template <class T = JSEntity> const common::AutoPtr<T> getEntity() const {
    return _store->getEntity().cast<T>();
  }

  JSStore *getStore();

  const JSStore *getStore() const;

  common::AutoPtr<JSScope> getScope();

  void setEntity(const common::AutoPtr<JSEntity> &entity);

  void setStore(JSStore *store);

  std::optional<double> getNumber() const;

  std::optional<std::wstring> getString() const;

  std::optional<bool> getBoolean() const;

  std::optional<common::BigInt<>> getBigInt() const;

  template <class T> T &getOpaque() { return getEntity()->getOpaque<T>(); }

  template <class T> bool hasOpaque() { return getEntity()->hasOpaque<T>(); }

  template <class T> const T &getOpaque() const {
    return getEntity()->getOpaque<T>();
  }

  template <class T> void setOpaque(T &&value) {
    getEntity()->setOpaque(std::forward<T>(value));
  }

  bool isUndefined() const;

  bool isNull() const;

  bool isInfinity() const;

  bool isNaN() const;

  bool isFunction() const;

  bool isException() const;

  void setNumber(double value);

  void setString(const std::wstring &value);

  void setBoolean(bool value);

  std::wstring getTypeName();

  common::AutoPtr<JSValue>
  apply(common::AutoPtr<JSContext> ctx, common::AutoPtr<JSValue> self,
        std::vector<common::AutoPtr<JSValue>> args = {},
        const JSLocation &location = {});

  common::AutoPtr<JSValue> toPrimitive(common::AutoPtr<JSContext> ctx,
                                       const std::wstring &hint = L"default");

  common::AutoPtr<JSValue> pack(common::AutoPtr<JSContext> ctx);

  common::AutoPtr<JSValue> toNumber(common::AutoPtr<JSContext> ctx);

  common::AutoPtr<JSValue> toBoolean(common::AutoPtr<JSContext> ctx);

  common::AutoPtr<JSValue> toString(common::AutoPtr<JSContext> ctx);

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

  common::AutoPtr<JSValue> setPropertyDescriptor(
      common::AutoPtr<JSContext> ctx, const std::wstring &name,
      const common::AutoPtr<JSValue> &value, bool configurable = true,
      bool enumable = false, bool writable = true);

  common::AutoPtr<JSValue> setPropertyDescriptor(
      common::AutoPtr<JSContext> ctx, const std::wstring &name,
      const common::AutoPtr<JSValue> &get, const common::AutoPtr<JSValue> &set,
      bool configurable = true, bool enumable = false);

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

  common::AutoPtr<JSValue> getKeys(common::AutoPtr<JSContext> ctx);

  common::AutoPtr<JSValue> getBind(common::AutoPtr<JSContext> ctx);

  void setBind(common::AutoPtr<JSContext> ctx, common::AutoPtr<JSValue> bind);

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

  common::AutoPtr<JSValue> setPropertyDescriptor(
      common::AutoPtr<JSContext> ctx, common::AutoPtr<JSValue> name,
      const common::AutoPtr<JSValue> &value, bool configurable = true,
      bool enumable = false, bool writable = true);

  common::AutoPtr<JSValue> setPropertyDescriptor(
      common::AutoPtr<JSContext> ctx, common::AutoPtr<JSValue> name,
      const common::AutoPtr<JSValue> &get, const common::AutoPtr<JSValue> &set,
      bool configurable = true, bool enumable = false);

  common::AutoPtr<JSValue> getProperty(common::AutoPtr<JSContext> ctx,
                                       common::AutoPtr<JSValue> name);

  common::AutoPtr<JSValue> setProperty(common::AutoPtr<JSContext> ctx,
                                       common::AutoPtr<JSValue> name,
                                       const common::AutoPtr<JSValue> &field);

  common::AutoPtr<JSValue> removeProperty(common::AutoPtr<JSContext> ctx,
                                          common::AutoPtr<JSValue> name);

  common::AutoPtr<JSValue> unaryPlus(common::AutoPtr<JSContext> ctx); // +a

  common::AutoPtr<JSValue> unaryNetation(common::AutoPtr<JSContext> ctx); // -a

  void increment(common::AutoPtr<JSContext> ctx); // ++a

  void decrement(common::AutoPtr<JSContext> ctx); // --a

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

  common::AutoPtr<JSValue> pow(common::AutoPtr<JSContext> ctx,
                               common::AutoPtr<JSValue> another); // a**b

  common::AutoPtr<JSValue> shl(common::AutoPtr<JSContext> ctx,
                               common::AutoPtr<JSValue> another); // a<<b

  common::AutoPtr<JSValue> shr(common::AutoPtr<JSContext> ctx,
                               common::AutoPtr<JSValue> another); // a>>b

  common::AutoPtr<JSValue> ushr(common::AutoPtr<JSContext> ctx,
                                common::AutoPtr<JSValue> another); // a>>>b

  common::AutoPtr<JSValue> ge(common::AutoPtr<JSContext> ctx,
                              common::AutoPtr<JSValue> another); // a>=b

  common::AutoPtr<JSValue> le(common::AutoPtr<JSContext> ctx,
                              common::AutoPtr<JSValue> another); // a<=b

  common::AutoPtr<JSValue> gt(common::AutoPtr<JSContext> ctx,
                              common::AutoPtr<JSValue> another); // a>b

  common::AutoPtr<JSValue> lt(common::AutoPtr<JSContext> ctx,
                              common::AutoPtr<JSValue> another); // a<b

  common::AutoPtr<JSValue> and_(common::AutoPtr<JSContext> ctx,
                                common::AutoPtr<JSValue> another); // a&b

  common::AutoPtr<JSValue> or_(common::AutoPtr<JSContext> ctx,
                               common::AutoPtr<JSValue> another); // a|b

  common::AutoPtr<JSValue> xor_(common::AutoPtr<JSContext> ctx,
                                common::AutoPtr<JSValue> another); // a^b

  common::AutoPtr<JSValue> equal(common::AutoPtr<JSContext> ctx,
                                 common::AutoPtr<JSValue> another); // a==b

  common::AutoPtr<JSValue> notEqual(common::AutoPtr<JSContext> ctx,
                                    common::AutoPtr<JSValue> another); // a!=b

  common::AutoPtr<JSValue>
  strictEqual(common::AutoPtr<JSContext> ctx,
              common::AutoPtr<JSValue> another); // a===b

  common::AutoPtr<JSValue>
  strictNotEqual(common::AutoPtr<JSContext> ctx,
                 common::AutoPtr<JSValue> another); // a!==b
};
}; // namespace spark::engine