#pragma once
namespace spark::engine {
enum class JSEvalType {
  MODULE,
  BINARY,
  FUNCTION,
  ASYNC_FUNCTION,
  GENERATOR,
  ASYNC_GENERATOR,
  EXPRESSION
};
}