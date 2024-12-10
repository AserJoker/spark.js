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

void JSGenerator::resolvePrivateName(JSGeneratorContext &ctx,
                                     common::AutoPtr<JSModule> &module,
                                     const common::AutoPtr<JSNode> &node) {}

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

void JSGenerator::resolveLiteralComment(JSGeneratorContext &ctx,
                                        common::AutoPtr<JSModule> &module,
                                        const common::AutoPtr<JSNode> &node) {}

void JSGenerator::resolveLiteralMultilineComment(
    JSGeneratorContext &ctx, common::AutoPtr<JSModule> &module,
    const common::AutoPtr<JSNode> &node) {}

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
                                         const common::AutoPtr<JSNode> &node) {}

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
void JSGenerator::resolveStatementEmpty(JSGeneratorContext &ctx,
                                        common::AutoPtr<JSModule> &module,
                                        const common::AutoPtr<JSNode> &node) {}

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
    const common::AutoPtr<JSNode> &node) {}

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
}

void JSGenerator::resolveStatementSwitch(JSGeneratorContext &ctx,
                                         common::AutoPtr<JSModule> &module,
                                         const common::AutoPtr<JSNode> &node) {}

void JSGenerator::resolveStatementSwitchCase(
    JSGeneratorContext &ctx, common::AutoPtr<JSModule> &module,
    const common::AutoPtr<JSNode> &node) {}

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
  resolveNode(ctx, module, n->body);
  generate(module, vm::JSAsmOperator::JMP, start);
  auto end = (uint32_t)module->codes.size();
  *(uint32_t *)(module->codes.data() + endOffset) = end;
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
  auto start = (uint32_t)module->codes.size();
  resolveNode(ctx, module, n->body);
  resolveNode(ctx, module, n->condition);
  auto condition = (uint32_t)module->codes.size();
  generate(module, vm::JSAsmOperator::JTRUE, start);
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
  resolveNode(ctx, module, n->body);
  if (n->update != nullptr) {
    resolveNode(ctx, module, n->update);
  }
  generate(module, vm::JSAsmOperator::JMP, start);
  auto end = (uint32_t)module->codes.size();
  *(uint32_t *)(module->codes.data() + endOffset) = end;
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
                                        const std::wstring &label) {}

void JSGenerator::resolveStatementForOf(JSGeneratorContext &ctx,
                                        common::AutoPtr<JSModule> &module,
                                        const common::AutoPtr<JSNode> &node,
                                        const std::wstring &label) {}

void JSGenerator::resolveStatementForAwaitOf(
    JSGeneratorContext &ctx, common::AutoPtr<JSModule> &module,
    const common::AutoPtr<JSNode> &node, const std::wstring &label) {}

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
  if (n->identifier->type == JSNodeType::LITERAL_IDENTITY) {
    auto name = resolveConstant(
        ctx, module, n->identifier.cast<JSIdentifierLiteral>()->value);
    generate(module, vm::JSAsmOperator::STORE, name);
  }
}

void JSGenerator::resolveDecorator(JSGeneratorContext &ctx,
                                   common::AutoPtr<JSModule> &module,
                                   const common::AutoPtr<JSNode> &node) {}

void JSGenerator::resolveDirective(JSGeneratorContext &ctx,
                                   common::AutoPtr<JSModule> &module,
                                   const common::AutoPtr<JSNode> &node) {}

void JSGenerator::resolveInterpreterDirective(
    JSGeneratorContext &ctx, common::AutoPtr<JSModule> &module,
    const common::AutoPtr<JSNode> &node) {}

