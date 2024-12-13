#include "compiler/JSGenerator.hpp"
#include "common/AutoPtr.hpp"
#include "compiler/base/JSNode.hpp"
#include "compiler/base/JSNodeType.hpp"
#include "error/JSSyntaxError.hpp"
#include "vm/JSAsmOperator.hpp"
#include <cstdint>
#include <fmt/xchar.h>
#include <string>
#include <unordered_map>
#include <vector>

using namespace spark;
using namespace spark::compiler;
uint32_t JSGenerator::resolveConstant(JSGeneratorContext &ctx,
                                      common::AutoPtr<JSModule> &module,
                                      const std::wstring &source) {
  auto it =
      std::find(module->constants.begin(), module->constants.end(), source);
  if (it != module->constants.end()) {
    return it - module->constants.begin();
  } else {
    module->constants.push_back(source);
    return module->constants.size() - 1;
  }
}
void JSGenerator::resolveDeclaration(JSGeneratorContext &ctx,
                                     common::AutoPtr<JSModule> &module,
                                     const JSSourceDeclaration &declaration) {
  uint32_t name = resolveConstant(ctx, module, declaration.name);
  switch (declaration.type) {
  case JSSourceDeclaration::TYPE::ARGUMENT:
  case JSSourceDeclaration::TYPE::CATCH:
    return;
  case JSSourceDeclaration::TYPE::UNDEFINED:
    generate(module, vm::JSAsmOperator::PUSH_UNDEFINED);
    break;
  case JSSourceDeclaration::TYPE::UNINITIALIZED:
    generate(module, vm::JSAsmOperator::PUSH_UNINITIALIZED);
    break;
  case JSSourceDeclaration::TYPE::FUNCTION: {
    auto n = (JSFunctionDeclaration *)declaration.node;
    if (n->generator) {
      generate(module, vm::JSAsmOperator::PUSH_GENERATOR);
    } else {
      generate(module, vm::JSAsmOperator::PUSH_FUNCTION);
    }

    ctx.currentScope->functionAddr[declaration.node->id] =
        module->codes.size() + sizeof(uint16_t);

    generate(module, vm::JSAsmOperator::SET_FUNC_ADDRESS, 0U);
    generate(module, vm::JSAsmOperator::SET_FUNC_NAME, name);
    if (n->async) {
      generate(module, vm::JSAsmOperator::SET_FUNC_ASYNC, 1U);
    }
    generate(
        module, vm::JSAsmOperator::SET_FUNC_SOURCE,
        resolveConstant(ctx, module, n->location.getSource(module->source)));
    break;
  }
  }
  if (declaration.isConst) {
    generate(module, vm::JSAsmOperator::STORE_CONST, name);
  } else {
    generate(module, vm::JSAsmOperator::STORE, name);
  }
}

void JSGenerator::resolveClosure(JSGeneratorContext &ctx,
                                 common::AutoPtr<JSModule> &module,
                                 const JSSourceDeclaration &declaration) {
  uint32_t name = resolveConstant(ctx, module, declaration.name);
  std::vector<JSSourceScope *> workflow = {
      declaration.node->scope.getRawPointer()};
  std::vector<uint32_t> closure;
  while (!workflow.empty()) {
    auto item = *workflow.begin();
    workflow.erase(workflow.begin());
    for (auto &binding : item->bindings) {
      if (binding.name == L"arguments" || binding.name == L"this" ||
          binding.name == L"super") {
        continue;
      }
      auto it = item->declarations.begin();
      for (; it != item->declarations.end(); it++) {
        if (&*it == binding.declaration) {
          break;
        }
      }
      if (it == item->declarations.end()) {
        closure.push_back(resolveConstant(ctx, module, binding.name));
      }
    }
    for (auto &child : item->children) {
      workflow.push_back(child);
    }
  }
  if (!closure.empty()) {
    generate(module, vm::JSAsmOperator::LOAD, name);
    for (auto &item : closure) {
      generate(module, vm::JSAsmOperator::SET_CLOSURE, item);
    }
    generate(module, vm::JSAsmOperator::POP, 1U);
  }
}

void JSGenerator::pushLexScope(JSGeneratorContext &ctx,
                               common::AutoPtr<JSModule> &module,
                               const common::AutoPtr<JSSourceScope> &scope) {
  auto s = new JSLexScope;
  s->parent = ctx.currentScope;
  ctx.currentScope = s;
  pushScope(ctx, module, scope);
}

void JSGenerator::popLexScope(JSGeneratorContext &ctx,
                              common::AutoPtr<JSModule> &module) {
  auto old = ctx.currentScope;
  ctx.currentScope = ctx.currentScope->parent;
  delete old;
  popScope(ctx, module);
}

void JSGenerator::pushScope(JSGeneratorContext &ctx,
                            common::AutoPtr<JSModule> &module,
                            const common::AutoPtr<JSSourceScope> &scope) {
  generate(module, vm::JSAsmOperator::PUSH_SCOPE);
  std::vector<JSSourceDeclaration> closures;
  for (auto &declar : scope->declarations) {
    resolveDeclaration(ctx, module, declar);
    if (declar.type == JSSourceDeclaration::TYPE::FUNCTION) {
      closures.push_back(declar);
    }
  }
  for (auto &declar : closures) {
    resolveClosure(ctx, module, declar);
  }
  ctx.scopeChain++;
}

void JSGenerator::popScope(JSGeneratorContext &ctx,
                           common::AutoPtr<JSModule> &module) {
  ctx.scopeChain--;
  generate(module, vm::JSAsmOperator::POP_SCOPE);
}

void JSGenerator::resolveMemberChian(JSGeneratorContext &ctx,
                                     common::AutoPtr<JSModule> &module,
                                     const common::AutoPtr<JSNode> &node,
                                     std::vector<size_t> &offsets) {
  auto n = node.cast<JSBinaryExpression>();
  if (node->type == JSNodeType::EXPRESSION_MEMBER) {
    resolveNode(ctx, module, n->left);
    generate(module, vm::JSAsmOperator::LOAD_CONST,
             resolveConstant(ctx, module,
                             n->right.cast<JSIdentifierLiteral>()->value));
    generate(module, vm::JSAsmOperator::GET_FIELD);
  } else if (node->type == JSNodeType::EXPRESSION_COMPUTED_MEMBER) {
    resolveNode(ctx, module, n->left);
    resolveNode(ctx, module, n->right);
    generate(module, vm::JSAsmOperator::GET_FIELD);
  } else if (node->type == JSNodeType::EXPRESSION_OPTIONAL_MEMBER) {
    resolveNode(ctx, module, n->left);
    offsets.push_back(module->codes.size() + sizeof(uint16_t));
    generate(module, vm::JSAsmOperator::JNULL, 0U);
    generate(module, vm::JSAsmOperator::LOAD_CONST,
             resolveConstant(ctx, module,
                             n->right.cast<JSIdentifierLiteral>()->value));
    generate(module, vm::JSAsmOperator::GET_FIELD);
  } else if (node->type == JSNodeType::EXPRESSION_OPTIONAL_COMPUTED_MEMBER) {
    resolveNode(ctx, module, n->left);
    offsets.push_back(module->codes.size() + sizeof(uint16_t));
    generate(module, vm::JSAsmOperator::JNULL, 0U);
    resolveNode(ctx, module, n->right);
    generate(module, vm::JSAsmOperator::GET_FIELD);
  } else {
    resolveNode(ctx, module, node);
  }
}

void JSGenerator::resolvePrivateName(JSGeneratorContext &ctx,
                                     common::AutoPtr<JSModule> &module,
                                     const common::AutoPtr<JSNode> &node) {
  // TODO:
}

void JSGenerator::resolveLiteralRegex(JSGeneratorContext &ctx,
                                      common::AutoPtr<JSModule> &module,
                                      const common::AutoPtr<JSNode> &node) {
  auto n = node.cast<JSRegexLiteral>();
  generate(module, vm::JSAsmOperator::PUSH_REGEX,
           resolveConstant(ctx, module, n->value));

  if (n->hasIndices) {
    generate(module, vm::JSAsmOperator::PUSH_TRUE);
    generate(module, vm::JSAsmOperator::SET_REGEX_HAS_INDICES);
  }
  if (n->global) {
    generate(module, vm::JSAsmOperator::PUSH_TRUE);
    generate(module, vm::JSAsmOperator::SET_REGEX_GLOBAL);
  }
  if (n->ignoreCase) {
    generate(module, vm::JSAsmOperator::PUSH_TRUE);
    generate(module, vm::JSAsmOperator::SET_REGEX_IGNORE_CASES);
  }
  if (n->multiline) {
    generate(module, vm::JSAsmOperator::PUSH_TRUE);
    generate(module, vm::JSAsmOperator::SET_REGEX_MULTILINE);
  }
  if (n->dotAll) {
    generate(module, vm::JSAsmOperator::PUSH_TRUE);
    generate(module, vm::JSAsmOperator::SET_REGEX_DOT_ALL);
  }
  if (n->sticky) {
    generate(module, vm::JSAsmOperator::PUSH_TRUE);
    generate(module, vm::JSAsmOperator::SET_REGEX_STICKY);
  }
}

void JSGenerator::resolveLiteralNull(JSGeneratorContext &ctx,
                                     common::AutoPtr<JSModule> &module,
                                     const common::AutoPtr<JSNode> &node) {
  generate(module, vm::JSAsmOperator::PUSH_NULL);
}

void JSGenerator::resolveLiteralString(JSGeneratorContext &ctx,
                                       common::AutoPtr<JSModule> &module,
                                       const common::AutoPtr<JSNode> &node) {
  generate(module, vm::JSAsmOperator::LOAD_CONST,
           resolveConstant(ctx, module, node.cast<JSStringLiteral>()->value));
}

void JSGenerator::resolveLiteralBoolean(JSGeneratorContext &ctx,
                                        common::AutoPtr<JSModule> &module,
                                        const common::AutoPtr<JSNode> &node) {
  auto n = node.cast<JSBooleanLiteral>();
  if (n->value) {
    generate(module, vm::JSAsmOperator::PUSH_TRUE);
  } else {
    generate(module, vm::JSAsmOperator::PUSH_FALSE);
  }
}

void JSGenerator::resolveLiteralNumber(JSGeneratorContext &ctx,
                                       common::AutoPtr<JSModule> &module,
                                       const common::AutoPtr<JSNode> &node) {
  auto n = node.cast<JSNumberLiteral>();
  generate(module, vm::JSAsmOperator::PUSH, n->value);
}

void JSGenerator::resolveLiteralUndefined(JSGeneratorContext &ctx,
                                          common::AutoPtr<JSModule> &module,
                                          const common::AutoPtr<JSNode> &node) {
  generate(module, vm::JSAsmOperator::PUSH_UNDEFINED);
}

void JSGenerator::resolveLiteralIdentity(JSGeneratorContext &ctx,
                                         common::AutoPtr<JSModule> &module,
                                         const common::AutoPtr<JSNode> &node) {
  auto n = node.cast<JSIdentifierLiteral>();
  generate(module, vm::JSAsmOperator::LOAD,
           resolveConstant(ctx, module, n->value));
}

