#pragma once
#include "compiler/base/JSModule.hpp"
#include "engine/entity/JSObjectEntity.hpp"
#include "engine/runtime/JSStore.hpp"
#include <string>
#include <unordered_map>
namespace spark::engine {
class JSFunctionEntity : public JSObjectEntity {
private:
  bool _async;
  bool _generator;
  uint32_t _address;
  uint32_t _length;
  std::unordered_map<std::wstring, JSStore *> _closure;
  common::AutoPtr<compiler::JSModule> _module;
  std::wstring _name;
  std::wstring _source;
  JSStore *_bind;

public:
  JSFunctionEntity(JSStore *prototype,
                   const common::AutoPtr<compiler::JSModule> &module);
  void setAsync(bool async);
  void setGenerator(bool generator);
  void setAddress(uint32_t address);
  void setLength(uint32_t length);
  void setClosure(const std::wstring &name, JSStore *entity);
  void setFuncName(const std::wstring &name);
  void setSource(const std::wstring &name);

  void bind(JSStore *self);
  const JSStore *getBind() const;
  JSStore *getBind();

  bool isAsync() const;
  bool isGenerator() const;
  uint32_t getAddress() const;
  uint32_t getLength() const;
  const std::wstring &getFuncName() const;
  const std::wstring &getFunctionSource() const;

  const common::AutoPtr<compiler::JSModule> &getModule() const;
  const std::unordered_map<std::wstring, JSStore *> &getClosure() const;
};
}; // namespace spark::engine