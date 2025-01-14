#include "compiler/JSGenerator.hpp"
#include "common/AutoPtr.hpp"
#include "compiler/base/JSNode.hpp"
#include "compiler/base/JSNodeType.hpp"
#include "engine/base/JSEvalType.hpp"
#include "error/JSSyntaxError.hpp"
#include "vm/JSAsmOperator.hpp"
#include "vm/JSRegExpFlag.hpp"
#include <cstdint>
#include <fmt/xchar.h>
#include <string>
#include <unordered_map>
#include <vector>

using namespace spark;
using namespace spark::compiler;
uint32_t JSGenerator::resolveConstant(common::AutoPtr<JSModule> &module,
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

void JSGenerator::resolveExport(JSGeneratorContext &ctx,
                                common::AutoPtr<JSModule> &module,
                                common::AutoPtr<JSNode> node) {
  if (node->type == JSNodeType::DECLARATION_FUNCTION) {
    auto n = node.cast<JSFunctionDeclaration>();
    resolveExport(ctx, module, n->identifier);
  } else if (node->type == JSNodeType::DECLARATION_CLASS) {
    auto n = node.cast<JSClassDeclaration>();
    resolveExport(ctx, module, n->identifier);
  } else if (node->type == JSNodeType::VARIABLE_DECLARATION) {
    auto n = node.cast<JSVariableDeclaration>();
    for (auto &declaration : n->declarations) {
      resolveExport(ctx, module, declaration);
    }
  } else if (node->type == JSNodeType::VARIABLE_DECLARATOR) {
    auto n = node.cast<JSVariableDeclarator>();
    resolveExport(ctx, module, n->identifier);
  } else if (node->type == JSNodeType::PATTERN_ARRAY) {
    auto n = node.cast<JSArrayPattern>();
    for (auto &item : n->items) {
      resolveExport(ctx, module, item);
    }
  } else if (node->type == JSNodeType::PATTERN_OBJECT) {
    auto n = node.cast<JSObjectPattern>();
    for (auto &item : n->items) {
      resolveExport(ctx, module, item);
    }
  } else if (node->type == JSNodeType::PATTERN_REST_ITEM) {
    auto n = node.cast<JSRestPatternItem>();
    resolveExport(ctx, module, n->identifier);
  } else if (node->type == JSNodeType::PATTERN_ARRAY_ITEM) {
    auto n = node.cast<JSArrayPatternItem>();
    resolveExport(ctx, module, n->identifier);
  } else if (node->type == JSNodeType::PATTERN_OBJECT_ITEM) {
    auto n = node.cast<JSObjectPatternItem>();
    resolveExport(ctx, module, n->match);
  } else if (node->type == JSNodeType::LITERAL_IDENTITY) {
    generate(module, vm::JSAsmOperator::LOAD,
             node.cast<JSIdentifierLiteral>()->value);
    generate(module, vm::JSAsmOperator::EXPORT,
             node.cast<JSIdentifierLiteral>()->value);
  }
}

void JSGenerator::resolveDeclaration(JSGeneratorContext &ctx,
                                     common::AutoPtr<JSModule> &module,
                                     const JSSourceDeclaration &declaration) {
  switch (declaration.type) {
  case JSSourceDeclaration::TYPE::CATCH:
  case JSSourceDeclaration::TYPE::UNDEFINED:
    generate(module, vm::JSAsmOperator::PUSH_UNDEFINED);
    break;
  case JSSourceDeclaration::TYPE::ARGUMENT:
  case JSSourceDeclaration::TYPE::UNINITIALIZED:
    generate(module, vm::JSAsmOperator::PUSH_UNINITIALIZED);
    break;
  case JSSourceDeclaration::TYPE::FUNCTION: {
    if (declaration.node->type == JSNodeType::DECLARATION_CLASS) {
      generate(module, vm::JSAsmOperator::PUSH_UNINITIALIZED);
    } else {
      auto n = (JSFunctionDeclaration *)declaration.node;
      if (n->generator) {
        if (n->async) {
          generate(module, vm::JSAsmOperator::PUSH_ASYNC_GENERATOR);
        } else {
          generate(module, vm::JSAsmOperator::PUSH_GENERATOR);
        }
      } else {
        if (n->async) {
          generate(module, vm::JSAsmOperator::PUSH_ASYNC);
        } else {
          generate(module, vm::JSAsmOperator::PUSH_FUNCTION);
        }
      }
      ctx.currentScope->functionAddr[declaration.node->id] =
          module->codes.size() + sizeof(uint16_t);

      generate(module, vm::JSAsmOperator::SET_FUNC_ADDRESS, 0U);
      generate(module, vm::JSAsmOperator::SET_FUNC_NAME, declaration.name);
      generate(module, vm::JSAsmOperator::SET_FUNC_SOURCE,
               L"[function " + declaration.name + L"]");
    }
    break;
  }
  }
  if (declaration.isConst) {
    generate(module, vm::JSAsmOperator::CREATE_CONST, declaration.name);
  } else {
    generate(module, vm::JSAsmOperator::CREATE, declaration.name);
  }
}

void JSGenerator::resolveClosure(JSGeneratorContext &ctx,
                                 common::AutoPtr<JSModule> &module,
                                 const JSSourceDeclaration &declaration) {
  uint32_t name = resolveConstant(module, declaration.name);
  auto closure = resolveClosure(ctx, module, declaration.node);
  if (!closure.empty()) {
    generate(module, vm::JSAsmOperator::LOAD, name);
    for (auto &item : closure) {
      generate(module, vm::JSAsmOperator::SET_CLOSURE, item);
    }
    generate(module, vm::JSAsmOperator::POP, 1U);
  }
}
std::vector<uint32_t>
JSGenerator::resolveClosure(JSGeneratorContext &ctx,
                            common::AutoPtr<JSModule> &module,
                            common::AutoPtr<JSNode> node) {
  std::vector workflow = {node->scope};
  std::vector<uint32_t> closure;
  while (!workflow.empty()) {
    auto item = *workflow.begin();
    workflow.erase(workflow.begin());
    for (auto &binding : item->bindings) {
      if (binding.name == L"arguments" || binding.name == L"this" ||
          binding.name == L"super") {
        continue;
      }
      auto iname = resolveConstant(module, binding.name);
      if (std::find(closure.begin(), closure.end(), iname) != closure.end()) {
        continue;
      }
      auto scope = item;
      while (scope != node->scope->parent) {
        auto it = scope->declarations.begin();
        for (; it != scope->declarations.end(); it++) {
          if (&*it == binding.declaration) {
            break;
          }
        }
        if (it != scope->declarations.end()) {
          break;
        }
        scope = scope->parent;
      }
      if (scope == node->scope->parent) {
        closure.push_back(iname);
      }
    }
    for (auto &child : item->children) {
      workflow.push_back(child);
    }
  }
  return closure;
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
    if (declar.type == JSSourceDeclaration::TYPE::FUNCTION &&
        declar.node->type != JSNodeType::DECLARATION_CLASS) {
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
    if (n->left->type == JSNodeType::SUPER) {
      if (n->right->type == JSNodeType::PRIVATE_NAME) {
        throw error::JSSyntaxError(
            L"Unexpected private field",
            {
                .filename = module->filename,
                .line = n->right->location.start.line,
                .column = n->right->location.start.column,
                .funcname = L"_.compile",
            });
      }
      generate(module, vm::JSAsmOperator::LOAD_CONST,
               n->right.cast<JSIdentifierLiteral>()->value);
      generate(module, vm::JSAsmOperator::GET_SUPER_FIELD);
    } else {
      resolveNode(ctx, module, n->left);
      if (n->right->type == JSNodeType::LITERAL_IDENTITY) {
        generate(module, vm::JSAsmOperator::LOAD_CONST,
                 n->right.cast<JSIdentifierLiteral>()->value);
        generate(module, vm::JSAsmOperator::GET_FIELD);
      } else if (n->right->type == JSNodeType::PRIVATE_NAME) {
        auto f = n->right.cast<JSPrivateName>();
        if (n->left->type != JSNodeType::THIS) {
          throw error::JSSyntaxError(
              fmt::format(L"Private field '#{}' must be declared in an "
                          L"enclosing class",
                          f->value),
              {
                  .filename = module->filename,
                  .line = n->right->location.start.line,
                  .column = n->right->location.start.column,
                  .funcname = L"_.compile",
              });
        }
        generate(module, vm::JSAsmOperator::LOAD_CONST, L"#" + f->value);
        generate(module, vm::JSAsmOperator::PUSH, ctx.currentClass);
        generate(module, vm::JSAsmOperator::GET_PRIVATE_FIELD);
      }
    }
  } else if (node->type == JSNodeType::EXPRESSION_COMPUTED_MEMBER) {
    if (n->left->type == JSNodeType::SUPER) {
      if (n->right->type == JSNodeType::PRIVATE_NAME) {
        throw error::JSSyntaxError(
            L"Unexpected private field",
            {
                .filename = module->filename,
                .line = n->right->location.start.line,
                .column = n->right->location.start.column,
                .funcname = L"_.compile",
            });
      }
      resolveNode(ctx, module, n->right);
      generate(module, vm::JSAsmOperator::GET_SUPER_FIELD);
    } else {
      resolveNode(ctx, module, n->left);
      resolveNode(ctx, module, n->right);
      generate(module, vm::JSAsmOperator::GET_FIELD);
    }
  } else if (node->type == JSNodeType::EXPRESSION_OPTIONAL_MEMBER) {
    resolveNode(ctx, module, n->left);
    offsets.push_back(module->codes.size() + sizeof(uint16_t));
    generate(module, vm::JSAsmOperator::JNULL, 0U);
    generate(module, vm::JSAsmOperator::LOAD_CONST,
             n->right.cast<JSIdentifierLiteral>()->value);
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
void JSGenerator::resolveStatements(JSGeneratorContext &ctx,
                                    common::AutoPtr<JSModule> &module,
                                    const JSNodeArray &nodes) {
  bool clean = false;
  for (auto &item : nodes) {
    if (clean) {
      generate(module, vm::JSAsmOperator::POP, 1U);
      clean = false;
    }
    resolveNode(ctx, module, item);
    if (item->type == JSNodeType::STATEMENT_EXPRESSION) {
      auto expr = item.cast<JSExpressionStatement>()->expression;
      if (expr->type != JSNodeType::EXPRESSION_ASSIGMENT &&
          expr->type != JSNodeType::DECLARATION_CLASS &&
          expr->type != JSNodeType::DECLARATION_FUNCTION) {
        clean = true;
      }
    }
  }
}

void JSGenerator::resolveLiteralRegex(JSGeneratorContext &ctx,
                                      common::AutoPtr<JSModule> &module,
                                      const common::AutoPtr<JSNode> &node) {
  auto n = node.cast<JSRegexLiteral>();
  uint32_t flag = 0;
  if (n->hasIndices) {
  }
  if (n->global) {
    flag |= vm::JSRegExpFlag::GLOBAL;
  }
  if (n->ignoreCase) {
    flag |= vm::JSRegExpFlag::ICASE;
  }
  if (n->multiline) {
    flag |= vm::JSRegExpFlag::MULTILINE;
  }
  if (n->dotAll) {
    flag |= vm::JSRegExpFlag::DOTALL;
  }
  if (n->sticky) {
    flag |= vm::JSRegExpFlag::STICKY;
  }
  generate(module, vm::JSAsmOperator::LOAD_CONST, n->value);
  generate(module, vm::JSAsmOperator::PUSH_REGEX, flag);
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
           node.cast<JSStringLiteral>()->value);
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
  generate(module, vm::JSAsmOperator::LOAD, n->value);
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
        throw error::JSSyntaxError(L"Invalid optional chian for template",
                                   {
                                       .filename = module->filename,
                                       .line = node->location.start.line,
                                       .column = node->location.start.column,
                                   });
      }
      generate(module, vm::JSAsmOperator::LOAD_CONST,
               m->right.cast<JSIdentifierLiteral>()->value);
      opt = vm::JSAsmOperator::MEMBER_CALL;
    } else if (n->tag->type == JSNodeType::EXPRESSION_COMPUTED_MEMBER) {
      auto m = n->tag.cast<JSComputedMemberExpression>();
      resolveMemberChian(ctx, module, m->left, offsets);
      if (!offsets.empty()) {
        throw error::JSSyntaxError(L"Invalid optional chian for template",
                                   {
                                       .filename = module->filename,
                                       .line = node->location.start.line,
                                       .column = node->location.start.column,
                                   });
      }
      resolveNode(ctx, module, m->right);
      opt = vm::JSAsmOperator::MEMBER_CALL;
    } else if (n->tag->type == JSNodeType::EXPRESSION_OPTIONAL_MEMBER ||
               n->tag->type ==
                   JSNodeType::EXPRESSION_OPTIONAL_COMPUTED_MEMBER) {
      throw error::JSSyntaxError(L"Invalid optional chian for template",
                                 {
                                     .filename = module->filename,
                                     .line = node->location.start.line,
                                     .column = node->location.start.column,
                                 });
    } else {
      resolveNode(ctx, module, n->tag);
    }
    generate(module, vm::JSAsmOperator::PUSH_ARRAY);
    for (size_t index = 0; index < n->quasis.size(); index++) {
      generate(module, vm::JSAsmOperator::LOAD_CONST, n->quasis[index]);
      generate(module, vm::JSAsmOperator::PUSH, (double)index);
      generate(module, vm::JSAsmOperator::SET_FIELD);
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
      generate(module, vm::JSAsmOperator::LOAD_CONST, raw);
      generate(module, vm::JSAsmOperator::PUSH, (double)index);
      generate(module, vm::JSAsmOperator::SET_FIELD);
    }
    generate(module, vm::JSAsmOperator::LOAD_CONST, L"raw");
    generate(module, vm::JSAsmOperator::SET_FIELD);
    for (auto &exp : n->expressions) {
      resolveNode(ctx, module, exp);
    }
    module->sourceMap[module->codes.size()] = n->tag->location.end;
    generate(module, opt, (uint32_t)n->expressions.size() + 1);
  } else {
    generate(module, vm::JSAsmOperator::LOAD_CONST, n->quasis[0]);
    for (size_t i = 0; i < n->expressions.size(); i++) {
      resolveNode(ctx, module, n->expressions[i]);
      generate(module, vm::JSAsmOperator::LOAD_CONST, n->quasis[i + 1]);
      generate(module, vm::JSAsmOperator::ADD);
      generate(module, vm::JSAsmOperator::ADD);
    }
  }
}