void JSGenerator::resolveLiteralTemplate(JSGeneratorContext &ctx,
                                         common::AutoPtr<JSModule> &module,
                                         const common::AutoPtr<JSNode> &node) {
  auto n = node.cast<JSTemplateLiteral>();
  std::vector<size_t> offsets;
  if (n->tag != nullptr) {
    auto opt = vm::JSAsmOperator::CALL;
    if (n->tag->type == JSNodeType::EXPRESSION_MEMBER) {
      auto m = n->tag.cast<JSMemberExpression>();
      resolveMemberChian(ctx, module, m->left, offsets);
      if (!offsets.empty()) {
        throw error::JSSyntaxError(L"Invalid optional chian for template");
      }
      generate(module, vm::JSAsmOperator::LOAD_CONST,
               resolveConstant(ctx, module,
                               m->right.cast<JSIdentifierLiteral>()->value));
      opt = vm::JSAsmOperator::MEMBER_CALL;
    } else if (n->tag->type == JSNodeType::EXPRESSION_COMPUTED_MEMBER) {
      auto m = n->tag.cast<JSComputedMemberExpression>();
      resolveMemberChian(ctx, module, m->left, offsets);
      if (!offsets.empty()) {
        throw error::JSSyntaxError(L"Invalid optional chian for template");
      }
      resolveNode(ctx, module, m->right);
      opt = vm::JSAsmOperator::MEMBER_CALL;
    } else if (n->tag->type == JSNodeType::EXPRESSION_OPTIONAL_MEMBER ||
               n->tag->type ==
                   JSNodeType::EXPRESSION_OPTIONAL_COMPUTED_MEMBER) {
      throw error::JSSyntaxError(L"Invalid optional chian for template");
    } else {
      resolveNode(ctx, module, n->tag);
    }
    generate(module, vm::JSAsmOperator::PUSH_ARRAY);
    for (size_t index = 0; index < n->quasis.size(); index++) {
      generate(module, vm::JSAsmOperator::LOAD_CONST,
               resolveConstant(ctx, module, n->quasis[index]));
      generate(module, vm::JSAsmOperator::PUSH, (double)index);
      generate(module, vm::JSAsmOperator::SET_FIELD);
      generate(module, vm::JSAsmOperator::POP, 1U);
    }
    generate(module, vm::JSAsmOperator::PUSH_ARRAY);
    for (size_t index = 0; index < n->quasis.size(); index++) {
      std::wstring raw;
      for (auto &ch : n->quasis[index]) {
        if (ch == '\n') {
          raw += L"\\n";
        } else if (ch == '\r') {
          raw += L"\\r";
        } else if (ch == '\t') {
          raw += L"\\t";
        } else {
          raw += ch;
        }
      }
      generate(module, vm::JSAsmOperator::LOAD_CONST,
               resolveConstant(ctx, module, raw));
      generate(module, vm::JSAsmOperator::PUSH, (double)index);
      generate(module, vm::JSAsmOperator::SET_FIELD);
      generate(module, vm::JSAsmOperator::POP, 1U);
    }
    generate(module, vm::JSAsmOperator::LOAD_CONST,
             resolveConstant(ctx, module, L"raw"));
    generate(module, vm::JSAsmOperator::SET_FIELD);
    generate(module, vm::JSAsmOperator::POP, 1U);
    for (auto &exp : n->expressions) {
      resolveNode(ctx, module, exp);
    }
    module->sourceMap[module->codes.size()] = n->tag->location.end;
    generate(module, opt, (uint32_t)n->expressions.size() + 1);
  } else {
    generate(module, vm::JSAsmOperator::LOAD_CONST,
             resolveConstant(ctx, module, n->quasis[0]));
    if (n->quasis.size() > 1) {
      for (size_t i = 0; i < n->expressions.size(); i++) {
        resolveNode(ctx, module, n->expressions[i]);
        generate(module, vm::JSAsmOperator::ADD);
        generate(module, vm::JSAsmOperator::LOAD_CONST,
                 resolveConstant(ctx, module, n->quasis[i + 1]));
      }
      generate(module, vm::JSAsmOperator::ADD);
    }
  }
}

void JSGenerator::resolveLiteralBigint(JSGeneratorContext &ctx,
                                       common::AutoPtr<JSModule> &module,
                                       const common::AutoPtr<JSNode> &node) {
  auto n = node.cast<JSBigIntLiteral>();
  generate(module, vm::JSAsmOperator::PUSH_BIGINT,
           resolveConstant(ctx, module, n->value));
}

void JSGenerator::resolveThis(JSGeneratorContext &ctx,
                              common::AutoPtr<JSModule> &module,
                              const common::AutoPtr<JSNode> &node) {
  generate(module, vm::JSAsmOperator::LOAD,
           resolveConstant(ctx, module, L"this"));
}

void JSGenerator::resolveSuper(JSGeneratorContext &ctx,
                               common::AutoPtr<JSModule> &module,
                               const common::AutoPtr<JSNode> &node) {
  generate(module, vm::JSAsmOperator::LOAD,
           resolveConstant(ctx, module, L"super"));
}

void JSGenerator::resolveProgram(JSGeneratorContext &ctx,
                                 common::AutoPtr<JSModule> &module,
                                 const common::AutoPtr<JSNode> &node) {
  pushLexScope(ctx, module, node->scope);
  auto n = node.cast<JSProgram>();
  for (auto &dec : n->directives) {
    resolveNode(ctx, module, dec);
  }
  for (auto &item : n->body) {
    resolveNode(ctx, module, item);
  }
  generate(module, vm::JSAsmOperator::PUSH_UNDEFINED);
  generate(module, vm::JSAsmOperator::RET);
  for (auto &item : ctx.currentScope->functionDeclarations) {
    resolveDeclarationFunction(ctx, module, item);
  }
  popLexScope(ctx, module);
}

void JSGenerator::resolveStatementBlock(JSGeneratorContext &ctx,
                                        common::AutoPtr<JSModule> &module,
                                        const common::AutoPtr<JSNode> &node) {
  auto n = node.cast<JSBlockStatement>();
  pushScope(ctx, module, n->scope);
  for (auto &sts : n->body) {
    resolveNode(ctx, module, sts);
  }
  popScope(ctx, module);
}

void JSGenerator::resolveStatementDebugger(
    JSGeneratorContext &ctx, common::AutoPtr<JSModule> &module,
    const common::AutoPtr<JSNode> &node) {
  // TODO:
}

void JSGenerator::resolveStatementReturn(JSGeneratorContext &ctx,
                                         common::AutoPtr<JSModule> &module,
                                         const common::AutoPtr<JSNode> &node) {
  auto n = node.cast<JSReturnStatement>();
  if (n->value != nullptr) {
    resolveNode(ctx, module, n->value);
  } else {
    generate(module, vm::JSAsmOperator::PUSH_UNDEFINED);
  }
  generate(module, vm::JSAsmOperator::RET);
}

void JSGenerator::resolveExpressionYield(JSGeneratorContext &ctx,
                                         common::AutoPtr<JSModule> &module,
                                         const common::AutoPtr<JSNode> &node) {
  auto n = node.cast<JSYieldExpression>();
  if (n->value != nullptr) {
    resolveNode(ctx, module, n->value);
  } else {
    generate(module, vm::JSAsmOperator::PUSH_UNDEFINED);
  }
  generate(module, vm::JSAsmOperator::YIELD);
}

void JSGenerator::resolveExpressionYieldDelegate(
    JSGeneratorContext &ctx, common::AutoPtr<JSModule> &module,
    const common::AutoPtr<JSNode> &node) {
  auto n = node.cast<JSYieldDelegateExpression>();
  resolveNode(ctx, module, n->value);
  generate(module, vm::JSAsmOperator::PUSH_UNDEFINED); // generator
  generate(module, vm::JSAsmOperator::PUSH_UNDEFINED); // argument
  generate(module, vm::JSAsmOperator::YIELD_DELEGATE);
}

void JSGenerator::resolveStatementLabel(JSGeneratorContext &ctx,
                                        common::AutoPtr<JSModule> &module,
                                        const common::AutoPtr<JSNode> &node) {
  auto n = node.cast<JSLabelStatement>();
  auto label = n->label.cast<JSIdentifierLiteral>()->value;
  if (n->statement->type == JSNodeType::STATEMENT_FOR) {
    resolveStatementFor(ctx, module, n->statement, label);
  } else if (n->statement->type == JSNodeType::STATEMENT_FOR_IN) {
    resolveStatementForIn(ctx, module, n->statement, label);
  } else if (n->statement->type == JSNodeType::STATEMENT_FOR_OF) {
    resolveStatementForOf(ctx, module, n->statement, label);
  } else if (n->statement->type == JSNodeType::STATEMENT_FOR_AWAIT_OF) {
    resolveStatementForAwaitOf(ctx, module, n->statement, label);
  } else if (n->statement->type == JSNodeType::STATEMENT_WHILE) {
    resolveStatementWhile(ctx, module, n->statement, label);
  } else if (n->statement->type == JSNodeType::STATEMENT_DO_WHILE) {
    resolveStatementWhile(ctx, module, n->statement, label);
  } else {
    _labels.push_back({{label, ctx.scopeChain}, {}});
    resolveNode(ctx, module, n->statement);
    auto &chunk = *_labels.rbegin();
    for (auto &[jmpnode, offset] : chunk.second) {
      *(uint32_t *)(module->codes.data() + offset) =
          (uint32_t)module->codes.size();
    }
    _labels.pop_back();
  }
}

void JSGenerator::resolveStatementBreak(JSGeneratorContext &ctx,
                                        common::AutoPtr<JSModule> &module,
                                        const common::AutoPtr<JSNode> &node) {
  auto n = node.cast<JSBreakStatement>();
  std::wstring label;
  if (n->label != nullptr) {
    label = n->label.cast<JSIdentifierLiteral>()->value;
  }
  auto it = _labels.rbegin();
  for (; it != _labels.rend(); it++) {
    if (it->first.first == label) {
      while (ctx.scopeChain != it->first.second) {
        popScope(ctx, module);
      }
      it->second.push_back({(JSNode *)node.getRawPointer(),
                            module->codes.size() + sizeof(uint16_t)});
      break;
    }
  }
  if (it == _labels.rend()) {
    if (label.empty()) {
      throw error::JSSyntaxError(L"Illegal break statement");
    } else {
      throw error::JSSyntaxError(fmt::format(L"Undefined label '{}'", label));
    }
  }
  generate(module, vm::JSAsmOperator::JMP, 0U);
}

void JSGenerator::resolveStatementContinue(
    JSGeneratorContext &ctx, common::AutoPtr<JSModule> &module,
    const common::AutoPtr<JSNode> &node) {
  auto n = node.cast<JSContinueStatement>();
  std::wstring label;
  if (n->label != nullptr) {
    label = n->label.cast<JSIdentifierLiteral>()->value;
  }
  auto it = _labels.rbegin();
  for (; it != _labels.rend(); it++) {
    if (it->first.first == label) {
      while (it->first.second != ctx.scopeChain) {
        popScope(ctx, module);
      }
      it->second.push_back({(JSNode *)node.getRawPointer(),
                            module->codes.size() + sizeof(uint16_t)});
      break;
    }
  }
  if (it == _labels.rend()) {
    if (label.empty()) {
      throw error::JSSyntaxError(L"Illegal continue statement");
    } else {
      throw error::JSSyntaxError(fmt::format(L"Undefined label '{}'", label));
    }
  }
  generate(module, vm::JSAsmOperator::JMP, 0U);
}

