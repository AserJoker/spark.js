#include "engine/runtime/JSRuntime.hpp"
#include "compiler/JSGenerator.hpp"
#include "compiler/JSParser.hpp"
#include "vm/JSVirtualMachine.hpp"
#include <codecvt>
#include <cstdlib>
#include <filesystem>
#include <locale>
#include <string>
#include <vector>

using namespace spark;
using namespace spark::engine;

JSRuntime::JSRuntime(int argc, char **argv) {
  std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
  _sources = {{0, L"<spark:internal>"}};
  _parser = new compiler::JSParser();
  _generator = new compiler::JSGenerator();
  _vm = new vm::JSVirtualMachine();
  for (int i = 0; i < argc; i++) {
    _argv.push_back(converter.from_bytes(argv[i]));
  }
  _pathResolver = &JSRuntime::defaultPathResolver;
};

std::wstring JSRuntime::normalizePath(const std::wstring &path) {
  std::vector<std::wstring> parts;
  std::wstring current;
  for (auto &ch : path) {
    if (ch == '\\' || ch == '/') {
      parts.push_back(current);
      current.clear();
    } else {
      current += ch;
    }
  }
  parts.push_back(current);
  std::wstring result;
  for (auto it = parts.rbegin(); it != parts.rend(); it++) {
    if (it + 1 != parts.rend()) {
      if (*it == L".") {
        continue;
      }
      if (*it == L"..") {
        it++;
        continue;
      }
    }
    if (it == parts.rbegin()) {
      result = *it;
    } else {
      result = *it + L'/' + result;
    }
  }
  return result;
}

std::wstring JSRuntime::defaultPathResolver(const std::wstring &current,
                                            const std::wstring &source) {
  std::filesystem::path input = source;
  if (input.is_absolute()) {
    return normalizePath(source);
  }
  std::filesystem::path p = current;
  return normalizePath(p.parent_path().append(source).wstring());
}

JSRuntime::~JSRuntime(){};

const std::wstring &JSRuntime::getSourceFilename(uint32_t index) {
  return _sources.at(index);
}

uint32_t JSRuntime::setSourceFilename(const std::wstring &filename) {
  static uint32_t index = 1;
  for (auto &[k, v] : _sources) {
    if (v == filename) {
      return k;
    }
  }
  _sources[index] = filename;
  index++;
  return (uint32_t)(_sources.size() - 1);
}

std::wstring JSRuntime::getCurrentPath() {
  auto current = std::filesystem::current_path().wstring();
  return normalizePath(current);
}

void JSRuntime::setPathResolver(const PathResolveFunc &resolver) {
  _pathResolver = resolver;
}

const JSRuntime::PathResolveFunc &JSRuntime::getPathResolver() const {
  return _pathResolver;
}

common::AutoPtr<compiler::JSParser> &JSRuntime::getParser() { return _parser; }
common::AutoPtr<compiler::JSGenerator> &JSRuntime::getGenerator() {
  return _generator;
}
common::AutoPtr<vm::JSVirtualMachine> &JSRuntime::getVirtualMachine() {
  return _vm;
}