#pragma once
#include "base/JSModule.hpp"
#include "base/JSNode.hpp"
#include "common/AutoPtr.hpp"
#include "common/Object.hpp"
#include "vm/JSAsmOperator.hpp"
#include <string>
#include <unordered_map>
#include <vector>

namespace spark::compiler {
class JSGenerator : public common::Object {
private:
  struct JSLexScope : public common::Object {
    std::unordered_map<uint32_t, size_t> functionAddr;
    std::vector<JSNode *> functionDeclarations;
    JSLexScope *parent;
    JSLexScope(JSLexScope *parent = nullptr) : parent(parent) {}
  };

  struct JSGeneratorContext {
    JSLexScope *currentScope;
    size_t scopeChain;
    JSGeneratorContext() {
      currentScope = nullptr;
      scopeChain = 0;
    }
    ~JSGeneratorContext() {
      while (currentScope) {
        auto tmp = currentScope;
        currentScope = currentScope->parent;
        delete tmp;
      }
    }
  };

  std::vector<std::pair<std::pair<std::wstring, size_t>,
                        std::vector<std::pair<JSNode *, size_t>>>>
      _labels;

private:
  void resolveDeclaration(JSGeneratorContext &ctx,
                          common::AutoPtr<JSModule> &module,
                          const JSSourceDeclaration &declaration);

  void resolveClosure(JSGeneratorContext &ctx,
                      common::AutoPtr<JSModule> &module,
                      const JSSourceDeclaration &declaration);

  uint32_t resolveConstant(JSGeneratorContext &ctx,
                           common::AutoPtr<JSModule> &module,
                           const std::wstring &source);

  void reset(JSGeneratorContext &ctx, common::AutoPtr<JSModule> &module,
             common::AutoPtr<JSNode> node);

  void pushLexScope(JSGeneratorContext &ctx, common::AutoPtr<JSModule> &module,
                    const common::AutoPtr<JSSourceScope> &scope);

  void popLexScope(JSGeneratorContext &ctx, common::AutoPtr<JSModule> &module);

  void pushScope(JSGeneratorContext &ctx, common::AutoPtr<JSModule> &module,
                 const common::AutoPtr<JSSourceScope> &scope);

  void popScope(JSGeneratorContext &ctx, common::AutoPtr<JSModule> &module);

  void resolveMemberChian(JSGeneratorContext &ctx,
                          common::AutoPtr<JSModule> &module,
                          const common::AutoPtr<JSNode> &node,
                          std::vector<size_t> &offsets);

private:
  void resolvePrivateName(JSGeneratorContext &ctx,
                          common::AutoPtr<JSModule> &module,
                          const common::AutoPtr<JSNode> &node);

  void resolveLiteralRegex(JSGeneratorContext &ctx,
                           common::AutoPtr<JSModule> &module,
                           const common::AutoPtr<JSNode> &node);

  void resolveLiteralNull(JSGeneratorContext &ctx,
                          common::AutoPtr<JSModule> &module,
                          const common::AutoPtr<JSNode> &node);

  void resolveLiteralString(JSGeneratorContext &ctx,
                            common::AutoPtr<JSModule> &module,
                            const common::AutoPtr<JSNode> &node);

  void resolveLiteralBoolean(JSGeneratorContext &ctx,
                             common::AutoPtr<JSModule> &module,
                             const common::AutoPtr<JSNode> &node);

  void resolveLiteralNumber(JSGeneratorContext &ctx,
                            common::AutoPtr<JSModule> &module,
                            const common::AutoPtr<JSNode> &node);

  void resolveLiteralUndefined(JSGeneratorContext &ctx,
                               common::AutoPtr<JSModule> &module,
                               const common::AutoPtr<JSNode> &node);

  void resolveLiteralIdentity(JSGeneratorContext &ctx,
                              common::AutoPtr<JSModule> &module,
                              const common::AutoPtr<JSNode> &node);

  void resolveLiteralTemplate(JSGeneratorContext &ctx,
                              common::AutoPtr<JSModule> &module,
                              const common::AutoPtr<JSNode> &node);

  void resolveLiteralBigint(JSGeneratorContext &ctx,
                            common::AutoPtr<JSModule> &module,
                            const common::AutoPtr<JSNode> &node);

  void resolveThis(JSGeneratorContext &ctx, common::AutoPtr<JSModule> &module,
                   const common::AutoPtr<JSNode> &node);

  void resolveSuper(JSGeneratorContext &ctx, common::AutoPtr<JSModule> &module,
                    const common::AutoPtr<JSNode> &node);

