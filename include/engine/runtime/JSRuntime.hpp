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
class JSContext;
class JSRuntime : public common::Object {
public:
  static std::wstring defaultPathResolver(const std::wstring &current,
                                          const std::wstring &next);

  using PathResolveFunc = std::function<std::wstring(
      const std::wstring &current, const std::wstring &next)>;

  using JSHook = std::function<void(common::AutoPtr<JSContext> ctx)>;

private:
  common::AutoPtr<compiler::JSParser> _parser;

  common::AutoPtr<compiler::JSGenerator> _generator;

  common::AutoPtr<vm::JSVirtualMachine> _vm;

  std::vector<std::wstring> _argv;

  PathResolveFunc _pathResolver;

  std::unordered_map<std::wstring, std::pair<JSHook, JSHook>> _directives;

  std::unordered_map<std::wstring, std::wstring> _importAttributes;

private:
  static std::wstring normalizePath(const std::wstring &path);

public:
  JSRuntime(int argc, char **argv);

  ~JSRuntime() override;

  std::wstring getCurrentPath();

  void setPathResolver(const PathResolveFunc &resolver);

  const PathResolveFunc &getPathResolver() const;

  common::AutoPtr<compiler::JSParser> &getParser();

  common::AutoPtr<compiler::JSGenerator> &getGenerator();

  common::AutoPtr<vm::JSVirtualMachine> &getVirtualMachine();

  void setDirectiveCallback(const std::wstring &name, const JSHook &setup,
                            const JSHook &cleanup);

  const std::unordered_map<std::wstring, std::pair<JSHook, JSHook>> &
  getDirectives() const;

  void setImportAttribute(const std::wstring &key, const std::wstring &value);

  void clearImportAttribute();

  const std::unordered_map<std::wstring, std::wstring> &
  getImportAttributes() const;
};
} // namespace spark::engine