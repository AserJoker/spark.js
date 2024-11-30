#include "vm/JSVirtualMachine.hpp"
#include "common/AutoPtr.hpp"
#include "compiler/base/JSAsmOperator.hpp"
#include "engine/base/JSValueType.hpp"
#include "engine/entity/JSEntity.hpp"
#include "engine/entity/JSExceptionEntity.hpp"
#include "engine/entity/JSFunctionEntity.hpp"
#include "engine/entity/JSObjectEntity.hpp"
#include "engine/runtime/JSContext.hpp"
#include "engine/runtime/JSValue.hpp"
#include "error/JSError.hpp"
#include "vm/JSErrorFrame.hpp"
#include <cstdint>
using namespace spark;
using namespace spark::vm;
JSVirtualMachine::JSVirtualMachine() { _ctx = nullptr; }
compiler::JSAsmOperator
JSVirtualMachine::next(const common::AutoPtr<compiler::JSModule> &module) {
  auto codes = module->codes.data() + _ctx->pc;
  _ctx->pc += sizeof(uint16_t);
  return (compiler::JSAsmOperator) * (uint16_t *)codes;
}

uint32_t
JSVirtualMachine::argi(const common::AutoPtr<compiler::JSModule> &module) {
  auto codes = module->codes.data() + _ctx->pc;
  _ctx->pc += sizeof(uint32_t);
  return *(uint32_t *)codes;
}
double
JSVirtualMachine::argf(const common::AutoPtr<compiler::JSModule> &module) {
  auto codes = module->codes.data() + _ctx->pc;
  _ctx->pc += sizeof(double);
  return *(double *)codes;
}

const std::wstring &
JSVirtualMachine::args(const common::AutoPtr<compiler::JSModule> &module) {
  auto index = argi(module);
  return module->constants.at(index);
}

void JSVirtualMachine::handleError(
    common::AutoPtr<engine::JSContext> ctx,
    const common::AutoPtr<compiler::JSModule> &module,
    const error::JSError &e) {
  auto exception =
      ctx->createException(e.getType(), e.getMessage(), e.getLocation());
  handleError(ctx, module, exception);
}

void JSVirtualMachine::handleError(
    common::AutoPtr<engine::JSContext> ctx,
    const common::AutoPtr<compiler::JSModule> &module,
    common::AutoPtr<engine::JSValue> exception) {
  if (_ctx->errorStacks != nullptr) {
    auto frame = *_ctx->errorStacks;
    delete _ctx->errorStacks;
    _ctx->errorStacks = frame.parent;
    auto entity = exception->getEntity<engine::JSExceptionEntity>();
    frame.scope->getRoot()->appendChild(entity);
    while (ctx->getScope() != frame.scope) {
      popScope(ctx, module);
    }
    if (frame.defer != 0) {
      auto res = eval(ctx, module, frame.defer);
      if (res->getType() == engine::JSValueType::JS_EXCEPTION) {
        return handleError(ctx, module, res);
      }
    }
    if (entity->getTarget()) {
      _ctx->stack.push_back(ctx->createValue(entity->getTarget()));
    } else {
      _ctx->stack.push_back(exception);
    }
    _ctx->pc = frame.handle;
  } else {
    _ctx->stack.push_back(exception);
    _ctx->pc = module->codes.size();
  }
}

JS_OPT(JSVirtualMachine::pushNull) { _ctx->stack.push_back(ctx->null()); }

JS_OPT(JSVirtualMachine::pushUndefined) {
  _ctx->stack.push_back(ctx->undefined());
}

JS_OPT(JSVirtualMachine::pushTrue) { _ctx->stack.push_back(ctx->truly()); }

JS_OPT(JSVirtualMachine::pushFalse) { _ctx->stack.push_back(ctx->falsely()); }

JS_OPT(JSVirtualMachine::pushUninitialized) {
  _ctx->stack.push_back(ctx->uninitialized());
}

JS_OPT(JSVirtualMachine::push) {
  auto value = argf(module);
  _ctx->stack.push_back(ctx->createNumber(value));
}

JS_OPT(JSVirtualMachine::pushObject) {
  _ctx->stack.push_back(ctx->createObject());
}

JS_OPT(JSVirtualMachine::pushArray) {}

JS_OPT(JSVirtualMachine::pushFunction) {
  _ctx->stack.push_back(ctx->createFunction(module));
}

JS_OPT(JSVirtualMachine::pushGenerator) {}

JS_OPT(JSVirtualMachine::pushArrow) {}

JS_OPT(JSVirtualMachine::pushThis) {
  _ctx->stack.push_back(ctx->load(L"this"));
}

