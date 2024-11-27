#pragma once
#include "compiler/base/JSModule.hpp"
#include "engine/entity/JSEntity.hpp"
#include "engine/entity/JSObjectEntity.hpp"
#include <string>
#include <unordered_map>
namespace spark::engine {
class JSFunctionEntity : public JSObjectEntity {
private:
  bool _async;
  uint32_t _address;
  uint32_t _length;
  std::unordered_map<std::wstring, JSEntity *> _closure;
  common::AutoPtr<compiler::JSModule> _module;
  std::wstring _name;

public:
  JSFunctionEntity(JSEntity *prototype,
                   const common::AutoPtr<compiler::JSModule> &module);
  void setAsync(bool async);
  void setAddress(uint32_t address);
  void setLength(uint32_t length);
  void setClosure(const std::wstring &name, JSEntity *entity);
  void setFuncName(const std::wstring &name);
  bool getAsync() const;
  uint32_t getAddress() const;
  uint32_t getLength() const;
  const std::wstring &getFuncName() const;
  const common::AutoPtr<compiler::JSModule> &getModule() const;
  const std::unordered_map<std::wstring, JSEntity *> &getClosure() const;
};
}; // namespace spark::engine