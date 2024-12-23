#pragma once
#include "common/AutoPtr.hpp"
#include "common/Object.hpp"
#include "compiler/JSGenerator.hpp"
#include "compiler/JSParser.hpp"
#include "vm/JSVirtualMachine.hpp"
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace spark::engine {
class JSRuntime : public common::Object {
public:
  static std::wstring defaultPathResolver(const std::wstring &current,
                                          const std::wstring &next);

  using PathResolveFunc = std::function<std::wstring(
      const std::wstring &current, const std::wstring &next)>;

private:
  std::unordered_map<uint32_t, std::wstring> _sources;

  common::AutoPtr<compiler::JSParser> _parser;

  common::AutoPtr<compiler::JSGenerator> _generator;

  common::AutoPtr<vm::JSVirtualMachine> _vm;

  std::vector<std::wstring> _argv;

  PathResolveFunc _pathResolver;

private:
  static std::wstring normalizePath(const std::wstring &path);

public:
  JSRuntime(int argc, char **argv);

  ~JSRuntime() override;

  const std::wstring &getSourceFilename(uint32_t index);

  uint32_t setSourceFilename(const std::wstring &filename);

  std::wstring getCurrentPath();

  void setPathResolver(const PathResolveFunc &resolver);

  const PathResolveFunc &getPathResolver() const;

  common::AutoPtr<compiler::JSParser> &getParser();

  common::AutoPtr<compiler::JSGenerator> &getGenerator();

  common::AutoPtr<vm::JSVirtualMachine> &getVirtualMachine();
};
} // namespace spark::engine