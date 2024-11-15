#include "common/AutoPtr.hpp"
#include "engine/base/JSValueType.hpp"
#include "engine/runtime/JSContext.hpp"
#include "engine/runtime/JSRuntime.hpp"
#include "engine/runtime/JSValue.hpp"
#include <exception>
#include <fmt/xchar.h>
#include <iostream>

using namespace spark;

int main(int argc, char *argv[]) {
  try {
    common::AutoPtr runtime = new engine::JSRuntime();
    common::AutoPtr ctx = new engine::JSContext(runtime);
  } catch (std::exception &e) {
    std::cerr << e.what() << std::endl;
  }
  return 0;
}