void JSGenerator::resolveObjectProperty(JSGeneratorContext &ctx,
                                        common::AutoPtr<JSModule> &module,
                                        const common::AutoPtr<JSNode> &node) {
  auto n = node.cast<JSObjectProperty>();
  if (n->implement != nullptr) {
    resolveNode(ctx, module, n->implement);
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
  auto n = node.cast<JSMemberExpression>();
  resolveNode(ctx, module, n->left);
  generate(module, vm::JSAsmOperator::LOAD_CONST,
           resolveConstant(ctx, module,
                           n->right.cast<JSIdentifierLiteral>()->value));
  generate(module, vm::JSAsmOperator::GET_FIELD);
}

void JSGenerator::resolveExpressionOptionalMember(
    JSGeneratorContext &ctx, common::AutoPtr<JSModule> &module,
    const common::AutoPtr<JSNode> &node) {}

void JSGenerator::resolveExpressionComputedMember(
    JSGeneratorContext &ctx, common::AutoPtr<JSModule> &module,
    const common::AutoPtr<JSNode> &node) {
  auto n = node.cast<JSComputedMemberExpression>();
  resolveNode(ctx, module, n->left);
  resolveNode(ctx, module, n->right);
  generate(module, vm::JSAsmOperator::GET_FIELD);
}

void JSGenerator::resolveExpressionOptionalComputedMember(
    JSGeneratorContext &ctx, common::AutoPtr<JSModule> &module,
    const common::AutoPtr<JSNode> &node) {}

void JSGenerator::resolveExpressionCondition(
    JSGeneratorContext &ctx, common::AutoPtr<JSModule> &module,
    const common::AutoPtr<JSNode> &node) {}

void JSGenerator::resolveExpressionCall(JSGeneratorContext &ctx,
                                        common::AutoPtr<JSModule> &module,
                                        const common::AutoPtr<JSNode> &node) {
  auto n = node.cast<JSCallExpression>();
  auto func = n->left;
  vm::JSAsmOperator opt = vm::JSAsmOperator::CALL;
  if (func->type == JSNodeType::EXPRESSION_MEMBER ||
      func->type == JSNodeType::EXPRESSION_COMPUTED_MEMBER) {
    auto member = func.cast<JSBinaryExpression>();
    resolveNode(ctx, module, member->left);
    if (func->type == JSNodeType::EXPRESSION_COMPUTED_MEMBER) {
      resolveNode(ctx, module, member->right);
    } else {
      generate(
          module, vm::JSAsmOperator::LOAD_CONST,
          resolveConstant(ctx, module,
                          member->right.cast<JSIdentifierLiteral>()->value));
    }
    opt = vm::JSAsmOperator::MEMBER_CALL;
  } else {
    resolveNode(ctx, module, n->left);
  }
  for (auto &arg : n->arguments) {
    resolveNode(ctx, module, arg);
  }
  module->sourceMap[module->codes.size()] = node->location.start;
  generate(module, opt, (uint32_t)n->arguments.size());
}

void JSGenerator::resolveExpressionOptionalCall(
    JSGeneratorContext &ctx, common::AutoPtr<JSModule> &module,
    const common::AutoPtr<JSNode> &node) {}

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
  if (host->type == JSNodeType::LITERAL_IDENTITY) {
    resolveNode(ctx, module, value);
    generate(
        module, vm::JSAsmOperator::STORE,
        resolveConstant(ctx, module, host.cast<JSIdentifierLiteral>()->value));
  } else if (host->type == JSNodeType::EXPRESSION_MEMBER) {
    auto h = host.cast<JSMemberExpression>();
    resolveNode(ctx, module, h->left);
    resolveNode(ctx, module, value);
    generate(module, vm::JSAsmOperator::LOAD_CONST,
             resolveConstant(ctx, module,
                             h->right.cast<JSIdentifierLiteral>()->value));
    generate(module, vm::JSAsmOperator::SET_FIELD);
  } else if (host->type == JSNodeType::EXPRESSION_COMPUTED_MEMBER) {
    auto h = host.cast<JSComputedMemberExpression>();
    resolveNode(ctx, module, h->left);
    resolveNode(ctx, module, value);
    resolveNode(ctx, module, h->right);
    generate(module, vm::JSAsmOperator::SET_FIELD);
    generate(module, vm::JSAsmOperator::POP, 1U);
  }
}

void JSGenerator::resolveExpressionRest(JSGeneratorContext &ctx,
                                        common::AutoPtr<JSModule> &module,
                                        const common::AutoPtr<JSNode> &node) {}

