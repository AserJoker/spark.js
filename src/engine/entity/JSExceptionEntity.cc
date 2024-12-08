#include "engine/entity/JSExceptionEntity.hpp"
#include "engine/base/JSValueType.hpp"
#include "engine/entity/JSEntity.hpp"
#include "engine/runtime/JSContext.hpp"
#include <fmt/xchar.h>
#include <string>
using namespace spark;
using namespace spark::engine;

JSExceptionEntity::JSExceptionEntity(const std::wstring &type,
                                     const std::wstring &message,
                                     const std::vector<JSLocation> &stack)
    : JSEntity(JSValueType::JS_EXCEPTION), _errorType(type), _message(message),
      _stack(stack), _target(nullptr) {}

JSExceptionEntity::JSExceptionEntity(JSStore *target)
    : JSEntity(JSValueType::JS_EXCEPTION), _target(target) {}

JSStore *JSExceptionEntity::getTarget() { return _target; }

const std::wstring &JSExceptionEntity::getMessage() const { return _message; }

const std::wstring &JSExceptionEntity::getExceptionType() const {
  return _errorType;
}

const std::vector<JSLocation> &JSExceptionEntity::getStack() const {
  return _stack;
}

std::wstring JSExceptionEntity::toString(common::AutoPtr<JSContext> ctx) const {
  if (_target) {
    return fmt::format(L"Uncaught {}", _target->getEntity()->toString(ctx));
  }
  std::wstring result =
      fmt::format(L"{}: {}", getExceptionType(), getMessage());
  for (auto &[fnindex, line, column, funcname] : getStack()) {
    auto &filename = ctx->getRuntime()->getSourceFilename(fnindex);
    if (fnindex != 0) {
      result +=
          fmt::format(L"\n at {}({}:{}:{})", funcname, filename, line, column);
    } else {
      result += fmt::format(L"\n at {} ({})", funcname, filename);
    }
  }
  return result;
}

bool JSExceptionEntity::toBoolean(common::AutoPtr<JSContext> ctx) const {
  return true;
}