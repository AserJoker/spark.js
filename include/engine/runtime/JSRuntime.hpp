#pragma once
#include "common/Object.hpp"
#include "engine/runtime/JSScope.hpp"
#include "engine/runtime/JSValue.hpp"
#include <string>
#include <vector>

namespace spark::engine {
class JSRuntime : public common::Object {
private:
  JSScope *_root;

  std::vector<std::wstring> _sources;

public:
  JSRuntime();
  ~JSRuntime() override;

  const std::wstring &getSourceFilename(uint32_t index);
  uint32_t setSourceFilename(const std::wstring &filename);

  JSScope *getRoot();
};
} // namespace spark::engine