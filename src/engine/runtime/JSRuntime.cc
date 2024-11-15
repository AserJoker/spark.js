#include "engine/runtime/JSRuntime.hpp"
#include "engine/entity/JSNaNEntity.hpp"
#include "engine/entity/JSNullEntity.hpp"
#include "engine/entity/JSUndefinedEntity.hpp"
#include "engine/runtime/JSScope.hpp"

using namespace spark;
using namespace spark::engine;
JSRuntime::JSRuntime() {
  _sources = {L"<spark:internal>"};
  _root = new JSScope(nullptr);
  _undefined = _root->createValue(new JSUndefinedEntity(), L"undefined");
  _null = _root->createValue(new JSNullEntity(), L"null");
  _NaN = _root->createValue(new JSNaNEntity(), L"NaN");
};

JSRuntime::~JSRuntime() { delete _root; };

const std::wstring &JSRuntime::getSourceFilename(uint32_t index) {
  return _sources.at(index);
}

uint32_t JSRuntime::setSourceFilename(const std::wstring &filename) {
  _sources.pushBack(filename);
  return _sources.size() - 1;
}

JSScope *JSRuntime::getRoot() { return _root; }

common::AutoPtr<JSValue> JSRuntime::NaN() { return _NaN; }
common::AutoPtr<JSValue> JSRuntime::Null() { return _null; }
common::AutoPtr<JSValue> JSRuntime::Undefined() { return _undefined; }
common::AutoPtr<JSValue> JSRuntime::Global() { return _global; }
common::AutoPtr<JSValue> JSRuntime::Object() { return _Object; }
common::AutoPtr<JSValue> JSRuntime::Function() { return _Function; }
common::AutoPtr<JSValue> JSRuntime::Array() { return _Array; }
common::AutoPtr<JSValue> JSRuntime::Number() { return _Number; }
common::AutoPtr<JSValue> JSRuntime::String() { return _String; }
common::AutoPtr<JSValue> JSRuntime::Boolean() { return _Boolean; }
common::AutoPtr<JSValue> JSRuntime::Symbol() { return _Symbol; }