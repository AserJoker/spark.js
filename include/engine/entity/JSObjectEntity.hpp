#pragma once
#include "engine/entity/JSEntity.hpp"
#include <string>
#include <unordered_map>
namespace spark::engine {

class JSObjectEntity : public JSEntity {
public:
  struct JSField {
    enum class TYPE { ACCESSOR, DATA } type;
    bool configurable;
    bool enumable;
    JSField(const TYPE &type)
        : type(type), configurable(true), enumable(true) {}
  };

  struct JSDataField : public JSField {
    JSEntity *value;
    bool writable;
    JSDataField(JSEntity *value)
        : JSField(JSField::TYPE::DATA), value(value), writable(true) {}
  };

  struct JSAccessorField : public JSField {
    JSEntity *get;
    JSEntity *set;
    JSAccessorField(JSEntity *get, JSEntity *set)
        : JSField(JSField::TYPE::ACCESSOR), get(get), set(set) {}
  };

private:
  JSEntity *_prototype;
  std::unordered_map<JSEntity *, JSEntity *> _symbolFields;
  std::unordered_map<std::wstring, JSEntity *> _fields;
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

  const std::unordered_map<JSEntity *, JSEntity *> &getSymbolFields() const;

  const std::unordered_map<std::wstring, JSEntity *> &getFields() const;

  std::unordered_map<JSEntity *, JSEntity *> &getSymbolFields();

  std::unordered_map<std::wstring, JSEntity *> &getFields();

  std::wstring toString(common::AutoPtr<JSContext> ctx) const override;

  std::optional<double> toNumber(common::AutoPtr<JSContext> ctx) const override;

  bool toBoolean(common::AutoPtr<JSContext> ctx) const override;

  std::wstring getTypeName(common::AutoPtr<JSContext> ctx) const override;
};
}; // namespace spark::engine