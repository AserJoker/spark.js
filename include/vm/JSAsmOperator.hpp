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
  PUSH_CLASS,
  PUSH_FUNCTION,
  PUSH_GENERATOR,
  PUSH_ASYNC,
  PUSH_ASYNC_GENERATOR,
  PUSH_ASYNC_ARROW,
  PUSH_ARROW,
  PUSH_THIS,
  PUSH_BIGINT,
  PUSH_REGEX,
  PUSH_VALUE,
  SET_FUNC_ADDRESS,
  SET_FUNC_NAME,
  SET_FUNC_LEN,
  SET_FUNC_SOURCE,
  SET_CLASS_INITIALIZE,
  SET_CLOSURE,
  SET_FIELD,
  SET_SUPER_FIELD,
  GET_FIELD,
  GET_SUPER_FIELD,
  GET_KEYS,
  SET_ACCESSOR,
  MERGE,
  SET_REGEX_HAS_INDICES,
  SET_REGEX_GLOBAL,
  SET_REGEX_IGNORE_CASES,
  SET_REGEX_MULTILINE,
  SET_REGEX_DOT_ALL,
  SET_REGEX_STICKY,
  POP,
  STORE,
  CREATE,
  CREATE_CONST,
  LOAD,
  LOAD_CONST,
  RET,
  HLT,
  NEXT,
  AWAIT_NEXT,
  REST_ARRAY,
  REST_OBJECT,
  THROW,
  YIELD,
  YIELD_DELEGATE,
  AWAIT,
  VOID,
  TYPE_OF,
  NEW,
  DELETE,
  PUSH_SCOPE,
  POP_SCOPE,
  CALL,
  MEMBER_CALL,
  SUPER_MEMBER_CALL,
  SUPER_CALL,
  OPTIONAL_CALL,
  MEMBER_OPTIONAL_CALL,
  JMP,
  JFALSE,
  JTRUE,
  JNOT_NULL,
  JNULL,
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
  XOR,
  INSTANCE_OF,
  IMPORT,
  IMPORT_MODULE,
  EXPORT,
};
}