void JSGenerator::resolveLiteralBigint(JSGeneratorContext &ctx,
                                       common::AutoPtr<JSModule> &module,
                                       const common::AutoPtr<JSNode> &node) {
  auto n = node.cast<JSBigIntLiteral>();
  generate(module, vm::JSAsmOperator::PUSH_BIGINT, n->value);
}

void JSGenerator::resolveThis(JSGeneratorContext &ctx,
                              common::AutoPtr<JSModule> &module,
                              const common::AutoPtr<JSNode> &node) {
  generate(module, vm::JSAsmOperator::LOAD, L"this");
}

void JSGenerator::resolveSuper(JSGeneratorContext &ctx,
                               common::AutoPtr<JSModule> &module,
                               const common::AutoPtr<JSNode> &node) {
  throw error::JSSyntaxError(L"Invalid super",
                             {.filename = module->filename,
                              .line = node->location.start.line,
                              .column = node->location.start.column,
                              .funcname = L"constructor"});
}

void JSGenerator::resolveProgram(JSGeneratorContext &ctx,
                                 common::AutoPtr<JSModule> &module,
                                 const common::AutoPtr<JSNode> &node) {
  auto n = node.cast<JSProgram>();
  pushLexScope(ctx, module, node->scope);
  for (auto &dir : n->directives) {
    auto d = dir.cast<JSDirective>();
    generate(module, vm::JSAsmOperator::SETUP_DIRECTIVE, d->value);
  }
  JSNodeArray body;
  for (auto &item : n->body) {
    if (item->type == JSNodeType::IMPORT_DECLARATION) {
      resolveNode(ctx, module, item);
    } else {
      body.push_back(item);
    }
  }
  resolveStatements(ctx, module, body);
  if (ctx.evalType == engine::JSEvalType::FUNCTION) {
    generate(module, vm::JSAsmOperator::PUSH_UNDEFINED);
    generate(module, vm::JSAsmOperator::RET);
  } else {
    generate(module, vm::JSAsmOperator::HLT);
  }
  for (auto &item : ctx.currentScope->functionDeclarations) {
    resolveDeclarationFunction(ctx, module, item);
  }
  for (auto i = n->directives.rbegin(); i != n->directives.rend(); i++) {
    auto d = i->cast<JSDirective>();
    generate(module, vm::JSAsmOperator::CLEANUP_DIRECTIVE, d->value);
  }
  popLexScope(ctx, module);
}

