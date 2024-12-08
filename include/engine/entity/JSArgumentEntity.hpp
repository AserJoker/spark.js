#pragma once
#include "engine/entity/JSObjectEntity.hpp"
#include "engine/runtime/JSStore.hpp"
#include <vector>
namespace spark::engine {
class JSArgumentEntity : public JSObjectEntity {
private:
  std::vector<JSStore *> _arguments;

public:
  JSArgumentEntity(JSStore *prototype, const std::vector<JSStore *> &args);

  const std::vector<JSStore *> &getArguments() const;
};
}; // namespace spark::engine