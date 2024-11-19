#include "engine/runtime/JSRuntime.hpp"
#include "engine/entity/JSEntity.hpp"
#include "engine/entity/JSFunctionEntity.hpp"
#include "engine/entity/JSNaNEntity.hpp"
#include "engine/entity/JSNullEntity.hpp"
#include "engine/entity/JSObjectEntity.hpp"
#include "engine/entity/JSUndefinedEntity.hpp"
#include "engine/runtime/JSScope.hpp"
#include "engine/runtime/JSContext.hpp"

using namespace spark;
using namespace spark::engine;

JS_FUNCTION(JSRuntime::JSObjectConstructor){
  return ctx->Undefined();
}
JS_FUNCTION(JSRuntime::JSFunctionConstructor){
  return ctx->Undefined();
}
JS_FUNCTION(JSRuntime::JSArrayConstructor){
  return ctx->Undefined();
}
JS_FUNCTION(JSRuntime::JSErrorConstructor){
  return ctx->Undefined();
}
JS_FUNCTION(JSRuntime::JSSymbolConstructor){
  return ctx->Undefined();
}
JS_FUNCTION(JSRuntime::JSNumberConstructor){
  return ctx->Undefined();
}
JS_FUNCTION(JSRuntime::JSStringConstructor){
  return ctx->Undefined();
}
JS_FUNCTION(JSRuntime::JSBooleanConstructor){
  return ctx->Undefined();
}
JS_FUNCTION(JSRuntime::JSBigIntConstructor){
  return ctx->Undefined();
}

JSRuntime::JSRuntime() {
  _sources = {L"<spark:internal>"};
  _root = new JSScope(nullptr);
  _undefined = _root->createValue(new JSUndefinedEntity(), L"undefined");
  _null = _root->createValue(new JSNullEntity(), L"null");
  _NaN = _root->createValue(new JSNaNEntity(), L"NaN");
  _global = _root->createValue(_root->getRoot(), L"globalThis");

  auto objectPrototype = new JSObjectEntity(_null->getEntity());
  auto functionPrototype = new JSObjectEntity(objectPrototype);
  auto arrayPrototype = new JSObjectEntity(objectPrototype);
  auto errorPrototype = new JSObjectEntity(objectPrototype);
  auto symbolPrototype = new JSObjectEntity(objectPrototype);
  auto numberPrototype = new JSObjectEntity(objectPrototype);
  auto stringPrototype = new JSObjectEntity(objectPrototype);
  auto booleanPrototype = new JSObjectEntity(objectPrototype);
  auto bigintPrototype = new JSObjectEntity(objectPrototype);

  JSFunctionEntity *ObjectConstructorEntity = new JSFunctionEntity(
      functionPrototype, L"ObjectConstructor", &JSObjectConstructor, {});
  ObjectConstructorEntity->setPrototype(objectPrototype);
  _Object = _root->createValue(ObjectConstructorEntity, L"Object");

  JSFunctionEntity *FunctionConstructorEntity = new JSFunctionEntity(
      functionPrototype, L"FunctionConstructor", &JSFunctionConstructor, {});
  FunctionConstructorEntity->setPrototype(functionPrototype);
  _Function = _root->createValue(FunctionConstructorEntity, L"Function");

  JSFunctionEntity *ArrayConstructorEntity = new JSFunctionEntity(
      functionPrototype, L"ArrayConstructor", &JSArrayConstructor, {});
  ArrayConstructorEntity->setPrototype(arrayPrototype);
  _Array = _root->createValue(ArrayConstructorEntity, L"Array");

  JSFunctionEntity *ErrorConstructorEntity = new JSFunctionEntity(
      functionPrototype, L"ErrorConstructor", &JSErrorConstructor, {});
  ErrorConstructorEntity->setPrototype(errorPrototype);
  _Error = _root->createValue(ErrorConstructorEntity, L"Error");

  JSFunctionEntity *SymbolConstructorEntity = new JSFunctionEntity(
      functionPrototype, L"SymbolConstructor", &JSSymbolConstructor, {});
  SymbolConstructorEntity->setPrototype(symbolPrototype);
  _Symbol = _root->createValue(SymbolConstructorEntity, L"Symbol");

  JSFunctionEntity *NumberConstructorEntity = new JSFunctionEntity(
      functionPrototype, L"NumberConstructor", &JSNumberConstructor, {});
  NumberConstructorEntity->setPrototype(numberPrototype);
  _Number = _root->createValue(NumberConstructorEntity, L"Number");

  JSFunctionEntity *StringConstructorEntity = new JSFunctionEntity(
      functionPrototype, L"StringConstructor", &JSStringConstructor, {});
  StringConstructorEntity->setPrototype(stringPrototype);
  _String = _root->createValue(StringConstructorEntity, L"String");

  JSFunctionEntity *BooleanConstructorEntity = new JSFunctionEntity(
      functionPrototype, L"BooleanConstructor", &JSBooleanConstructor, {});
  BooleanConstructorEntity->setPrototype(booleanPrototype);
  _Boolean = _root->createValue(BooleanConstructorEntity, L"Boolean");

  JSFunctionEntity *BigIntConstructorEntity = new JSFunctionEntity(
      functionPrototype, L"BigIntConstructor", &JSBigIntConstructor, {});
  BigIntConstructorEntity->setPrototype(bigintPrototype);
  _BigInt = _root->createValue(BigIntConstructorEntity, L"BigInt");
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
common::AutoPtr<JSValue> JSRuntime::Error() { return _Error; }
common::AutoPtr<JSValue> JSRuntime::BigInt() { return _BigInt; }