void JSGenerator::resolvePatternRestItem(JSGeneratorContext &ctx,
                                         common::AutoPtr<JSModule> &module,
                                         const common::AutoPtr<JSNode> &node) {}

void JSGenerator::resolvePatternObject(JSGeneratorContext &ctx,
                                       common::AutoPtr<JSModule> &module,
                                       const common::AutoPtr<JSNode> &node) {}

void JSGenerator::resolvePatternObjectItem(
    JSGeneratorContext &ctx, common::AutoPtr<JSModule> &module,
    const common::AutoPtr<JSNode> &node) {}

void JSGenerator::resolvePatternArray(JSGeneratorContext &ctx,
                                      common::AutoPtr<JSModule> &module,
                                      const common::AutoPtr<JSNode> &node) {}

void JSGenerator::resolvePatternArrayItem(JSGeneratorContext &ctx,
                                          common::AutoPtr<JSModule> &module,
                                          const common::AutoPtr<JSNode> &node) {
}

void JSGenerator::resolveClassMethod(JSGeneratorContext &ctx,
                                     common::AutoPtr<JSModule> &module,
                                     const common::AutoPtr<JSNode> &node) {}

void JSGenerator::resolveClassProperty(JSGeneratorContext &ctx,
                                       common::AutoPtr<JSModule> &module,
                                       const common::AutoPtr<JSNode> &node) {}

void JSGenerator::resolveClassAccessor(JSGeneratorContext &ctx,
                                       common::AutoPtr<JSModule> &module,
                                       const common::AutoPtr<JSNode> &node) {}

void JSGenerator::resolveStaticBlock(JSGeneratorContext &ctx,
                                     common::AutoPtr<JSModule> &module,
                                     const common::AutoPtr<JSNode> &node) {}

void JSGenerator::resolveImportDeclaration(
    JSGeneratorContext &ctx, common::AutoPtr<JSModule> &module,
    const common::AutoPtr<JSNode> &node) {}

void JSGenerator::resolveImportSpecifier(JSGeneratorContext &ctx,
                                         common::AutoPtr<JSModule> &module,
                                         const common::AutoPtr<JSNode> &node) {}

void JSGenerator::resolveImportDefault(JSGeneratorContext &ctx,
                                       common::AutoPtr<JSModule> &module,
                                       const common::AutoPtr<JSNode> &node) {}

void JSGenerator::resolveImportNamespace(JSGeneratorContext &ctx,
                                         common::AutoPtr<JSModule> &module,
                                         const common::AutoPtr<JSNode> &node) {}

void JSGenerator::resolveImportAttartube(JSGeneratorContext &ctx,
                                         common::AutoPtr<JSModule> &module,
                                         const common::AutoPtr<JSNode> &node) {}

void JSGenerator::resolveExportDeclaration(
    JSGeneratorContext &ctx, common::AutoPtr<JSModule> &module,
    const common::AutoPtr<JSNode> &node) {}

void JSGenerator::resolveExportDefault(JSGeneratorContext &ctx,
                                       common::AutoPtr<JSModule> &module,
                                       const common::AutoPtr<JSNode> &node) {}

void JSGenerator::resolveExportSpecifier(JSGeneratorContext &ctx,
                                         common::AutoPtr<JSModule> &module,
                                         const common::AutoPtr<JSNode> &node) {}

void JSGenerator::resolveExportAll(JSGeneratorContext &ctx,
                                   common::AutoPtr<JSModule> &module,
                                   const common::AutoPtr<JSNode> &node) {}

void JSGenerator::resolveDeclarationArrowFunction(
    JSGeneratorContext &ctx, common::AutoPtr<JSModule> &module,
    const common::AutoPtr<JSNode> &node) {}