JS_OPT(JSVirtualMachine::pushSuper) {}

JS_OPT(JSVirtualMachine::pushArgument) {
  auto arguments = ctx->load(L"arguments");
  auto index = argi(module);
  _ctx->stack.push_back(arguments->getIndex(ctx, index));
}

JS_OPT(JSVirtualMachine::pushBigint) {
  auto s = args(module);
  _ctx->stack.push_back(ctx->createBigInt(s));
}

JS_OPT(JSVirtualMachine::pushRegex) {}

JS_OPT(JSVirtualMachine::setAddress) {
  auto addr = argi(module);
  auto func = _ctx->stack[_ctx->stack.size() - 1]
                  ->getEntity<engine::JSFunctionEntity>();
  func->setAddress(addr);
}

JS_OPT(JSVirtualMachine::setAsync) {
  auto async = argi(module);
  auto func = _ctx->stack[_ctx->stack.size() - 1]
                  ->getEntity<engine::JSFunctionEntity>();
  func->setAsync(async);
}

JS_OPT(JSVirtualMachine::setFuncName) {
  auto name = args(module);
  auto func = _ctx->stack[_ctx->stack.size() - 1]
                  ->getEntity<engine::JSFunctionEntity>();
  func->setFuncName(name);
}

JS_OPT(JSVirtualMachine::setFuncLen) {
  auto len = argi(module);
  auto func = _ctx->stack[_ctx->stack.size() - 1]
                  ->getEntity<engine::JSFunctionEntity>();
  func->setAsync(len);
}

JS_OPT(JSVirtualMachine::setClosure) {
  auto name = args(module);
  auto func = _ctx->stack[_ctx->stack.size() - 1]
                  ->getEntity<engine::JSFunctionEntity>();
  func->setClosure(name, ctx->load(name)->getEntity());
}

JS_OPT(JSVirtualMachine::setField) {
  auto name = *_ctx->stack.rbegin();
  _ctx->stack.pop_back();
  auto field = *_ctx->stack.rbegin();
  _ctx->stack.pop_back();
  auto obj = *_ctx->stack.rbegin();
  _ctx->stack.push_back(obj->setProperty(ctx, name, field));
}

JS_OPT(JSVirtualMachine::getField) {
  auto name = *_ctx->stack.rbegin();
  _ctx->stack.pop_back();
  auto obj = *_ctx->stack.rbegin();
  _ctx->stack.pop_back();
  _ctx->stack.push_back(obj->getProperty(ctx, name));
}

JS_OPT(JSVirtualMachine::setAccessor) {
  auto type = argi(module);
  auto name = *_ctx->stack.rbegin();
  _ctx->stack.pop_back();
  auto accessor = *_ctx->stack.rbegin();
  _ctx->stack.pop_back();
  auto obj = *_ctx->stack.rbegin();
  auto prop = obj->getOwnPropertyDescriptor(ctx, name);
  if (!prop) {
    obj->setPropertyDescriptor(ctx, name,
                               {
                                   .configurable = true,
                                   .enumable = true,
                                   .value = ctx->undefined()->getEntity(),
                                   .writable = true,
                               });
    prop = obj->getOwnPropertyDescriptor(ctx, name);
  }
  if (prop->value) {
    obj->getEntity()->removeChild(prop->value);
    prop->value = nullptr;
  }
  obj->getEntity()->appendChild(accessor->getEntity());
  if (type) {
    if (prop->get && prop->get != accessor->getEntity()) {
      obj->getEntity()->removeChild(prop->get);
    }
    prop->get = accessor->getEntity();
  } else {
    if (prop->set && prop->set != accessor->getEntity()) {
      obj->getEntity()->removeChild(prop->set);
    }
    prop->set = accessor->getEntity();
  }
  _ctx->stack.push_back(ctx->truly());
}

JS_OPT(JSVirtualMachine::setRegexHasIndices) {}

JS_OPT(JSVirtualMachine::setRegexGlobal) {}

JS_OPT(JSVirtualMachine::setRegexIgnoreCases) {}

JS_OPT(JSVirtualMachine::setRegexMultiline) {}

JS_OPT(JSVirtualMachine::setRegexDotAll) {}

JS_OPT(JSVirtualMachine::setRegexSticky) {}

JS_OPT(JSVirtualMachine::pop) {
  auto size = argi(module);
  while (size > 0) {
    _ctx->stack.pop_back();
    size--;
  }
}

