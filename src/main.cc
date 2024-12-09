#include "common/AutoPtr.hpp"
#include "compiler/base/JSModule.hpp"
#include "engine/base/JSValueType.hpp"
#include "engine/entity/JSEntity.hpp"
#include "engine/runtime/JSContext.hpp"
#include "engine/runtime/JSRuntime.hpp"
#include "engine/runtime/JSValue.hpp"
#include "error/JSError.hpp"
#include "vm/JSAsmOperator.hpp"
#include <cstdint>
#include <exception>
#include <fmt/xchar.h>
#include <fstream>
#include <iostream>
#include <ostream>
#include <string>

using namespace spark;
using namespace spark::engine;

JS_FUNC(print) {
  if (args.empty()) {
    fmt::print(L"{}", ctx->undefined()->toString(ctx)->getString().value());
  } else {
    fmt::print(L"{}\n", args[0]->toString(ctx)->getString().value());
  }
  return ctx->undefined();
}

std::wstring read(const std::wstring &filename) {
  std::wifstream in("index.js", std::ios::binary);
  if (in.is_open()) {
    in.seekg(0, std::ios::end);
    size_t len = in.tellg();
    in.seekg(0, std::ios::beg);
    wchar_t *buf = new wchar_t[len + 1];
    buf[len] = 0;
    in.read(buf, len);
    in.close();
    std::wstring result = buf;
    delete[] buf;
    return result;
  } else {
    throw error::JSError(L"Cannot find module index.js");
  }
}

