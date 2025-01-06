#include "engine/lib/JSRegexConstructor.hpp"
#include "common/AutoPtr.hpp"
#include "engine/base/JSValueType.hpp"
#include "engine/runtime/JSValue.hpp"
#include "vm/JSRegExpFlag.hpp"
#include <regex>
#include <string>
using namespace spark;
using namespace spark::engine;

JS_FUNC(JSRegexConstructor::constructor) {
  common::AutoPtr<JSValue> value = ctx->createString(L"(?:)");
  common::AutoPtr<JSValue> flag = ctx->createNumber();
  if (args.size() > 0) {
    if (args[0]->getType() == JSValueType::JS_STRING) {
      value = args[0];
    }
  }
  if (args.size() > 1) {
    if (args[1]->getType() == JSValueType::JS_NUMBER) {
      flag = args[1];
    }
  }
  self->setProperty(ctx, ctx->internalSymbol(L"regex_value"), value);
  self->setProperty(ctx, ctx->internalSymbol(L"regex_flag"), flag);
  return ctx->undefined();
}

JS_FUNC(JSRegexConstructor::test) {
  auto value = self->getProperty(ctx, ctx->internalSymbol(L"regex_value"))
                   ->getString()
                   .value();
  auto flag =
      (uint32_t)self->getProperty(ctx, ctx->internalSymbol(L"regex_flag"))
          ->getNumber()
          .value();
  auto cppflag = std::regex::ECMAScript;
  if (flag & vm::DOTALL) {
    cppflag = std::regex::extended;
  }
  if (flag & vm::ICASE) {
    cppflag |= std::regex::icase;
  }
  if (flag & vm::MULTILINE) {
    cppflag |= std::regex::multiline;
  }

  auto arg = args[0]->toString(ctx)->getString().value();
  std::wregex pattern(value, cppflag);
  return ctx->createBoolean(std::regex_search(arg, pattern));
}

JS_FUNC(JSRegexConstructor::exec) {
  auto arg = args[0]->toString(ctx)->getString().value();
  auto result = ctx->createArray();
  result->setProperty(ctx, L"input", args[0]->toString(ctx));
  auto lastIndex =
      (uint32_t)self->getProperty(ctx, L"lastIndex")->getNumber().value();

  auto value = self->getProperty(ctx, ctx->internalSymbol(L"regex_value"))
                   ->getString()
                   .value();
  auto flag =
      (uint32_t)self->getProperty(ctx, ctx->internalSymbol(L"regex_flag"))
          ->getNumber()
          .value();

  auto cppflag = std::regex::ECMAScript;
  if (flag & vm::DOTALL) {
    cppflag = std::regex::extended;
  }
  if (flag & vm::ICASE) {
    cppflag |= std::regex::icase;
  }
  if (flag & vm::MULTILINE) {
    cppflag |= std::regex::multiline;
  }
  std::wregex pattern(value, cppflag);

  std::match_results<std::wstring::const_iterator> re;
  std::wstring next(arg.begin() + lastIndex, arg.end());
  std::regex_search(next, re, pattern);
  if (re.empty()) {
    self->setProperty(ctx, L"lastIndex", ctx->createNumber(0));
    return ctx->null();
  }
  result->setIndex(ctx, 0, ctx->createString(re.str()));
  result->setProperty(ctx, L"index",
                      ctx->createNumber(re.position() + lastIndex));
  if (flag & vm::GLOBAL) {
    self->setProperty(ctx, L"lastIndex",
                      ctx->createNumber(lastIndex + re.prefix().length() +
                                        re.str().length()));
  }
  if (re.size()) {
    auto group = ctx->createArray();

    for (size_t i = 1; i < re.size(); i++) {
      std::sub_match sub = re[i];
      group->setIndex(ctx, i - 1, ctx->createString(sub.str()));
    }
    result->setProperty(ctx, L"groups", group);
  } else {
    self->setProperty(ctx, L"groups", ctx->undefined());
  }
  return result;
}

common::AutoPtr<JSValue>
JSRegexConstructor::initialize(common::AutoPtr<JSContext> ctx) {
  auto RegExp = ctx->createNativeFunction(constructor, L"RegExp");
  auto prototype = ctx->createObject();
  RegExp->setProperty(ctx, L"prototype", prototype);
  prototype->setProperty(ctx, L"constructor", RegExp);
  prototype->setProperty(ctx, L"test",
                         ctx->createNativeFunction(test, L"test"));
  prototype->setProperty(ctx, L"exec",
                         ctx->createNativeFunction(exec, L"exec"));
  prototype->setProperty(ctx, L"lastIndex", ctx->createNumber());
  return RegExp;
}