JS_OPT(JSVirtualMachine::storeConst) {
  auto name = args(module);
  auto value = *_ctx->stack.rbegin();
  _ctx->stack.pop_back();
  if (!ctx->getScope()->getValues().contains(name)) {
    ctx->createValue(value, name);
  } else {
    auto current = ctx->load(name);
    current->setEntity(value->getEntity());
  }
}

JS_OPT(JSVirtualMachine::store) {
  auto name = args(module);
  auto value = *_ctx->stack.rbegin();
  _ctx->stack.pop_back();
  if (!ctx->getScope()->getValues().contains(name)) {
    ctx->createValue(value, name);
  } else {
    auto current = ctx->load(name);
    current->setEntity(value->getEntity());
  }
}

JS_OPT(JSVirtualMachine::load) {
  auto name = args(module);
  auto value = ctx->load(name);
  _ctx->stack.push_back(value);
}

JS_OPT(JSVirtualMachine::loadConst) {
  auto val = args(module);
  _ctx->stack.push_back(ctx->createString(val));
}

JS_OPT(JSVirtualMachine::ret) { _ctx->pc = module->codes.size(); }

JS_OPT(JSVirtualMachine::throw_) {
  auto value = *_ctx->stack.rbegin();
  _ctx->stack.pop_back();
  if (value->getType() != engine::JSValueType::JS_EXCEPTION) {
    value = ctx->createException(value);
  }
  _ctx->stack.push_back(value);
  _ctx->pc = module->codes.size();
}
JS_OPT(JSVirtualMachine::new_) {
  auto offset = _ctx->pc - sizeof(uint16_t);
  auto size = argi(module);
  auto now = _ctx->stack.size();
  auto func = _ctx->stack[now - 1 - size];
  std::vector<common::AutoPtr<engine::JSValue>> args;
  args.resize(size, nullptr);
  for (auto i = 0; i < size; i++) {
    args[i] = _ctx->stack[_ctx->stack.size() - size + i];
  }
  auto loc = module->sourceMap.at(offset);
  _ctx->stack.push_back(ctx->constructObject(
      func, args,
      {
          .filename = ctx->getRuntime()->setSourceFilename(module->filename),
          .line = loc.line + 1,
          .column = loc.column + 1,
          .funcname = func->getName(),
      }));
};

JS_OPT(JSVirtualMachine::yield) {}

JS_OPT(JSVirtualMachine::await) {}

JS_OPT(JSVirtualMachine::nullishCoalescing) {
  auto arg2 = *_ctx->stack.rbegin();
  _ctx->stack.pop_back();
  auto arg1 = *_ctx->stack.rbegin();
  _ctx->stack.pop_back();
  if (arg1->isNull() || arg1->isUndefined()) {
    _ctx->stack.push_back(arg2);
  } else {
    _ctx->stack.push_back(arg1);
  }
}

JS_OPT(JSVirtualMachine::pushScope) {
  _ctx->scopeChain.push_back(ctx->pushScope());
}

JS_OPT(JSVirtualMachine::popScope) {
  auto scope = *_ctx->scopeChain.rbegin();
  _ctx->scopeChain.pop_back();
  ctx->popScope(scope);
}

JS_OPT(JSVirtualMachine::call) {
  auto offset = _ctx->pc - sizeof(uint16_t);
  auto size = argi(module);
  std::vector<common::AutoPtr<engine::JSValue>> args;
  args.resize(size, nullptr);
  for (auto i = 0; i < size; i++) {
    args[size - 1 - i] = *_ctx->stack.rbegin();
    _ctx->stack.pop_back();
  }
  auto func = *_ctx->stack.rbegin();
  _ctx->stack.pop_back();
  auto name = func->getProperty(ctx, L"name")->getString().value();
  auto loc = module->sourceMap.at(offset);
  auto res = func->apply(
      ctx, ctx->undefined(), args,
      {
          .filename = ctx->getRuntime()->setSourceFilename(module->filename),
          .line = loc.line + 1,
          .column = loc.column + 1,
          .funcname = name,
      });
  _ctx->stack.push_back(res);
  if (res->getType() == engine::JSValueType::JS_EXCEPTION) {
    _ctx->pc = module->codes.size();
  }
}
JS_OPT(JSVirtualMachine::memberCall) {
  auto offset = _ctx->pc - sizeof(uint16_t);
  auto size = argi(module);
  std::vector<common::AutoPtr<engine::JSValue>> args;
  args.resize(size, nullptr);
  for (auto i = 0; i < size; i++) {
    args[size - 1 - i] = *_ctx->stack.rbegin();
    _ctx->stack.pop_back();
  }
  auto field = *_ctx->stack.rbegin();
  _ctx->stack.pop_back();
  auto self = *_ctx->stack.rbegin();
  _ctx->stack.pop_back();
  auto loc = module->sourceMap.at(offset);
  auto func = self->getProperty(ctx, field);
  auto name = func->getProperty(ctx, L"name")->getString().value();
  auto res = func->apply(
      ctx, self, args,
      {
          .filename = ctx->getRuntime()->setSourceFilename(module->filename),
          .line = loc.line + 1,
          .column = loc.column + 1,
          .funcname = name,
      });
  _ctx->stack.push_back(res);
  if (res->getType() == engine::JSValueType::JS_EXCEPTION) {
    _ctx->pc = module->codes.size();
  }
}