void JSGenerator::resolveStatementIf(JSGeneratorContext &ctx,
                                     common::AutoPtr<JSModule> &module,
                                     const common::AutoPtr<JSNode> &node) {
  auto n = node.cast<JSIfStatement>();
  resolveNode(ctx, module, n->condition);
  size_t alt = module->codes.size() + sizeof(uint16_t);
  generate(module, vm::JSAsmOperator::JFALSE, 0U);
  resolveNode(ctx, module, n->alternate);
  if (n->consequent != nullptr) {
    size_t end = module->codes.size() + sizeof(uint16_t);
    generate(module, vm::JSAsmOperator::JMP, 0U);
    *(uint32_t *)(module->codes.data() + alt) = (uint32_t)module->codes.size();
    resolveNode(ctx, module, n->consequent);
    *(uint32_t *)(module->codes.data() + end) = (uint32_t)module->codes.size();
  } else {
    *(uint32_t *)(module->codes.data() + alt) = (uint32_t)module->codes.size();
  }
  generate(module, vm::JSAsmOperator::POP, 1U);
}

void JSGenerator::resolveStatementSwitch(JSGeneratorContext &ctx,
                                         common::AutoPtr<JSModule> &module,
                                         const common::AutoPtr<JSNode> &node) {
  auto n = node.cast<JSSwitchStatement>();
  _labels.push_back({{L"", ctx.scopeChain}, {}});
  resolveNode(ctx, module, n->expression);
  std::vector<size_t> offsets;
  common::AutoPtr<JSNode> default_;
  for (auto &ca : n->cases) {
    auto c = ca.cast<JSSwitchCaseStatement>();
    if (c->match != nullptr) {
      generate(module, vm::JSAsmOperator::PUSH_VALUE, 1U);
      resolveNode(ctx, module, c->match);
      generate(module, vm::JSAsmOperator::SEQ);
      offsets.push_back(module->codes.size() + sizeof(uint16_t));
      generate(module, vm::JSAsmOperator::JTRUE, 0U);
      generate(module, vm::JSAsmOperator::POP, 1U);
    } else {
      default_ = c;
    }
  }
  if (default_ != nullptr) {
    generate(module, vm::JSAsmOperator::PUSH_UNDEFINED);
    offsets.push_back(module->codes.size() + sizeof(uint16_t));
    generate(module, vm::JSAsmOperator::JMP, 0U);
  }
  generate(module, vm::JSAsmOperator::POP, 1U);
  auto endOffset = module->codes.size() + sizeof(uint16_t);
  generate(module, vm::JSAsmOperator::JMP, 0U);
  size_t index = 0;
  for (auto &cc : n->cases) {
    generate(module, vm::JSAsmOperator::POP, 2U);
    auto c = cc.cast<JSSwitchCaseStatement>();
    if (c->match != nullptr) {
      *(uint32_t *)(module->codes.data() + offsets[index]) =
          module->codes.size();
      for (auto &sts : c->statements) {
        resolveNode(ctx, module, sts);
      }
      index++;
    } else {
      *(uint32_t *)(module->codes.data() + *offsets.rbegin()) =
          module->codes.size();
      auto c = default_.cast<JSSwitchCaseStatement>();
      for (auto &sts : c->statements) {
        resolveNode(ctx, module, sts);
      }
    }
  }

  auto &chunk = *_labels.rbegin();
  *(uint32_t *)(module->codes.data() + endOffset) =
      (uint32_t)module->codes.size();
  for (auto &[node, offset] : chunk.second) {
    if (node->type == JSNodeType::STATEMENT_CONTINUE) {
      throw error::JSSyntaxError(L"invalid continue");
    } else {
      *(uint32_t *)(module->codes.data() + offset) =
          (uint32_t)module->codes.size();
    }
  }
  _labels.pop_back();
}

void JSGenerator::resolveStatementThrow(JSGeneratorContext &ctx,
                                        common::AutoPtr<JSModule> &module,
                                        const common::AutoPtr<JSNode> &node) {
  auto n = node.cast<JSThrowStatement>();
  if (n->value != nullptr) {
    resolveNode(ctx, module, n->value);
  } else {
    generate(module, vm::JSAsmOperator::PUSH_UNDEFINED);
  }
  generate(module, vm::JSAsmOperator::THROW);
}

void JSGenerator::resolveStatementTry(JSGeneratorContext &ctx,
                                      common::AutoPtr<JSModule> &module,
                                      const common::AutoPtr<JSNode> &node) {
  auto n = node.cast<JSTryStatement>();
  auto catchStart = (module->codes.size() + sizeof(uint16_t));
  generate(module, vm::JSAsmOperator::TRY, 0U);
  auto finallyStart = (module->codes.size() + sizeof(uint16_t));
  if (n->finally != nullptr) {
    generate(module, vm::JSAsmOperator::DEFER, 0U);
  }
  resolveNode(ctx, module, n->try_);
  generate(module, vm::JSAsmOperator::END_TRY);
  auto catchEnd = (module->codes.size() + sizeof(uint16_t));
  generate(module, vm::JSAsmOperator::JMP, 0U);
  if (n->catch_ != nullptr) {
    *(uint32_t *)(module->codes.data() + catchStart) =
        (uint32_t)(module->codes.size());
    resolveNode(ctx, module, n->catch_);
  }
  *(uint32_t *)(module->codes.data() + catchEnd) =
      (uint32_t)(module->codes.size());
  if (n->finally != nullptr) {
    auto finallyEnd = (module->codes.size() + sizeof(uint16_t));
    generate(module, vm::JSAsmOperator::JMP, 0U);
    *(uint32_t *)(module->codes.data() + finallyStart) =
        (uint32_t)(module->codes.size());
    if (n->finally != nullptr) {
      resolveNode(ctx, module, n->finally);
    }
    generate(module, vm::JSAsmOperator::END_DEFER);
    *(uint32_t *)(module->codes.data() + finallyEnd) =
        (uint32_t)(module->codes.size());
  }
}

void JSGenerator::resolveStatementTryCatch(
    JSGeneratorContext &ctx, common::AutoPtr<JSModule> &module,
    const common::AutoPtr<JSNode> &node) {
  auto n = node.cast<JSTryCatchStatement>();
  generate(module, vm::JSAsmOperator::PUSH_SCOPE);
  if (n->binding != nullptr) {
    generate(module, vm::JSAsmOperator::STORE,
             resolveConstant(ctx, module,
                             n->binding.cast<JSIdentifierLiteral>()->value));
  }
  resolveNode(ctx, module, n->statement);
  generate(module, vm::JSAsmOperator::POP_SCOPE);
}

void JSGenerator::resolveStatementWhile(JSGeneratorContext &ctx,
                                        common::AutoPtr<JSModule> &module,
                                        const common::AutoPtr<JSNode> &node,
                                        const std::wstring &label) {
  auto n = node.cast<JSWhileStatement>();
  _labels.push_back({{label, ctx.scopeChain}, {}});
  auto start = (uint32_t)module->codes.size();
  resolveNode(ctx, module, n->condition);
  auto endOffset = module->codes.size() + sizeof(uint16_t);
  generate(module, vm::JSAsmOperator::JFALSE, 0U);
  generate(module, vm::JSAsmOperator::POP, 1U);
  resolveNode(ctx, module, n->body);
  generate(module, vm::JSAsmOperator::JMP, start);
  auto end = (uint32_t)module->codes.size();
  *(uint32_t *)(module->codes.data() + endOffset) = end;
  generate(module, vm::JSAsmOperator::POP, 1U);
  auto &chunk = *_labels.rbegin();
  for (auto &[node, offset] : chunk.second) {
    if (node->type == JSNodeType::STATEMENT_BREAK) {
      *(uint32_t *)(module->codes.data() + offset) = end;
    } else {
      *(uint32_t *)(module->codes.data() + offset) = start;
    }
  }
  _labels.pop_back();
}

void JSGenerator::resolveStatementDoWhile(JSGeneratorContext &ctx,
                                          common::AutoPtr<JSModule> &module,
                                          const common::AutoPtr<JSNode> &node,
                                          const std::wstring &label) {
  auto n = node.cast<JSDoWhileStatement>();
  _labels.push_back({{label, ctx.scopeChain}, {}});
  generate(module, vm::JSAsmOperator::PUSH_UNDEFINED);
  auto start = (uint32_t)module->codes.size();
  generate(module, vm::JSAsmOperator::POP, 1U);
  resolveNode(ctx, module, n->body);
  resolveNode(ctx, module, n->condition);
  auto condition = (uint32_t)module->codes.size();
  generate(module, vm::JSAsmOperator::JTRUE, start);
  generate(module, vm::JSAsmOperator::POP, 1U);
  auto end = (uint32_t)module->codes.size();
  auto &chunk = *_labels.rbegin();
  for (auto &[node, offset] : chunk.second) {
    if (node->type == JSNodeType::STATEMENT_BREAK) {
      *(uint32_t *)(module->codes.data() + offset) = end;
    } else {
      *(uint32_t *)(module->codes.data() + offset) = condition;
    }
  }
  _labels.pop_back();
}

void JSGenerator::resolveStatementFor(JSGeneratorContext &ctx,
                                      common::AutoPtr<JSModule> &module,
                                      const common::AutoPtr<JSNode> &node,
                                      const std::wstring &label) {
  auto n = node.cast<JSForStatement>();
  _labels.push_back({{label, ctx.scopeChain}, {}});
  pushScope(ctx, module, n->scope);
  if (n->init != nullptr) {
    resolveNode(ctx, module, n->init);
  }
  auto start = (uint32_t)module->codes.size();
  if (n->condition != nullptr) {
    resolveNode(ctx, module, n->condition);
  } else {
    generate(module, vm::JSAsmOperator::PUSH_TRUE);
  }
  auto endOffset = module->codes.size() + sizeof(uint16_t);
  generate(module, vm::JSAsmOperator::JFALSE, 0U);
  generate(module, vm::JSAsmOperator::POP, 1U);
  resolveNode(ctx, module, n->body);
  if (n->update != nullptr) {
    resolveNode(ctx, module, n->update);
  }
  generate(module, vm::JSAsmOperator::JMP, start);
  auto end = (uint32_t)module->codes.size();
  *(uint32_t *)(module->codes.data() + endOffset) = end;
  generate(module, vm::JSAsmOperator::POP, 1U);
  auto &chunk = *_labels.rbegin();
  for (auto &[node, offset] : chunk.second) {
    if (node->type == JSNodeType::STATEMENT_BREAK) {
      *(uint32_t *)(module->codes.data() + offset) = end;
    } else {
      *(uint32_t *)(module->codes.data() + offset) = start;
    }
  }
  popScope(ctx, module);
  _labels.pop_back();
}

