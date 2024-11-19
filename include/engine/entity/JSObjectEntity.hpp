#pragma once
#include "engine/entity/JSEntity.hpp"
#include <string>
#include <unordered_map>
namespace spark::engine {

class JSObjectEntity : public JSEntity {
public:
  struct JSField {
    bool configurable;
    bool enumable;
    JSEntity *value;
    bool writable;
    JSEntity *get;
    JSEntity *set;
  };

private:
  JSEntity *_prototype;
  std::unordered_map<JSEntity *, JSField> _symbolFields;
  std::unordered_map<std::wstring, JSField> _fields;
  bool _extensible;
  bool _sealed;
  bool _frozen;

public:
  JSObjectEntity(JSEntity *prototype);

  JSEntity *getPrototype() const;

  bool isExtensible() const;

  bool isSealed() const;

  bool isFrozen() const;

  void preventExtensions();

  void seal();

  void freeze();

  const std::unordered_map<JSEntity *, JSField> &getSymbolProperties() const;

  const std::unordered_map<std::wstring, JSField> &getProperties() const;

  std::unordered_map<JSEntity *, JSField> &getSymbolProperties();

  std::unordered_map<std::wstring, JSField> &getProperties();

  std::wstring toString(common::AutoPtr<JSContext> ctx) const override;

  std::optional<double> toNumber(common::AutoPtr<JSContext> ctx) const override;

  bool toBoolean(common::AutoPtr<JSContext> ctx) const override;
};
}; // namespace spark::engine