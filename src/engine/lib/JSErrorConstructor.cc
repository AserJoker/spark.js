#include "engine/lib/JSErrorConstructor.hpp"
#include "engine/base/JSValueType.hpp"
#include <fmt/xchar.h>
#include <string>
using namespace spark;
using namespace spark::engine;
JS_FUNC(JSErrorConstructor::constructor) {
  if (self->getType() != JSValueType::JS_OBJECT) {
    auto prop = ctx->Error()->getProperty(ctx, L"prototype");
    self = ctx->createObject(prop);
  }
  if (args.empty()) {
    self->setProperty(ctx, L"message", ctx->createString(L""));
  } else {
    self->setProperty(
        ctx, L"message",
        ctx->createString(args[0]->toString(ctx)->getString().value()));
  }
  auto constructor = self->getProperty(ctx, L"constructor");
  std::wstring type =
      constructor->getProperty(ctx, L"name")->getString().value();
  auto trace =
      ctx->trace({.filename = 0, .line = 0, .column = 0, .funcname = type});
  std::wstring stack;
  for (auto it = trace.begin() + 1; it != trace.end(); it++) {
    auto &loc = *it;
    if (!loc.filename.empty()) {
      stack += fmt::format(L"  at {} ({}:{}:{})", loc.funcname, loc.filename,
                           loc.line, loc.column);
    } else {
      stack += fmt::format(L"  at {} ({})", loc.funcname, loc.filename);
    }
    if (it != trace.end() - 1) {
      stack += L"\n";
    }
  }
  self->setProperty(ctx, L"stack", ctx->createString(stack));
  return ctx->undefined();
}

JS_FUNC(JSErrorConstructor::toString) {
  auto message = self->getProperty(ctx, L"message");
  std::wstring result;
  auto constructor = self->getProperty(ctx, L"constructor");
  std::wstring type =
      constructor->getProperty(ctx, L"name")->getString().value();
  auto msg = message->toString(ctx)->getString().value();
  if (!message->isUndefined() && !msg.empty()) {
    result = fmt::format(L"{}: {}", type, msg);
  } else {
    result = type;
  }
  auto stack = self->getProperty(ctx, L"stack");
  if (!stack->isUndefined()) {
    result += L"\n" + stack->toString(ctx)->getString().value();
  }
  return ctx->createString(result);
}

common::AutoPtr<JSValue>
JSErrorConstructor::initialize(common::AutoPtr<JSContext> ctx) {
  auto prototype = ctx->createObject();
  auto Error = ctx->createNativeFunction(&constructor, L"Error", L"Error");
  Error->setPropertyDescriptor(ctx, L"prototype", prototype);
  prototype->setPropertyDescriptor(ctx, L"constructor", prototype);
  prototype->setPropertyDescriptor(
      ctx, L"toString", ctx->createNativeFunction(toString, L"toString"));
  return Error;
}