#pragma once
#include "engine/entity/JSEntity.hpp"
#include "engine/entity/JSObjectEntity.hpp"
#include <vector>
namespace spark::engine {
class JSArgumentEntity : public JSObjectEntity {
private:
  std::vector<JSEntity *> _arguments;

public:
  JSArgumentEntity(JSEntity *prototype, const std::vector<JSEntity *> &args);
  
  const std::vector<JSEntity *> &getArguments() const;
};
}; // namespace spark::engine