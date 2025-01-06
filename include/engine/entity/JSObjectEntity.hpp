#pragma once
#include "engine/entity/JSEntity.hpp"
#include "engine/runtime/JSStore.hpp"
#include <string>
#include <unordered_map>
namespace spark::engine {

class JSObjectEntity : public JSEntity {
public:
  struct JSField {
    bool configurable;
    bool enumable;
    JSStore *value;
    bool writable;
    JSStore *get;
    JSStore *set;
  };

private:
  JSStore *_prototype;
  std::unordered_map<JSStore *, JSField> _symbolFields;
  std::unordered_map<std::wstring, JSField> _fields;
  std::unordered_map<std::wstring, JSField> _privateFields;
  bool _extensible;
  bool _sealed;
  bool _frozen;

public:
  JSObjectEntity(JSStore *prototype);

  const JSStore *getPrototype() const;

  JSStore *getPrototype();

  bool isExtensible() const;

  bool isSealed() const;

  bool isFrozen() const;

  void preventExtensions();

  void seal();

  void freeze();

  const std::unordered_map<JSStore *, JSField> &getSymbolProperties() const;

  const std::unordered_map<std::wstring, JSField> &getProperties() const;

  std::unordered_map<JSStore *, JSField> &getSymbolProperties();

  std::unordered_map<std::wstring, JSField> &getProperties();

  const std::unordered_map<std::wstring, JSField> &getPrivateProperties() const;

  std::unordered_map<std::wstring, JSField> &getPrivateProperties();

  std::wstring toString(common::AutoPtr<JSContext> ctx) const override;

  std::optional<double> toNumber(common::AutoPtr<JSContext> ctx) const override;

  bool toBoolean(common::AutoPtr<JSContext> ctx) const override;
};
}; // namespace spark::engine