  void resolveProgram(JSGeneratorContext &ctx,
                      common::AutoPtr<JSModule> &module,
                      const common::AutoPtr<JSNode> &node);

  void resolveStatementBlock(JSGeneratorContext &ctx,
                             common::AutoPtr<JSModule> &module,
                             const common::AutoPtr<JSNode> &node);

  void resolveStatementDebugger(JSGeneratorContext &ctx,
                                common::AutoPtr<JSModule> &module,
                                const common::AutoPtr<JSNode> &node);

  void resolveStatementReturn(JSGeneratorContext &ctx,
                              common::AutoPtr<JSModule> &module,
                              const common::AutoPtr<JSNode> &node);

  void resolveExpressionYield(JSGeneratorContext &ctx,
                              common::AutoPtr<JSModule> &module,
                              const common::AutoPtr<JSNode> &node);

  void resolveExpressionYieldDelegate(JSGeneratorContext &ctx,
                                      common::AutoPtr<JSModule> &module,
                                      const common::AutoPtr<JSNode> &node);

  void resolveStatementLabel(JSGeneratorContext &ctx,
                             common::AutoPtr<JSModule> &module,
                             const common::AutoPtr<JSNode> &node);

  void resolveStatementBreak(JSGeneratorContext &ctx,
                             common::AutoPtr<JSModule> &module,
                             const common::AutoPtr<JSNode> &node);

  void resolveStatementContinue(JSGeneratorContext &ctx,
                                common::AutoPtr<JSModule> &module,
                                const common::AutoPtr<JSNode> &node);

  void resolveStatementIf(JSGeneratorContext &ctx,
                          common::AutoPtr<JSModule> &module,
                          const common::AutoPtr<JSNode> &node);

  void resolveStatementSwitch(JSGeneratorContext &ctx,
                              common::AutoPtr<JSModule> &module,
                              const common::AutoPtr<JSNode> &node);

  void resolveStatementThrow(JSGeneratorContext &ctx,
                             common::AutoPtr<JSModule> &module,
                             const common::AutoPtr<JSNode> &node);

  void resolveStatementTry(JSGeneratorContext &ctx,
                           common::AutoPtr<JSModule> &module,
                           const common::AutoPtr<JSNode> &node);

  void resolveStatementTryCatch(JSGeneratorContext &ctx,
                                common::AutoPtr<JSModule> &module,
                                const common::AutoPtr<JSNode> &node);

  void resolveStatementWhile(JSGeneratorContext &ctx,
                             common::AutoPtr<JSModule> &module,
                             const common::AutoPtr<JSNode> &node,
                             const std::wstring &label = L"");

  void resolveStatementDoWhile(JSGeneratorContext &ctx,
                               common::AutoPtr<JSModule> &module,
                               const common::AutoPtr<JSNode> &node,
                               const std::wstring &label = L"");

  void resolveStatementFor(JSGeneratorContext &ctx,
                           common::AutoPtr<JSModule> &module,
                           const common::AutoPtr<JSNode> &node,
                           const std::wstring &label = L"");

  void resolveStatementForIn(JSGeneratorContext &ctx,
                             common::AutoPtr<JSModule> &module,
                             const common::AutoPtr<JSNode> &node,
                             const std::wstring &label = L"");

  void resolveStatementForOf(JSGeneratorContext &ctx,
                             common::AutoPtr<JSModule> &module,
                             const common::AutoPtr<JSNode> &node,
                             const std::wstring &label = L"");

  void resolveStatementForAwaitOf(JSGeneratorContext &ctx,
                                  common::AutoPtr<JSModule> &module,
                                  const common::AutoPtr<JSNode> &node,
                                  const std::wstring &label = L"");
  
  void resolveStatementExpression(JSGeneratorContext &ctx,
                                  common::AutoPtr<JSModule> &module,
                                  const common::AutoPtr<JSNode> &node);

  void resolveVariableDeclaration(JSGeneratorContext &ctx,
                                  common::AutoPtr<JSModule> &module,
                                  const common::AutoPtr<JSNode> &node);

  void resolveVariableDeclarator(JSGeneratorContext &ctx,
                                 common::AutoPtr<JSModule> &module,
                                 const common::AutoPtr<JSNode> &node);

  void resolveVariableIdentifier(JSGeneratorContext &ctx,
                                 common::AutoPtr<JSModule> &module,
                                 const common::AutoPtr<JSNode> &node);

  void resolveDirective(JSGeneratorContext &ctx,
                        common::AutoPtr<JSModule> &module,
                        const common::AutoPtr<JSNode> &node);
  void resolveObjectProperty(JSGeneratorContext &ctx,
                             common::AutoPtr<JSModule> &module,
                             const common::AutoPtr<JSNode> &node);