void JSGenerator::resolveStatementForIn(JSGeneratorContext &ctx,
                                        common::AutoPtr<JSModule> &module,
                                        const common::AutoPtr<JSNode> &node,
                                        const std::wstring &label) {
  auto n = node.cast<JSForInStatement>();
  _labels.push_back({{label, ctx.scopeChain}, {}});
  resolveNode(ctx, module, n->expression);
  generate(module, vm::JSAsmOperator::GET_KEYS);
  generate(module, vm::JSAsmOperator::PUSH_UNDEFINED); // gen
  generate(module, vm::JSAsmOperator::PUSH_UNDEFINED); // res
  generate(module, vm::JSAsmOperator::PUSH_UNDEFINED); // done
  auto start = (uint32_t)module->codes.size();
  generate(module, vm::JSAsmOperator::POP, 1U); // remove done
  generate(module, vm::JSAsmOperator::POP, 1U); // remove res
  generate(module, vm::JSAsmOperator::NEXT);
  pushScope(ctx, module, n->scope);
  auto endOffset = module->codes.size() + sizeof(uint16_t);
  generate(module, vm::JSAsmOperator::JTRUE, 0U);
  generate(module, vm::JSAsmOperator::PUSH_VALUE, 2U);
  resolveVariableIdentifier(ctx, module, n->declaration);
  resolveNode(ctx, module, n->body);
  popScope(ctx, module);
  generate(module, vm::JSAsmOperator::JMP, start);
  auto end = (uint32_t)module->codes.size();
  *(uint32_t *)(module->codes.data() + endOffset) = end;
  popScope(ctx, module);
  generate(module, vm::JSAsmOperator::POP, 1U);
  generate(module, vm::JSAsmOperator::POP, 1U);
  generate(module, vm::JSAsmOperator::POP, 1U);
  generate(module, vm::JSAsmOperator::POP, 1U);
  end = (uint32_t)module->codes.size();
  auto &chunk = *_labels.rbegin();
  for (auto &[node, offset] : chunk.second) {
    if (node->type == JSNodeType::STATEMENT_BREAK) {
      *(uint32_t *)(module->codes.data() + offset) = end;
    } else {
      *(uint32_t *)(module->codes.data() + offset) = start;
    }
  }
  _labels.pop_back();
}

void JSGenerator::resolveStatementForOf(JSGeneratorContext &ctx,
                                        common::AutoPtr<JSModule> &module,
                                        const common::AutoPtr<JSNode> &node,
                                        const std::wstring &label) {
  auto n = node.cast<JSForOfStatement>();
  _labels.push_back({{label, ctx.scopeChain}, {}});
  resolveNode(ctx, module, n->expression);
  generate(module, vm::JSAsmOperator::PUSH_UNDEFINED); // gen
  generate(module, vm::JSAsmOperator::PUSH_UNDEFINED); // res
  generate(module, vm::JSAsmOperator::PUSH_UNDEFINED); // done
  auto start = (uint32_t)module->codes.size();
  generate(module, vm::JSAsmOperator::POP, 1U); // remove done
  generate(module, vm::JSAsmOperator::POP, 1U); // remove res
  generate(module, vm::JSAsmOperator::NEXT);
  pushScope(ctx, module, n->scope);
  auto endOffset = module->codes.size() + sizeof(uint16_t);
  generate(module, vm::JSAsmOperator::JTRUE, 0U);
  generate(module, vm::JSAsmOperator::PUSH_VALUE, 2U);
  resolveVariableIdentifier(ctx, module, n->declaration);
  resolveNode(ctx, module, n->body);
  popScope(ctx, module);
  generate(module, vm::JSAsmOperator::JMP, start);
  auto end = (uint32_t)module->codes.size();
  *(uint32_t *)(module->codes.data() + endOffset) = end;
  popScope(ctx, module);
  generate(module, vm::JSAsmOperator::POP, 1U);
  generate(module, vm::JSAsmOperator::POP, 1U);
  generate(module, vm::JSAsmOperator::POP, 1U);
  generate(module, vm::JSAsmOperator::POP, 1U);
  end = (uint32_t)module->codes.size();
  auto &chunk = *_labels.rbegin();
  for (auto &[node, offset] : chunk.second) {
    if (node->type == JSNodeType::STATEMENT_BREAK) {
      *(uint32_t *)(module->codes.data() + offset) = end;
    } else {
      *(uint32_t *)(module->codes.data() + offset) = start;
    }
  }
  _labels.pop_back();
}

void JSGenerator::resolveStatementForAwaitOf(
    JSGeneratorContext &ctx, common::AutoPtr<JSModule> &module,
    const common::AutoPtr<JSNode> &node, const std::wstring &label) {
  // TODO:
}

void JSGenerator::resolveVariableDeclaration(
    JSGeneratorContext &ctx, common::AutoPtr<JSModule> &module,
    const common::AutoPtr<JSNode> &node) {
  auto n = node.cast<JSVariableDeclaration>();
  for (auto &d : n->declarations) {
    resolveNode(ctx, module, d);
  }
}

void JSGenerator::resolveVariableDeclarator(
    JSGeneratorContext &ctx, common::AutoPtr<JSModule> &module,
    const common::AutoPtr<JSNode> &node) {
  auto n = node.cast<JSVariableDeclarator>();
  if (n->value != nullptr) {
    resolveNode(ctx, module, n->value);
  } else {
    generate(module, vm::JSAsmOperator::PUSH_UNDEFINED);
  }
  resolveVariableIdentifier(ctx, module, n->identifier);
}

void JSGenerator::resolveVariableIdentifier(
    JSGeneratorContext &ctx, common::AutoPtr<JSModule> &module,
    const common::AutoPtr<JSNode> &node) {
  std::vector<size_t> offsets;
  if (node->type == JSNodeType::LITERAL_IDENTITY) {
    auto name =
        resolveConstant(ctx, module, node.cast<JSIdentifierLiteral>()->value);
    generate(module, vm::JSAsmOperator::STORE, name);
  } else if (node->type == JSNodeType::EXPRESSION_MEMBER) {
    auto n = node.cast<JSMemberExpression>();
    resolveMemberChian(ctx, module, n->left, offsets);
    if (!offsets.empty()) {
      throw error::JSSyntaxError(L"Invalid left-hand side in assignment");
    }
    generate(module, vm::JSAsmOperator::PUSH_VALUE, 2U);
    generate(module, vm::JSAsmOperator::LOAD_CONST,
             resolveConstant(ctx, module,
                             n->right.cast<JSIdentifierLiteral>()->value));
    generate(module, vm::JSAsmOperator::SET_FIELD);
    generate(module, vm::JSAsmOperator::POP, 1U); // set_field result
    generate(module, vm::JSAsmOperator::POP, 1U); // self
    generate(module, vm::JSAsmOperator::POP, 1U); // raw_value
  } else if (node->type == JSNodeType::EXPRESSION_COMPUTED_MEMBER) {
    auto n = node.cast<JSComputedMemberExpression>();
    resolveMemberChian(ctx, module, n->left, offsets);
    if (!offsets.empty()) {
      throw error::JSSyntaxError(L"Invalid left-hand side in assignment");
    }
    generate(module, vm::JSAsmOperator::PUSH_VALUE, 2U);
    resolveNode(ctx, module, n->right);
    generate(module, vm::JSAsmOperator::SET_FIELD);
    generate(module, vm::JSAsmOperator::POP, 1U); // set_field result
    generate(module, vm::JSAsmOperator::POP, 1U); // self
    generate(module, vm::JSAsmOperator::POP, 1U); // raw_value
  } else if (node->type == JSNodeType::PATTERN_ARRAY) {
    auto arr = node.cast<JSArrayPattern>();
    generate(module, vm::JSAsmOperator::PUSH_UNDEFINED); // generator
    for (auto &item : arr->items) {
      if (item == nullptr) {
        generate(module, vm::JSAsmOperator::NEXT);
        generate(module, vm::JSAsmOperator::POP, 1U);
        generate(module, vm::JSAsmOperator::POP, 1U);
      } else if (item->type == JSNodeType::PATTERN_ARRAY_ITEM) {
        generate(module, vm::JSAsmOperator::NEXT);
        generate(module, vm::JSAsmOperator::POP, 1U);
        auto aitem = item.cast<JSArrayPatternItem>();
        if (aitem->value != nullptr) {
          auto offset = module->codes.size() + sizeof(uint16_t);
          generate(module, vm::JSAsmOperator::JNOT_NULL, 0U);
          generate(module, vm::JSAsmOperator::POP, 1U);
          resolveNode(ctx, module, aitem->value);
          *(uint32_t *)(module->codes.data() + offset) =
              (uint32_t)module->codes.size();
        }
        resolveVariableIdentifier(ctx, module, aitem->identifier);
      } else if (item->type == JSNodeType::PATTERN_REST_ITEM) {
        auto aitem = item.cast<JSRestPatternItem>();
        generate(module, vm::JSAsmOperator::REST_ARRAY);
        resolveVariableIdentifier(ctx, module, aitem->identifier);
      }
    }
    generate(module, vm::JSAsmOperator::POP, 1U); // generator
    generate(module, vm::JSAsmOperator::POP, 1U);
  } else if (node->type == JSNodeType::PATTERN_OBJECT) {
    common::AutoPtr<JSRestPatternItem> rest;
    auto obj = node.cast<JSObjectPattern>();
    uint32_t index = 0;
    for (auto &item : obj->items) {
      if (item->type == JSNodeType::PATTERN_REST_ITEM) {
        rest = item.cast<JSRestPatternItem>();
      } else {
        auto oitem = item.cast<JSObjectPatternItem>();
        if (oitem->identifier->type == JSNodeType::LITERAL_IDENTITY) {
          generate(module, vm::JSAsmOperator::LOAD_CONST,
                   resolveConstant(
                       ctx, module,
                       oitem->identifier.cast<JSIdentifierLiteral>()->value));

        } else {
          resolveNode(ctx, module, oitem->identifier);
        }
        generate(module, vm::JSAsmOperator::PUSH_VALUE, 2U + index);
        generate(module, vm::JSAsmOperator::PUSH_VALUE, 2U);
        generate(module, vm::JSAsmOperator::GET_FIELD);
        if (oitem->value != nullptr) {
          auto offset = module->codes.size() + sizeof(uint16_t);
          generate(module, vm::JSAsmOperator::JNOT_NULL, 0U);
          generate(module, vm::JSAsmOperator::POP, 1U);
          resolveNode(ctx, module, oitem->value);
          *(uint32_t *)(module->codes.data() + offset) =
              (uint32_t)module->codes.size();
        }
        resolveVariableIdentifier(ctx, module, oitem->match);
        index++;
      }
    }
    if (rest != nullptr) {
      generate(module, vm::JSAsmOperator::REST_OBJECT, index);
      resolveVariableIdentifier(ctx, module, rest->identifier);
    }
    generate(module, vm::JSAsmOperator::POP, index);
    generate(module, vm::JSAsmOperator::POP, 1U);
  } else {
    throw error::JSSyntaxError(L"Invalid left-hand side in assignment");
  }
}