void JSGenerator::resolveStatementBlock(JSGeneratorContext &ctx,
                                        common::AutoPtr<JSModule> &module,
                                        const common::AutoPtr<JSNode> &node) {
  auto n = node.cast<JSBlockStatement>();
  pushScope(ctx, module, n->scope);
  resolveStatements(ctx, module, n->body);
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
      throw error::JSSyntaxError(L"Illegal break statement",
                                 {
                                     .filename = module->filename,
                                     .line = node->location.start.line,
                                     .column = node->location.start.column,
                                 });
    } else {
      throw error::JSSyntaxError(fmt::format(L"Undefined label '{}'", label),
                                 {
                                     .filename = module->filename,
                                     .line = node->location.start.line,
                                     .column = node->location.start.column,
                                 });
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
      throw error::JSSyntaxError(L"Illegal continue statement",
                                 {
                                     .filename = module->filename,
                                     .line = node->location.start.line,
                                     .column = node->location.start.column,
                                 });
    } else {
      throw error::JSSyntaxError(fmt::format(L"Undefined label '{}'", label),
                                 {
                                     .filename = module->filename,
                                     .line = node->location.start.line,
                                     .column = node->location.start.column,
                                 });
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
      throw error::JSSyntaxError(L"invalid continue",
                                 {
                                     .filename = module->filename,
                                     .line = node->location.start.line,
                                     .column = node->location.start.column,
                                 });
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
             n->binding.cast<JSIdentifierLiteral>()->value);
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
  auto n = node.cast<JSForOfStatement>();
  _labels.push_back({{label, ctx.scopeChain}, {}});
  resolveNode(ctx, module, n->expression);
  generate(module, vm::JSAsmOperator::PUSH_UNDEFINED); // gen
  generate(module, vm::JSAsmOperator::PUSH_UNDEFINED); // res
  generate(module, vm::JSAsmOperator::PUSH_UNDEFINED); // value
  generate(module, vm::JSAsmOperator::PUSH_UNDEFINED); // done
  auto start = (uint32_t)module->codes.size();
  generate(module, vm::JSAsmOperator::POP, 1U); // remove res
  generate(module, vm::JSAsmOperator::POP, 1U); // remove done
  generate(module, vm::JSAsmOperator::POP, 1U); // remove value
  generate(module, vm::JSAsmOperator::AWAIT_NEXT);
  generate(module, vm::JSAsmOperator::PUSH_VALUE, 1U);
  generate(module, vm::JSAsmOperator::LOAD_CONST, L"value");
  generate(module, vm::JSAsmOperator::GET_FIELD);
  generate(module, vm::JSAsmOperator::PUSH_VALUE, 2U);
  generate(module, vm::JSAsmOperator::LOAD_CONST, L"done");
  generate(module, vm::JSAsmOperator::GET_FIELD); // gen,res,done,value
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

void JSGenerator::resolveStatementExpression(
    JSGeneratorContext &ctx, common::AutoPtr<JSModule> &module,
    const common::AutoPtr<JSNode> &node) {
  auto n = node.cast<JSExpressionStatement>();
  auto old = ctx.lexContextType;
  ctx.lexContextType = engine::JSEvalType::EXPRESSION;
  resolveNode(ctx, module, n->expression);
  ctx.lexContextType = old;
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
        resolveConstant(module, node.cast<JSIdentifierLiteral>()->value);
    generate(module, vm::JSAsmOperator::STORE, name);
  } else if (node->type == JSNodeType::EXPRESSION_MEMBER) {
    auto n = node.cast<JSMemberExpression>();
    if (n->left->type == JSNodeType::SUPER) {
      if (n->right->type == JSNodeType::PRIVATE_NAME) {
        throw error::JSSyntaxError(
            L"Unexpected private field",
            {
                .filename = module->filename,
                .line = n->right->location.start.line,
                .column = n->right->location.start.column,
                .funcname = L"_.compile",
            });
      }
      generate(module, vm::JSAsmOperator::LOAD_CONST,
               n->right.cast<JSIdentifierLiteral>()->value);
      generate(module, vm::JSAsmOperator::SET_SUPER_FIELD);
    } else {
      resolveMemberChian(ctx, module, n->left, offsets);
      if (!offsets.empty()) {
        throw error::JSSyntaxError(L"Invalid left-hand side in assignment",
                                   {
                                       .filename = module->filename,
                                       .line = node->location.start.line,
                                       .column = node->location.start.column,
                                   });
      }
      generate(module, vm::JSAsmOperator::PUSH_VALUE, 2U);
      if (n->right->type == JSNodeType::LITERAL_IDENTITY) {
        generate(module, vm::JSAsmOperator::LOAD_CONST,
                 n->right.cast<JSIdentifierLiteral>()->value);
        generate(module, vm::JSAsmOperator::SET_FIELD);
      } else if (n->right->type == JSNodeType::PRIVATE_NAME) {
        auto f = n->right.cast<JSPrivateName>();
        if (n->left->type != JSNodeType::THIS) {
          throw error::JSSyntaxError(
              fmt::format(L"Private field '#{}' must be declared in an "
                          L"enclosing class",
                          f->value),
              {
                  .filename = module->filename,
                  .line = n->right->location.start.line,
                  .column = n->right->location.start.column,
                  .funcname = L"_.compile",
              });
        }
        generate(module, vm::JSAsmOperator::LOAD_CONST, L"#" + f->value);
        generate(module, vm::JSAsmOperator::PUSH, ctx.currentClass);
        generate(module, vm::JSAsmOperator::SET_PRIVATE_FIELD);
      }
      generate(module, vm::JSAsmOperator::POP, 1U); // self
      generate(module, vm::JSAsmOperator::POP, 1U); // raw_value
    }
  } else if (node->type == JSNodeType::EXPRESSION_COMPUTED_MEMBER) {
    auto n = node.cast<JSComputedMemberExpression>();
    if (n->left->type == JSNodeType::SUPER) {
      if (n->right->type == JSNodeType::PRIVATE_NAME) {
        throw error::JSSyntaxError(
            L"Unexpected private field",
            {
                .filename = module->filename,
                .line = n->right->location.start.line,
                .column = n->right->location.start.column,
                .funcname = L"_.compile",
            });
      }
      resolveNode(ctx, module, n->right);
      generate(module, vm::JSAsmOperator::SET_SUPER_FIELD);
    } else {
      resolveMemberChian(ctx, module, n->left, offsets);
      if (!offsets.empty()) {
        throw error::JSSyntaxError(L"Invalid left-hand side in assignment",
                                   {
                                       .filename = module->filename,
                                       .line = node->location.start.line,
                                       .column = node->location.start.column,
                                   });
      }
      generate(module, vm::JSAsmOperator::PUSH_VALUE, 2U);
      resolveNode(ctx, module, n->right);
      generate(module, vm::JSAsmOperator::SET_FIELD);
      generate(module, vm::JSAsmOperator::POP, 1U); // self
      generate(module, vm::JSAsmOperator::POP, 1U); // raw_value
    }
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
                   oitem->identifier.cast<JSIdentifierLiteral>()->value);

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
    throw error::JSSyntaxError(L"Invalid left-hand side in assignment",
                               {
                                   .filename = module->filename,
                                   .line = node->location.start.line,
                                   .column = node->location.start.column,
                               });
  }
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
             n->identifier.cast<JSIdentifierLiteral>()->value);
  } else {
    resolveNode(ctx, module, n->identifier);
  }
  generate(module, vm::JSAsmOperator::SET_FIELD);
}