  void resolveObjectMethod(JSGeneratorContext &ctx,
                           common::AutoPtr<JSModule> &module,
                           const common::AutoPtr<JSNode> &node);

  void resolveObjectAccessor(JSGeneratorContext &ctx,
                             common::AutoPtr<JSModule> &module,
                             const common::AutoPtr<JSNode> &node);

  void resolveExpressionUnary(JSGeneratorContext &ctx,
                              common::AutoPtr<JSModule> &module,
                              const common::AutoPtr<JSNode> &node);

  void resolveExpressionUpdate(JSGeneratorContext &ctx,
                               common::AutoPtr<JSModule> &module,
                               const common::AutoPtr<JSNode> &node);

  void resolveExpressionBinary(JSGeneratorContext &ctx,
                               common::AutoPtr<JSModule> &module,
                               const common::AutoPtr<JSNode> &node);

  void resolveExpressionMember(JSGeneratorContext &ctx,
                               common::AutoPtr<JSModule> &module,
                               const common::AutoPtr<JSNode> &node);

  void resolveExpressionOptionalMember(JSGeneratorContext &ctx,
                                       common::AutoPtr<JSModule> &module,
                                       const common::AutoPtr<JSNode> &node);

  void resolveExpressionComputedMember(JSGeneratorContext &ctx,
                                       common::AutoPtr<JSModule> &module,
                                       const common::AutoPtr<JSNode> &node);
  void
  resolveExpressionOptionalComputedMember(JSGeneratorContext &ctx,
                                          common::AutoPtr<JSModule> &module,
                                          const common::AutoPtr<JSNode> &node);

  void resolveExpressionCondition(JSGeneratorContext &ctx,
                                  common::AutoPtr<JSModule> &module,
                                  const common::AutoPtr<JSNode> &node);

  void resolveExpressionCall(JSGeneratorContext &ctx,
                             common::AutoPtr<JSModule> &module,
                             const common::AutoPtr<JSNode> &node);

  void resolveExpressionOptionalCall(JSGeneratorContext &ctx,
                                     common::AutoPtr<JSModule> &module,
                                     const common::AutoPtr<JSNode> &node);

  void resolveExpressionNew(JSGeneratorContext &ctx,
                            common::AutoPtr<JSModule> &module,
                            const common::AutoPtr<JSNode> &node);

  void resolveExpressionDelete(JSGeneratorContext &ctx,
                               common::AutoPtr<JSModule> &module,
                               const common::AutoPtr<JSNode> &node);

  void resolveExpressionAwait(JSGeneratorContext &ctx,
                              common::AutoPtr<JSModule> &module,
                              const common::AutoPtr<JSNode> &node);

  void resolveExpressionVoid(JSGeneratorContext &ctx,
                             common::AutoPtr<JSModule> &module,
                             const common::AutoPtr<JSNode> &node);

  void resolveExpressionTypeof(JSGeneratorContext &ctx,
                               common::AutoPtr<JSModule> &module,
                               const common::AutoPtr<JSNode> &node);

  void resolveExpressionGroup(JSGeneratorContext &ctx,
                              common::AutoPtr<JSModule> &module,
                              const common::AutoPtr<JSNode> &node);

  void resolveExpressionAssigment(JSGeneratorContext &ctx,
                                  common::AutoPtr<JSModule> &module,
                                  const common::AutoPtr<JSNode> &node);

  void resolveClassMethod(JSGeneratorContext &ctx,
                          common::AutoPtr<JSModule> &module,
                          const common::AutoPtr<JSNode> &node);

  void resolveClassProperty(JSGeneratorContext &ctx,
                            common::AutoPtr<JSModule> &module,
                            const common::AutoPtr<JSNode> &node);

  void resolveClassAccessor(JSGeneratorContext &ctx,
                            common::AutoPtr<JSModule> &module,
                            const common::AutoPtr<JSNode> &node);

  void resolveStaticBlock(JSGeneratorContext &ctx,
                          common::AutoPtr<JSModule> &module,
                          const common::AutoPtr<JSNode> &node);

  void resolveImportDeclaration(JSGeneratorContext &ctx,
                                common::AutoPtr<JSModule> &module,
                                const common::AutoPtr<JSNode> &node);

  void resolveImportSpecifier(JSGeneratorContext &ctx,
                              common::AutoPtr<JSModule> &module,
                              const common::AutoPtr<JSNode> &node);

