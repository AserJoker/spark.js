#include "vm/JSVirtualMachine.hpp"
#include "common/AutoPtr.hpp"
#include "compiler/base/JSAsmOperator.hpp"
#include "engine/base/JSValueType.hpp"
#include "engine/runtime/JSContext.hpp"
using namespace spark;
using namespace spark::vm;
JSVirtualMachine::JSVirtualMachine() : _pc(0) {}
compiler::JSAsmOperator
JSVirtualMachine::next(const common::AutoPtr<compiler::JSModule> &module) {
  auto codes = module->codes.data() + _pc;
  _pc += sizeof(uint16_t);
  return (compiler::JSAsmOperator) * (uint16_t *)codes;
}

uint32_t
JSVirtualMachine::argi(const common::AutoPtr<compiler::JSModule> &module) {
  auto codes = module->codes.data() + _pc;
  _pc += sizeof(uint32_t);
  return *(uint32_t *)codes;
}
double
JSVirtualMachine::argf(const common::AutoPtr<compiler::JSModule> &module) {
  auto codes = module->codes.data() + _pc;
  _pc += sizeof(double);
  return *(double *)codes;
}
const std::wstring &
JSVirtualMachine::args(const common::AutoPtr<compiler::JSModule> &module) {
  auto index = argi(module);
  return module->constants.at(index);
}

void JSVirtualMachine::pushNull(
    common::AutoPtr<engine::JSContext> ctx,
    const common::AutoPtr<compiler::JSModule> &module) {
  _stack.push_back(ctx->null());
}

void JSVirtualMachine::pushUndefined(
    common::AutoPtr<engine::JSContext> ctx,
    const common::AutoPtr<compiler::JSModule> &module) {
  _stack.push_back(ctx->undefined());
}

void JSVirtualMachine::pushTrue(
    common::AutoPtr<engine::JSContext> ctx,
    const common::AutoPtr<compiler::JSModule> &module) {
  _stack.push_back(ctx->truly());
}

void JSVirtualMachine::pushFalse(
    common::AutoPtr<engine::JSContext> ctx,
    const common::AutoPtr<compiler::JSModule> &module) {
  _stack.push_back(ctx->falsely());
}

void JSVirtualMachine::pushUninitialized(
    common::AutoPtr<engine::JSContext> ctx,
    const common::AutoPtr<compiler::JSModule> &module) {
  _stack.push_back(ctx->uninitialized());
}

void JSVirtualMachine::push(common::AutoPtr<engine::JSContext> ctx,
                            const common::AutoPtr<compiler::JSModule> &module) {
  auto value = argf(module);
  _stack.push_back(ctx->createNumber(value));
}

void JSVirtualMachine::pushObject(
    common::AutoPtr<engine::JSContext> ctx,
    const common::AutoPtr<compiler::JSModule> &module) {
  _stack.push_back(ctx->createObject());
}

void JSVirtualMachine::pushArray(
    common::AutoPtr<engine::JSContext> ctx,
    const common::AutoPtr<compiler::JSModule> &module) {}

void JSVirtualMachine::pushFunction(
    common::AutoPtr<engine::JSContext> ctx,
    const common::AutoPtr<compiler::JSModule> &module) {}

void JSVirtualMachine::pushGenerator(
    common::AutoPtr<engine::JSContext> ctx,
    const common::AutoPtr<compiler::JSModule> &module) {}

void JSVirtualMachine::pushArrow(
    common::AutoPtr<engine::JSContext> ctx,
    const common::AutoPtr<compiler::JSModule> &module) {}

void JSVirtualMachine::pushThis(
    common::AutoPtr<engine::JSContext> ctx,
    const common::AutoPtr<compiler::JSModule> &module) {
  _stack.push_back(ctx->load(L"this"));
}

void JSVirtualMachine::pushSuper(
    common::AutoPtr<engine::JSContext> ctx,
    const common::AutoPtr<compiler::JSModule> &module) {}

void JSVirtualMachine::pushArgument(
    common::AutoPtr<engine::JSContext> ctx,
    const common::AutoPtr<compiler::JSModule> &module) {
  auto arguments = ctx->load(L"arguments");
  auto index = argi(module);
  _stack.push_back(arguments->getIndex(ctx, index));
}

void JSVirtualMachine::pushBigint(
    common::AutoPtr<engine::JSContext> ctx,
    const common::AutoPtr<compiler::JSModule> &module) {
  auto s = args(module);
  _stack.push_back(ctx->createBigInt(s));
}