void JSGenerator::resolveObjectMethod(JSGeneratorContext &ctx,
                                      common::AutoPtr<JSModule> &module,
                                      const common::AutoPtr<JSNode> &node) {
  auto n = node.cast<JSObjectMethod>();
  ctx.currentScope->functionDeclarations.push_back(
      (JSNode *)node.getRawPointer());
  if (n->generator) {
    if (n->async) {
      generate(module, vm::JSAsmOperator::PUSH_ASYNC_GENERATOR);
    } else {
      generate(module, vm::JSAsmOperator::PUSH_GENERATOR);
    }
  } else {
    if (n->async) {
      generate(module, vm::JSAsmOperator::PUSH_ASYNC);
    } else {
      generate(module, vm::JSAsmOperator::PUSH_FUNCTION);
    }
  }
  ctx.currentScope->functionAddr[node->id] =
      module->codes.size() + sizeof(uint16_t);
  generate(module, vm::JSAsmOperator::SET_FUNC_ADDRESS, 0U);
  generate(module, vm::JSAsmOperator::SET_FUNC_SOURCE, L"[method (anonymous)]");
  if (n->identifier->type == JSNodeType::LITERAL_IDENTITY) {
    generate(module, vm::JSAsmOperator::LOAD_CONST,
             n->identifier.cast<JSIdentifierLiteral>()->value);
  } else {
    resolveNode(ctx, module, n->identifier);
  }
  generate(module, vm::JSAsmOperator::SET_FIELD);
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
  generate(module, vm::JSAsmOperator::SET_FUNC_SOURCE,
           std::wstring(L"[") +
               (n->kind == JSAccessorKind::GET ? L"get" : L"set") +
               L" accessor]");
  if (n->identifier->type == JSNodeType::LITERAL_IDENTITY) {
    generate(module, vm::JSAsmOperator::LOAD_CONST,
             n->identifier.cast<JSIdentifierLiteral>()->value);
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
  if (n->opt == L",") {
    generate(module, vm::JSAsmOperator::POP, 1U);
    resolveNode(ctx, module, n->right);
  } else if (n->opt == L"&&" || n->opt == L"||" || n->opt == L"??") {
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
    } else if (n->opt == L"instanceof") {
      generate(module, vm::JSAsmOperator::INSTANCE_OF);
    }
  }
}