  void resolveImportDefault(JSGeneratorContext &ctx,
                            common::AutoPtr<JSModule> &module,
                            const common::AutoPtr<JSNode> &node);

  void resolveImportNamespace(JSGeneratorContext &ctx,
                              common::AutoPtr<JSModule> &module,
                              const common::AutoPtr<JSNode> &node);

  void resolveImportAttartube(JSGeneratorContext &ctx,
                              common::AutoPtr<JSModule> &module,
                              const common::AutoPtr<JSNode> &node);

  void resolveExportDeclaration(JSGeneratorContext &ctx,
                                common::AutoPtr<JSModule> &module,
                                const common::AutoPtr<JSNode> &node);

  void resolveExportDefault(JSGeneratorContext &ctx,
                            common::AutoPtr<JSModule> &module,
                            const common::AutoPtr<JSNode> &node);

  void resolveExportSpecifier(JSGeneratorContext &ctx,
                              common::AutoPtr<JSModule> &module,
                              const common::AutoPtr<JSNode> &node);

  void resolveExportAll(JSGeneratorContext &ctx,
                        common::AutoPtr<JSModule> &module,
                        const common::AutoPtr<JSNode> &node);

  void resolveDeclarationArrowFunction(JSGeneratorContext &ctx,
                                       common::AutoPtr<JSModule> &module,
                                       const common::AutoPtr<JSNode> &node);

  void resolveDeclarationFunction(JSGeneratorContext &ctx,
                                  common::AutoPtr<JSModule> &module,
                                  const common::AutoPtr<JSNode> &node);

  void resolveFunction(JSGeneratorContext &ctx,
                       common::AutoPtr<JSModule> &module,
                       const common::AutoPtr<JSNode> &node);

  void resolveDeclarationFunctionBody(JSGeneratorContext &ctx,
                                      common::AutoPtr<JSModule> &module,
                                      const common::AutoPtr<JSNode> &node);

  void resolveDeclarationParameter(JSGeneratorContext &ctx,
                                   common::AutoPtr<JSModule> &module,
                                   const common::AutoPtr<JSNode> &node);

  void resolveDeclarationObject(JSGeneratorContext &ctx,
                                common::AutoPtr<JSModule> &module,
                                const common::AutoPtr<JSNode> &node);

  void resolveDeclarationArray(JSGeneratorContext &ctx,
                               common::AutoPtr<JSModule> &module,
                               const common::AutoPtr<JSNode> &node);

  void resolveDeclarationClass(JSGeneratorContext &ctx,
                               common::AutoPtr<JSModule> &module,
                               const common::AutoPtr<JSNode> &node);

  void resolveNode(JSGeneratorContext &ctx, common::AutoPtr<JSModule> &module,
                   const common::AutoPtr<JSNode> &node);

  void generate(common::AutoPtr<JSModule> &module, const vm::JSAsmOperator &opt,
                uint32_t arg) {
    auto size = module->codes.size();
    module->codes.push_back(0);
    module->codes.push_back(0);

    module->codes.push_back(0);
    module->codes.push_back(0);
    module->codes.push_back(0);
    module->codes.push_back(0);

    auto buffer = module->codes.data() + size;
    uint16_t code = (uint16_t)opt;
    *(uint16_t *)buffer = code;
    buffer += sizeof(uint16_t);
    *(uint32_t *)buffer = arg;
  }

  void generate(common::AutoPtr<JSModule> &module, const vm::JSAsmOperator &opt,
                double arg) {
    auto size = module->codes.size();
    module->codes.push_back(0);
    module->codes.push_back(0);
    module->codes.push_back(0);
    module->codes.push_back(0);
    module->codes.push_back(0);
    module->codes.push_back(0);
    module->codes.push_back(0);
    module->codes.push_back(0);
    module->codes.push_back(0);
    module->codes.push_back(0);
    auto buffer = module->codes.data() + size;
    uint16_t code = (uint16_t)opt;
    *(uint16_t *)buffer = code;
    buffer += sizeof(uint16_t);
    *(uint64_t *)buffer = *(uint64_t *)&arg;
  }

  void generate(common::AutoPtr<JSModule> &module,
                const vm::JSAsmOperator &opt) {
    auto size = module->codes.size();
    module->codes.resize(size + sizeof(uint16_t), 0);
    auto buffer = module->codes.data() + size;
    uint16_t code = (uint16_t)opt;
    *(uint16_t *)buffer = code;
  }

public:
  common::AutoPtr<JSModule> resolve(const std::wstring &filename,
                                    const std::wstring &source,
                                    const common::AutoPtr<JSNode> &root);
};
} // namespace spark::compiler