void write(common::AutoPtr<compiler::JSModule> module) {

  std::wofstream out("1.asm");
  out << L"[section .data]" << std::endl;
  out << L".filename \"" << module->filename << L"\"" << std::endl;
  size_t index = 0;
  for (auto &constant : module->constants) {
    std::wstring str;
    for (auto &ch : constant) {
      if (ch == L'\n') {
        str += L"\\n";
      } else if (ch == L'\r') {
        str += L"\\r";
      } else if (ch == '\"') {
        str += L"\\\"";
      } else {
        str += ch;
      }
    }
    out << L".const_" << index << " \"" << str << "\"" << std::endl;
    index++;
  }
  out << L"[section .text]" << std::endl;
  size_t offset = 0;
  auto buffer = module->codes.data();
  while (offset < module->codes.size()) {
    auto code = (vm::JSAsmOperator) * (uint16_t *)(buffer + offset);
    out << fmt::format(L"{:<8} :", offset);
    offset += sizeof(uint16_t);
    switch (code) {
    case vm::JSAsmOperator::PUSH_NULL:
      out << L"push_null";
      break;
    case vm::JSAsmOperator::PUSH_UNDEFINED:
      out << L"push_undefined";
      break;
    case vm::JSAsmOperator::PUSH_TRUE:
      out << L"push_true";
      break;
    case vm::JSAsmOperator::PUSH_FALSE:
      out << L"push_false";
      break;
    case vm::JSAsmOperator::PUSH_UNINITIALIZED:
      out << L"push_uninitialized";
      break;
    case vm::JSAsmOperator::PUSH:
      out << L"push " << *(double *)(buffer + offset);
      offset += sizeof(double);
      break;
    case vm::JSAsmOperator::PUSH_OBJECT:
      out << L"push_object";
      break;
    case vm::JSAsmOperator::PUSH_ARRAY:
      out << L"push_array";
      break;
    case vm::JSAsmOperator::PUSH_FUNCTION:
      out << L"push_function";
      break;
    case vm::JSAsmOperator::PUSH_GENERATOR:
      out << L"push_generator";
      break;
    case vm::JSAsmOperator::PUSH_ARROW:
      out << L"push_arrow";
      break;
    case vm::JSAsmOperator::PUSH_THIS:
      out << L"push_this";
      break;
    case vm::JSAsmOperator::PUSH_SUPER:
      out << L"push_super";
      break;
    case vm::JSAsmOperator::PUSH_VALUE:
      out << L"push_value " << *(uint32_t *)(buffer + offset);
      offset += sizeof(uint32_t);
      break;
    case vm::JSAsmOperator::PUSH_ARGUMENT:
      out << L"push_argument " << *(uint32_t *)(buffer + offset);
      offset += sizeof(uint32_t);
      break;
    case vm::JSAsmOperator::PUSH_BIGINT:
      out << L"push_bigint " << *(uint32_t *)(buffer + offset);
      offset += sizeof(uint32_t);
      break;
    case vm::JSAsmOperator::PUSH_REGEX:
      out << L"push_regex " << *(uint32_t *)(buffer + offset);
      offset += sizeof(uint32_t);
      break;
    case vm::JSAsmOperator::SET_FUNC_ADDRESS:
      out << L"set_address " << *(uint32_t *)(buffer + offset);
      offset += sizeof(uint32_t);
      break;
    case vm::JSAsmOperator::SET_FUNC_ASYNC:
      out << L"set_async";
      break;
    case vm::JSAsmOperator::SET_FUNC_NAME:
      out << L"set_func_name " << *(uint32_t *)(buffer + offset);
      offset += sizeof(uint32_t);
      break;
    case vm::JSAsmOperator::SET_FUNC_LEN:
      out << L"set_func_len " << *(uint32_t *)(buffer + offset);
      offset += sizeof(uint32_t);
      break;
    case vm::JSAsmOperator::SET_FUNC_SOURCE:
      out << L"set_func_source " << *(uint32_t *)(buffer + offset);
      offset += sizeof(uint32_t);
      break;
    case vm::JSAsmOperator::SET_CLOSURE:
      out << L"set_closure " << *(uint32_t *)(buffer + offset);
      offset += sizeof(uint32_t);
      break;
    case vm::JSAsmOperator::SET_FIELD:
      out << L"set_field";
      break;
    case vm::JSAsmOperator::GET_FIELD:
      out << L"get_field";
      break;
    case vm::JSAsmOperator::SET_ACCESSOR:
      out << L"set_accessor " << *(uint32_t *)(buffer + offset);
      offset += sizeof(uint32_t);
      break;
    case vm::JSAsmOperator::SET_REGEX_HAS_INDICES:
      out << L"set_regex_has_indices";
      break;
    case vm::JSAsmOperator::SET_REGEX_GLOBAL:
      out << L"set_regex_global";
      break;
    case vm::JSAsmOperator::SET_REGEX_IGNORE_CASES:
      out << L"set_regex_ignore_cases";
      break;
    case vm::JSAsmOperator::SET_REGEX_MULTILINE:
      out << L"set_regex_multiline";
      break;
    case vm::JSAsmOperator::SET_REGEX_DOT_ALL:
      out << L"set_regex_dot_all";
      break;
    case vm::JSAsmOperator::SET_REGEX_STICKY:
      out << L"set_regex_sticky";
      break;
    case vm::JSAsmOperator::POP:
      out << L"pop " << *(uint32_t *)(buffer + offset);
      offset += sizeof(uint32_t);
      break;
    case vm::JSAsmOperator::STORE_CONST:
      out << L"store_const " << *(uint32_t *)(buffer + offset);
      offset += sizeof(uint32_t);
      break;
    case vm::JSAsmOperator::STORE:
      out << L"store " << *(uint32_t *)(buffer + offset);
      offset += sizeof(uint32_t);
      break;
    case vm::JSAsmOperator::LOAD:
      out << L"load " << *(uint32_t *)(buffer + offset);
      offset += sizeof(uint32_t);
      break;
    case vm::JSAsmOperator::LOAD_CONST:
      out << L"load_const " << *(uint32_t *)(buffer + offset);
      offset += sizeof(uint32_t);
      break;
    case vm::JSAsmOperator::RET:
      out << L"ret";
      break;
    case vm::JSAsmOperator::END_DEFER:
      out << L"end_defer";
      break;
    case vm::JSAsmOperator::THROW:
      out << L"throw";
      break;
    case vm::JSAsmOperator::YIELD:
      out << L"yield";
      break;
    case vm::JSAsmOperator::YIELD_DELEGATE:
      out << L"yield_delegate";
      break;
    case vm::JSAsmOperator::VOID:
      out << L"void";
      break;
    case vm::JSAsmOperator::TYPE_OF:
      out << L"typeof";
      break;
    case vm::JSAsmOperator::AWAIT:
      out << L"await";
      break;
    case vm::JSAsmOperator::PUSH_SCOPE:
      out << L"push_scope";
      break;
    case vm::JSAsmOperator::POP_SCOPE:
      out << L"pop_scope";
      break;
    case vm::JSAsmOperator::CALL:
      out << L"call " << *(uint32_t *)(buffer + offset);
      offset += sizeof(uint32_t);
      break;
    case vm::JSAsmOperator::MEMBER_CALL:
      out << L"member_call " << *(uint32_t *)(buffer + offset);
      offset += sizeof(uint32_t);
      break;
    case vm::JSAsmOperator::JMP:
      out << L"jmp " << *(uint32_t *)(buffer + offset);
      offset += sizeof(uint32_t);
      break;
    case vm::JSAsmOperator::JFALSE:
      out << L"jfalse " << *(uint32_t *)(buffer + offset);
      offset += sizeof(uint32_t);
      break;
    case vm::JSAsmOperator::JNOT_NULL:
      out << L"jnotNull " << *(uint32_t *)(buffer + offset);
      offset += sizeof(uint32_t);
      break;
    case vm::JSAsmOperator::TRY:
      out << L"try " << *(uint32_t *)(buffer + offset);
      offset += sizeof(uint32_t);
      break;
    case vm::JSAsmOperator::DEFER:
      out << L"defer " << *(uint32_t *)(buffer + offset);
      offset += sizeof(uint32_t);
      break;
    case vm::JSAsmOperator::END_TRY:
      out << L"endtry";
      break;
    case vm::JSAsmOperator::ADD:
      out << L"add";
      break;
    case vm::JSAsmOperator::NEW:
      out << L"new " << *(uint32_t *)(buffer + offset);
      offset += sizeof(uint32_t);
      break;
    case vm::JSAsmOperator::JTRUE:
      out << L"jtrue " << *(uint32_t *)(buffer + offset);
      offset += sizeof(uint32_t);
      break;
    case vm::JSAsmOperator::POW:
      out << L"pow";
      break;
    case vm::JSAsmOperator::MUL:
      out << L"mul";
      break;
    case vm::JSAsmOperator::DIV:
      out << L"div";
      break;
    case vm::JSAsmOperator::MOD:
      out << L"mod";
      break;
    case vm::JSAsmOperator::SUB:
      out << L"sub";
      break;
    case vm::JSAsmOperator::INC:
      out << L"inc " << *(uint32_t *)(buffer + offset);
      offset += sizeof(uint32_t);
      break;
    case vm::JSAsmOperator::DEC:
      out << L"dec " << *(uint32_t *)(buffer + offset);
      offset += sizeof(uint32_t);
      break;
    case vm::JSAsmOperator::USHR:
      out << L"ushr";
      break;
    case vm::JSAsmOperator::NOT:
      out << L"not";
      break;
    case vm::JSAsmOperator::LNOT:
      out << L"lnot";
      break;
    case vm::JSAsmOperator::PLUS:
      out << L"plus";
      break;
    case vm::JSAsmOperator::NETA:
      out << L"netation";
      break;
    case vm::JSAsmOperator::SHR:
      out << L"shr";
      break;
    case vm::JSAsmOperator::SHL:
      out << L"shl";
      break;
    case vm::JSAsmOperator::GE:
      out << L"ge";
      break;
    case vm::JSAsmOperator::LE:
      out << L"le";
      break;
    case vm::JSAsmOperator::GT:
      out << L"gt";
      break;
    case vm::JSAsmOperator::LT:
      out << L"lt";
      break;
    case vm::JSAsmOperator::SEQ:
      out << L"seq";
      break;
    case vm::JSAsmOperator::SNE:
      out << L"sne";
      break;
    case vm::JSAsmOperator::EQ:
      out << L"eq";
      break;
    case vm::JSAsmOperator::NE:
      out << L"ne";
      break;
    case vm::JSAsmOperator::AND:
      out << L"and";
      break;
    case vm::JSAsmOperator::OR:
      out << L"or";
      break;
    case vm::JSAsmOperator::XOR:
      out << L"xor";
      break;
    }
    out << std::endl;
  }
  out << L"[section .map]" << std::endl;
  for (auto &[offset, mapping] : module->sourceMap) {
    out << L"." << offset << " " << mapping.column << "," << mapping.line
        << std::endl;
  }
  out.close();
}

int main(int argc, char *argv[]) {
  common::AutoPtr runtime = new engine::JSRuntime();
  try {
    common::AutoPtr ctx = new engine::JSContext(runtime);
    ctx->createNativeFunction(print, L"print", L"print");
    auto source = read(L"index.js");
    auto module = ctx->compile(source, L"index.js");
    write(module);
    auto res = ctx->getRuntime()->getVirtualMachine()->eval(ctx, module);
    if (res->getType() == spark::engine::JSValueType::JS_EXCEPTION) {
      fmt::print(L"Uncaught {}\n", res->toString(ctx)->getString().value());
    } else {
      fmt::print(L"{}\n", res->toString(ctx)->getString().value());
    }
  } catch (error::JSError &e) {
    std::cout << e.what();
    fmt::print(L"  at {} ({}:{}:{})", e.getLocation().funcname,
               runtime->getSourceFilename(e.getLocation().filename),
               e.getLocation().line + 1, e.getLocation().column + 1);
  } catch (std::exception &e) {
    std::cout << e.what() << std::endl;
  }
  return 0;
}