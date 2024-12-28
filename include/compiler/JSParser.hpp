#pragma once
#include "base/JSNode.hpp"
#include "common/AutoPtr.hpp"
#include "common/Object.hpp"
#include <cstdint>
#include <fmt/format.h>
#include <string>

namespace spark::compiler {
class JSParser : public common::Object {
public:
  struct JSToken : public common::Object {
    JSSourceLocation location;
  };

private:
  JSSourceScope *_currentScope;

private:
  std::wstring formatException(const std::wstring &message,
                               const std::wstring &filename,
                               const std::wstring &source,
                               JSSourceLocation::Position position);

  JSSourceLocation getLocation(const std::wstring &source,
                               JSSourceLocation::Position &start,
                               JSSourceLocation::Position &end);

  void declareVariable(const std::wstring &filename, const std::wstring &source,
                       common::AutoPtr<JSNode> declarator,
                       common::AutoPtr<JSNode> identifier,
                       JSSourceDeclaration::TYPE type, bool isConst);

  void bindScope(common::AutoPtr<JSNode> node);

  void bindDeclaration(common::AutoPtr<JSIdentifierLiteral> node);

private:
  bool skipWhiteSpace(const std::wstring &filename, const std::wstring &source,
                      JSSourceLocation::Position &position);

  bool skipComment(const std::wstring &filename, const std::wstring &source,
                   JSSourceLocation::Position &position);

  bool skipLineTerminatior(const std::wstring &filename,
                           const std::wstring &source,
                           JSSourceLocation::Position &position);

  bool skipSemi(const std::wstring &filename, const std::wstring &source,
                JSSourceLocation::Position &position);

  void skipInvisible(const std::wstring &filename, const std::wstring &source,
                     JSSourceLocation::Position &position,
                     bool *isNewline = nullptr);

  void skipNewLine(const std::wstring &filename, const std::wstring &source,
                   JSSourceLocation::Position &position,
                   bool *isNewline = nullptr);

private:
  common::AutoPtr<JSToken>
  readStringToken(const std::wstring &filename, const std::wstring &source,
                  JSSourceLocation::Position &position);

  common::AutoPtr<JSToken>
  readNumberToken(const std::wstring &filename, const std::wstring &source,
                  JSSourceLocation::Position &position);

  common::AutoPtr<JSToken>
  readBigIntToken(const std::wstring &filename, const std::wstring &source,
                  JSSourceLocation::Position &position);

  common::AutoPtr<JSToken> readRegexToken(const std::wstring &filename,
                                          const std::wstring &source,
                                          JSSourceLocation::Position &position);

  common::AutoPtr<JSToken>
  readCommentToken(const std::wstring &filename, const std::wstring &source,
                   JSSourceLocation::Position &position);

  common::AutoPtr<JSToken>
  readBooleanToken(const std::wstring &filename, const std::wstring &source,
                   JSSourceLocation::Position &position);

  common::AutoPtr<JSToken> readNullToken(const std::wstring &filename,
                                         const std::wstring &source,
                                         JSSourceLocation::Position &position);

  common::AutoPtr<JSToken>
  readUndefinedToken(const std::wstring &filename, const std::wstring &source,
                     JSSourceLocation::Position &position);

  common::AutoPtr<JSToken>
  readKeywordToken(const std::wstring &filename, const std::wstring &source,
                   JSSourceLocation::Position &position);

  common::AutoPtr<JSToken>
  readIdentifierToken(const std::wstring &filename, const std::wstring &source,
                      JSSourceLocation::Position &position,
                      bool allowKeyword = false);

  common::AutoPtr<JSToken>
  readSymbolToken(const std::wstring &filename, const std::wstring &source,
                  JSSourceLocation::Position &position);

  common::AutoPtr<JSToken>
  readTemplateToken(const std::wstring &filename, const std::wstring &source,
                    JSSourceLocation::Position &position);

  common::AutoPtr<JSToken>
  readTemplateStartToken(const std::wstring &filename,
                         const std::wstring &source,
                         JSSourceLocation::Position &position);

  common::AutoPtr<JSToken>
  readTemplatePatternToken(const std::wstring &filename,
                           const std::wstring &source,
                           JSSourceLocation::Position &position);

  common::AutoPtr<JSToken>
  readTemplateEndToken(const std::wstring &filename, const std::wstring &source,
                       JSSourceLocation::Position &position);

private:
  common::AutoPtr<JSNode> readProgram(const std::wstring &filename,
                                      const std::wstring &source,
                                      JSSourceLocation::Position &position);

