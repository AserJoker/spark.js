#pragma once
namespace spark::vm {
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
  PUSH_VALUE,
  SET_FUNC_ADDRESS,
  SET_FUNC_ASYNC,
  SET_FUNC_NAME,
  SET_FUNC_LEN,
  SET_FUNC_SOURCE,
  SET_CLOSURE,
  SET_FIELD,
  GET_FIELD,
  SET_ACCESSOR,
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
  THROW,
  YIELD,
  YIELD_DELEGATE,
  AWAIT,
  VOID,
  TYPE_OF,
  NEW,
  PUSH_SCOPE,
  POP_SCOPE,
  CALL,
  MEMBER_CALL,
  JMP,
  JFALSE,
  JTRUE,
  JNOT_NULL,
  TRY,
  DEFER,
  END_DEFER,
  END_TRY,
  POW,
  MUL,
  DIV,
  MOD,
  ADD,
  SUB,
  INC,
  DEC,
  PLUS,
  NETA,
  NOT,
  LNOT,
  USHR,
  SHR,
  SHL,
  GE,
  LE,
  GT,
  LT,
  SEQ,
  SNE,
  EQ,
  NE,
  AND,
  OR,
  XOR
};
}