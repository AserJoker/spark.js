#include "common/AutoPtr.hpp"
#include "engine/base/JSValueType.hpp"
#include "engine/entity/JSEntity.hpp"
#include "engine/entity/JSObjectEntity.hpp"
#include "engine/runtime/JSContext.hpp"
#include "engine/runtime/JSRuntime.hpp"
#include "engine/runtime/JSValue.hpp"
#include <exception>
#include <fmt/xchar.h>
#include <fstream>
#include <iostream>

using namespace spark;

class Test : public engine::JSObjectEntity {
public:
  Test(engine::JSEntity *proto) : engine::JSObjectEntity(proto) {
    std::cout << "Test" << std::endl;
  }
  ~Test() override { std::cout << "~Test" << std::endl; }
};

int main(int argc, char *argv[]) {
  try {
    common::AutoPtr runtime = new engine::JSRuntime();
    common::AutoPtr ctx = new engine::JSContext(runtime);
    std::wifstream in("index.js", std::ios::binary);
    in.seekg(0, std::ios::end);
    size_t len = in.tellg();
    in.seekg(0, std::ios::beg);
    wchar_t *buf = new wchar_t[len + 1];
    buf[len] = 0;
    in.read(buf, len);
    in.close();
    ctx->eval(buf, L"index.js");
    delete[] buf;
  } catch (std::exception &e) {
    std::cout << e.what() << std::endl;
  }
  return 0;
}