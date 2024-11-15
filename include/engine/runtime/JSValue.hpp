#pragma once
#include "common/AutoPtr.hpp"
#include "common/Object.hpp"
#include "engine/base/JSLocation.hpp"
#include "engine/base/JSUnaryOperatorType.hpp"
#include "engine/base/JSValueType.hpp"
#include "engine/entity/JSEntity.hpp"
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

  JSEntity *getEntity();

  const JSEntity *getEntity() const;

  void setEntity(JSEntity *entity);

  void setMetatable(const common::AutoPtr<JSValue> &meta);

  const common::AutoPtr<JSValue> getMetatable() const;

  std::optional<double> getNumber() const;

  std::optional<std::wstring> getString() const;

  std::optional<bool> getBoolean() const;

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

  common::AutoPtr<JSValue>
  apply(common::AutoPtr<JSContext> ctx, common::AutoPtr<JSValue> self,
        common::Array<common::AutoPtr<JSValue>> args = {},
        const JSLocation &location = {});

  std::wstring toStringValue(common::AutoPtr<JSContext> ctx);

  std::optional<double> toNumberValue(common::AutoPtr<JSContext> ctx);

  bool toBooleanValue(common::AutoPtr<JSContext> ctx);

  std::wstring getTypeName(common::AutoPtr<JSContext> ctx);

  common::AutoPtr<JSValue> toNumber(common::AutoPtr<JSContext> ctx);

  common::AutoPtr<JSValue> toString(common::AutoPtr<JSContext> ctx);

  common::AutoPtr<JSValue> toBoolean(common::AutoPtr<JSContext> ctx);

  common::AutoPtr<JSValue> unary(common::AutoPtr<JSContext> ctx,
                                 const JSUnaryOperatorType &opt);
};
}; // namespace spark::engine