void JSGenerator::resolveExpressionMember(JSGeneratorContext &ctx,
                                          common::AutoPtr<JSModule> &module,
                                          const common::AutoPtr<JSNode> &node) {
  std::vector<size_t> offsets;
  auto n = node.cast<JSMemberExpression>();
  if (n->left->type == JSNodeType::SUPER) {
    if (n->right->type == JSNodeType::PRIVATE_NAME) {
      throw error::JSSyntaxError(L"Unexpected private field",
                                 {
                                     .filename = module->filename,
                                     .line = n->right->location.start.line,
                                     .column = n->right->location.start.column,
                                     .funcname = L"_.compile",
                                 });
    }
    generate(module, vm::JSAsmOperator::LOAD_CONST,
             n->right.cast<JSIdentifierLiteral>()->value);
    generate(module, vm::JSAsmOperator::GET_SUPER_FIELD);
  } else {
    resolveMemberChian(ctx, module, n->left, offsets);
    if (n->right->type == JSNodeType::PRIVATE_NAME) {
      auto f = n->right.cast<JSPrivateName>();
      if (n->left->type != JSNodeType::THIS) {
        throw error::JSSyntaxError(
            fmt::format(L"Private field '#{}' must be declared in an "
                        L"enclosing class",
                        f->value),
            {
                .filename = module->filename,
                .line = n->right->location.start.line,
                .column = n->right->location.start.column,
                .funcname = L"_.compile",
            });
      }
      generate(module, vm::JSAsmOperator::LOAD_CONST, L"#" + f->value);
      generate(module, vm::JSAsmOperator::PUSH, ctx.currentClass);
      generate(module, vm::JSAsmOperator::GET_PRIVATE_FIELD);
    } else {
      generate(module, vm::JSAsmOperator::LOAD_CONST,
               n->right.cast<JSIdentifierLiteral>()->value);
      generate(module, vm::JSAsmOperator::GET_FIELD);
    }
    for (auto &offset : offsets) {
      *(uint32_t *)(module->codes.data() + offset) =
          (uint32_t)(module->codes.size());
    }
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
           n->right.cast<JSIdentifierLiteral>()->value);
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
  if (n->left->type == JSNodeType::SUPER) {
    if (n->right->type == JSNodeType::PRIVATE_NAME) {
      throw error::JSSyntaxError(L"Unexpected private field",
                                 {
                                     .filename = module->filename,
                                     .line = n->right->location.start.line,
                                     .column = n->right->location.start.column,
                                     .funcname = L"_.compile",
                                 });
    }
    resolveNode(ctx, module, n->right);
    generate(module, vm::JSAsmOperator::GET_SUPER_FIELD);
  } else {
    resolveMemberChian(ctx, module, n->left, offsets);
    resolveNode(ctx, module, n->right);
    generate(module, vm::JSAsmOperator::GET_FIELD);
    for (auto &offset : offsets) {
      *(uint32_t *)(module->codes.data() + offset) =
          (uint32_t)(module->codes.size());
    }
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
    if (member->left->type == JSNodeType::SUPER) {
      if (func->type == JSNodeType::EXPRESSION_COMPUTED_MEMBER) {
        resolveNode(ctx, module, member->right);
      } else {
        if (member->right->type == JSNodeType::PRIVATE_NAME) {
          throw error::JSSyntaxError(
              L"Unexpected private field",
              {
                  .filename = module->filename,
                  .line = member->right->location.start.line,
                  .column = member->right->location.start.column,
                  .funcname = L"_.compile",
              });
        }
        generate(module, vm::JSAsmOperator::LOAD_CONST,
                 member->right.cast<JSIdentifierLiteral>()->value);
      }
      opt = vm::JSAsmOperator::SUPER_MEMBER_CALL;
    } else {
      resolveMemberChian(ctx, module, member->left, offsets);
      if (func->type == JSNodeType::EXPRESSION_COMPUTED_MEMBER) {
        resolveNode(ctx, module, member->right);
      } else {
        if (member->right->type == JSNodeType::PRIVATE_NAME) {
          auto f = member->right.cast<JSPrivateName>();
          if (member->left->type != JSNodeType::THIS) {
            throw error::JSSyntaxError(
                fmt::format(L"Private field '#{}' must be declared in an "
                            L"enclosing class",
                            f->value),
                {
                    .filename = module->filename,
                    .line = member->right->location.start.line,
                    .column = member->right->location.start.column,
                    .funcname = L"_.compile",
                });
          }
          generate(module, vm::JSAsmOperator::LOAD_CONST, L"#" + f->value);
          opt = vm::JSAsmOperator::MEMBER_PRIVATE_CALL;
        } else {
          generate(module, vm::JSAsmOperator::LOAD_CONST,
                   member->right.cast<JSIdentifierLiteral>()->value);
          opt = vm::JSAsmOperator::MEMBER_CALL;
        }
      }
    }
  } else if (func->type == JSNodeType::EXPRESSION_OPTIONAL_MEMBER) {
    auto member = func.cast<JSOptionalMemberExpression>();
    resolveMemberChian(ctx, module, member->left, offsets);
    offsets.push_back(module->codes.size() + sizeof(uint16_t));
    generate(module, vm::JSAsmOperator::JNULL, 0U);
    generate(module, vm::JSAsmOperator::LOAD_CONST,
             member->right.cast<JSIdentifierLiteral>()->value);
    opt = vm::JSAsmOperator::MEMBER_CALL;
  } else if (func->type == JSNodeType::EXPRESSION_OPTIONAL_COMPUTED_MEMBER) {
    auto member = func.cast<JSOptionalComputedMemberExpression>();
    resolveMemberChian(ctx, module, member->left, offsets);
    offsets.push_back(module->codes.size() + sizeof(uint16_t));
    generate(module, vm::JSAsmOperator::JNULL, 0U);
    resolveNode(ctx, module, member->right);
    opt = vm::JSAsmOperator::MEMBER_CALL;
  } else if (func->type == JSNodeType::SUPER) {
    opt = vm::JSAsmOperator::SUPER_CALL;
  } else {
    resolveNode(ctx, module, n->left);
  }
  for (auto &arg : n->arguments) {
    resolveNode(ctx, module, arg);
  }
  if (opt == vm::JSAsmOperator::MEMBER_PRIVATE_CALL) {
    generate(module, vm::JSAsmOperator::PUSH, ctx.currentClass);
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
      generate(module, vm::JSAsmOperator::LOAD_CONST,
               member->right.cast<JSIdentifierLiteral>()->value);
    }
    opt = vm::JSAsmOperator::MEMBER_OPTIONAL_CALL;
  } else if (func->type == JSNodeType::EXPRESSION_OPTIONAL_MEMBER) {
    auto member = func.cast<JSOptionalMemberExpression>();
    resolveMemberChian(ctx, module, member->left, offsets);
    offsets.push_back(module->codes.size() + sizeof(uint16_t));
    generate(module, vm::JSAsmOperator::JNULL, 0U);
    generate(module, vm::JSAsmOperator::LOAD_CONST,
             member->right.cast<JSIdentifierLiteral>()->value);
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
                    n->right.cast<JSIdentifierLiteral>()->value),
        {
            .filename = module->filename,
            .line = node->location.start.line,
            .column = node->location.start.column,
        });
  } else if (n->right->type == JSNodeType::EXPRESSION_MEMBER) {
    auto m = n->right.cast<JSMemberExpression>();
    resolveMemberChian(ctx, module, m->left, offsets);
    generate(module, vm::JSAsmOperator::LOAD_CONST,
             m->right.cast<JSIdentifierLiteral>()->value);
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
             m->right.cast<JSIdentifierLiteral>()->value);
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
  if (n->opt == L"+=") {
    resolveNode(ctx, module, host);
    resolveNode(ctx, module, value);
    generate(module, vm::JSAsmOperator::ADD);
  } else if (n->opt == L"-=") {
    resolveNode(ctx, module, host);
    resolveNode(ctx, module, value);
    generate(module, vm::JSAsmOperator::SUB);
  } else if (n->opt == L"**=") {
    resolveNode(ctx, module, host);
    resolveNode(ctx, module, value);
    generate(module, vm::JSAsmOperator::POW);
  } else if (n->opt == L"*=") {
    resolveNode(ctx, module, host);
    resolveNode(ctx, module, value);
    generate(module, vm::JSAsmOperator::MUL);
  } else if (n->opt == L"/=") {
    resolveNode(ctx, module, host);
    resolveNode(ctx, module, value);
    generate(module, vm::JSAsmOperator::DIV);
  } else if (n->opt == L"%=") {
    resolveNode(ctx, module, host);
    resolveNode(ctx, module, value);
    generate(module, vm::JSAsmOperator::MOD);
  } else if (n->opt == L">>>=") {
    resolveNode(ctx, module, host);
    resolveNode(ctx, module, value);
    generate(module, vm::JSAsmOperator::USHR);
  } else if (n->opt == L"<<=") {
    resolveNode(ctx, module, host);
    resolveNode(ctx, module, value);
    generate(module, vm::JSAsmOperator::SHL);
  } else if (n->opt == L">>=") {
    resolveNode(ctx, module, host);
    resolveNode(ctx, module, value);
    generate(module, vm::JSAsmOperator::SHR);
  } else if (n->opt == L"&&=") {
    resolveNode(ctx, module, host);
    auto offset = module->codes.size() + sizeof(uint16_t);
    generate(module, vm::JSAsmOperator::JFALSE, 0U);
    generate(module, vm::JSAsmOperator::POP, 1U);
    resolveNode(ctx, module, value);
    *(uint32_t *)(module->codes.data() + offset) =
        (uint32_t)module->codes.size();
  } else if (n->opt == L"||=") {
    resolveNode(ctx, module, host);
    auto offset = module->codes.size() + sizeof(uint16_t);
    generate(module, vm::JSAsmOperator::JTRUE, 0U);
    generate(module, vm::JSAsmOperator::POP, 1U);
    resolveNode(ctx, module, value);
    *(uint32_t *)(module->codes.data() + offset) =
        (uint32_t)module->codes.size();
  } else if (n->opt == L"&=") {
    resolveNode(ctx, module, host);
    resolveNode(ctx, module, value);
    generate(module, vm::JSAsmOperator::AND);
  } else if (n->opt == L"^=") {
    resolveNode(ctx, module, host);
    resolveNode(ctx, module, value);
    generate(module, vm::JSAsmOperator::XOR);
  } else if (n->opt == L"|=") {
    resolveNode(ctx, module, host);
    resolveNode(ctx, module, value);
    generate(module, vm::JSAsmOperator::OR);
  } else {
    resolveNode(ctx, module, value);
  }
  resolveVariableIdentifier(ctx, module, host);
}

void JSGenerator::resolveClassMethod(JSGeneratorContext &ctx,
                                     common::AutoPtr<JSModule> &module,
                                     const common::AutoPtr<JSNode> &node) {
  auto n = node.cast<JSClassMethod>();
  if (!n->static_) {
    generate(module, vm::JSAsmOperator::PUSH_VALUE, 1U);
    generate(module, vm::JSAsmOperator::LOAD_CONST, L"prototype");
    generate(module, vm::JSAsmOperator::GET_FIELD);
  }
  ctx.currentScope->functionDeclarations.push_back(
      (JSNode *)node.getRawPointer());
  if (n->generator) {
    if (n->async) {
      generate(module, vm::JSAsmOperator::PUSH_ASYNC_GENERATOR);
    } else {
      generate(module, vm::JSAsmOperator::PUSH_GENERATOR);
    }
  } else {
    if (n->async) {
      generate(module, vm::JSAsmOperator::PUSH_ASYNC);
    } else {
      generate(module, vm::JSAsmOperator::PUSH_FUNCTION);
    }
  }
  ctx.currentScope->functionAddr[node->id] =
      module->codes.size() + sizeof(uint16_t);
  generate(module, vm::JSAsmOperator::SET_FUNC_ADDRESS, 0U);
  generate(module, vm::JSAsmOperator::SET_FUNC_SOURCE, L"[method (anonymous)]");
  if (n->identifier->type == JSNodeType::PRIVATE_NAME) {
    generate(module, vm::JSAsmOperator::LOAD_CONST,
             L"#" + n->identifier.cast<JSPrivateName>()->value);
    generate(module, vm::JSAsmOperator::PUSH, ctx.currentClass);
    generate(module, vm::JSAsmOperator::SET_PRIVATE_METHOD);
  } else {
    if (n->identifier->type == JSNodeType::LITERAL_IDENTITY) {
      generate(module, vm::JSAsmOperator::LOAD_CONST,
               n->identifier.cast<JSIdentifierLiteral>()->value);
    } else {
      resolveNode(ctx, module, n->identifier);
    }
    generate(module, vm::JSAsmOperator::SET_FIELD);
  }
  if (!n->static_) {
    generate(module, vm::JSAsmOperator::POP, 1U);
  }
}

