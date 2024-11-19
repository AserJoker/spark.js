#pragma once
#include "common/AutoPtr.hpp"
#include "common/Object.hpp"
#include "engine/runtime/JSScope.hpp"
#include "engine/runtime/JSValue.hpp"
#include <string>
#include <vector>

namespace spark::engine {
class JSRuntime : public common::Object {
private:
  static JS_FUNCTION(JSObjectConstructor);

  static JS_FUNCTION(JSFunctionConstructor);

  static JS_FUNCTION(JSArrayConstructor);

  static JS_FUNCTION(JSErrorConstructor);

  static JS_FUNCTION(JSSymbolConstructor);

  static JS_FUNCTION(JSNumberConstructor);

  static JS_FUNCTION(JSStringConstructor);

  static JS_FUNCTION(JSBooleanConstructor);

  static JS_FUNCTION(JSBigIntConstructor);

private:
  JSScope *_root;

  common::AutoPtr<JSValue> _NaN;
  common::AutoPtr<JSValue> _null;
  common::AutoPtr<JSValue> _undefined;
  common::AutoPtr<JSValue> _global;

  common::AutoPtr<JSValue> _Object;
  common::AutoPtr<JSValue> _Function;
  common::AutoPtr<JSValue> _Array;
  common::AutoPtr<JSValue> _Error;
  common::AutoPtr<JSValue> _Symbol;
  common::AutoPtr<JSValue> _Number;
  common::AutoPtr<JSValue> _String;
  common::AutoPtr<JSValue> _Boolean;
  common::AutoPtr<JSValue> _BigInt;

  std::vector<std::wstring> _sources;

public:
  JSRuntime();
  ~JSRuntime() override;

  const std::wstring &getSourceFilename(uint32_t index);
  uint32_t setSourceFilename(const std::wstring &filename);

  JSScope *getRoot();

  common::AutoPtr<JSValue> NaN();
  common::AutoPtr<JSValue> Null();
  common::AutoPtr<JSValue> Undefined();
  common::AutoPtr<JSValue> Global();
  common::AutoPtr<JSValue> Object();
  common::AutoPtr<JSValue> Function();
  common::AutoPtr<JSValue> Array();
  common::AutoPtr<JSValue> Number();
  common::AutoPtr<JSValue> String();
  common::AutoPtr<JSValue> Boolean();
  common::AutoPtr<JSValue> Symbol();
  common::AutoPtr<JSValue> Error();
  common::AutoPtr<JSValue> BigInt();
};
} // namespace spark::engine