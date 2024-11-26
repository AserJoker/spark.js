#include "common/AutoPtr.hpp"
#include "engine/base/JSValueType.hpp"
#include "engine/entity/JSEntity.hpp"
#include "engine/runtime/JSContext.hpp"
#include "engine/runtime/JSRuntime.hpp"
#include "engine/runtime/JSValue.hpp"
#include "error/JSError.hpp"
#include <exception>
#include <fmt/xchar.h>
#include <fstream>
#include <iostream>

using namespace spark;
using namespace spark::engine;
JS_FUNC(print) {
  auto arg = args[0];
  std::wcout << arg->convertToString(ctx) << std::endl;
  return ctx->undefined();
}

int main(int argc, char *argv[]) {
  try {
    common::AutoPtr runtime = new engine::JSRuntime();
    common::AutoPtr ctx = new engine::JSContext(runtime);
    ctx->createNativeFunction(&print, L"print", L"print");
    std::wifstream in("index.js", std::ios::binary);
    if (in.is_open()) {
      in.seekg(0, std::ios::end);
      size_t len = in.tellg();
      in.seekg(0, std::ios::beg);
      wchar_t *buf = new wchar_t[len + 1];
      buf[len] = 0;
      in.read(buf, len);
      in.close();
      ctx->eval(buf, L"index.js");
      delete[] buf;
    } else {
      throw error::JSError(L"Cannot find module index.js");
    }
  } catch (std::exception &e) {
    std::cout << e.what() << std::endl;
  }
  return 0;
}