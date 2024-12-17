#include "vm/JSVirtualMachine.hpp"
#include "common/AutoPtr.hpp"
#include "compiler/base/JSNode.hpp"
#include "engine/base/JSValueType.hpp"
#include "engine/entity/JSArrayEntity.hpp"
#include "engine/entity/JSEntity.hpp"
#include "engine/entity/JSExceptionEntity.hpp"
#include "engine/entity/JSFunctionEntity.hpp"
#include "engine/entity/JSNativeFunctionEntity.hpp"
#include "engine/entity/JSObjectEntity.hpp"
#include "engine/entity/JSTasKEntity.hpp"
#include "engine/runtime/JSContext.hpp"
#include "engine/runtime/JSScope.hpp"
#include "engine/runtime/JSValue.hpp"
#include "error/JSError.hpp"
#include "error/JSSyntaxError.hpp"
#include "error/JSTypeError.hpp"
#include "vm/JSAsmOperator.hpp"
#include "vm/JSCoroutineContext.hpp"
#include "vm/JSErrorFrame.hpp"
#include "vm/JSEvalContext.hpp"
#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

using namespace spark;
using namespace spark::vm;
JSVirtualMachine::JSVirtualMachine() { _ctx = new JSEvalContext; }

vm::JSAsmOperator
JSVirtualMachine::next(const common::AutoPtr<compiler::JSModule> &module) {
  auto codes = module->codes.data() + _pc;
  _pc += sizeof(uint16_t);
  return (vm::JSAsmOperator) * (uint16_t *)codes;
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

JS_OPT(JSVirtualMachine::pushArray) {
  _ctx->stack.push_back(ctx->createArray());
}

JS_OPT(JSVirtualMachine::pushFunction) {
  _ctx->stack.push_back(ctx->createFunction(module));
}

JS_OPT(JSVirtualMachine::pushGenerator) {
  _ctx->stack.push_back(ctx->createGenerator(module));
}

JS_OPT(JSVirtualMachine::pushArrow) {
  _ctx->stack.push_back(ctx->createArrow(module));
}

JS_OPT(JSVirtualMachine::pushThis) {
  _ctx->stack.push_back(ctx->load(L"this"));
}

JS_OPT(JSVirtualMachine::pushSuper) {}

JS_OPT(JSVirtualMachine::pushBigint) {
  auto s = args(module);
  _ctx->stack.push_back(ctx->createBigInt(s));
}

JS_OPT(JSVirtualMachine::pushRegex) {}

JS_OPT(JSVirtualMachine::pushValue) {
  auto offset = argi(module);
  if (offset <= _ctx->stack.size()) {
    auto value = _ctx->stack[_ctx->stack.size() - offset];
    _ctx->stack.push_back(ctx->createValue(value));
  } else {
    _ctx->stack.push_back(ctx->undefined());
  }
}

JS_OPT(JSVirtualMachine::setAddress) {
  auto addr = argi(module);
  auto func = (*_ctx->stack.rbegin())->getEntity<engine::JSFunctionEntity>();
  func->setAddress(addr);
}

JS_OPT(JSVirtualMachine::setAsync) {
  auto async = argi(module);
  auto func = (*_ctx->stack.rbegin())->getEntity<engine::JSFunctionEntity>();
  func->setAsync(async);
}

JS_OPT(JSVirtualMachine::setFuncName) {
  auto name = args(module);
  auto func = (*_ctx->stack.rbegin())->getEntity<engine::JSFunctionEntity>();
  func->setFuncName(name);
}

JS_OPT(JSVirtualMachine::setFuncLen) {
  auto len = argi(module);
  auto func = (*_ctx->stack.rbegin())->getEntity<engine::JSFunctionEntity>();
  func->setAsync(len);
}
JS_OPT(JSVirtualMachine::setFuncSource) {
  auto source = args(module);
  auto func = (*_ctx->stack.rbegin())->getEntity<engine::JSFunctionEntity>();
  func->setSource(source);
}

JS_OPT(JSVirtualMachine::setClosure) {
  auto name = args(module);
  auto func = (*_ctx->stack.rbegin());
  auto store = ctx->load(name)->getStore();
  func->getEntity<engine::JSFunctionEntity>()->setClosure(
      name, ctx->load(name)->getStore());
  func->getStore()->appendChild(store);
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

JS_OPT(JSVirtualMachine::getKeys) {
  auto obj = *_ctx->stack.rbegin();
  _ctx->stack.pop_back();
  _ctx->stack.push_back(obj->getKeys(ctx));
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
    obj->setPropertyDescriptor(ctx, name, ctx->undefined(), true, true);
    prop = obj->getOwnPropertyDescriptor(ctx, name);
  }
  if (prop->value) {
    obj->getStore()->removeChild(prop->value);
    prop->value = nullptr;
  }
  obj->getStore()->appendChild(accessor->getStore());
  if (type) {
    if (prop->get && prop->get != accessor->getStore()) {
      obj->getStore()->removeChild(prop->get);
    }
    prop->get = accessor->getStore();
  } else {
    if (prop->set && prop->set != accessor->getStore()) {
      obj->getStore()->removeChild(prop->set);
    }
    prop->set = accessor->getStore();
  }
  _ctx->stack.push_back(ctx->truly());
}

JS_OPT(JSVirtualMachine::merge) {
  auto obj1 = *_ctx->stack.rbegin();
  _ctx->stack.pop_back();
  auto obj = *_ctx->stack.rbegin();
  if (obj->getType() == engine::JSValueType::JS_OBJECT) {
    auto entity = obj1->getEntity<engine::JSObjectEntity>();
    while (entity != nullptr) {
      auto props = entity->getProperties();
      for (auto &[key, field] : props) {
        if (field.enumable) {
          obj->setProperty(ctx, key, obj1->getProperty(ctx, key));
        }
      }
      obj1 = obj1->getPrototype(ctx);
      entity = obj1->getEntity<engine::JSObjectEntity>();
    }
  } else if (obj->getType() == engine::JSValueType::JS_ARRAY) {
    auto size = obj->getEntity<engine::JSArrayEntity>()->getItems().size();
    auto iterator =
        obj1->getProperty(ctx, ctx->Symbol()->getProperty(ctx, L"iterator"));
    if (!iterator->isFunction()) {
      throw error::JSTypeError(L"array pattern rest require iterator");
    }
    auto gen = iterator->apply(ctx, obj1);
    if (gen->getType() != engine::JSValueType::JS_OBJECT) {
      throw error::JSTypeError(
          L"Result of the Symbol.iterator method is not an object");
    }
    auto next = gen->getProperty(ctx, L"next");
    if (!next->isFunction()) {
      throw error::JSTypeError(L"array pattern require iterator");
    }
    for (;;) {
      auto res = next->apply(ctx, gen);
      if (res->getType() != engine::JSValueType::JS_OBJECT) {
        throw error::JSTypeError(
            fmt::format(L"Iterator result '{}' is not an object",
                        res->toString(ctx)->getString().value()));
      }
      auto val = res->getProperty(ctx, L"value");
      auto done = res->getProperty(ctx, L"done");
      if (done->toBoolean(ctx)->getBoolean().value()) {
        break;
      }
      obj->setIndex(ctx, size++, val);
    }
  } else {
    throw error::JSSyntaxError(L"Invalid operator");
  }
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
  auto val = ctx->getScope()->getValue(name);
  if (!val || val->getType() == engine::JSValueType::JS_UNINITIALIZED) {
    ctx->createValue(value, name);
  } else {
    val->setEntity(value->getEntity());
  }
}

JS_OPT(JSVirtualMachine::store) {
  auto name = args(module);
  auto value = *_ctx->stack.rbegin();
  _ctx->stack.pop_back();
  auto val = ctx->getScope()->getValue(name);
  if (val == nullptr) {
    ctx->createValue(value, name);
  } else {
    val->setEntity(value->getEntity());
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

JS_OPT(JSVirtualMachine::ret) { _pc = module->codes.size(); }

JS_OPT(JSVirtualMachine::throw_) {
  auto value = *_ctx->stack.rbegin();
  _ctx->stack.pop_back();
  if (value->getType() != engine::JSValueType::JS_EXCEPTION) {
    value = ctx->createException(value);
  }
  _ctx->stack.push_back(value);
  _pc = module->codes.size();
}
JS_OPT(JSVirtualMachine::new_) {
  auto offset = _pc - sizeof(uint16_t);
  auto size = argi(module);
  auto now = _ctx->stack.size();
  std::vector<common::AutoPtr<engine::JSValue>> args;
  args.resize(size, nullptr);
  for (auto i = 0; i < size; i++) {
    args[size - 1 - i] = *_ctx->stack.rbegin();
    _ctx->stack.pop_back();
  }
  auto func = *_ctx->stack.rbegin();
  _ctx->stack.pop_back();
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

JS_OPT(JSVirtualMachine::yield) {
  auto value = *_ctx->stack.rbegin();
  _ctx->stack.pop_back();
  _ctx->stack.push_back(ctx->createValue(
      new engine::JSStore(new engine::JSTaskEntity(value->getStore(), _pc))));
  _pc = module->codes.size();
}

JS_OPT(JSVirtualMachine::yieldDelegate) {
  auto arg = *_ctx->stack.rbegin();
  _ctx->stack.pop_back();
  auto gen = *_ctx->stack.rbegin();
  _ctx->stack.pop_back();
  auto value = *_ctx->stack.rbegin();
  _ctx->stack.pop_back();
  auto nextpc = _pc;
  if (gen->isUndefined()) {
    auto iterator =
        value->getProperty(ctx, ctx->Symbol()->getProperty(ctx, L"iterator"));
    if (!iterator->isFunction()) {
      throw error::JSTypeError(L"yield delegate require iterator");
    }
    gen = iterator->apply(ctx, value);
    if (gen->getType() != engine::JSValueType::JS_OBJECT) {
      throw error::JSTypeError(
          L"Result of the Symbol.iterator method is not an object");
    }
  }
  auto next = gen->getProperty(ctx, L"next");
  if (!next->isFunction()) {
    throw error::JSTypeError(L"yield delegate require iterator");
  }
  std::vector<common::AutoPtr<engine::JSValue>> args;
  if (!arg->isUndefined()) {
    args.push_back(arg);
  }
  auto val = next->apply(ctx, gen, args);
  if (val->getType() != engine::JSValueType::JS_OBJECT) {
    throw error::JSTypeError(
        fmt::format(L"Iterator result '{}' is not an object",
                    val->toString(ctx)->getString().value()));
  }
  auto done = val->getProperty(ctx, L"done");
  auto result = val->getProperty(ctx, L"value");
  if (done->toBoolean(ctx)->getBoolean().value()) {
    _pc = nextpc;
  } else {
    _ctx->stack.push_back(value);
    _ctx->stack.push_back(gen);
    _ctx->stack.push_back(
        ctx->createValue(new engine::JSStore(new engine::JSTaskEntity(
            result->getStore(), nextpc - sizeof(uint16_t)))));
    _pc = module->codes.size();
  }
}

JS_OPT(JSVirtualMachine::next) {
  auto gen = *_ctx->stack.rbegin();
  auto pc = _pc;
  if (gen->isUndefined()) {
    _ctx->stack.pop_back();
    auto value = *_ctx->stack.rbegin();
    auto iterator =
        value->getProperty(ctx, ctx->Symbol()->getProperty(ctx, L"iterator"));
    if (!iterator->isFunction()) {
      throw error::JSTypeError(L"array pattern require iterator");
    }
    gen = iterator->apply(ctx, value);
    if (gen->getType() != engine::JSValueType::JS_OBJECT) {
      throw error::JSTypeError(
          L"Result of the Symbol.iterator method is not an object");
    }
    _ctx->stack.push_back(gen);
  }
  auto next = gen->getProperty(ctx, L"next");
  if (!next->isFunction()) {
    throw error::JSTypeError(L"array pattern require iterator");
  }
  auto res = next->apply(ctx, gen);
  if (res->getType() != engine::JSValueType::JS_OBJECT) {
    throw error::JSTypeError(
        fmt::format(L"Iterator result '{}' is not an object",
                    res->toString(ctx)->getString().value()));
  }
  auto val = res->getProperty(ctx, L"value");
  auto done = res->getProperty(ctx, L"done");
  _ctx->stack.push_back(val);
  _ctx->stack.push_back(done);
  _pc = pc;
}

JS_OPT(JSVirtualMachine::restArray) {
  auto gen = *_ctx->stack.rbegin();
  auto pc = _pc;
  if (gen->isUndefined()) {
    _ctx->stack.pop_back();
    auto value = *_ctx->stack.rbegin();
    auto iterator =
        value->getProperty(ctx, ctx->Symbol()->getProperty(ctx, L"iterator"));
    if (!iterator->isFunction()) {
      throw error::JSTypeError(L"array pattern require iterator");
    }
    gen = iterator->apply(ctx, value);
    if (gen->getType() != engine::JSValueType::JS_OBJECT) {
      throw error::JSTypeError(
          L"Result of the Symbol.iterator method is not an object");
    }
    _ctx->stack.push_back(gen);
  }
  auto next = gen->getProperty(ctx, L"next");
  if (!next->isFunction()) {
    throw error::JSTypeError(L"array pattern require iterator");
  }
  auto arr = ctx->createArray();
  uint32_t index = 0;
  for (;;) {
    auto res = next->apply(ctx, gen);
    if (res->getType() != engine::JSValueType::JS_OBJECT) {
      throw error::JSTypeError(
          fmt::format(L"Iterator result '{}' is not an object",
                      res->toString(ctx)->getString().value()));
    }
    auto val = res->getProperty(ctx, L"value");
    auto done = res->getProperty(ctx, L"done");
    if (done->toBoolean(ctx)->getBoolean().value()) {
      break;
    }
    arr->setIndex(ctx, index++, val);
  }
  _ctx->stack.push_back(arr);
  _pc = pc;
}

JS_OPT(JSVirtualMachine::restObject) {
  auto size = argi(module);
  std::vector<std::wstring> keys;
  for (uint32_t i = 0; i < size; i++) {
    auto val = *_ctx->stack.rbegin();
    _ctx->stack.pop_back();
    keys.push_back(val->toString(ctx)->getString().value());
  }
  auto obj = *_ctx->stack.rbegin();
  auto res = ctx->createObject();
  auto entity = obj->getEntity<engine::JSObjectEntity>();
  while (entity != nullptr) {
    auto props = entity->getProperties();
    for (auto &[key, field] : props) {
      if (field.enumable) {
        auto it = std::find(keys.begin(), keys.end(), key);
        if (it == keys.end()) {
          res->setProperty(ctx, key, obj->getProperty(ctx, key));
        }
      }
    }
    obj = obj->getPrototype(ctx);
    entity = obj->getEntity<engine::JSObjectEntity>();
  }
  _ctx->stack.push_back(res);
}

JS_OPT(JSVirtualMachine::await) {}

JS_OPT(JSVirtualMachine::void_) {
  auto value = *_ctx->stack.rbegin();
  _ctx->stack.pop_back();
  _ctx->stack.push_back(ctx->undefined());
}
JS_OPT(JSVirtualMachine::delete_) {
  auto field = *_ctx->stack.rbegin();
  _ctx->stack.pop_back();
  auto host = *_ctx->stack.rbegin();
  _ctx->stack.pop_back();
  _ctx->stack.push_back(host->removeProperty(ctx, field));
}

JS_OPT(JSVirtualMachine::typeof_) {
  auto value = *_ctx->stack.rbegin();
  _ctx->stack.pop_back();
  _ctx->stack.push_back(ctx->createString(value->getTypeName()));
}

JS_OPT(JSVirtualMachine::pushScope) {
  ctx->pushScope();
  _ctx->stackTops.push_back(_ctx->stack.size());
}

JS_OPT(JSVirtualMachine::popScope) {
  _ctx->stack.resize(*_ctx->stackTops.rbegin());
  _ctx->stackTops.pop_back();
  ctx->popScope();
}

JS_OPT(JSVirtualMachine::call) {
  auto offset = _pc - sizeof(uint16_t);
  auto size = argi(module);
  std::vector<common::AutoPtr<engine::JSValue>> args;
  args.resize(size, nullptr);
  for (auto i = 0; i < size; i++) {
    args[size - 1 - i] = *_ctx->stack.rbegin();
    _ctx->stack.pop_back();
  }
  auto func = *_ctx->stack.rbegin();
  _ctx->stack.pop_back();
  auto name =
      func->getProperty(ctx, L"name")->toString(ctx)->getString().value();
  compiler::JSSourceLocation::Position loc = {0, 0, 0};
  if (module->sourceMap.contains(offset)) {
    loc = module->sourceMap.at(offset);
  }
  auto pc = _pc;
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
    _pc = module->codes.size();
  } else {
    _pc = pc;
  }
}

JS_OPT(JSVirtualMachine::memberCall) {
  auto offset = _pc - sizeof(uint16_t);
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
  if (!func->isFunction()) {
    throw error::JSTypeError(
        fmt::format(L"cannot convert {} to function", func->getTypeName()));
  }
  auto name = func->getProperty(ctx, L"name")->getString().value();
  auto pc = _pc;
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
    _pc = module->codes.size();
  } else {
    _pc = pc;
  }
}

JS_OPT(JSVirtualMachine::optionalCall) {
  auto offset = _pc - sizeof(uint16_t);
  auto size = argi(module);
  std::vector<common::AutoPtr<engine::JSValue>> args;
  args.resize(size, nullptr);
  for (auto i = 0; i < size; i++) {
    args[size - 1 - i] = *_ctx->stack.rbegin();
    _ctx->stack.pop_back();
  }
  auto func = *_ctx->stack.rbegin();
  if (func->isNull() || func->isUndefined()) {
    return;
  }
  _ctx->stack.pop_back();
  auto name =
      func->getProperty(ctx, L"name")->toString(ctx)->getString().value();
  compiler::JSSourceLocation::Position loc = {0, 0, 0};
  if (module->sourceMap.contains(offset)) {
    loc = module->sourceMap.at(offset);
  }
  auto pc = _pc;
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
    _pc = module->codes.size();
  } else {
    _pc = pc;
  }
}

JS_OPT(JSVirtualMachine::memberOptionalCall) {
  auto offset = _pc - sizeof(uint16_t);
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
  if (func->isNull() || func->isUndefined()) {
    _ctx->stack.push_back(func);
    return;
  }
  if (!func->isFunction()) {
    throw error::JSTypeError(
        fmt::format(L"cannot convert {} to function", func->getTypeName()));
  }
  auto name = func->getProperty(ctx, L"name")->getString().value();
  auto pc = _pc;
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
    _pc = module->codes.size();
  } else {
    _pc = pc;
  }
}

