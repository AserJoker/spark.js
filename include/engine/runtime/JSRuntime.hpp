#pragma once
#include "common/AutoPtr.hpp"
#include "common/Object.hpp"
#include "compiler/JSGenerator.hpp"
#include "compiler/JSParser.hpp"
#include "engine/runtime/JSScope.hpp"
#include "engine/runtime/JSValue.hpp"
#include "vm/JSVirtualMachine.hpp"
#include <string>
#include <unordered_map>

namespace spark::engine {
class JSRuntime : public common::Object {
private:
  JSScope *_root;

  std::unordered_map<uint32_t, std::wstring> _sources;

  common::AutoPtr<compiler::JSParser> _parser;

  common::AutoPtr<compiler::JSGenerator> _generator;

  common::AutoPtr<vm::JSVirtualMachine> _vm;

public:
  JSRuntime();

  ~JSRuntime() override;

  const std::wstring &getSourceFilename(uint32_t index);

  uint32_t setSourceFilename(const std::wstring &filename);

  JSScope *getRoot();

  common::AutoPtr<compiler::JSParser> &getParser();
  common::AutoPtr<compiler::JSGenerator> &getGenerator();
  common::AutoPtr<vm::JSVirtualMachine> &getVirtualMachine();
};
} // namespace spark::engine