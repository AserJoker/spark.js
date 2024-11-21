#pragma once
#include "common/AutoPtr.hpp"
#include "common/Object.hpp"
#include "compiler/JSParser.hpp"
#include <functional>
#include <unordered_map>
#include <vector>
namespace spark::compiler {
class JSTransormer : public common::Object {
public:
  using Visitor =
      std::unordered_map<JSParser::NodeType,
                         std::function<common::AutoPtr<JSParser::Node>(
                             common::AutoPtr<JSParser::Node>)>>;

private:
  std::vector<Visitor> _visitors;

public:
};
} // namespace spark::compiler