void JSGenerator::resolveClassProperty(JSGeneratorContext &ctx,
                                       common::AutoPtr<JSModule> &module,
                                       const common::AutoPtr<JSNode> &node) {
  auto n = node.cast<JSClassProperty>();
  if (n->static_) {
    if (n->value != nullptr) {
      resolveNode(ctx, module, n->value);
    } else {
      generate(module, vm::JSAsmOperator::PUSH_UNDEFINED);
    }
    if (n->identifier->type == JSNodeType::PRIVATE_NAME) {
      generate(module, vm::JSAsmOperator::LOAD_CONST,
               L"#" + n->identifier.cast<JSPrivateName>()->value);
      generate(module, vm::JSAsmOperator::PUSH, ctx.currentClass);
      generate(module, vm::JSAsmOperator::SET_PRIVATE_FIELD);
    } else {
      if (n->identifier->type == JSNodeType::LITERAL_IDENTITY) {
        generate(module, vm::JSAsmOperator::LOAD_CONST,
                 n->identifier.cast<JSIdentifierLiteral>()->value);
      } else if (n->identifier->type == JSNodeType::LITERAL_STRING) {
        generate(module, vm::JSAsmOperator::LOAD_CONST,
                 n->identifier.cast<JSStringLiteral>()->value);
      } else {
        resolveNode(ctx, module, n->identifier);
      }
      generate(module, vm::JSAsmOperator::SET_FIELD);
    }
  }
}

void JSGenerator::resolveClassAccessor(JSGeneratorContext &ctx,
                                       common::AutoPtr<JSModule> &module,
                                       const common::AutoPtr<JSNode> &node) {

  auto n = node.cast<JSClassAccessor>();
  if (!n->static_) {
    generate(module, vm::JSAsmOperator::PUSH_VALUE, 1U);
    generate(module, vm::JSAsmOperator::LOAD_CONST, L"prototype");
    generate(module, vm::JSAsmOperator::GET_FIELD);
  }
  ctx.currentScope->functionDeclarations.push_back(
      (JSNode *)node.getRawPointer());
  generate(module, vm::JSAsmOperator::PUSH_FUNCTION);
  ctx.currentScope->functionAddr[node->id] =
      module->codes.size() + sizeof(uint16_t);
  generate(module, vm::JSAsmOperator::SET_FUNC_ADDRESS, 0U);
  generate(module, vm::JSAsmOperator::SET_FUNC_SOURCE,
           std::wstring(L"[") +
               (n->kind == JSAccessorKind::GET ? L"get" : L"set") + L"]");
  if (n->identifier->type == JSNodeType::PRIVATE_NAME) {
    generate(module, vm::JSAsmOperator::LOAD_CONST,
             L"#" + n->identifier.cast<JSPrivateName>()->value);
    generate(module, vm::JSAsmOperator::PUSH, ctx.currentClass);
    generate(module, vm::JSAsmOperator::SET_PRIVATE_ACCESSOR,
             n->kind == JSAccessorKind::GET ? 1U : 0U);
  } else {
    if (n->identifier->type == JSNodeType::LITERAL_IDENTITY) {
      generate(module, vm::JSAsmOperator::LOAD_CONST,
               n->identifier.cast<JSIdentifierLiteral>()->value);
    } else {
      resolveNode(ctx, module, n->identifier);
    }
    generate(module, vm::JSAsmOperator::SET_ACCESSOR,
             n->kind == JSAccessorKind::GET ? 1U : 0U);
  }
  if (!n->static_) {
    generate(module, vm::JSAsmOperator::POP, 1U);
  }
}

void JSGenerator::resolveStaticBlock(JSGeneratorContext &ctx,
                                     common::AutoPtr<JSModule> &module,
                                     const common::AutoPtr<JSNode> &node) {
  auto n = node.cast<JSBlockStatement>();
  generate(module, vm::JSAsmOperator::PUSH_SCOPE);
  generate(module, vm::JSAsmOperator::PUSH_VALUE, 1U);
  generate(module, vm::JSAsmOperator::CREATE_CONST, L"this");
  resolveStatementBlock(ctx, module, node);
  generate(module, vm::JSAsmOperator::POP_SCOPE);
}

void JSGenerator::resolveImportDeclaration(
    JSGeneratorContext &ctx, common::AutoPtr<JSModule> &module,
    const common::AutoPtr<JSNode> &node) {
  auto n = node.cast<JSImportDeclaration>();
  if (!n->attributes.empty()) {
    for (auto &item : n->attributes) {
      resolveNode(ctx, module, item);
    }
  }
  generate(module, vm::JSAsmOperator::IMPORT_MODULE,
           n->source.cast<JSStringLiteral>()->value);
  for (auto &item : n->items) {
    resolveNode(ctx, module, item);
  }
  generate(module, vm::JSAsmOperator::POP, 1U);
}

void JSGenerator::resolveImportSpecifier(JSGeneratorContext &ctx,
                                         common::AutoPtr<JSModule> &module,
                                         const common::AutoPtr<JSNode> &node) {
  auto n = node.cast<JSImportSpecifier>();
  std::wstring name;
  if (n->identifier->type == JSNodeType::LITERAL_IDENTITY) {
    name = n->identifier.cast<JSIdentifierLiteral>()->value;
  } else {
    name = n->identifier.cast<JSStringLiteral>()->value;
  }
  generate(module, vm::JSAsmOperator::IMPORT, name);
  if (n->alias != nullptr) {
    if (n->alias->type == JSNodeType::LITERAL_IDENTITY) {
      name = n->alias.cast<JSIdentifierLiteral>()->value;
    } else {
      name = n->alias.cast<JSStringLiteral>()->value;
    }
  }
  generate(module, vm::JSAsmOperator::STORE, name);
}

void JSGenerator::resolveImportDefault(JSGeneratorContext &ctx,
                                       common::AutoPtr<JSModule> &module,
                                       const common::AutoPtr<JSNode> &node) {
  auto n = node.cast<JSImportDefaultSpecifier>();
  generate(module, vm::JSAsmOperator::IMPORT, L"default");
  std::wstring name;
  if (n->identifier->type == JSNodeType::LITERAL_IDENTITY) {
    name = n->identifier.cast<JSIdentifierLiteral>()->value;
  } else {
    name = n->identifier.cast<JSStringLiteral>()->value;
  }
  generate(module, vm::JSAsmOperator::STORE, name);
}

void JSGenerator::resolveImportNamespace(JSGeneratorContext &ctx,
                                         common::AutoPtr<JSModule> &module,
                                         const common::AutoPtr<JSNode> &node) {
  auto n = node.cast<JSImportNamespaceSpecifier>();
  generate(module, vm::JSAsmOperator::PUSH_VALUE, 1U);
  std::wstring name;
  if (n->alias->type == JSNodeType::LITERAL_IDENTITY) {
    name = n->alias.cast<JSIdentifierLiteral>()->value;
  } else {
    name = n->alias.cast<JSStringLiteral>()->value;
  }
  generate(module, vm::JSAsmOperator::STORE, name);
}

void JSGenerator::resolveImportAttartube(JSGeneratorContext &ctx,
                                         common::AutoPtr<JSModule> &module,
                                         const common::AutoPtr<JSNode> &node) {
  auto attr = node.cast<JSImportAttribute>();
  auto key = attr->key.cast<JSStringLiteral>()->value;
  auto value = attr->value.cast<JSStringLiteral>()->value;
  generate(module, vm::JSAsmOperator::SET_IMPORT_ATTRIBUTE, key, value);
}