void JSVirtualMachine::pushRegex(
    common::AutoPtr<engine::JSContext> ctx,
    const common::AutoPtr<compiler::JSModule> &module) {}

void JSVirtualMachine::setAddress(
    common::AutoPtr<engine::JSContext> ctx,
    const common::AutoPtr<compiler::JSModule> &module) {}

void JSVirtualMachine::setAsync(
    common::AutoPtr<engine::JSContext> ctx,
    const common::AutoPtr<compiler::JSModule> &module) {}

void JSVirtualMachine::setFuncName(
    common::AutoPtr<engine::JSContext> ctx,
    const common::AutoPtr<compiler::JSModule> &module) {}

void JSVirtualMachine::setFuncLen(
    common::AutoPtr<engine::JSContext> ctx,
    const common::AutoPtr<compiler::JSModule> &module) {}

void JSVirtualMachine::setClosure(
    common::AutoPtr<engine::JSContext> ctx,
    const common::AutoPtr<compiler::JSModule> &module) {}

void JSVirtualMachine::setField(
    common::AutoPtr<engine::JSContext> ctx,
    const common::AutoPtr<compiler::JSModule> &module) {}

void JSVirtualMachine::getField(
    common::AutoPtr<engine::JSContext> ctx,
    const common::AutoPtr<compiler::JSModule> &module) {}

void JSVirtualMachine::setAccessor(
    common::AutoPtr<engine::JSContext> ctx,
    const common::AutoPtr<compiler::JSModule> &module) {}

void JSVirtualMachine::getAccessor(
    common::AutoPtr<engine::JSContext> ctx,
    const common::AutoPtr<compiler::JSModule> &module) {}

void JSVirtualMachine::setMethod(
    common::AutoPtr<engine::JSContext> ctx,
    const common::AutoPtr<compiler::JSModule> &module) {}

void JSVirtualMachine::getMethod(
    common::AutoPtr<engine::JSContext> ctx,
    const common::AutoPtr<compiler::JSModule> &module) {}

void JSVirtualMachine::setIndex(
    common::AutoPtr<engine::JSContext> ctx,
    const common::AutoPtr<compiler::JSModule> &module) {}

void JSVirtualMachine::getIndex(
    common::AutoPtr<engine::JSContext> ctx,
    const common::AutoPtr<compiler::JSModule> &module) {}

void JSVirtualMachine::setRegexHasIndices(
    common::AutoPtr<engine::JSContext> ctx,
    const common::AutoPtr<compiler::JSModule> &module) {}

void JSVirtualMachine::setRegexGlobal(
    common::AutoPtr<engine::JSContext> ctx,
    const common::AutoPtr<compiler::JSModule> &module) {}

void JSVirtualMachine::setRegexIgnoreCases(
    common::AutoPtr<engine::JSContext> ctx,
    const common::AutoPtr<compiler::JSModule> &module) {}

void JSVirtualMachine::setRegexMultiline(
    common::AutoPtr<engine::JSContext> ctx,
    const common::AutoPtr<compiler::JSModule> &module) {}

void JSVirtualMachine::setRegexDotAll(
    common::AutoPtr<engine::JSContext> ctx,
    const common::AutoPtr<compiler::JSModule> &module) {}

void JSVirtualMachine::setRegexSticky(
    common::AutoPtr<engine::JSContext> ctx,
    const common::AutoPtr<compiler::JSModule> &module) {}

void JSVirtualMachine::pop(common::AutoPtr<engine::JSContext> ctx,
                           const common::AutoPtr<compiler::JSModule> &module) {
  auto size = argi(module);
  while (size > 0) {
    _stack.pop_back();
    size--;
  }
}

void JSVirtualMachine::storeConst(
    common::AutoPtr<engine::JSContext> ctx,
    const common::AutoPtr<compiler::JSModule> &module) {
  auto name = args(module);
  auto value = _stack[_stack.size() - 1];
  if (!ctx->getScope()->getValues().contains(name)) {
    ctx->createValue(value, name);
  } else {
    auto current = ctx->load(name);
    current->setEntity(value->getEntity());
  }
}

void JSVirtualMachine::store(
    common::AutoPtr<engine::JSContext> ctx,
    const common::AutoPtr<compiler::JSModule> &module) {
  auto name = args(module);
  auto value = _stack[_stack.size() - 1];
  if (!ctx->getScope()->getValues().contains(name)) {
    ctx->createValue(value, name);
  } else {
    auto current = ctx->load(name);
    current->setEntity(value->getEntity());
  }
}

