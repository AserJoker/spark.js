#include "engine/lib/JSErrorConstructor.hpp"
#include "engine/base/JSValueType.hpp"
using namespace spark;
using namespace spark::engine;
JS_FUNC(JSErrorConstructor::constructor) {
  if (args.empty()) {
    self->setProperty(ctx, L"message", ctx->createString(L""));
  } else {
    self->setProperty(ctx, L"message",
                      ctx->createString(args[0]->convertToString(ctx)));
  }
  auto trace =
      ctx->trace({.filename = 0, .line = 0, .column = 0, .funcname = L"Error"});
  std::wstring stack;
  for (auto it = trace.begin() + 1; it != trace.end(); it++) {
    auto &loc = *it;
    stack += fmt::format(L"  at {} ({}:{}:{})", loc.funcname,
                         ctx->getRuntime()->getSourceFilename(loc.filename),
                         loc.line, loc.column);
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
  auto msg = message->convertToString(ctx);
  if (!message->isUndefined() && !msg.empty()) {
    result = fmt::format(L"Error: {}", msg);
  } else {
    result = L"Error";
  }
  auto stack = self->getProperty(ctx, L"stack");
  if (!stack->isUndefined()) {
    result += L"\n" + stack->convertToString(ctx);
  }
  return ctx->createString(result);
}
void JSErrorConstructor::initialize(common::AutoPtr<JSContext> ctx,
                                    common::AutoPtr<JSValue> Error) {
  auto prototype = Error->getProperty(ctx, L"prototype");
  prototype->setProperty(ctx, L"toString",
                         ctx->createNativeFunction(toString, L"toString"));
}