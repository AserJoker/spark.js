#include "common/AutoPtr.hpp"
#include "engine/base/JSValueType.hpp"
#include "engine/entity/JSEntity.hpp"
#include "engine/entity/JSObjectEntity.hpp"
#include "engine/runtime/JSContext.hpp"
#include "engine/runtime/JSRuntime.hpp"
#include "engine/runtime/JSValue.hpp"
#include <exception>
#include <fmt/xchar.h>
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
    auto scope = ctx->pushScope();
    auto val1 = ctx->createObject(ctx->Null());
    auto scope2 = ctx->pushScope();
    val1->setProperty(ctx, L"test",
                      ctx->createValue(new Test(ctx->Null()->getEntity())));
    ctx->popScope(scope2);
    val1->removeProperty(ctx, L"test");
    ctx->popScope(scope);
  } catch (std::exception &e) {
    std::cout << e.what() << std::endl;
  }
  return 0;
}