void JSVirtualMachine::load(common::AutoPtr<engine::JSContext> ctx,
                            const common::AutoPtr<compiler::JSModule> &module) {
  auto name = args(module);
  auto value = ctx->load(name);
  _stack.push_back(value);
}

void JSVirtualMachine::loadConst(
    common::AutoPtr<engine::JSContext> ctx,
    const common::AutoPtr<compiler::JSModule> &module) {
  auto val = args(module);
  _stack.push_back(ctx->createString(val));
}

void JSVirtualMachine::ret(common::AutoPtr<engine::JSContext> ctx,
                           const common::AutoPtr<compiler::JSModule> &module) {}

void JSVirtualMachine::yield(
    common::AutoPtr<engine::JSContext> ctx,
    const common::AutoPtr<compiler::JSModule> &module) {}

void JSVirtualMachine::await(
    common::AutoPtr<engine::JSContext> ctx,
    const common::AutoPtr<compiler::JSModule> &module) {}

void JSVirtualMachine::nullishCoalescing(
    common::AutoPtr<engine::JSContext> ctx,
    const common::AutoPtr<compiler::JSModule> &module) {
  auto arg2 = *_stack.rbegin();
  _stack.pop_back();
  auto arg1 = *_stack.rbegin();
  _stack.pop_back();
  if (arg1->isNull() || arg1->isUndefined()) {
    _stack.push_back(arg2);
  } else {
    _stack.push_back(arg1);
  }
}

void JSVirtualMachine::pushScope(
    common::AutoPtr<engine::JSContext> ctx,
    const common::AutoPtr<compiler::JSModule> &module) {
  _scopeChain.push_back(ctx->pushScope());
  _stackTops.push_back(_stack.size());
}

void JSVirtualMachine::popScope(
    common::AutoPtr<engine::JSContext> ctx,
    const common::AutoPtr<compiler::JSModule> &module) {
  auto size = *_stackTops.rbegin();
  _stackTops.pop_back();
  _stack.resize(size);
  auto scope = *_scopeChain.rbegin();
  _scopeChain.pop_back();
  ctx->popScope(scope);
}
void JSVirtualMachine::call(common::AutoPtr<engine::JSContext> ctx,
                            const common::AutoPtr<compiler::JSModule> &module) {
  auto size = argi(module);
  auto func = _stack[_stack.size() - 1 - size];
  auto self = ctx->load(L"this");
  std::vector<common::AutoPtr<engine::JSValue>> args;
  args.resize(size, nullptr);
  for (auto i = 0; i < size; i++) {
    args[i] = _stack[_stack.size() - size + i];
  }
  auto res = func->apply(ctx, self, args);
  _stack.push_back(res);
  _stack.resize(_stack.size() - 1 - size);
}
void JSVirtualMachine::add(common::AutoPtr<engine::JSContext> ctx,
                           const common::AutoPtr<compiler::JSModule> &module) {
  auto arg2 = *_stack.rbegin();
  _stack.pop_back();
  auto arg1 = *_stack.rbegin();
  _stack.pop_back();
  _stack.push_back(arg1->add(ctx, arg2));
}