  common::AutoPtr<JSNode> readStatement(const std::wstring &filename,
                                        const std::wstring &source,
                                        JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readEmptyStatement(const std::wstring &filename, const std::wstring &source,
                     JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readDebuggerStatement(const std::wstring &filename,
                        const std::wstring &source,
                        JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readWhileStatement(const std::wstring &filename, const std::wstring &source,
                     JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readDoWhileStatement(const std::wstring &filename, const std::wstring &source,
                       JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readForStatement(const std::wstring &filename, const std::wstring &source,
                   JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readForInStatement(const std::wstring &filename, const std::wstring &source,
                     JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readForOfStatement(const std::wstring &filename, const std::wstring &source,
                     JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readBlockStatement(const std::wstring &filename, const std::wstring &source,
                     JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readReturnStatement(const std::wstring &filename, const std::wstring &source,
                      JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readThrowStatement(const std::wstring &filename, const std::wstring &source,
                     JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readBreakStatement(const std::wstring &filename, const std::wstring &source,
                     JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readContinueStatement(const std::wstring &filename,
                        const std::wstring &source,
                        JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readLabelStatement(const std::wstring &filename, const std::wstring &source,
                     JSSourceLocation::Position &position);

  common::AutoPtr<JSNode> readIfStatement(const std::wstring &filename,
                                          const std::wstring &source,
                                          JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readSwitchStatement(const std::wstring &filename, const std::wstring &source,
                      JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readSwitchCaseStatement(const std::wstring &filename,
                          const std::wstring &source,
                          JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readTryStatement(const std::wstring &filename, const std::wstring &source,
                   JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readTryCatchStatement(const std::wstring &filename,
                        const std::wstring &source,
                        JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readExpressionStatement(const std::wstring &filename,
                          const std::wstring &source,
                          JSSourceLocation::Position &position);

  common::AutoPtr<JSNode> readValue(const std::wstring &filename,
                                    const std::wstring &source,
                                    JSSourceLocation::Position &position);

  common::AutoPtr<JSNode> readRValue(const std::wstring &filename,
                                     const std::wstring &source,
                                     JSSourceLocation::Position &position,
                                     int level);

  common::AutoPtr<JSNode> readDecorator(const std::wstring &filename,
                                        const std::wstring &source,
                                        JSSourceLocation::Position &position);

  common::AutoPtr<JSNode> readExpression(const std::wstring &filename,
                                         const std::wstring &source,
                                         JSSourceLocation::Position &position);

  common::AutoPtr<JSNode> readExpressions(const std::wstring &filename,
                                          const std::wstring &source,
                                          JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readInterpreterDirective(const std::wstring &filename,
                           const std::wstring &source,
                           JSSourceLocation::Position &position);

  common::AutoPtr<JSNode> readDirective(const std::wstring &filename,
                                        const std::wstring &source,
                                        JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readStringLiteral(const std::wstring &filename, const std::wstring &source,
                    JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readNumberLiteral(const std::wstring &filename, const std::wstring &source,
                    JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readBigIntLiteral(const std::wstring &filename, const std::wstring &source,
                    JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readBooleanLiteral(const std::wstring &filename, const std::wstring &source,
                     JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readUndefinedLiteral(const std::wstring &filename, const std::wstring &source,
                       JSSourceLocation::Position &position);

  common::AutoPtr<JSNode> readNullLiteral(const std::wstring &filename,
                                          const std::wstring &source,
                                          JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readIdentifierLiteral(const std::wstring &filename,
                        const std::wstring &source,
                        JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readMemberLiteral(const std::wstring &filename, const std::wstring &source,
                    JSSourceLocation::Position &position);

  common::AutoPtr<JSNode> readPrivateName(const std::wstring &filename,
                                          const std::wstring &source,
                                          JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readRegexLiteral(const std::wstring &filename, const std::wstring &source,
                   JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readTemplateLiteral(const std::wstring &filename, const std::wstring &source,
                      JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readBinaryExpression(const std::wstring &filename, const std::wstring &source,
                       JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readAssigmentExpression(const std::wstring &filename,
                          const std::wstring &source,
                          JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readConditionExpression(const std::wstring &filename,
                          const std::wstring &source,
                          JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readUpdateExpression(const std::wstring &filename, const std::wstring &source,
                       JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readUnaryExpression(const std::wstring &filename, const std::wstring &source,
                      JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readGroupExpression(const std::wstring &filename, const std::wstring &source,
                      JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readMemberExpression(const std::wstring &filename, const std::wstring &source,
                       JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readCallExpression(const std::wstring &filename, const std::wstring &source,
                     JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readRestExpression(const std::wstring &filename, const std::wstring &source,
                     JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readAwaitExpression(const std::wstring &filename, const std::wstring &source,
                      JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readYieldExpression(const std::wstring &filename, const std::wstring &source,
                      JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readTypeofExpression(const std::wstring &filename, const std::wstring &source,
                       JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readVoidExpression(const std::wstring &filename, const std::wstring &source,
                     JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readDeleteExpression(const std::wstring &filename, const std::wstring &source,
                       JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readNewExpression(const std::wstring &filename, const std::wstring &source,
                    JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readInExpression(const std::wstring &filename, const std::wstring &source,
                   JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readInstanceOfExpression(const std::wstring &filename,
                           const std::wstring &source,
                           JSSourceLocation::Position &position);

  common::AutoPtr<JSNode> readParameter(const std::wstring &filename,
                                        const std::wstring &source,
                                        JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readArrowFunctionDeclaration(const std::wstring &filename,
                               const std::wstring &source,
                               JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readFunctionDeclaration(const std::wstring &filename,
                          const std::wstring &source,
                          JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readFunctionBody(const std::wstring &filename, const std::wstring &source,
                   JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readArrayDeclaration(const std::wstring &filename, const std::wstring &source,
                       JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readObjectDeclaration(const std::wstring &filename,
                        const std::wstring &source,
                        JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readObjectProperty(const std::wstring &filename, const std::wstring &source,
                     JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readObjectMethod(const std::wstring &filename, const std::wstring &source,
                   JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readObjectAccessor(const std::wstring &filename, const std::wstring &source,
                     JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readClassDeclaration(const std::wstring &filename, const std::wstring &source,
                       JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readClassProperty(const std::wstring &filename, const std::wstring &source,
                    JSSourceLocation::Position &position);

  common::AutoPtr<JSNode> readClassMethod(const std::wstring &filename,
                                          const std::wstring &source,
                                          JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readClassAccessor(const std::wstring &filename, const std::wstring &source,
                    JSSourceLocation::Position &position);

  common::AutoPtr<JSNode> readStaticBlock(const std::wstring &filename,
                                          const std::wstring &source,
                                          JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readVariableDeclarator(const std::wstring &filename,
                         const std::wstring &source,
                         JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readVariableDeclaration(const std::wstring &filename,
                          const std::wstring &source,
                          JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readObjectPattern(const std::wstring &filename, const std::wstring &source,
                    JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readObjectPatternItem(const std::wstring &filename,
                        const std::wstring &source,
                        JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readArrayPattern(const std::wstring &filename, const std::wstring &source,
                   JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readArrayPatternItem(const std::wstring &filename, const std::wstring &source,
                       JSSourceLocation::Position &position);

  common::AutoPtr<JSNode> readRestPattern(const std::wstring &filename,
                                          const std::wstring &source,
                                          JSSourceLocation::Position &position);

  common::AutoPtr<JSNode> readComment(const std::wstring &filename,
                                      const std::wstring &source,
                                      JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readImportSpecifier(const std::wstring &filename, const std::wstring &source,
                      JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readImportAttriabue(const std::wstring &filename, const std::wstring &source,
                      JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readImportDefaultSpecifier(const std::wstring &filename,
                             const std::wstring &source,
                             JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readImportNamespaceSpecifier(const std::wstring &filename,
                               const std::wstring &source,
                               JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readImportDeclaration(const std::wstring &filename,
                        const std::wstring &source,
                        JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readExportSpecifier(const std::wstring &filename, const std::wstring &source,
                      JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readExportDefaultSpecifier(const std::wstring &filename,
                             const std::wstring &source,
                             JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readExportAllSpecifier(const std::wstring &filename,
                         const std::wstring &source,
                         JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readExportDeclaration(const std::wstring &filename,
                        const std::wstring &source,
                        JSSourceLocation::Position &position);

public:
  common::AutoPtr<JSNode> parse(const std::wstring &filename,
                                const std::wstring &source);

  std::wstring toJSON(const std::wstring &filename, const std::wstring &source,
                      common::AutoPtr<JSNode> node);
};
}; // namespace spark::compiler