JS_OPT(JSVirtualMachine::pow) {
  auto arg2 = *_ctx->stack.rbegin();
  _ctx->stack.pop_back();
  auto arg1 = *_ctx->stack.rbegin();
  _ctx->stack.pop_back();
  _ctx->stack.push_back(arg1->pow(ctx, arg2));
}
JS_OPT(JSVirtualMachine::mul) {
  auto arg2 = *_ctx->stack.rbegin();
  _ctx->stack.pop_back();
  auto arg1 = *_ctx->stack.rbegin();
  _ctx->stack.pop_back();
  _ctx->stack.push_back(arg1->mul(ctx, arg2));
}
JS_OPT(JSVirtualMachine::div) {
  auto arg2 = *_ctx->stack.rbegin();
  _ctx->stack.pop_back();
  auto arg1 = *_ctx->stack.rbegin();
  _ctx->stack.pop_back();
  _ctx->stack.push_back(arg1->div(ctx, arg2));
}
JS_OPT(JSVirtualMachine::mod) {
  auto arg2 = *_ctx->stack.rbegin();
  _ctx->stack.pop_back();
  auto arg1 = *_ctx->stack.rbegin();
  _ctx->stack.pop_back();
  _ctx->stack.push_back(arg1->mod(ctx, arg2));
}
JS_OPT(JSVirtualMachine::add) {
  auto arg2 = *_ctx->stack.rbegin();
  _ctx->stack.pop_back();
  auto arg1 = *_ctx->stack.rbegin();
  _ctx->stack.pop_back();
  _ctx->stack.push_back(arg1->add(ctx, arg2));
}
JS_OPT(JSVirtualMachine::sub) {
  auto arg2 = *_ctx->stack.rbegin();
  _ctx->stack.pop_back();
  auto arg1 = *_ctx->stack.rbegin();
  _ctx->stack.pop_back();
  _ctx->stack.push_back(arg1->sub(ctx, arg2));
}
JS_OPT(JSVirtualMachine::inc) {
  auto dir = argi(module);
  auto val = *_ctx->stack.rbegin();
  _ctx->stack.pop_back();
  if (dir == 1) {
    _ctx->stack.push_back(ctx->createValue(val));
  }
  val->increment(ctx);
  if (dir == 0) {
    _ctx->stack.push_back(ctx->createValue(val));
  }
}
JS_OPT(JSVirtualMachine::dec) {
  auto dir = argi(module);
  auto val = *_ctx->stack.rbegin();
  _ctx->stack.pop_back();
  if (dir == 1) {
    _ctx->stack.push_back(ctx->createValue(val));
  }
  val->decrement(ctx);
  if (dir == 0) {
    _ctx->stack.push_back(ctx->createValue(val));
  }
}
JS_OPT(JSVirtualMachine::plus) {
  auto val = *_ctx->stack.rbegin();
  _ctx->stack.pop_back();
  _ctx->stack.push_back(val->unaryPlus(ctx));
}
JS_OPT(JSVirtualMachine::netation) {
  auto val = *_ctx->stack.rbegin();
  _ctx->stack.pop_back();
  _ctx->stack.push_back(val->unaryNetation(ctx));
}
JS_OPT(JSVirtualMachine::not_) {
  auto val = *_ctx->stack.rbegin();
  _ctx->stack.pop_back();
  _ctx->stack.push_back(val->bitwiseNot(ctx));
}
JS_OPT(JSVirtualMachine::logicalNot) {
  auto val = *_ctx->stack.rbegin();
  _ctx->stack.pop_back();
  _ctx->stack.push_back(val->logicalNot(ctx));
}
JS_OPT(JSVirtualMachine::ushr) {
  auto arg2 = *_ctx->stack.rbegin();
  _ctx->stack.pop_back();
  auto arg1 = *_ctx->stack.rbegin();
  _ctx->stack.pop_back();
  _ctx->stack.push_back(arg1->ushr(ctx, arg2));
}
JS_OPT(JSVirtualMachine::shr) {
  auto arg2 = *_ctx->stack.rbegin();
  _ctx->stack.pop_back();
  auto arg1 = *_ctx->stack.rbegin();
  _ctx->stack.pop_back();
  _ctx->stack.push_back(arg1->shr(ctx, arg2));
}
JS_OPT(JSVirtualMachine::shl) {
  auto arg2 = *_ctx->stack.rbegin();
  _ctx->stack.pop_back();
  auto arg1 = *_ctx->stack.rbegin();
  _ctx->stack.pop_back();
  _ctx->stack.push_back(arg1->shl(ctx, arg2));
}
JS_OPT(JSVirtualMachine::le) {
  auto arg2 = *_ctx->stack.rbegin();
  _ctx->stack.pop_back();
  auto arg1 = *_ctx->stack.rbegin();
  _ctx->stack.pop_back();
  _ctx->stack.push_back(arg1->le(ctx, arg2));
}
JS_OPT(JSVirtualMachine::ge) {
  auto arg2 = *_ctx->stack.rbegin();
  _ctx->stack.pop_back();
  auto arg1 = *_ctx->stack.rbegin();
  _ctx->stack.pop_back();
  _ctx->stack.push_back(arg1->ge(ctx, arg2));
}
JS_OPT(JSVirtualMachine::gt) {
  auto arg2 = *_ctx->stack.rbegin();
  _ctx->stack.pop_back();
  auto arg1 = *_ctx->stack.rbegin();
  _ctx->stack.pop_back();
  _ctx->stack.push_back(arg1->gt(ctx, arg2));
}
JS_OPT(JSVirtualMachine::lt) {
  auto arg2 = *_ctx->stack.rbegin();
  _ctx->stack.pop_back();
  auto arg1 = *_ctx->stack.rbegin();
  _ctx->stack.pop_back();
  _ctx->stack.push_back(arg1->lt(ctx, arg2));
}
JS_OPT(JSVirtualMachine::seq) {
  auto arg2 = *_ctx->stack.rbegin();
  _ctx->stack.pop_back();
  auto arg1 = *_ctx->stack.rbegin();
  _ctx->stack.pop_back();
  _ctx->stack.push_back(arg1->strictEqual(ctx, arg2));
}
JS_OPT(JSVirtualMachine::sne) {
  auto arg2 = *_ctx->stack.rbegin();
  _ctx->stack.pop_back();
  auto arg1 = *_ctx->stack.rbegin();
  _ctx->stack.pop_back();
  _ctx->stack.push_back(arg1->strictNotEqual(ctx, arg2));
}
JS_OPT(JSVirtualMachine::eq) {
  auto arg2 = *_ctx->stack.rbegin();
  _ctx->stack.pop_back();
  auto arg1 = *_ctx->stack.rbegin();
  _ctx->stack.pop_back();
  _ctx->stack.push_back(arg1->equal(ctx, arg2));
}
JS_OPT(JSVirtualMachine::ne) {
  auto arg2 = *_ctx->stack.rbegin();
  _ctx->stack.pop_back();
  auto arg1 = *_ctx->stack.rbegin();
  _ctx->stack.pop_back();
  _ctx->stack.push_back(arg1->notEqual(ctx, arg2));
}
JS_OPT(JSVirtualMachine::and_) {
  auto arg2 = *_ctx->stack.rbegin();
  _ctx->stack.pop_back();
  auto arg1 = *_ctx->stack.rbegin();
  _ctx->stack.pop_back();
  _ctx->stack.push_back(arg1->and_(ctx, arg2));
}
JS_OPT(JSVirtualMachine::or_) {
  auto arg2 = *_ctx->stack.rbegin();
  _ctx->stack.pop_back();
  auto arg1 = *_ctx->stack.rbegin();
  _ctx->stack.pop_back();
  _ctx->stack.push_back(arg1->or_(ctx, arg2));
}
JS_OPT(JSVirtualMachine::xor_) {
  auto arg2 = *_ctx->stack.rbegin();
  _ctx->stack.pop_back();
  auto arg1 = *_ctx->stack.rbegin();
  _ctx->stack.pop_back();
  _ctx->stack.push_back(arg1->xor_(ctx, arg2));
}