JS_OPT(JSVirtualMachine::add) {
  auto arg2 = *_ctx->stack.rbegin();
  _ctx->stack.pop_back();
  auto arg1 = *_ctx->stack.rbegin();
  _ctx->stack.pop_back();
  _ctx->stack.push_back(arg1->add(ctx, arg2));
}

JS_OPT(JSVirtualMachine::tryStart) {
  auto handle = argi(module);
  auto frame = new JSErrorFrame{
      .scope = ctx->getScope(),
      .defer = 0,
      .handle = handle,
      .parent = _ctx->errorStacks,
  };
  _ctx->errorStacks = frame;
}

JS_OPT(JSVirtualMachine::tryEnd) {
  auto frame = *_ctx->errorStacks;
  delete _ctx->errorStacks;
  _ctx->errorStacks = frame.parent;
  auto pc = _ctx->pc;
  if (frame.defer) {
    auto res = eval(ctx, module, frame.defer);
    if (res->getType() == engine::JSValueType::JS_EXCEPTION) {
      return handleError(ctx, module, res);
    }
  }
  _ctx->pc = pc;
}

JS_OPT(JSVirtualMachine::defer) {
  auto addr = argi(module);
  _ctx->errorStacks->defer = addr;
}

JS_OPT(JSVirtualMachine::jmp) {
  auto offset = argi(module);
  _ctx->pc = offset;
}

common::AutoPtr<engine::JSValue>
JSVirtualMachine::eval(common::AutoPtr<engine::JSContext> ctx,
                       const common::AutoPtr<compiler::JSModule> &module,
                       size_t offset) {
  auto scope = ctx->getScope();
  auto current = _ctx;
  _ctx = new JSEvalContext(offset);
  while (_ctx->pc != module->codes.size()) {
    try {
      auto code = next(module);
      if (code == compiler::JSAsmOperator::HLT) {
        break;
      }
      switch (code) {
      case compiler::JSAsmOperator::HLT:
        break;
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
      case compiler::JSAsmOperator::THROW:
        throw_(ctx, module);
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
      case compiler::JSAsmOperator::MEMBER_CALL:
        memberCall(ctx, module);
        break;
      case compiler::JSAsmOperator::ADD:
        add(ctx, module);
        break;
      case compiler::JSAsmOperator::JMP:
        jmp(ctx, module);
        break;
      case compiler::JSAsmOperator::TRY:
        tryStart(ctx, module);
        break;
      case compiler::JSAsmOperator::DEFER:
        defer(ctx, module);
        break;
      case compiler::JSAsmOperator::ENDTRY:
        tryEnd(ctx, module);
        break;
      case compiler::JSAsmOperator::NEW:
        new_(ctx, module);
        break;
      }
      if (_ctx->pc == module->codes.size()) {
        common::AutoPtr<engine::JSValue> exception;
        if (_ctx->errorStacks != nullptr) {
          auto res = eval(ctx, module, _ctx->errorStacks->defer);
          if (res->getType() == engine::JSValueType::JS_EXCEPTION) {
            exception = res;
          }
        } else {
          if (!_ctx->stack.empty()) {
            auto res = _ctx->stack[_ctx->stack.size() - 1];
            if (res->getType() == engine::JSValueType::JS_EXCEPTION) {
              exception = res;
            }
          }
        }
        if (exception != nullptr) {
          handleError(ctx, module, exception);
        }
      }
    } catch (error::JSError &e) {
      handleError(ctx, module, e);
    }
  }
  auto value = ctx->undefined();
  if (!_ctx->stack.empty()) {
    value = _ctx->stack[_ctx->stack.size() - 1];
    if (ctx->getScope() != scope) {
      auto entity = value->getEntity();
      value = scope->createValue(entity);
    }
  }
  while (ctx->getScope() != scope) {
    popScope(ctx, module);
  }
  _ctx = current;
  return value;
}