void JSGenerator::resolveDirective(JSGeneratorContext &ctx,
                                   common::AutoPtr<JSModule> &module,
                                   const common::AutoPtr<JSNode> &node) {
  // TODO:
}

void JSGenerator::resolveObjectProperty(JSGeneratorContext &ctx,
                                        common::AutoPtr<JSModule> &module,
                                        const common::AutoPtr<JSNode> &node) {
  auto n = node.cast<JSObjectProperty>();
  if (n->implement != nullptr) {
    resolveNode(ctx, module, n->implement);
  } else {
    resolveNode(ctx, module, n->identifier);
  }
  if (n->identifier->type == JSNodeType::LITERAL_IDENTITY) {
    generate(module, vm::JSAsmOperator::LOAD_CONST,
             resolveConstant(ctx, module,
                             n->identifier.cast<JSIdentifierLiteral>()->value));
  } else {
    resolveNode(ctx, module, n->identifier);
  }
  generate(module, vm::JSAsmOperator::SET_FIELD);
  generate(module, vm::JSAsmOperator::POP, 1U);
}

void JSGenerator::resolveObjectMethod(JSGeneratorContext &ctx,
                                      common::AutoPtr<JSModule> &module,
                                      const common::AutoPtr<JSNode> &node) {
  auto n = node.cast<JSObjectMethod>();
  ctx.currentScope->functionDeclarations.push_back(
      (JSNode *)node.getRawPointer());
  if (n->generator) {
    generate(module, vm::JSAsmOperator::PUSH_GENERATOR);
  } else {
    generate(module, vm::JSAsmOperator::PUSH_FUNCTION);
  }
  if (n->async) {
    generate(module, vm::JSAsmOperator::SET_FUNC_ASYNC);
  }
  ctx.currentScope->functionAddr[node->id] =
      module->codes.size() + sizeof(uint16_t);
  generate(module, vm::JSAsmOperator::SET_FUNC_ADDRESS, 0U);
  generate(
      module, vm::JSAsmOperator::SET_FUNC_SOURCE,
      resolveConstant(ctx, module, node->location.getSource(module->source)));
  if (n->identifier->type == JSNodeType::LITERAL_IDENTITY) {
    generate(module, vm::JSAsmOperator::LOAD_CONST,
             resolveConstant(ctx, module,
                             n->identifier.cast<JSIdentifierLiteral>()->value));
  } else {
    resolveNode(ctx, module, n->identifier);
  }
  generate(module, vm::JSAsmOperator::SET_FIELD);
  generate(module, vm::JSAsmOperator::POP, 1U);
}

void JSGenerator::resolveObjectAccessor(JSGeneratorContext &ctx,
                                        common::AutoPtr<JSModule> &module,
                                        const common::AutoPtr<JSNode> &node) {
  auto n = node.cast<JSObjectAccessor>();
  ctx.currentScope->functionDeclarations.push_back(
      (JSNode *)node.getRawPointer());
  generate(module, vm::JSAsmOperator::PUSH_FUNCTION);
  ctx.currentScope->functionAddr[node->id] =
      module->codes.size() + sizeof(uint16_t);
  generate(module, vm::JSAsmOperator::SET_FUNC_ADDRESS, 0U);
  generate(
      module, vm::JSAsmOperator::SET_FUNC_SOURCE,
      resolveConstant(ctx, module, node->location.getSource(module->source)));
  if (n->identifier->type == JSNodeType::LITERAL_IDENTITY) {
    generate(module, vm::JSAsmOperator::LOAD_CONST,
             resolveConstant(ctx, module,
                             n->identifier.cast<JSIdentifierLiteral>()->value));
  } else {
    resolveNode(ctx, module, n->identifier);
  }
  generate(module, vm::JSAsmOperator::SET_ACCESSOR,
           n->kind == JSAccessorKind::GET ? 1U : 0U);
  generate(module, vm::JSAsmOperator::POP, 1U);
}

void JSGenerator::resolveExpressionUnary(JSGeneratorContext &ctx,
                                         common::AutoPtr<JSModule> &module,
                                         const common::AutoPtr<JSNode> &node) {
  auto n = node.cast<JSUnaryExpression>();
  resolveNode(ctx, module, n->right);
  if (n->opt == L"!") {
    generate(module, vm::JSAsmOperator::LNOT);
  } else if (n->opt == L"~") {
    generate(module, vm::JSAsmOperator::NOT);
  } else if (n->opt == L"++") {
    generate(module, vm::JSAsmOperator::INC, 0U);
  } else if (n->opt == L"--") {
    generate(module, vm::JSAsmOperator::DEC, 0U);
  } else if (n->opt == L"+") {
    generate(module, vm::JSAsmOperator::PLUS);
  } else if (n->opt == L"-") {
    generate(module, vm::JSAsmOperator::NETA);
  }
}

void JSGenerator::resolveExpressionUpdate(JSGeneratorContext &ctx,
                                          common::AutoPtr<JSModule> &module,
                                          const common::AutoPtr<JSNode> &node) {
  auto n = node.cast<JSUpdateExpression>();
  resolveNode(ctx, module, n->left);
  if (n->opt == L"++") {
    generate(module, vm::JSAsmOperator::INC, 1U);
  } else if (n->opt == L"--") {
    generate(module, vm::JSAsmOperator::DEC, 1U);
  }
}

void JSGenerator::resolveExpressionBinary(JSGeneratorContext &ctx,
                                          common::AutoPtr<JSModule> &module,
                                          const common::AutoPtr<JSNode> &node) {
  auto n = node.cast<JSBinaryExpression>();
  resolveNode(ctx, module, n->left);
  if (n->opt == L"&&" || n->opt == L"||" || n->opt == L"??") {
    generate(module, vm::JSAsmOperator::PUSH_VALUE, 1U);
    auto pin = (module->codes.size() + sizeof(uint16_t));
    if (n->opt == L"&&") {
      generate(module, vm::JSAsmOperator::JFALSE, 0U);
    } else if (n->opt == L"??") {
      generate(module, vm::JSAsmOperator::JNOT_NULL, 0U);
    } else {
      generate(module, vm::JSAsmOperator::JTRUE, 0U);
    }
    generate(module, vm::JSAsmOperator::POP, 1U);
    resolveNode(ctx, module, n->right);
    *(uint32_t *)(module->codes.data() + pin) =
        (uint32_t)(module->codes.size());
  } else {
    resolveNode(ctx, module, n->right);
    if (n->opt == L"**") {
      generate(module, vm::JSAsmOperator::POW);
    } else if (n->opt == L"*") {
      generate(module, vm::JSAsmOperator::MUL);
    } else if (n->opt == L"/") {
      generate(module, vm::JSAsmOperator::DIV);
    } else if (n->opt == L"%") {
      generate(module, vm::JSAsmOperator::MOD);
    } else if (n->opt == L"+") {
      generate(module, vm::JSAsmOperator::ADD);
    } else if (n->opt == L"-") {
      generate(module, vm::JSAsmOperator::SUB);
    } else if (n->opt == L">>>") {
      generate(module, vm::JSAsmOperator::USHR);
    } else if (n->opt == L">>") {
      generate(module, vm::JSAsmOperator::SHR);
    } else if (n->opt == L">>") {
      generate(module, vm::JSAsmOperator::SHL);
    } else if (n->opt == L">=") {
      generate(module, vm::JSAsmOperator::GE);
    } else if (n->opt == L"<=") {
      generate(module, vm::JSAsmOperator::LE);
    } else if (n->opt == L">") {
      generate(module, vm::JSAsmOperator::GT);
    } else if (n->opt == L"<") {
      generate(module, vm::JSAsmOperator::LT);
    } else if (n->opt == L"===") {
      generate(module, vm::JSAsmOperator::SEQ);
    } else if (n->opt == L"!==") {
      generate(module, vm::JSAsmOperator::SNE);
    } else if (n->opt == L"==") {
      generate(module, vm::JSAsmOperator::EQ);
    } else if (n->opt == L"!=") {
      generate(module, vm::JSAsmOperator::NE);
    } else if (n->opt == L"&") {
      generate(module, vm::JSAsmOperator::AND);
    } else if (n->opt == L"|") {
      generate(module, vm::JSAsmOperator::OR);
    } else if (n->opt == L"^") {
      generate(module, vm::JSAsmOperator::XOR);
    }
  }
}

void JSGenerator::resolveExpressionMember(JSGeneratorContext &ctx,
                                          common::AutoPtr<JSModule> &module,
                                          const common::AutoPtr<JSNode> &node) {
  std::vector<size_t> offsets;
  auto n = node.cast<JSMemberExpression>();
  resolveMemberChian(ctx, module, n->left, offsets);
  generate(module, vm::JSAsmOperator::LOAD_CONST,
           resolveConstant(ctx, module,
                           n->right.cast<JSIdentifierLiteral>()->value));
  generate(module, vm::JSAsmOperator::GET_FIELD);
  for (auto &offset : offsets) {
    *(uint32_t *)(module->codes.data() + offset) =
        (uint32_t)(module->codes.size());
  }
}

void JSGenerator::resolveExpressionOptionalMember(
    JSGeneratorContext &ctx, common::AutoPtr<JSModule> &module,
    const common::AutoPtr<JSNode> &node) {
  std::vector<size_t> offsets;
  auto n = node.cast<JSOptionalMemberExpression>();
  resolveMemberChian(ctx, module, n->left, offsets);
  offsets.push_back(module->codes.size() + sizeof(uint16_t));
  generate(module, vm::JSAsmOperator::JNULL, 0U);
  generate(module, vm::JSAsmOperator::LOAD_CONST,
           resolveConstant(ctx, module,
                           n->right.cast<JSIdentifierLiteral>()->value));
  generate(module, vm::JSAsmOperator::GET_FIELD);
  for (auto &offset : offsets) {
    *(uint32_t *)(module->codes.data() + offset) =
        (uint32_t)(module->codes.size());
  }
}

void JSGenerator::resolveExpressionComputedMember(
    JSGeneratorContext &ctx, common::AutoPtr<JSModule> &module,
    const common::AutoPtr<JSNode> &node) {
  std::vector<size_t> offsets;
  auto n = node.cast<JSComputedMemberExpression>();
  resolveMemberChian(ctx, module, n->left, offsets);
  resolveNode(ctx, module, n->right);
  generate(module, vm::JSAsmOperator::GET_FIELD);
  for (auto &offset : offsets) {
    *(uint32_t *)(module->codes.data() + offset) =
        (uint32_t)(module->codes.size());
  }
}

void JSGenerator::resolveExpressionOptionalComputedMember(
    JSGeneratorContext &ctx, common::AutoPtr<JSModule> &module,
    const common::AutoPtr<JSNode> &node) {

  std::vector<size_t> offsets;
  auto n = node.cast<JSOptionalComputedMemberExpression>();
  resolveMemberChian(ctx, module, n->left, offsets);
  offsets.push_back(module->codes.size() + sizeof(uint16_t));
  generate(module, vm::JSAsmOperator::JNULL, 0U);
  resolveNode(ctx, module, n->right);
  generate(module, vm::JSAsmOperator::GET_FIELD);
  for (auto &offset : offsets) {
    *(uint32_t *)(module->codes.data() + offset) =
        (uint32_t)(module->codes.size());
  }
}