JS_OPT(JSVirtualMachine::tryStart) {
  auto handle = argi(module);
  _ctx->errorStacks = new JSErrorFrame(_ctx->errorStacks, ctx->getScope());
  _ctx->errorStacks->handle = handle;
}

JS_OPT(JSVirtualMachine::tryEnd) {
  auto defer = _ctx->errorStacks->defer;
  _ctx->errorStacks = _ctx->errorStacks->parent;
  if (defer) {
    _ctx->deferStack.push_back(_pc);
    _pc = defer;
  }
}

JS_OPT(JSVirtualMachine::defer) {
  auto addr = argi(module);
  _ctx->errorStacks->defer = addr;
}

JS_OPT(JSVirtualMachine::deferEnd) {
  _pc = *_ctx->deferStack.rbegin();
  _ctx->deferStack.pop_back();
}

JS_OPT(JSVirtualMachine::jmp) {
  auto offset = argi(module);
  _pc = offset;
}

JS_OPT(JSVirtualMachine::jfalse) {
  auto offset = argi(module);
  auto value = *_ctx->stack.rbegin();
  if (!value->toBoolean(ctx)->getBoolean().value()) {
    _pc = offset;
  }
}

JS_OPT(JSVirtualMachine::jtrue) {
  auto offset = argi(module);
  auto value = *_ctx->stack.rbegin();
  if (value->toBoolean(ctx)->getBoolean().value()) {
    _pc = offset;
  }
}