common::AutoPtr<engine::JSValue>
JSVirtualMachine::run(common::AutoPtr<engine::JSContext> ctx,
                      const common::AutoPtr<compiler::JSModule> &module,
                      size_t offset) {
  _pc = offset;
  auto code = next(module);
  while (_pc != module->codes.size()) {
    switch (code) {
    case compiler::JSAsmOperator::PUSH_NULL:
      pushNull(ctx, module);
      break;
    case compiler::JSAsmOperator::PUSH_UNDEFINED:
      pushUndefined(ctx, module);
      break;
    case compiler::JSAsmOperator::PUSH_TRUE:
      pushTrue(ctx, module);
      break;
    case compiler::JSAsmOperator::PUSH_FALSE:
      pushFalse(ctx, module);
      break;
    case compiler::JSAsmOperator::PUSH_UNINITIALIZED:
      pushUninitialized(ctx, module);
      break;
    case compiler::JSAsmOperator::PUSH:
      push(ctx, module);
      break;
    case compiler::JSAsmOperator::PUSH_OBJECT:
      pushObject(ctx, module);
      break;
    case compiler::JSAsmOperator::PUSH_ARRAY:
      pushArray(ctx, module);
      break;
    case compiler::JSAsmOperator::PUSH_FUNCTION:
      pushFunction(ctx, module);
      break;
    case compiler::JSAsmOperator::PUSH_GENERATOR:
      pushGenerator(ctx, module);
      break;
    case compiler::JSAsmOperator::PUSH_ARROW:
      pushArrow(ctx, module);
      break;
    case compiler::JSAsmOperator::PUSH_THIS:
      pushThis(ctx, module);
      break;
    case compiler::JSAsmOperator::PUSH_SUPER:
      pushSuper(ctx, module);
      break;
    case compiler::JSAsmOperator::PUSH_ARGUMENT:
      pushArgument(ctx, module);
      break;
    case compiler::JSAsmOperator::PUSH_BIGINT:
      pushBigint(ctx, module);
      break;
    case compiler::JSAsmOperator::PUSH_REGEX:
      pushRegex(ctx, module);
      break;
    case compiler::JSAsmOperator::SET_ADDRESS:
      setAddress(ctx, module);
      break;
    case compiler::JSAsmOperator::SET_ASYNC:
      setAsync(ctx, module);
      break;
    case compiler::JSAsmOperator::SET_FUNC_NAME:
      setFuncName(ctx, module);
      break;
    case compiler::JSAsmOperator::SET_FUNC_LEN:
      setFuncLen(ctx, module);
      break;
    case compiler::JSAsmOperator::SET_CLOSURE:
      setClosure(ctx, module);
      break;
    case compiler::JSAsmOperator::SET_FIELD:
      setField(ctx, module);
      break;
    case compiler::JSAsmOperator::GET_FIELD:
      getField(ctx, module);
      break;
    case compiler::JSAsmOperator::SET_ACCESSOR:
      setAccessor(ctx, module);
      break;
    case compiler::JSAsmOperator::GET_ACCESSOR:
      getAccessor(ctx, module);
      break;
    case compiler::JSAsmOperator::SET_METHOD:
      setMethod(ctx, module);
      break;
    case compiler::JSAsmOperator::GET_METHOD:
      getMethod(ctx, module);
      break;
    case compiler::JSAsmOperator::SET_INDEX:
      setIndex(ctx, module);
      break;
    case compiler::JSAsmOperator::GET_INDEX:
      getIndex(ctx, module);
      break;
    case compiler::JSAsmOperator::SET_REGEX_HAS_INDICES:
      setRegexHasIndices(ctx, module);
      break;
    case compiler::JSAsmOperator::SET_REGEX_GLOBAL:
      setRegexGlobal(ctx, module);
      break;
    case compiler::JSAsmOperator::SET_REGEX_IGNORE_CASES:
      setRegexIgnoreCases(ctx, module);
      break;
    case compiler::JSAsmOperator::SET_REGEX_MULTILINE:
      setRegexMultiline(ctx, module);
      break;
    case compiler::JSAsmOperator::SET_REGEX_DOT_ALL:
      setRegexDotAll(ctx, module);
      break;
    case compiler::JSAsmOperator::SET_REGEX_STICKY:
      setRegexSticky(ctx, module);
      break;
    case compiler::JSAsmOperator::POP:
      pop(ctx, module);
      break;
    case compiler::JSAsmOperator::STORE_CONST:
      storeConst(ctx, module);
      break;
    case compiler::JSAsmOperator::STORE:
      store(ctx, module);
      break;
    case compiler::JSAsmOperator::LOAD:
      load(ctx, module);
      break;
    case compiler::JSAsmOperator::LOAD_CONST:
      loadConst(ctx, module);
      break;
    case compiler::JSAsmOperator::RET:
      ret(ctx, module);
      break;
    case compiler::JSAsmOperator::YIELD:
      yield(ctx, module);
      break;
    case compiler::JSAsmOperator::AWAIT:
      await(ctx, module);
      break;
    case compiler::JSAsmOperator::NULLISH_COALESCING:
      nullishCoalescing(ctx, module);
      break;
    case compiler::JSAsmOperator::PUSH_SCOPE:
      pushScope(ctx, module);
      break;
    case compiler::JSAsmOperator::POP_SCOPE:
      popScope(ctx, module);
      break;
    case compiler::JSAsmOperator::CALL:
      call(ctx, module);
      break;
    case compiler::JSAsmOperator::ADD:
      add(ctx, module);
      break;
    }
    code = next(module);
  }
  return ctx->undefined();
}