void JSGenerator::resolveExpressionCondition(
    JSGeneratorContext &ctx, common::AutoPtr<JSModule> &module,
    const common::AutoPtr<JSNode> &node) {
  auto n = node.cast<JSConditionExpression>();
  auto right = n->right.cast<JSBinaryExpression>();
  resolveNode(ctx, module, n->left);
  auto start = module->codes.size() + sizeof(uint16_t);
  generate(module, vm::JSAsmOperator::JFALSE, 0U);
  resolveNode(ctx, module, right->left);
  auto end = module->codes.size() + sizeof(uint16_t);
  generate(module, vm::JSAsmOperator::JMP, 0U);
  *(uint32_t *)(module->codes.data() + start) = (uint32_t)module->codes.size();
  resolveNode(ctx, module, right->right);
  *(uint32_t *)(module->codes.data() + end) = (uint32_t)module->codes.size();
  generate(module, vm::JSAsmOperator::POP, 1U);
}

void JSGenerator::resolveExpressionCall(JSGeneratorContext &ctx,
                                        common::AutoPtr<JSModule> &module,
                                        const common::AutoPtr<JSNode> &node) {
  auto n = node.cast<JSCallExpression>();
  auto func = n->left;
  std::vector<size_t> offsets;
  vm::JSAsmOperator opt = vm::JSAsmOperator::CALL;
  if (func->type == JSNodeType::EXPRESSION_MEMBER ||
      func->type == JSNodeType::EXPRESSION_COMPUTED_MEMBER) {
    auto member = func.cast<JSBinaryExpression>();
    resolveMemberChian(ctx, module, member->left, offsets);
    if (func->type == JSNodeType::EXPRESSION_COMPUTED_MEMBER) {
      resolveNode(ctx, module, member->right);
    } else {
      generate(
          module, vm::JSAsmOperator::LOAD_CONST,
          resolveConstant(ctx, module,
                          member->right.cast<JSIdentifierLiteral>()->value));
    }
    opt = vm::JSAsmOperator::MEMBER_CALL;
  } else if (func->type == JSNodeType::EXPRESSION_OPTIONAL_MEMBER) {
    auto member = func.cast<JSOptionalMemberExpression>();
    resolveMemberChian(ctx, module, member->left, offsets);
    offsets.push_back(module->codes.size() + sizeof(uint16_t));
    generate(module, vm::JSAsmOperator::JNULL, 0U);
    generate(module, vm::JSAsmOperator::LOAD_CONST,
             resolveConstant(ctx, module,
                             member->right.cast<JSIdentifierLiteral>()->value));
    opt = vm::JSAsmOperator::MEMBER_CALL;
  } else if (func->type == JSNodeType::EXPRESSION_OPTIONAL_COMPUTED_MEMBER) {
    auto member = func.cast<JSOptionalComputedMemberExpression>();
    resolveMemberChian(ctx, module, member->left, offsets);
    offsets.push_back(module->codes.size() + sizeof(uint16_t));
    generate(module, vm::JSAsmOperator::JNULL, 0U);
    resolveNode(ctx, module, member->right);
    opt = vm::JSAsmOperator::MEMBER_CALL;
  } else {
    resolveNode(ctx, module, n->left);
  }
  for (auto &arg : n->arguments) {
    resolveNode(ctx, module, arg);
  }
  module->sourceMap[module->codes.size()] = node->location.start;
  generate(module, opt, (uint32_t)n->arguments.size());
  for (auto &offset : offsets) {
    *(uint32_t *)(module->codes.data() + offset) =
        (uint32_t)(module->codes.size());
  }
}

void JSGenerator::resolveExpressionOptionalCall(
    JSGeneratorContext &ctx, common::AutoPtr<JSModule> &module,
    const common::AutoPtr<JSNode> &node) {
  auto n = node.cast<JSCallExpression>();
  auto func = n->left;
  std::vector<size_t> offsets;
  vm::JSAsmOperator opt = vm::JSAsmOperator::OPTIONAL_CALL;
  if (func->type == JSNodeType::EXPRESSION_MEMBER ||
      func->type == JSNodeType::EXPRESSION_COMPUTED_MEMBER) {
    auto member = func.cast<JSBinaryExpression>();
    resolveMemberChian(ctx, module, member->left, offsets);
    if (func->type == JSNodeType::EXPRESSION_COMPUTED_MEMBER) {
      resolveNode(ctx, module, member->right);
    } else {
      generate(
          module, vm::JSAsmOperator::LOAD_CONST,
          resolveConstant(ctx, module,
                          member->right.cast<JSIdentifierLiteral>()->value));
    }
    opt = vm::JSAsmOperator::MEMBER_OPTIONAL_CALL;
  } else if (func->type == JSNodeType::EXPRESSION_OPTIONAL_MEMBER) {
    auto member = func.cast<JSOptionalMemberExpression>();
    resolveMemberChian(ctx, module, member->left, offsets);
    offsets.push_back(module->codes.size() + sizeof(uint16_t));
    generate(module, vm::JSAsmOperator::JNULL, 0U);
    generate(module, vm::JSAsmOperator::LOAD_CONST,
             resolveConstant(ctx, module,
                             member->right.cast<JSIdentifierLiteral>()->value));
    opt = vm::JSAsmOperator::MEMBER_OPTIONAL_CALL;
  } else if (func->type == JSNodeType::EXPRESSION_OPTIONAL_COMPUTED_MEMBER) {
    auto member = func.cast<JSOptionalComputedMemberExpression>();
    resolveMemberChian(ctx, module, member->left, offsets);
    offsets.push_back(module->codes.size() + sizeof(uint16_t));
    generate(module, vm::JSAsmOperator::JNULL, 0U);
    resolveNode(ctx, module, member->right);
    opt = vm::JSAsmOperator::MEMBER_OPTIONAL_CALL;
  } else {
    resolveNode(ctx, module, n->left);
  }
  for (auto &arg : n->arguments) {
    resolveNode(ctx, module, arg);
  }
  module->sourceMap[module->codes.size()] = node->location.start;
  generate(module, opt, (uint32_t)n->arguments.size());
  for (auto &offset : offsets) {
    *(uint32_t *)(module->codes.data() + offset) =
        (uint32_t)(module->codes.size());
  }
}

void JSGenerator::resolveExpressionNew(JSGeneratorContext &ctx,
                                       common::AutoPtr<JSModule> &module,
                                       const common::AutoPtr<JSNode> &node) {
  auto n = node.cast<JSNewExpression>();
  if (n->right->type == JSNodeType::EXPRESSION_CALL) {
    auto c = n->right.cast<JSCallExpression>();
    resolveNode(ctx, module, c->left);
    for (auto &arg : c->arguments) {
      resolveNode(ctx, module, arg);
    }
    module->sourceMap[module->codes.size()] = node->location.start;
    generate(module, vm::JSAsmOperator::NEW, (uint32_t)c->arguments.size());
  } else {
    resolveNode(ctx, module, n->right);
    module->sourceMap[module->codes.size()] = node->location.start;
    generate(module, vm::JSAsmOperator::NEW, 0U);
  }
}

void JSGenerator::resolveExpressionDelete(JSGeneratorContext &ctx,
                                          common::AutoPtr<JSModule> &module,
                                          const common::AutoPtr<JSNode> &node) {
  auto n = node.cast<JSDeleteExpression>();
  std::vector<size_t> offsets;
  if (n->right->type == JSNodeType::LITERAL_IDENTITY) {
    throw error::JSSyntaxError(
        fmt::format(L"Cannot delete identifier: '{}'",
                    n->right.cast<JSIdentifierLiteral>()->value));
  } else if (n->right->type == JSNodeType::EXPRESSION_MEMBER) {
    auto m = n->right.cast<JSMemberExpression>();
    resolveMemberChian(ctx, module, m->left, offsets);
    generate(module, vm::JSAsmOperator::LOAD_CONST,
             resolveConstant(ctx, module,
                             m->right.cast<JSIdentifierLiteral>()->value));
    generate(module, vm::JSAsmOperator::DELETE);
  } else if (n->right->type == JSNodeType::EXPRESSION_COMPUTED_MEMBER) {
    auto m = n->right.cast<JSComputedMemberExpression>();
    resolveMemberChian(ctx, module, m->left, offsets);
    resolveNode(ctx, module, m->right);
    generate(module, vm::JSAsmOperator::DELETE);
  } else if (n->right->type == JSNodeType::EXPRESSION_OPTIONAL_MEMBER) {
    auto m = n->right.cast<JSOptionalMemberExpression>();
    resolveMemberChian(ctx, module, m->left, offsets);
    offsets.push_back(module->codes.size() + sizeof(uint16_t));
    generate(module, vm::JSAsmOperator::JNULL, 0U);
    generate(module, vm::JSAsmOperator::LOAD_CONST,
             resolveConstant(ctx, module,
                             m->right.cast<JSIdentifierLiteral>()->value));
    generate(module, vm::JSAsmOperator::DELETE);
  } else if (n->right->type ==
             JSNodeType::EXPRESSION_OPTIONAL_COMPUTED_MEMBER) {
    auto m = n->right.cast<JSOptionalComputedMemberExpression>();
    resolveMemberChian(ctx, module, m->left, offsets);
    offsets.push_back(module->codes.size() + sizeof(uint16_t));
    generate(module, vm::JSAsmOperator::JNULL, 0U);
    resolveNode(ctx, module, m->right);
    generate(module, vm::JSAsmOperator::DELETE);
  } else {
    resolveNode(ctx, module, n->right);
    generate(module, vm::JSAsmOperator::PUSH_TRUE);
  }
  for (auto &offset : offsets) {
    *(uint32_t *)(module->codes.data() + offset) =
        (uint32_t)(module->codes.size());
  }
}

void JSGenerator::resolveExpressionAwait(JSGeneratorContext &ctx,
                                         common::AutoPtr<JSModule> &module,
                                         const common::AutoPtr<JSNode> &node) {
  auto n = node.cast<JSAwaitExpression>();
  resolveNode(ctx, module, n->right);
  generate(module, vm::JSAsmOperator::AWAIT);
}

void JSGenerator::resolveExpressionVoid(JSGeneratorContext &ctx,
                                        common::AutoPtr<JSModule> &module,
                                        const common::AutoPtr<JSNode> &node) {
  auto n = node.cast<JSVoidExpression>();
  resolveNode(ctx, module, n->right);
  generate(module, vm::JSAsmOperator::VOID);
}

void JSGenerator::resolveExpressionTypeof(JSGeneratorContext &ctx,
                                          common::AutoPtr<JSModule> &module,
                                          const common::AutoPtr<JSNode> &node) {
  auto n = node.cast<JSTypeofExpression>();
  resolveNode(ctx, module, n->right);
  generate(module, vm::JSAsmOperator::TYPE_OF);
}

void JSGenerator::resolveExpressionGroup(JSGeneratorContext &ctx,
                                         common::AutoPtr<JSModule> &module,
                                         const common::AutoPtr<JSNode> &node) {
  auto n = node.cast<JSGroupExpression>();
  resolveNode(ctx, module, n->expression);
}

