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
  std::wstring formatException(const std::wstring &message, uint32_t filename,
                               const std::wstring &source,
                               JSSourceLocation::Position position);

  JSSourceLocation getLocation(const std::wstring &source,
                               JSSourceLocation::Position &start,
                               JSSourceLocation::Position &end);

  void declareVariable(uint32_t filename, const std::wstring &source,
                       common::AutoPtr<JSNode> declarator,
                       common::AutoPtr<JSNode> identifier,
                       JSSourceDeclaration::TYPE type, bool isConst);

  void bindScope(common::AutoPtr<JSNode> node);

  void bindDeclaration(common::AutoPtr<JSIdentifierLiteral> node);

private:
  bool skipWhiteSpace(uint32_t filename, const std::wstring &source,
                      JSSourceLocation::Position &position);

  bool skipComment(uint32_t filename, const std::wstring &source,
                   JSSourceLocation::Position &position);

  bool skipLineTerminatior(uint32_t filename, const std::wstring &source,
                           JSSourceLocation::Position &position);

  bool skipSemi(uint32_t filename, const std::wstring &source,
                JSSourceLocation::Position &position);

  void skipInvisible(uint32_t filename, const std::wstring &source,
                     JSSourceLocation::Position &position,
                     bool *isNewline = nullptr);

  void skipNewLine(uint32_t filename, const std::wstring &source,
                   JSSourceLocation::Position &position,
                   bool *isNewline = nullptr);

private:
  common::AutoPtr<JSToken>
  readStringToken(uint32_t filename, const std::wstring &source,
                  JSSourceLocation::Position &position);

  common::AutoPtr<JSToken>
  readNumberToken(uint32_t filename, const std::wstring &source,
                  JSSourceLocation::Position &position);

  common::AutoPtr<JSToken>
  readBigIntToken(uint32_t filename, const std::wstring &source,
                  JSSourceLocation::Position &position);

  common::AutoPtr<JSToken> readRegexToken(uint32_t filename,
                                          const std::wstring &source,
                                          JSSourceLocation::Position &position);

  common::AutoPtr<JSToken>
  readCommentToken(uint32_t filename, const std::wstring &source,
                   JSSourceLocation::Position &position);

  common::AutoPtr<JSToken>
  readBooleanToken(uint32_t filename, const std::wstring &source,
                   JSSourceLocation::Position &position);

  common::AutoPtr<JSToken> readNullToken(uint32_t filename,
                                         const std::wstring &source,
                                         JSSourceLocation::Position &position);

  common::AutoPtr<JSToken>
  readUndefinedToken(uint32_t filename, const std::wstring &source,
                     JSSourceLocation::Position &position);

  common::AutoPtr<JSToken>
  readKeywordToken(uint32_t filename, const std::wstring &source,
                   JSSourceLocation::Position &position);

  common::AutoPtr<JSToken>
  readIdentifierToken(uint32_t filename, const std::wstring &source,
                      JSSourceLocation::Position &position,
                      bool allowKeyword = false);

  common::AutoPtr<JSToken>
  readSymbolToken(uint32_t filename, const std::wstring &source,
                  JSSourceLocation::Position &position);

  common::AutoPtr<JSToken>
  readTemplateToken(uint32_t filename, const std::wstring &source,
                    JSSourceLocation::Position &position);

  common::AutoPtr<JSToken>
  readTemplateStartToken(uint32_t filename, const std::wstring &source,
                         JSSourceLocation::Position &position);

  common::AutoPtr<JSToken>
  readTemplatePatternToken(uint32_t filename, const std::wstring &source,
                           JSSourceLocation::Position &position);

  common::AutoPtr<JSToken>
  readTemplateEndToken(uint32_t filename, const std::wstring &source,
                       JSSourceLocation::Position &position);

