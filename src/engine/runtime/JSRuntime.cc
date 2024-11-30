#include "engine/runtime/JSRuntime.hpp"
#include "compiler/JSGenerator.hpp"
#include "compiler/JSParser.hpp"
#include "engine/runtime/JSScope.hpp"
#include "vm/JSVirtualMachine.hpp"

using namespace spark;
using namespace spark::engine;

JSRuntime::JSRuntime() {
  _sources = {{0, L"<spark:internal>"}};
  _root = new JSScope(nullptr);
  _parser = new compiler::JSParser();
  _generator = new compiler::JSGenerator();
  _vm = new vm::JSVirtualMachine();
};

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

common::AutoPtr<JSScope> JSRuntime::getRoot() { return _root; }

common::AutoPtr<compiler::JSParser> &JSRuntime::getParser() { return _parser; }
common::AutoPtr<compiler::JSGenerator> &JSRuntime::getGenerator() {
  return _generator;
}
common::AutoPtr<vm::JSVirtualMachine> &JSRuntime::getVirtualMachine() {
  return _vm;
}