#pragma once
namespace spark::engine {
enum class JSUnaryOperatorType {
  INC = 0,
  DEC,
  BNOT,
  LNOT,
  ADD,
  NEG
};
}