void JSGenerator::resolveExpressionAssigment(
    JSGeneratorContext &ctx, common::AutoPtr<JSModule> &module,
    const common::AutoPtr<JSNode> &node) {
  auto n = node.cast<JSBinaryExpression>();
  auto host = n->left;
  auto value = n->right;
  resolveNode(ctx, module, value);
  resolveVariableIdentifier(ctx, module, host);
}

void JSGenerator::resolveClassMethod(JSGeneratorContext &ctx,
                                     common::AutoPtr<JSModule> &module,
                                     const common::AutoPtr<JSNode> &node) {
  // TODO:
}

void JSGenerator::resolveClassProperty(JSGeneratorContext &ctx,
                                       common::AutoPtr<JSModule> &module,
                                       const common::AutoPtr<JSNode> &node) {
  // TODO:
}

void JSGenerator::resolveClassAccessor(JSGeneratorContext &ctx,
                                       common::AutoPtr<JSModule> &module,
                                       const common::AutoPtr<JSNode> &node) {
  // TODO:
}

void JSGenerator::resolveStaticBlock(JSGeneratorContext &ctx,
                                     common::AutoPtr<JSModule> &module,
                                     const common::AutoPtr<JSNode> &node) {
  // TODO:
}

void JSGenerator::resolveImportDeclaration(
    JSGeneratorContext &ctx, common::AutoPtr<JSModule> &module,
    const common::AutoPtr<JSNode> &node) {
  // TODO:
}

void JSGenerator::resolveImportSpecifier(JSGeneratorContext &ctx,
                                         common::AutoPtr<JSModule> &module,
                                         const common::AutoPtr<JSNode> &node) {
  // TODO:
}

void JSGenerator::resolveImportDefault(JSGeneratorContext &ctx,
                                       common::AutoPtr<JSModule> &module,
                                       const common::AutoPtr<JSNode> &node) {
  // TODO:
}

void JSGenerator::resolveImportNamespace(JSGeneratorContext &ctx,
                                         common::AutoPtr<JSModule> &module,
                                         const common::AutoPtr<JSNode> &node) {
  // TODO:
}

void JSGenerator::resolveImportAttartube(JSGeneratorContext &ctx,
                                         common::AutoPtr<JSModule> &module,
                                         const common::AutoPtr<JSNode> &node) {
  // TODO:
}

void JSGenerator::resolveExportDeclaration(
    JSGeneratorContext &ctx, common::AutoPtr<JSModule> &module,
    const common::AutoPtr<JSNode> &node) {
  // TODO:
}

void JSGenerator::resolveExportDefault(JSGeneratorContext &ctx,
                                       common::AutoPtr<JSModule> &module,
                                       const common::AutoPtr<JSNode> &node) {
  // TODO:
}

void JSGenerator::resolveExportSpecifier(JSGeneratorContext &ctx,
                                         common::AutoPtr<JSModule> &module,
                                         const common::AutoPtr<JSNode> &node) {
  // TODO:
}

void JSGenerator::resolveExportAll(JSGeneratorContext &ctx,
                                   common::AutoPtr<JSModule> &module,
                                   const common::AutoPtr<JSNode> &node) {
  // TODO:
}

void JSGenerator::resolveDeclarationArrowFunction(
    JSGeneratorContext &ctx, common::AutoPtr<JSModule> &module,
    const common::AutoPtr<JSNode> &node) {
  auto n = node.cast<JSArrowFunctionDeclaration>();
  ctx.currentScope->functionDeclarations.push_back(
      (JSNode *)node.getRawPointer());
  generate(module, vm::JSAsmOperator::PUSH_ARROW);
  ctx.currentScope->functionAddr[node->id] =
      module->codes.size() + sizeof(uint16_t);
  generate(module, vm::JSAsmOperator::SET_FUNC_ADDRESS, 0U);
  if (n->async) {
    generate(module, vm::JSAsmOperator::SET_FUNC_ASYNC, 1U);
  }
  generate(
      module, vm::JSAsmOperator::SET_FUNC_SOURCE,
      resolveConstant(ctx, module, node->location.getSource(module->source)));
  std::vector<JSSourceScope *> workflow = {n->scope.getRawPointer()};
  while (!workflow.empty()) {
    auto item = *workflow.begin();
    workflow.erase(workflow.begin());
    for (auto &binding : item->bindings) {
      auto it = item->declarations.begin();
      for (; it != item->declarations.end(); it++) {
        if (&*it == binding.declaration) {
          break;
        }
      }
      if (it == item->declarations.end()) {
        auto iname = resolveConstant(ctx, module, binding.declaration->name);
        generate(module, vm::JSAsmOperator::SET_CLOSURE, iname);
      }
    }
    for (auto &child : item->children) {
      workflow.push_back(child);
    }
  }
}

void JSGenerator::resolveDeclarationFunction(
    JSGeneratorContext &ctx, common::AutoPtr<JSModule> &module,
    const common::AutoPtr<JSNode> &node) {
  if (node->type == JSNodeType::DECLARATION_ARROW_FUNCTION) {
    auto n = node.cast<JSArrowFunctionDeclaration>();
    auto offset = ctx.currentScope->functionAddr.at(n->id);
    *(uint32_t *)(module->codes.data() + offset) =
        (uint32_t)module->codes.size();
    pushLexScope(ctx, module, node->scope);
    if (n->arguments.size()) {
      generate(module, vm::JSAsmOperator::LOAD,
               resolveConstant(ctx, module, L"arguments"));
      generate(module, vm::JSAsmOperator::PUSH_UNDEFINED);
      for (auto &arg : n->arguments) {
        if (arg->type == JSNodeType::DECLARATION_PARAMETER) {
          auto a = arg.cast<JSParameterDeclaration>();
          generate(module, vm::JSAsmOperator::NEXT);
          generate(module, vm::JSAsmOperator::POP, 1U);
          resolveDeclarationParameter(ctx, module, arg);
        } else {
          auto a = arg.cast<JSRestPatternItem>();
          generate(module, vm::JSAsmOperator::REST_ARRAY);
          resolveVariableIdentifier(ctx, module, a->identifier);
        }
      }
      generate(module, vm::JSAsmOperator::POP, 1U);
      generate(module, vm::JSAsmOperator::POP, 1U);
    }
    resolveNode(ctx, module, n->body);
    if (n->body->type != JSNodeType::DECLARATION_FUNCTION_BODY) {
      generate(module, vm::JSAsmOperator::RET);
    }
    popLexScope(ctx, module);
  } else {
    auto n = node.cast<JSFunctionDeclaration>();
    auto offset = ctx.currentScope->functionAddr.at(n->id);
    *(uint32_t *)(module->codes.data() + offset) =
        (uint32_t)module->codes.size();
    pushLexScope(ctx, module, node->scope);
    if (n->arguments.size()) {
      generate(module, vm::JSAsmOperator::LOAD,
               resolveConstant(ctx, module, L"arguments"));
      generate(module, vm::JSAsmOperator::PUSH_UNDEFINED);
      for (auto &arg : n->arguments) {
        if (arg->type == JSNodeType::DECLARATION_PARAMETER) {
          auto a = arg.cast<JSParameterDeclaration>();
          generate(module, vm::JSAsmOperator::NEXT);
          generate(module, vm::JSAsmOperator::POP, 1U);
          resolveDeclarationParameter(ctx, module, arg);
        } else {
          auto a = arg.cast<JSRestPatternItem>();
          generate(module, vm::JSAsmOperator::REST_ARRAY);
          resolveVariableIdentifier(ctx, module, a->identifier);
        }
      }
      generate(module, vm::JSAsmOperator::POP, 1U);
      generate(module, vm::JSAsmOperator::POP, 1U);
    }
    resolveNode(ctx, module, n->body);
    popLexScope(ctx, module);
  }
}

void JSGenerator::resolveFunction(JSGeneratorContext &ctx,
                                  common::AutoPtr<JSModule> &module,
                                  const common::AutoPtr<JSNode> &node) {
  auto n = node.cast<JSFunctionDeclaration>();
  ctx.currentScope->functionDeclarations.push_back(
      (JSNode *)node.getRawPointer());
  if (n->identifier == nullptr) {
    if (n->generator) {
      generate(module, vm::JSAsmOperator::PUSH_GENERATOR);
    } else {
      generate(module, vm::JSAsmOperator::PUSH_FUNCTION);
    }
    ctx.currentScope->functionAddr[node->id] =
        module->codes.size() + sizeof(uint16_t);
    generate(module, vm::JSAsmOperator::SET_FUNC_ADDRESS, 0U);
    if (n->async) {
      generate(module, vm::JSAsmOperator::SET_FUNC_ASYNC, 1U);
    }
    generate(
        module, vm::JSAsmOperator::SET_FUNC_SOURCE,
        resolveConstant(ctx, module, node->location.getSource(module->source)));
    std::vector<JSSourceScope *> workflow = {n->scope.getRawPointer()};
    while (!workflow.empty()) {
      auto item = *workflow.begin();
      workflow.erase(workflow.begin());
      for (auto &binding : item->bindings) {
        auto it = item->declarations.begin();
        for (; it != item->declarations.end(); it++) {
          if (&*it == binding.declaration) {
            break;
          }
        }
        if (it == item->declarations.end()) {
          auto iname = resolveConstant(ctx, module, binding.declaration->name);
          generate(module, vm::JSAsmOperator::SET_CLOSURE, iname);
        }
      }
      for (auto &child : item->children) {
        workflow.push_back(child);
      }
    }
  }
}

void JSGenerator::resolveDeclarationFunctionBody(
    JSGeneratorContext &ctx, common::AutoPtr<JSModule> &module,
    const common::AutoPtr<JSNode> &node) {
  auto n = node.cast<JSFunctionBodyDeclaration>();
  for (auto &item : n->statements) {
    resolveNode(ctx, module, item);
  }
  generate(module, vm::JSAsmOperator::PUSH_UNDEFINED);
  generate(module, vm::JSAsmOperator::RET);
  for (auto &item : ctx.currentScope->functionDeclarations) {
    resolveDeclarationFunction(ctx, module, item);
  }
}

void JSGenerator::resolveDeclarationParameter(
    JSGeneratorContext &ctx, common::AutoPtr<JSModule> &module,
    const common::AutoPtr<JSNode> &node) {
  auto a = node.cast<JSParameterDeclaration>();
  if (a->value != nullptr) {
    auto pin = module->codes.size() + sizeof(uint16_t);
    generate(module, vm::JSAsmOperator::JNOT_NULL, 0U);
    generate(module, vm::JSAsmOperator::POP, 1U);
    resolveNode(ctx, module, a->value);
    *(uint32_t *)(module->codes.data() + pin) = (uint32_t)module->codes.size();
  }
  resolveVariableIdentifier(ctx, module, a->identifier);
}