JS_OPT(JSVirtualMachine::jnotNull) {
  auto offset = argi(module);
  auto value = *_ctx->stack.rbegin();
  if (!value->isNull() && !value->isUndefined()) {
    _pc = offset;
  }
}

JS_OPT(JSVirtualMachine::jnull) {
  auto offset = argi(module);
  auto value = *_ctx->stack.rbegin();
  if (value->isNull() || value->isUndefined()) {
    _pc = offset;
  }
}

void JSVirtualMachine::run(common::AutoPtr<engine::JSContext> ctx,
                           const common::AutoPtr<compiler::JSModule> &module,
                           size_t offset) {
  _pc = offset;
  for (;;) {
    if (_pc == module->codes.size()) {
      if (_ctx->errorStacks != nullptr) {
        auto result = *_ctx->stack.rbegin();
        if (result->getType() == engine::JSValueType::JS_TASK) {
          break;
        }
        auto handle = _ctx->errorStacks->handle;
        auto defer = _ctx->errorStacks->defer;
        auto scope = _ctx->errorStacks->scope;
        _ctx->errorStacks = _ctx->errorStacks->parent;
        scope->getRootScope()->getRoot()->appendChild(result->getStore());
        _ctx->stack.pop_back();
        while (ctx->getScope() != scope) {
          popScope(ctx, module);
        }
        _ctx->stack.push_back(result);
        if (result->getType() == engine::JSValueType::JS_EXCEPTION) {
          if (handle != 0) {
            _ctx->stack.pop_back();
            auto entity = result->getEntity<engine::JSExceptionEntity>();
            _ctx->stack.push_back(ctx->createError(result));
            _pc = handle;
          }
        }
        if (defer != 0) {
          _ctx->deferStack.push_back(_pc);
          _pc = defer;
        }
      }
    }
    if (_pc == module->codes.size()) {
      break;
    }
    try {
      auto code = next(module);
      switch (code) {
      case vm::JSAsmOperator::PUSH_NULL:
        pushNull(ctx, module);
        break;
      case vm::JSAsmOperator::PUSH_UNDEFINED:
        pushUndefined(ctx, module);
        break;
      case vm::JSAsmOperator::PUSH_TRUE:
        pushTrue(ctx, module);
        break;
      case vm::JSAsmOperator::PUSH_FALSE:
        pushFalse(ctx, module);
        break;
      case vm::JSAsmOperator::PUSH_UNINITIALIZED:
        pushUninitialized(ctx, module);
        break;
      case vm::JSAsmOperator::PUSH:
        push(ctx, module);
        break;
      case vm::JSAsmOperator::PUSH_OBJECT:
        pushObject(ctx, module);
        break;
      case vm::JSAsmOperator::PUSH_ARRAY:
        pushArray(ctx, module);
        break;
      case vm::JSAsmOperator::PUSH_FUNCTION:
        pushFunction(ctx, module);
        break;
      case vm::JSAsmOperator::PUSH_GENERATOR:
        pushGenerator(ctx, module);
        break;
      case vm::JSAsmOperator::PUSH_ARROW:
        pushArrow(ctx, module);
        break;
      case vm::JSAsmOperator::PUSH_THIS:
        pushThis(ctx, module);
        break;
      case vm::JSAsmOperator::PUSH_SUPER:
        pushSuper(ctx, module);
        break;
      case vm::JSAsmOperator::PUSH_BIGINT:
        pushBigint(ctx, module);
        break;
      case vm::JSAsmOperator::PUSH_REGEX:
        pushRegex(ctx, module);
        break;
      case vm::JSAsmOperator::PUSH_VALUE:
        pushValue(ctx, module);
        break;
      case vm::JSAsmOperator::SET_FUNC_ADDRESS:
        setAddress(ctx, module);
        break;
      case vm::JSAsmOperator::SET_FUNC_ASYNC:
        setAsync(ctx, module);
        break;
      case vm::JSAsmOperator::SET_FUNC_NAME:
        setFuncName(ctx, module);
        break;
      case vm::JSAsmOperator::SET_FUNC_LEN:
        setFuncLen(ctx, module);
        break;
      case vm::JSAsmOperator::SET_FUNC_SOURCE:
        setFuncSource(ctx, module);
        break;
      case vm::JSAsmOperator::SET_CLOSURE:
        setClosure(ctx, module);
        break;
      case vm::JSAsmOperator::SET_FIELD:
        setField(ctx, module);
        break;
      case vm::JSAsmOperator::GET_FIELD:
        getField(ctx, module);
        break;
      case vm::JSAsmOperator::GET_KEYS:
        getKeys(ctx, module);
        break;
      case vm::JSAsmOperator::SET_ACCESSOR:
        setAccessor(ctx, module);
        break;
      case vm::JSAsmOperator::MERGE:
        merge(ctx, module);
        break;
      case vm::JSAsmOperator::SET_REGEX_HAS_INDICES:
        setRegexHasIndices(ctx, module);
        break;
      case vm::JSAsmOperator::SET_REGEX_GLOBAL:
        setRegexGlobal(ctx, module);
        break;
      case vm::JSAsmOperator::SET_REGEX_IGNORE_CASES:
        setRegexIgnoreCases(ctx, module);
        break;
      case vm::JSAsmOperator::SET_REGEX_MULTILINE:
        setRegexMultiline(ctx, module);
        break;
      case vm::JSAsmOperator::SET_REGEX_DOT_ALL:
        setRegexDotAll(ctx, module);
        break;
      case vm::JSAsmOperator::SET_REGEX_STICKY:
        setRegexSticky(ctx, module);
        break;
      case vm::JSAsmOperator::POP:
        pop(ctx, module);
        break;
      case vm::JSAsmOperator::STORE_CONST:
        storeConst(ctx, module);
        break;
      case vm::JSAsmOperator::STORE:
        store(ctx, module);
        break;
      case vm::JSAsmOperator::LOAD:
        load(ctx, module);
        break;
      case vm::JSAsmOperator::LOAD_CONST:
        loadConst(ctx, module);
        break;
      case vm::JSAsmOperator::RET:
        ret(ctx, module);
        break;
      case vm::JSAsmOperator::THROW:
        throw_(ctx, module);
        break;
      case vm::JSAsmOperator::YIELD:
        yield(ctx, module);
        break;
      case vm::JSAsmOperator::YIELD_DELEGATE:
        yieldDelegate(ctx, module);
        break;
      case vm::JSAsmOperator::NEXT:
        next(ctx, module);
        break;
      case vm::JSAsmOperator::REST_ARRAY:
        restArray(ctx, module);
        break;
      case vm::JSAsmOperator::REST_OBJECT:
        restObject(ctx, module);
        break;
      case vm::JSAsmOperator::AWAIT:
        await(ctx, module);
        break;
      case vm::JSAsmOperator::VOID:
        void_(ctx, module);
        break;
      case vm::JSAsmOperator::TYPE_OF:
        typeof_(ctx, module);
        break;
      case vm::JSAsmOperator::DELETE:
        delete_(ctx, module);
        break;
      case vm::JSAsmOperator::PUSH_SCOPE:
        pushScope(ctx, module);
        break;
      case vm::JSAsmOperator::POP_SCOPE:
        popScope(ctx, module);
        break;
      case vm::JSAsmOperator::CALL:
        call(ctx, module);
        break;
      case vm::JSAsmOperator::MEMBER_CALL:
        memberCall(ctx, module);
        break;
      case vm::JSAsmOperator::OPTIONAL_CALL:
        optionalCall(ctx, module);
        break;
      case vm::JSAsmOperator::MEMBER_OPTIONAL_CALL:
        memberOptionalCall(ctx, module);
        break;
      case vm::JSAsmOperator::POW:
        pow(ctx, module);
        break;
      case vm::JSAsmOperator::MUL:
        mul(ctx, module);
        break;
      case vm::JSAsmOperator::DIV:
        div(ctx, module);
        break;
      case vm::JSAsmOperator::MOD:
        mod(ctx, module);
        break;
      case vm::JSAsmOperator::ADD:
        add(ctx, module);
        break;
      case vm::JSAsmOperator::SUB:
        sub(ctx, module);
        break;
      case vm::JSAsmOperator::INC:
        inc(ctx, module);
        break;
      case vm::JSAsmOperator::DEC:
        dec(ctx, module);
        break;
      case vm::JSAsmOperator::PLUS:
        plus(ctx, module);
        break;
      case vm::JSAsmOperator::NETA:
        netation(ctx, module);
        break;
      case vm::JSAsmOperator::NOT:
        not_(ctx, module);
        break;
      case vm::JSAsmOperator::LNOT:
        logicalNot(ctx, module);
        break;
      case vm::JSAsmOperator::USHR:
        ushr(ctx, module);
        break;
      case vm::JSAsmOperator::SHR:
        shr(ctx, module);
        break;
      case vm::JSAsmOperator::SHL:
        shl(ctx, module);
        break;
      case vm::JSAsmOperator::GE:
        ge(ctx, module);
        break;
      case vm::JSAsmOperator::LE:
        le(ctx, module);
        break;
      case vm::JSAsmOperator::GT:
        gt(ctx, module);
        break;
      case vm::JSAsmOperator::LT:
        lt(ctx, module);
        break;
      case vm::JSAsmOperator::SEQ:
        seq(ctx, module);
        break;
      case vm::JSAsmOperator::SNE:
        sne(ctx, module);
        break;
      case vm::JSAsmOperator::EQ:
        eq(ctx, module);
        break;
      case vm::JSAsmOperator::NE:
        ne(ctx, module);
        break;
      case vm::JSAsmOperator::AND:
        and_(ctx, module);
        break;
      case vm::JSAsmOperator::OR:
        or_(ctx, module);
        break;
      case vm::JSAsmOperator::XOR:
        xor_(ctx, module);
        break;
      case vm::JSAsmOperator::JMP:
        jmp(ctx, module);
        break;
      case vm::JSAsmOperator::JFALSE:
        jfalse(ctx, module);
        break;
      case vm::JSAsmOperator::JTRUE:
        jtrue(ctx, module);
        break;
      case vm::JSAsmOperator::JNOT_NULL:
        jnotNull(ctx, module);
        break;
      case vm::JSAsmOperator::JNULL:
        jnull(ctx, module);
        break;
      case vm::JSAsmOperator::TRY:
        tryStart(ctx, module);
        break;
      case vm::JSAsmOperator::DEFER:
        defer(ctx, module);
        break;
      case vm::JSAsmOperator::END_DEFER:
        deferEnd(ctx, module);
        break;
      case vm::JSAsmOperator::END_TRY:
        tryEnd(ctx, module);
        break;
      case vm::JSAsmOperator::NEW:
        new_(ctx, module);
        break;
      }
    } catch (error::JSError &e) {
      auto exp =
          ctx->createException(e.getType(), e.getMessage(), e.getLocation());
      _ctx->stack.push_back(exp);
      _pc = module->codes.size();
    }
  }
}

