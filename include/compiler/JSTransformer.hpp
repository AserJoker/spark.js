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
                         std::function<void(common::AutoPtr<JSParser::Node>)>>;

private:
  std::vector<Visitor> _visitors;

public:
  void setVisitor(const Visitor &visitor);
  void transform(common::AutoPtr<JSParser::Node> node);
};
} // namespace spark::compiler