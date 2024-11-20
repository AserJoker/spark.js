#include "engine/runtime/JSRuntime.hpp"
#include "engine/runtime/JSScope.hpp"

using namespace spark;
using namespace spark::engine;

JSRuntime::JSRuntime() {
  _sources = {L"<spark:internal>"};
  _root = new JSScope(nullptr);
};

JSRuntime::~JSRuntime() {
  delete _root;
};

const std::wstring &JSRuntime::getSourceFilename(uint32_t index) {
  return _sources.at(index);
}

uint32_t JSRuntime::setSourceFilename(const std::wstring &filename) {
  _sources.push_back(filename);
  return (uint32_t)(_sources.size() - 1);
}

JSScope *JSRuntime::getRoot() { return _root; }