void JSGenerator::resolveExportDeclaration(
    JSGeneratorContext &ctx, common::AutoPtr<JSModule> &module,
    const common::AutoPtr<JSNode> &node) {
  auto n = node.cast<JSExportDeclaration>();
  if (n->source != nullptr) {
    if (!n->attributes.empty()) {
      for (auto &item : n->attributes) {
        resolveNode(ctx, module, item);
      }
    }
    generate(module, vm::JSAsmOperator::IMPORT_MODULE,
             n->source.cast<JSStringLiteral>()->value);

    for (auto &item : n->items) {
      if (item->type == JSNodeType::EXPORT_SPECIFIER) {
        auto n = item.cast<JSExportSpecifier>();
        if (n->identifier->type == JSNodeType::LITERAL_IDENTITY) {
          generate(module, vm::JSAsmOperator::IMPORT,
                   n->identifier.cast<JSIdentifierLiteral>()->value);
        } else {
          generate(module, vm::JSAsmOperator::IMPORT,
                   n->identifier.cast<JSStringLiteral>()->value);
        }
        if (n->alias != nullptr) {
          if (n->alias->type == JSNodeType::LITERAL_IDENTITY) {
            generate(module, vm::JSAsmOperator::EXPORT,
                     n->alias.cast<JSIdentifierLiteral>()->value);
          } else {
            generate(module, vm::JSAsmOperator::EXPORT,
                     n->alias.cast<JSStringLiteral>()->value);
          }
        } else {
          if (n->identifier->type == JSNodeType::LITERAL_IDENTITY) {
            generate(module, vm::JSAsmOperator::EXPORT,
                     n->identifier.cast<JSIdentifierLiteral>()->value);
          } else {
            generate(module, vm::JSAsmOperator::EXPORT,
                     n->identifier.cast<JSStringLiteral>()->value);
          }
        }
      } else if (item->type == JSNodeType::EXPORT_ALL) {
        auto n = item.cast<JSExportAllSpecifier>();
        generate(module, vm::JSAsmOperator::PUSH_VALUE, 1U);
        if (n->alias->type == JSNodeType::LITERAL_IDENTITY) {
          generate(module, vm::JSAsmOperator::EXPORT,
                   n->alias.cast<JSIdentifierLiteral>()->value);
        } else {
          generate(module, vm::JSAsmOperator::EXPORT,
                   n->alias.cast<JSStringLiteral>()->value);
        }
      } else if (item->type == JSNodeType::EXPORT_DEFAULT) {
        generate(module, vm::JSAsmOperator::IMPORT, L"default");
        generate(module, vm::JSAsmOperator::EXPORT, L"default");
      }
    }
    generate(module, vm::JSAsmOperator::POP, 1U);
  } else {
    for (auto &item : n->items) {
      resolveNode(ctx, module, item);
      resolveExport(ctx, module, item);
    }
  }
}

void JSGenerator::resolveExportDefault(JSGeneratorContext &ctx,
                                       common::AutoPtr<JSModule> &module,
                                       const common::AutoPtr<JSNode> &node) {
  auto n = node.cast<JSExportDefaultSpecifier>();
  resolveNode(ctx, module, n->value);
  if (n->value->type == JSNodeType::DECLARATION_FUNCTION) {
    auto func = n->value.cast<JSFunctionDeclaration>();
    generate(module, vm::JSAsmOperator::LOAD,
             func->identifier.cast<JSIdentifierLiteral>()->value);
  } else if (n->value->type == JSNodeType::DECLARATION_CLASS) {
    auto clazz = n->value.cast<JSClassDeclaration>();
    generate(module, vm::JSAsmOperator::LOAD,
             clazz->identifier.cast<JSIdentifierLiteral>()->value);
  }
  generate(module, vm::JSAsmOperator::EXPORT, L"default");
}

void JSGenerator::resolveExportSpecifier(JSGeneratorContext &ctx,
                                         common::AutoPtr<JSModule> &module,
                                         const common::AutoPtr<JSNode> &node) {
  auto n = node.cast<JSExportSpecifier>();
  generate(module, vm::JSAsmOperator::LOAD,
           n->identifier.cast<JSIdentifierLiteral>()->value);
  if (n->alias != nullptr) {
    if (n->alias->type == JSNodeType::LITERAL_IDENTITY) {
      generate(module, vm::JSAsmOperator::EXPORT,
               n->alias.cast<JSIdentifierLiteral>()->value);
    } else {
      generate(module, vm::JSAsmOperator::EXPORT,
               n->alias.cast<JSStringLiteral>()->value);
    }
  } else {
    generate(module, vm::JSAsmOperator::EXPORT,
             n->identifier.cast<JSIdentifierLiteral>()->value);
  }
}

void JSGenerator::resolveDeclarationArrowFunction(
    JSGeneratorContext &ctx, common::AutoPtr<JSModule> &module,
    const common::AutoPtr<JSNode> &node) {
  auto n = node.cast<JSArrowFunctionDeclaration>();
  ctx.currentScope->functionDeclarations.push_back(
      (JSNode *)node.getRawPointer());
  if (n->async) {
    generate(module, vm::JSAsmOperator::PUSH_ASYNC_ARROW);
  } else {
    generate(module, vm::JSAsmOperator::PUSH_ARROW);
  }
  ctx.currentScope->functionAddr[node->id] =
      module->codes.size() + sizeof(uint16_t);
  generate(module, vm::JSAsmOperator::SET_FUNC_ADDRESS, 0U);
  generate(module, vm::JSAsmOperator::SET_FUNC_SOURCE, L"[lambda (anonymous)]");
  auto closures = resolveClosure(ctx, module, node);
  for (auto &closure : closures) {
    generate(module, vm::JSAsmOperator::SET_CLOSURE, closure);
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
      generate(module, vm::JSAsmOperator::LOAD, L"arguments");
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
    auto old = ctx.lexContextType;
    if (n->async) {
      ctx.lexContextType = engine::JSEvalType::ASYNC_FUNCTION;
    } else {
      ctx.lexContextType = engine::JSEvalType::FUNCTION;
    }
    resolveNode(ctx, module, n->body);
    ctx.lexContextType = old;
    if (n->body->type != JSNodeType::DECLARATION_FUNCTION_BODY) {
      generate(module, vm::JSAsmOperator::RET);
      for (auto &item : ctx.currentScope->functionDeclarations) {
        resolveDeclarationFunction(ctx, module, item);
      }
    }
    popLexScope(ctx, module);
  } else {
    auto n = node.cast<JSFunctionDeclaration>();
    auto offset = ctx.currentScope->functionAddr.at(n->id);
    *(uint32_t *)(module->codes.data() + offset) =
        (uint32_t)module->codes.size();
    pushLexScope(ctx, module, node->scope);
    if (n->arguments.size()) {
      generate(module, vm::JSAsmOperator::LOAD, L"arguments");
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
    auto old = ctx.lexContextType;
    if (n->generator) {
      if (n->async) {
        ctx.lexContextType = engine::JSEvalType::ASYNC_GENERATOR;
      } else {
        ctx.lexContextType = engine::JSEvalType::GENERATOR;
      }
    } else {
      if (n->async) {
        ctx.lexContextType = engine::JSEvalType::ASYNC_FUNCTION;
      } else {
        ctx.lexContextType = engine::JSEvalType::FUNCTION;
      }
    }
    resolveNode(ctx, module, n->body);
    ctx.lexContextType = old;
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
      if (n->async) {
        generate(module, vm::JSAsmOperator::PUSH_ASYNC_GENERATOR);
      } else {
        generate(module, vm::JSAsmOperator::PUSH_GENERATOR);
      }
    } else {
      if (n->async) {
        generate(module, vm::JSAsmOperator::PUSH_ASYNC);
      } else {
        generate(module, vm::JSAsmOperator::PUSH_FUNCTION);
      }
    }
    ctx.currentScope->functionAddr[node->id] =
        module->codes.size() + sizeof(uint16_t);
    generate(module, vm::JSAsmOperator::SET_FUNC_ADDRESS, 0U);
    generate(module, vm::JSAsmOperator::SET_FUNC_SOURCE,
             L"[function (anonymous)]");
    auto closures = resolveClosure(ctx, module, node);
    for (auto &closure : closures) {
      generate(module, vm::JSAsmOperator::SET_CLOSURE, closure);
    }
  }
}