void JSGenerator::resolveDeclarationFunction(
    JSGeneratorContext &ctx, common::AutoPtr<JSModule> &module,
    const common::AutoPtr<JSNode> &node) {
  auto n = node.cast<JSFunctionDeclaration>();
  auto offset = ctx.currentScope->functionAddr.at(n->id);
  *(uint32_t *)(module->codes.data() + offset) = (uint32_t)module->codes.size();
  pushLexScope(ctx, module, node->scope);
  uint32_t index = 0;
  for (auto &arg : n->arguments) {
    auto a = arg.cast<JSParameterDeclaration>();
    generate(module, vm::JSAsmOperator::PUSH_ARGUMENT, index);
    resolveDeclarationParameter(ctx, module, arg);
    index++;
  }
  resolveNode(ctx, module, n->body);
  popLexScope(ctx, module);
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
    resolveNode(ctx, module, a->value);
    *(uint32_t *)(module->codes.data() + pin) = (uint32_t)module->codes.size();
  }
  if (a->identifier->type == JSNodeType::LITERAL_IDENTITY) {
    generate(module, vm::JSAsmOperator::STORE,
             resolveConstant(ctx, module,
                             a->identifier.cast<JSIdentifierLiteral>()->value));
  } else {
    generate(module, vm::JSAsmOperator::POP, 1U);
  }
}

void JSGenerator::resolveDeclarationObject(
    JSGeneratorContext &ctx, common::AutoPtr<JSModule> &module,
    const common::AutoPtr<JSNode> &node) {
  auto n = node.cast<JSObjectDeclaration>();
  generate(module, vm::JSAsmOperator::PUSH_OBJECT);
  for (auto &prop : n->properties) {
    resolveNode(ctx, module, prop);
  }
}

void JSGenerator::resolveDeclarationArray(JSGeneratorContext &ctx,
                                          common::AutoPtr<JSModule> &module,
                                          const common::AutoPtr<JSNode> &node) {
  auto n = node.cast<JSArrayDeclaration>();
  generate(module, vm::JSAsmOperator::PUSH_ARRAY);
  for (size_t index = 0; index < n->items.size(); index++) {
    if (n->items[index] != nullptr) {
      resolveNode(ctx, module, n->items[index]);
    } else {
      generate(module, vm::JSAsmOperator::PUSH_UNINITIALIZED);
    }
    generate(module, vm::JSAsmOperator::PUSH, (double)index);
    generate(module, vm::JSAsmOperator::SET_FIELD);
    generate(module, vm::JSAsmOperator::POP, 1U);
  }
}

void JSGenerator::resolveDeclarationClass(JSGeneratorContext &ctx,
                                          common::AutoPtr<JSModule> &module,
                                          const common::AutoPtr<JSNode> &node) {
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
  case JSNodeType::LITERAL_COMMENT:
    resolveLiteralComment(ctx, module, node);
    break;
  case JSNodeType::LITERAL_MULTILINE_COMMENT:
    resolveLiteralMultilineComment(ctx, module, node);
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
  case JSNodeType::STATEMENT_EMPTY:
    resolveStatementEmpty(ctx, module, node);
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
  case JSNodeType::STATEMENT_SWITCH_CASE:
    resolveStatementSwitchCase(ctx, module, node);
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
  case JSNodeType::DECORATOR:
    resolveDecorator(ctx, module, node);
    break;
  case JSNodeType::DIRECTIVE:
    resolveDirective(ctx, module, node);
    break;
  case JSNodeType::INTERPRETER_DIRECTIVE:
    resolveInterpreterDirective(ctx, module, node);
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
  case JSNodeType::EXPRESSION_REST:
    resolveExpressionRest(ctx, module, node);
    break;
  case JSNodeType::PATTERN_REST_ITEM:
    resolvePatternRestItem(ctx, module, node);
    break;
  case JSNodeType::PATTERN_OBJECT:
    resolvePatternObject(ctx, module, node);
    break;
  case JSNodeType::PATTERN_OBJECT_ITEM:
    resolvePatternObjectItem(ctx, module, node);
    break;
  case JSNodeType::PATTERN_ARRAY:
    resolvePatternArray(ctx, module, node);
    break;
  case JSNodeType::PATTERN_ARRAY_ITEM:
    resolvePatternArrayItem(ctx, module, node);
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