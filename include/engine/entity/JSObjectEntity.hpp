#pragma once
#include "engine/entity/JSEntity.hpp"
namespace spark::engine {
struct JSObjectData {};
class JSObjectEntity : public JSBaseEntity<JSObjectData> {};
}; // namespace spark::engine