common::AutoPtr<engine::JSValue>
JSVirtualMachine::eval(common::AutoPtr<engine::JSContext> ctx,
                       const common::AutoPtr<compiler::JSModule> &module,
                       size_t offset) {
  auto scope = ctx->getScope();
  run(ctx, module, offset);
  auto value = ctx->undefined();
  if (!_ctx->stack.empty()) {
    value = *_ctx->stack.rbegin();
    if (ctx->getScope() != scope) {
      auto entity = value->getStore();
      value = scope->createValue(entity);
    }
  }
  while (ctx->getScope() != scope) {
    popScope(ctx, module);
  }
  return value;
}

common::AutoPtr<engine::JSValue>
JSVirtualMachine::apply(common::AutoPtr<engine::JSContext> ctx,
                        common::AutoPtr<engine::JSValue> func,
                        common::AutoPtr<engine::JSValue> self,
                        std::vector<common::AutoPtr<engine::JSValue>> args) {
  if (func->getType() == engine::JSValueType::JS_NATIVE_FUNCTION) {
    auto entity = func->getEntity<engine::JSNativeFunctionEntity>();
    auto closure = entity->getClosure();
    for (auto &[name, entity] : closure) {
      ctx->createValue(entity, name);
    }
  } else {
    auto entity = func->getEntity<engine::JSFunctionEntity>();
    if (!entity->isGenerator()) {
      auto closure = entity->getClosure();
      for (auto &[name, value] : closure) {
        ctx->createValue(value, name);
      }
    }
  }

  auto arguments = ctx->createValue(
      new engine::JSStore(new engine::JSObjectEntity(ctx->null()->getStore())),
      L"arguments");

  for (size_t index = 0; index < args.size(); index++) {
    arguments->setPropertyDescriptor(ctx, fmt::format(L"{}", index),
                                     args[index]);
  }

  arguments->setPropertyDescriptor(ctx, L"length",
                                   ctx->createNumber(args.size()));

  arguments->setPropertyDescriptor(ctx,
                                   ctx->Symbol()->getProperty(ctx, L"iterator"),
                                   ctx->Array()
                                       ->getProperty(ctx, L"prototype")
                                       ->getProperty(ctx, L"values"));
  auto bind = func->getBind(ctx);
  if (bind == nullptr) {
    bind = self;
  }
  ctx->createValue(bind, L"this");
  common::AutoPtr<engine::JSValue> result;
  if (func->getType() == engine::JSValueType::JS_NATIVE_FUNCTION) {
    auto entity = func->getEntity<engine::JSNativeFunctionEntity>();
    auto callee = entity->getCallee();
    result = callee(ctx, self, args);
  } else if (func->getType() == engine::JSValueType::JS_FUNCTION) {
    auto entity = func->getEntity<engine::JSFunctionEntity>();
    auto closure = entity->getClosure();
    if (entity->isGenerator()) {
      result = ctx->constructObject(ctx->Generator(), {}, {});
      common::AutoPtr scope = new engine::JSScope(ctx->getRoot());
      for (auto &[name, value] : closure) {
        scope->createValue(value, name);
      }
      scope->createValue(bind->getStore(), L"this");
      scope->createValue(arguments->getStore(), L"arguments");
      result->setOpaque(JSCoroutineContext{
          .eval = new JSEvalContext,
          .scope = scope,
          .module = entity->getModule(),
          .funcname = func->getName(),
          .pc = entity->getAddress(),
      });
      result->getStore()->appendChild(func->getStore());
    } else {
      auto current = _ctx;
      _ctx = new JSEvalContext;
      result = eval(ctx, entity->getModule(), entity->getAddress());
      _ctx = current;
    }
  }
  return result;
}
common::AutoPtr<JSEvalContext> JSVirtualMachine::getContext() { return _ctx; }

void JSVirtualMachine::setContext(common::AutoPtr<JSEvalContext> ctx) {
  _ctx = ctx;
}