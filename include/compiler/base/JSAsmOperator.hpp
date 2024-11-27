#pragma once
namespace spark::compiler {
enum class JSAsmOperator {
  PUSH_NULL = 0,
  PUSH_UNDEFINED,
  PUSH_TRUE,
  PUSH_FALSE,
  PUSH_UNINITIALIZED,
  PUSH,
  PUSH_OBJECT,
  PUSH_ARRAY,
  PUSH_FUNCTION,
  PUSH_GENERATOR,
  PUSH_ARROW,
  PUSH_THIS,
  PUSH_SUPER,
  PUSH_ARGUMENT,
  PUSH_BIGINT,
  PUSH_REGEX,
  SET_ADDRESS,
  SET_ASYNC,
  SET_FUNC_NAME,
  SET_FUNC_LEN,
  SET_CLOSURE,
  SET_FIELD,
  GET_FIELD,
  SET_ACCESSOR,
  GET_ACCESSOR,
  SET_METHOD,
  GET_METHOD,
  SET_INDEX,
  GET_INDEX,
  SET_REGEX_HAS_INDICES,
  SET_REGEX_GLOBAL,
  SET_REGEX_IGNORE_CASES,
  SET_REGEX_MULTILINE,
  SET_REGEX_DOT_ALL,
  SET_REGEX_STICKY,
  POP,
  STORE_CONST,
  STORE,
  LOAD,
  LOAD_CONST,
  RET,
  YIELD,
  AWAIT,
  NULLISH_COALESCING,
  PUSH_SCOPE,
  POP_SCOPE,
  CALL,
  JMP,
  TRY,
  DEFER,
  BACK,
  ENDTRY,
  ADD
};
}