private:
  common::AutoPtr<JSNode> readProgram(uint32_t filename,
                                      const std::wstring &source,
                                      JSSourceLocation::Position &position);

  common::AutoPtr<JSNode> readStatement(uint32_t filename,
                                        const std::wstring &source,
                                        JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readEmptyStatement(uint32_t filename, const std::wstring &source,
                     JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readDebuggerStatement(uint32_t filename, const std::wstring &source,
                        JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readWhileStatement(uint32_t filename, const std::wstring &source,
                     JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readDoWhileStatement(uint32_t filename, const std::wstring &source,
                       JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readForStatement(uint32_t filename, const std::wstring &source,
                   JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readForInStatement(uint32_t filename, const std::wstring &source,
                     JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readForOfStatement(uint32_t filename, const std::wstring &source,
                     JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readBlockStatement(uint32_t filename, const std::wstring &source,
                     JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readYieldStatement(uint32_t filename, const std::wstring &source,
                     JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readReturnStatement(uint32_t filename, const std::wstring &source,
                      JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readThrowStatement(uint32_t filename, const std::wstring &source,
                     JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readBreakStatement(uint32_t filename, const std::wstring &source,
                     JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readContinueStatement(uint32_t filename, const std::wstring &source,
                        JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readLabelStatement(uint32_t filename, const std::wstring &source,
                     JSSourceLocation::Position &position);

  common::AutoPtr<JSNode> readIfStatement(uint32_t filename,
                                          const std::wstring &source,
                                          JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readSwitchStatement(uint32_t filename, const std::wstring &source,
                      JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readSwitchCaseStatement(uint32_t filename, const std::wstring &source,
                          JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readTryStatement(uint32_t filename, const std::wstring &source,
                   JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readTryCatchStatement(uint32_t filename, const std::wstring &source,
                        JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readExpressionStatement(uint32_t filename, const std::wstring &source,
                          JSSourceLocation::Position &position);

  common::AutoPtr<JSNode> readValue(uint32_t filename,
                                    const std::wstring &source,
                                    JSSourceLocation::Position &position);

  common::AutoPtr<JSNode> readRValue(uint32_t filename,
                                     const std::wstring &source,
                                     JSSourceLocation::Position &position,
                                     int level);

  common::AutoPtr<JSNode> readDecorator(uint32_t filename,
                                        const std::wstring &source,
                                        JSSourceLocation::Position &position);

  common::AutoPtr<JSNode> readExpression(uint32_t filename,
                                         const std::wstring &source,
                                         JSSourceLocation::Position &position);

  common::AutoPtr<JSNode> readExpressions(uint32_t filename,
                                          const std::wstring &source,
                                          JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readInterpreterDirective(uint32_t filename, const std::wstring &source,
                           JSSourceLocation::Position &position);

  common::AutoPtr<JSNode> readDirective(uint32_t filename,
                                        const std::wstring &source,
                                        JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readStringLiteral(uint32_t filename, const std::wstring &source,
                    JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readNumberLiteral(uint32_t filename, const std::wstring &source,
                    JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readBigIntLiteral(uint32_t filename, const std::wstring &source,
                    JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readBooleanLiteral(uint32_t filename, const std::wstring &source,
                     JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readUndefinedLiteral(uint32_t filename, const std::wstring &source,
                       JSSourceLocation::Position &position);

  common::AutoPtr<JSNode> readNullLiteral(uint32_t filename,
                                          const std::wstring &source,
                                          JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readIdentifierLiteral(uint32_t filename, const std::wstring &source,
                        JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readMemberLiteral(uint32_t filename, const std::wstring &source,
                    JSSourceLocation::Position &position);

  common::AutoPtr<JSNode> readPrivateName(uint32_t filename,
                                          const std::wstring &source,
                                          JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readRegexLiteral(uint32_t filename, const std::wstring &source,
                   JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readTemplateLiteral(uint32_t filename, const std::wstring &source,
                      JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readBinaryExpression(uint32_t filename, const std::wstring &source,
                       JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readAssigmentExpression(uint32_t filename, const std::wstring &source,
                          JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readConditionExpression(uint32_t filename, const std::wstring &source,
                          JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readUpdateExpression(uint32_t filename, const std::wstring &source,
                       JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readUnaryExpression(uint32_t filename, const std::wstring &source,
                      JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readGroupExpression(uint32_t filename, const std::wstring &source,
                      JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readMemberExpression(uint32_t filename, const std::wstring &source,
                       JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readCallExpression(uint32_t filename, const std::wstring &source,
                     JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readRestExpression(uint32_t filename, const std::wstring &source,
                     JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readAwaitExpression(uint32_t filename, const std::wstring &source,
                      JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readTypeofExpression(uint32_t filename, const std::wstring &source,
                       JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readVoidExpression(uint32_t filename, const std::wstring &source,
                     JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readDeleteExpression(uint32_t filename, const std::wstring &source,
                       JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readNewExpression(uint32_t filename, const std::wstring &source,
                    JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readInExpression(uint32_t filename, const std::wstring &source,
                   JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readInstanceOfExpression(uint32_t filename, const std::wstring &source,
                           JSSourceLocation::Position &position);

  common::AutoPtr<JSNode> readParameter(uint32_t filename,
                                        const std::wstring &source,
                                        JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readArrowFunctionDeclaration(uint32_t filename, const std::wstring &source,
                               JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readFunctionDeclaration(uint32_t filename, const std::wstring &source,
                          JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readFunctionBody(uint32_t filename, const std::wstring &source,
                   JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readArrayDeclaration(uint32_t filename, const std::wstring &source,
                       JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readObjectDeclaration(uint32_t filename, const std::wstring &source,
                        JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readObjectProperty(uint32_t filename, const std::wstring &source,
                     JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readObjectMethod(uint32_t filename, const std::wstring &source,
                   JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readObjectAccessor(uint32_t filename, const std::wstring &source,
                     JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readClassDeclaration(uint32_t filename, const std::wstring &source,
                       JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readClassProperty(uint32_t filename, const std::wstring &source,
                    JSSourceLocation::Position &position);

  common::AutoPtr<JSNode> readClassMethod(uint32_t filename,
                                          const std::wstring &source,
                                          JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readClassAccessor(uint32_t filename, const std::wstring &source,
                    JSSourceLocation::Position &position);

  common::AutoPtr<JSNode> readStaticBlock(uint32_t filename,
                                          const std::wstring &source,
                                          JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readVariableDeclarator(uint32_t filename, const std::wstring &source,
                         JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readVariableDeclaration(uint32_t filename, const std::wstring &source,
                          JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readObjectPattern(uint32_t filename, const std::wstring &source,
                    JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readObjectPatternItem(uint32_t filename, const std::wstring &source,
                        JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readArrayPattern(uint32_t filename, const std::wstring &source,
                   JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readArrayPatternItem(uint32_t filename, const std::wstring &source,
                       JSSourceLocation::Position &position);

  common::AutoPtr<JSNode> readRestPattern(uint32_t filename,
                                          const std::wstring &source,
                                          JSSourceLocation::Position &position);

  common::AutoPtr<JSNode> readComment(uint32_t filename,
                                      const std::wstring &source,
                                      JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readImportSpecifier(uint32_t filename, const std::wstring &source,
                      JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readImportAttriabue(uint32_t filename, const std::wstring &source,
                      JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readImportDefaultSpecifier(uint32_t filename, const std::wstring &source,
                             JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readImportNamespaceSpecifier(uint32_t filename, const std::wstring &source,
                               JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readImportDeclaration(uint32_t filename, const std::wstring &source,
                        JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readExportSpecifier(uint32_t filename, const std::wstring &source,
                      JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readExportDefaultSpecifier(uint32_t filename, const std::wstring &source,
                             JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readExportAllSpecifier(uint32_t filename, const std::wstring &source,
                         JSSourceLocation::Position &position);

  common::AutoPtr<JSNode>
  readExportDeclaration(uint32_t filename, const std::wstring &source,
                        JSSourceLocation::Position &position);

public:
  common::AutoPtr<JSNode> parse(uint32_t filename, const std::wstring &source);

  std::wstring toJSON(const std::wstring &filename, const std::wstring &source,
                      common::AutoPtr<JSNode> node);
};
}; // namespace spark::compiler