void JSGenerator::resolveDeclarationFunctionBody(
    JSGeneratorContext &ctx, common::AutoPtr<JSModule> &module,
    const common::AutoPtr<JSNode> &node) {
  auto n = node.cast<JSFunctionBodyDeclaration>();
  for (auto &dir : n->directives) {
    auto d = dir.cast<JSDirective>();
    generate(module, vm::JSAsmOperator::SETUP_DIRECTIVE, d->value);
  }
  resolveStatements(ctx, module, n->statements);
  generate(module, vm::JSAsmOperator::PUSH_UNDEFINED);
  generate(module, vm::JSAsmOperator::RET);
  for (auto &item : ctx.currentScope->functionDeclarations) {
    resolveDeclarationFunction(ctx, module, item);
  }
  for (auto it = n->directives.rbegin(); it != n->directives.rend(); it++) {
    generate(module, vm::JSAsmOperator::CLEANUP_DIRECTIVE,
             it->cast<JSDirective>()->value);
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
    }
  }
}

void JSGenerator::resolveDeclarationClass(JSGeneratorContext &ctx,
                                          common::AutoPtr<JSModule> &module,
                                          const common::AutoPtr<JSNode> &node) {
  static uint32_t identify = 0;
  auto n = node.cast<JSClassDeclaration>();
  auto oldClass = ctx.currentClass;
  ctx.currentClass = ++identify;
  generate(module, vm::JSAsmOperator::PUSH, ctx.currentClass);
  if (n->extends != nullptr) {
    resolveNode(ctx, module, n->extends);
    generate(module, vm::JSAsmOperator::PUSH_CLASS);
  } else {
    generate(module, vm::JSAsmOperator::PUSH_UNDEFINED);
    generate(module, vm::JSAsmOperator::PUSH_CLASS);
  }
  auto address = module->codes.size() + sizeof(uint16_t);
  generate(module, vm::JSAsmOperator::SET_FUNC_ADDRESS, 0U);
  if (n->identifier != nullptr) {
    generate(module, vm::JSAsmOperator::SET_FUNC_NAME,
             n->identifier.cast<JSIdentifierLiteral>()->value);
    generate(module, vm::JSAsmOperator::SET_FUNC_SOURCE,
             L"[class " + n->identifier.cast<JSIdentifierLiteral>()->value +
                 L"]");
  } else {
    generate(module, vm::JSAsmOperator::SET_FUNC_SOURCE,
             L"[class (anonymous)]");
  }
  pushLexScope(ctx, module, n->scope);
  if (n->identifier != nullptr) {
    auto name = n->identifier.cast<JSIdentifierLiteral>()->value;
    generate(module, vm::JSAsmOperator::PUSH_VALUE, 1U);
    generate(module, vm::JSAsmOperator::CREATE, name);
  }
  auto closures = resolveClosure(ctx, module, node);
  for (auto &closure : closures) {
    generate(module, vm::JSAsmOperator::SET_CLOSURE, closure);
  }
  generate(module, vm::JSAsmOperator::PUSH_OBJECT);
  generate(module, vm::JSAsmOperator::CREATE, L"#initializer");
  generate(module, vm::JSAsmOperator::SET_CLOSURE, L"#initializer");
  if (n->identifier != nullptr) {
    auto identifier = n->identifier.cast<JSIdentifierLiteral>()->value;
    generate(module, vm::JSAsmOperator::STORE, identifier);
    generate(module, vm::JSAsmOperator::LOAD, identifier);
  }
  generate(module, vm::JSAsmOperator::PUSH_VALUE, 1U);
  generate(module, vm::JSAsmOperator::CREATE_CONST, L"this");
  common::AutoPtr<JSNode> constructor;
  std::vector<common::AutoPtr<JSNode>> properties;
  for (auto &item : n->properties) {
    if (item->type == JSNodeType::CLASS_METHOD) {
      auto citem = item.cast<JSClassMethod>();
      if ((citem->identifier->type == JSNodeType::LITERAL_IDENTITY &&
           citem->identifier.cast<JSIdentifierLiteral>()->value ==
               L"constructor") ||
          (citem->identifier->type == JSNodeType::LITERAL_STRING &&
           citem->identifier.cast<JSStringLiteral>()->value ==
               L"constructor")) {
        constructor = citem;
        continue;
      }
    }
    if (item->type == JSNodeType::CLASS_PROPERTY) {
      auto citem = item.cast<JSClassProperty>();
      if (!citem->static_) {
        properties.push_back(citem);
      }
    }
    resolveNode(ctx, module, item);
  }
  auto offset = module->codes.size() + sizeof(uint16_t);
  generate(module, vm::JSAsmOperator::JMP, 0U);
  *(uint32_t *)&module->codes[address] = (uint32_t)module->codes.size();
  generate(module, vm::JSAsmOperator::LOAD, L"this");
  for (auto &prop : properties) {
    auto item = prop.cast<JSClassProperty>();
    if (item->value != nullptr) {
      resolveNode(ctx, module, item->value);
    } else {
      generate(module, vm::JSAsmOperator::PUSH_UNDEFINED);
    }

    if (item->identifier->type == JSNodeType::PRIVATE_NAME) {
      generate(module, vm::JSAsmOperator::LOAD_CONST,
               L"#" + item->identifier.cast<JSPrivateName>()->value);
      generate(module, vm::JSAsmOperator::PUSH, ctx.currentClass);
      generate(module, vm::JSAsmOperator::SET_PRIVATE_FIELD);
    } else {
      if (item->identifier->type == JSNodeType::LITERAL_IDENTITY) {
        generate(module, vm::JSAsmOperator::LOAD_CONST,
                 item->identifier.cast<JSIdentifierLiteral>()->value);
      } else if (item->identifier->type == JSNodeType::LITERAL_STRING) {
        generate(module, vm::JSAsmOperator::LOAD_CONST,
                 item->identifier.cast<JSStringLiteral>()->value);
      } else {
        resolveNode(ctx, module, n->identifier);
      }
      generate(module, vm::JSAsmOperator::SET_FIELD);
    }
  }
  generate(module, vm::JSAsmOperator::POP, 1U);
  if (constructor != nullptr) {
    auto n = constructor.cast<JSClassMethod>();
    pushLexScope(ctx, module, n->scope);
    if (n->arguments.size()) {
      generate(module, vm::JSAsmOperator::LOAD, L"arguments");
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
    auto old = ctx.lexContextType;
    ctx.lexContextType = engine::JSEvalType::FUNCTION;
    resolveNode(ctx, module, n->body);
    ctx.lexContextType = old;
    popLexScope(ctx, module);
  }
  generate(module, vm::JSAsmOperator::PUSH_UNDEFINED);
  generate(module, vm::JSAsmOperator::RET);
  for (auto &item : ctx.currentScope->functionDeclarations) {
    resolveDeclarationFunction(ctx, module, item);
  }
  popLexScope(ctx, module);
  *(uint32_t *)&module->codes[offset] = (uint32_t)module->codes.size();
  if (n->identifier != nullptr) {
    auto identifier = n->identifier.cast<JSIdentifierLiteral>()->value;
    generate(module, vm::JSAsmOperator::STORE, identifier);
    generate(module, vm::JSAsmOperator::LOAD, identifier);
  }
  ctx.currentClass = oldClass;
}

void JSGenerator::resolveNode(JSGeneratorContext &ctx,
                              common::AutoPtr<JSModule> &module,
                              const common::AutoPtr<JSNode> &node) {
  switch (node->type) {
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
  case JSNodeType::STATEMENT_EXPRESSION:
    resolveStatementExpression(ctx, module, node);
    break;
  case JSNodeType::VARIABLE_DECLARATION:
    resolveVariableDeclaration(ctx, module, node);
    break;
  case JSNodeType::VARIABLE_DECLARATOR:
    resolveVariableDeclarator(ctx, module, node);
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
    throw error::JSSyntaxError(L"Unexcepted token",
                               {
                                   .filename = module->filename,
                                   .line = node->location.start.line,
                                   .column = node->location.start.column,
                               });
  }
}

common::AutoPtr<JSModule>
JSGenerator::resolve(const std::wstring &filename, const std::wstring &source,
                     const common::AutoPtr<JSNode> &node,
                     const engine::JSEvalType &type) {
  JSGeneratorContext ctx;
  ctx.evalType = type;
  ctx.lexContextType = type;
  common::AutoPtr<JSModule> module = new JSModule;
  module->filename = filename;
  resolveNode(ctx, module, node);
  return module;
}