void JSGenerator::resolveDeclarationObject(
    JSGeneratorContext &ctx, common::AutoPtr<JSModule> &module,
    const common::AutoPtr<JSNode> &node) {
  auto n = node.cast<JSObjectDeclaration>();
  generate(module, vm::JSAsmOperator::PUSH_OBJECT);
  for (auto &prop : n->properties) {
    if (prop->type == JSNodeType::EXPRESSION_REST) {
      auto n = prop.cast<JSBinaryExpression>();
      resolveNode(ctx, module, n->right);
      generate(module, vm::JSAsmOperator::MERGE);
    } else {
      resolveNode(ctx, module, prop);
    }
  }
}

void JSGenerator::resolveDeclarationArray(JSGeneratorContext &ctx,
                                          common::AutoPtr<JSModule> &module,
                                          const common::AutoPtr<JSNode> &node) {
  auto n = node.cast<JSArrayDeclaration>();
  generate(module, vm::JSAsmOperator::PUSH_ARRAY);
  for (size_t index = 0; index < n->items.size(); index++) {
    if (!n->items[index]) {
      continue;
    } else if (n->items[index]->type == JSNodeType::EXPRESSION_REST) {
      auto aitem = n->items[index].cast<JSBinaryExpression>();
      resolveNode(ctx, module, aitem->right);
      generate(module, vm::JSAsmOperator::MERGE);
    } else {
      resolveNode(ctx, module, n->items[index]);
      generate(module, vm::JSAsmOperator::PUSH, (double)index);
      generate(module, vm::JSAsmOperator::SET_FIELD);
      generate(module, vm::JSAsmOperator::POP, 1U);
    }
  }
}

void JSGenerator::resolveDeclarationClass(JSGeneratorContext &ctx,
                                          common::AutoPtr<JSModule> &module,
                                          const common::AutoPtr<JSNode> &node) {
  // TODO:
}

void JSGenerator::resolveNode(JSGeneratorContext &ctx,
                              common::AutoPtr<JSModule> &module,
                              const common::AutoPtr<JSNode> &node) {
  switch (node->type) {
  case JSNodeType::PRIVATE_NAME:
    resolvePrivateName(ctx, module, node);
    break;
  case JSNodeType::LITERAL_REGEX:
    resolveLiteralRegex(ctx, module, node);
    break;
  case JSNodeType::LITERAL_NULL:
    resolveLiteralNull(ctx, module, node);
    break;
  case JSNodeType::LITERAL_STRING:
    resolveLiteralString(ctx, module, node);
    break;
  case JSNodeType::LITERAL_BOOLEAN:
    resolveLiteralBoolean(ctx, module, node);
    break;
  case JSNodeType::LITERAL_NUMBER:
    resolveLiteralNumber(ctx, module, node);
    break;
  case JSNodeType::LITERAL_UNDEFINED:
    resolveLiteralUndefined(ctx, module, node);
    break;
  case JSNodeType::LITERAL_IDENTITY:
    resolveLiteralIdentity(ctx, module, node);
    break;
  case JSNodeType::LITERAL_TEMPLATE:
    resolveLiteralTemplate(ctx, module, node);
    break;
  case JSNodeType::LITERAL_BIGINT:
    resolveLiteralBigint(ctx, module, node);
    break;
  case JSNodeType::THIS:
    resolveThis(ctx, module, node);
    break;
  case JSNodeType::SUPER:
    resolveSuper(ctx, module, node);
    break;
  case JSNodeType::PROGRAM:
    resolveProgram(ctx, module, node);
    break;
  case JSNodeType::STATEMENT_BLOCK:
    resolveStatementBlock(ctx, module, node);
    break;
  case JSNodeType::STATEMENT_DEBUGGER:
    resolveStatementDebugger(ctx, module, node);
    break;
  case JSNodeType::STATEMENT_RETURN:
    resolveStatementReturn(ctx, module, node);
    break;

  case JSNodeType::STATEMENT_LABEL:
    resolveStatementLabel(ctx, module, node);
    break;
  case JSNodeType::STATEMENT_BREAK:
    resolveStatementBreak(ctx, module, node);
    break;
  case JSNodeType::STATEMENT_CONTINUE:
    resolveStatementContinue(ctx, module, node);
    break;
  case JSNodeType::STATEMENT_IF:
    resolveStatementIf(ctx, module, node);
    break;
  case JSNodeType::STATEMENT_SWITCH:
    resolveStatementSwitch(ctx, module, node);
    break;
  case JSNodeType::STATEMENT_THROW:
    resolveStatementThrow(ctx, module, node);
    break;
  case JSNodeType::STATEMENT_TRY:
    resolveStatementTry(ctx, module, node);
    break;
  case JSNodeType::STATEMENT_TRY_CATCH:
    resolveStatementTryCatch(ctx, module, node);
    break;
  case JSNodeType::STATEMENT_WHILE:
    resolveStatementWhile(ctx, module, node);
    break;
  case JSNodeType::STATEMENT_DO_WHILE:
    resolveStatementDoWhile(ctx, module, node);
    break;
  case JSNodeType::STATEMENT_FOR:
    resolveStatementFor(ctx, module, node);
    break;
  case JSNodeType::STATEMENT_FOR_IN:
    resolveStatementForIn(ctx, module, node);
    break;
  case JSNodeType::STATEMENT_FOR_OF:
    resolveStatementForOf(ctx, module, node);
    break;
  case JSNodeType::STATEMENT_FOR_AWAIT_OF:
    resolveStatementForAwaitOf(ctx, module, node);
    break;
  case JSNodeType::VARIABLE_DECLARATION:
    resolveVariableDeclaration(ctx, module, node);
    break;
  case JSNodeType::VARIABLE_DECLARATOR:
    resolveVariableDeclarator(ctx, module, node);
    break;
  case JSNodeType::DIRECTIVE:
    resolveDirective(ctx, module, node);
    break;
  case JSNodeType::OBJECT_PROPERTY:
    resolveObjectProperty(ctx, module, node);
    break;
  case JSNodeType::OBJECT_METHOD:
    resolveObjectMethod(ctx, module, node);
    break;
  case JSNodeType::OBJECT_ACCESSOR:
    resolveObjectAccessor(ctx, module, node);
    break;
  case JSNodeType::EXPRESSION_UNARY:
    resolveExpressionUnary(ctx, module, node);
    break;
  case JSNodeType::EXPRESSION_UPDATE:
    resolveExpressionUpdate(ctx, module, node);
    break;
  case JSNodeType::EXPRESSION_BINARY:
    resolveExpressionBinary(ctx, module, node);
    break;
  case JSNodeType::EXPRESSION_MEMBER:
    resolveExpressionMember(ctx, module, node);
    break;
  case JSNodeType::EXPRESSION_OPTIONAL_MEMBER:
    resolveExpressionOptionalMember(ctx, module, node);
    break;
  case JSNodeType::EXPRESSION_COMPUTED_MEMBER:
    resolveExpressionComputedMember(ctx, module, node);
    break;
  case JSNodeType::EXPRESSION_OPTIONAL_COMPUTED_MEMBER:
    resolveExpressionOptionalComputedMember(ctx, module, node);
    break;
  case JSNodeType::EXPRESSION_CONDITION:
    resolveExpressionCondition(ctx, module, node);
    break;
  case JSNodeType::EXPRESSION_CALL:
    resolveExpressionCall(ctx, module, node);
    break;
  case JSNodeType::EXPRESSION_OPTIONAL_CALL:
    resolveExpressionOptionalCall(ctx, module, node);
    break;
  case JSNodeType::EXPRESSION_NEW:
    resolveExpressionNew(ctx, module, node);
    break;
  case JSNodeType::EXPRESSION_DELETE:
    resolveExpressionDelete(ctx, module, node);
    break;
  case JSNodeType::EXPRESSION_AWAIT:
    resolveExpressionAwait(ctx, module, node);
    break;
  case JSNodeType::EXPRESSION_YIELD:
    resolveExpressionYield(ctx, module, node);
    break;
  case JSNodeType::EXPRESSION_YIELD_DELEGATE:
    resolveExpressionYieldDelegate(ctx, module, node);
    break;
  case JSNodeType::EXPRESSION_VOID:
    resolveExpressionVoid(ctx, module, node);
    break;
  case JSNodeType::EXPRESSION_TYPEOF:
    resolveExpressionTypeof(ctx, module, node);
    break;
  case JSNodeType::EXPRESSION_GROUP:
    resolveExpressionGroup(ctx, module, node);
    break;
  case JSNodeType::EXPRESSION_ASSIGMENT:
    resolveExpressionAssigment(ctx, module, node);
    break;
  case JSNodeType::CLASS_METHOD:
    resolveClassMethod(ctx, module, node);
    break;
  case JSNodeType::CLASS_PROPERTY:
    resolveClassProperty(ctx, module, node);
    break;
  case JSNodeType::CLASS_ACCESSOR:
    resolveClassAccessor(ctx, module, node);
    break;
  case JSNodeType::STATIC_BLOCK:
    resolveStaticBlock(ctx, module, node);
    break;
  case JSNodeType::IMPORT_DECLARATION:
    resolveImportDeclaration(ctx, module, node);
    break;
  case JSNodeType::IMPORT_SPECIFIER:
    resolveImportSpecifier(ctx, module, node);
    break;
  case JSNodeType::IMPORT_DEFAULT:
    resolveImportDefault(ctx, module, node);
    break;
  case JSNodeType::IMPORT_NAMESPACE:
    resolveImportNamespace(ctx, module, node);
    break;
  case JSNodeType::IMPORT_ATTARTUBE:
    resolveImportAttartube(ctx, module, node);
    break;
  case JSNodeType::EXPORT_DECLARATION:
    resolveExportDeclaration(ctx, module, node);
    break;
  case JSNodeType::EXPORT_DEFAULT:
    resolveExportDefault(ctx, module, node);
    break;
  case JSNodeType::EXPORT_SPECIFIER:
    resolveExportSpecifier(ctx, module, node);
    break;
  case JSNodeType::EXPORT_ALL:
    resolveExportAll(ctx, module, node);
    break;
  case JSNodeType::DECLARATION_ARROW_FUNCTION:
    resolveDeclarationArrowFunction(ctx, module, node);
    break;
  case JSNodeType::DECLARATION_FUNCTION:
    resolveFunction(ctx, module, node);
    break;
  case JSNodeType::DECLARATION_FUNCTION_BODY:
    resolveDeclarationFunctionBody(ctx, module, node);
    break;
  case JSNodeType::DECLARATION_PARAMETER:
    resolveDeclarationParameter(ctx, module, node);
    break;
  case JSNodeType::DECLARATION_OBJECT:
    resolveDeclarationObject(ctx, module, node);
    break;
  case JSNodeType::DECLARATION_ARRAY:
    resolveDeclarationArray(ctx, module, node);
    break;
  case JSNodeType::DECLARATION_CLASS:
    resolveDeclarationClass(ctx, module, node);
    break;
  default:
    throw error::JSSyntaxError(L"Unexcepted token");
  }
}

common::AutoPtr<JSModule>
JSGenerator::resolve(const std::wstring &filename, const std::wstring &source,
                     const common::AutoPtr<JSNode> &node) {
  JSGeneratorContext ctx;
  common::AutoPtr<JSModule> module = new JSModule;
  module->filename = filename;
  module->source = source;
  resolveNode(ctx, module, node);
  return module;
}