#pragma once
#include "JSNodeType.hpp"
#include "common/AutoPtr.hpp"
#include "common/Object.hpp"
#include <string>
#include <vector>

namespace spark::compiler {
enum class JSAccessorKind { GET, SET };

enum class JSDeclarationKind { CONST, LET, VAR, UNKNOWN };

struct JSSourceLocation {
  struct Position {
    uint32_t line;
    uint32_t column;
    uint32_t offset;
  };
  Position start;
  Position end;
  std::wstring getSource(const std::wstring &source) const {
    return source.substr(start.offset, end.offset - start.offset + 1);
  }
  bool isEqual(const std::wstring &source, const std::wstring &another) {
    auto len = end.offset - start.offset + 1;
    if (len != another.length()) {
      return false;
    }
    for (size_t index = 0; index < len; index++) {
      if (source[index + start.offset] != another[index]) {
        return false;
      }
    }
    return true;
  }
};

struct JSNode;

struct JSSourceScope;

struct JSSourceDeclaration {
  enum class TYPE { UNDEFINED, UNINITIALIZED, FUNCTION, ARGUMENT, CATCH } type;
  bool isConst;
  std::wstring name;
  JSNode *node;
  JSSourceScope *scope;
  JSSourceDeclaration()
      : node(nullptr), scope(nullptr), type(TYPE::UNINITIALIZED) {}
};

struct JSSourceBinding {
  JSSourceDeclaration *declaration;
  std::vector<JSNode *> references;
  std::wstring name;
  JSSourceBinding() : declaration(nullptr) {}
};

struct JSSourceScope : public common::Object {
  std::vector<JSSourceBinding> bindings;
  std::vector<JSSourceDeclaration> declarations;
  std::vector<JSSourceScope *> children;
  JSNode *node;
  uint32_t id;
  JSSourceScope *parent;
  JSSourceScope(JSSourceScope *parent = nullptr) : parent(parent) {
    static uint32_t id = 0;
    this->id = ++id;
    if (parent) {
      parent->children.push_back(this);
    }
  }
};

struct JSNode : public common::Object {
  JSNodeType type;
  JSSourceLocation location;
  int32_t level;
  JSNode *parent;
  std::vector<JSNode *> children;
  uint32_t id;
  common::AutoPtr<JSSourceScope> scope;
  void addParent(common::AutoPtr<JSNode> parent) {
    parent->children.push_back(this);
    this->parent = parent.getRawPointer();
  }
  JSNode(const JSNodeType &type, int32_t level = 0)
      : type(type), level(level), parent(nullptr) {
    static uint32_t id = 0;
    this->id = ++id;
  }
};

using JSNodeArray = std::vector<common::AutoPtr<JSNode>>;

struct JSProgram : public JSNode {
  common::AutoPtr<JSNode> interpreter;
  JSNodeArray directives;
  JSNodeArray body;
  JSProgram() : JSNode(JSNodeType::PROGRAM, 0) {}
};

struct JSStringLiteral : public JSNode {
  std::wstring value;
  JSStringLiteral() : JSNode(JSNodeType::LITERAL_STRING, -2) {}
};

struct JSNumberLiteral : public JSNode {
  double value;
  JSNumberLiteral() : JSNode(JSNodeType::LITERAL_NUMBER, -2) {}
};

struct JSBigIntLiteral : public JSNode {
  std::wstring value;
  JSBigIntLiteral() : JSNode(JSNodeType::LITERAL_BIGINT, -2) {}
};

struct JSRegexLiteral : public JSNode {
  std::wstring value;
  bool hasIndices{};
  bool global{};
  bool ignoreCase{};
  bool multiline{};
  bool dotAll{};
  bool sticky{};
  JSRegexLiteral() : JSNode(JSNodeType::LITERAL_REGEX, -2) {}
};

struct JSBooleanLiteral : public JSNode {
  bool value;
  JSBooleanLiteral() : JSNode(JSNodeType::LITERAL_BOOLEAN, -2) {}
};

struct JSNullLiteral : public JSNode {
  JSNullLiteral() : JSNode(JSNodeType::LITERAL_NULL, -2) {}
};

struct JSUndefinedLiteral : public JSNode {
  JSUndefinedLiteral() : JSNode(JSNodeType::LITERAL_UNDEFINED, -2) {}
};

struct JSIdentifierLiteral : public JSNode {
  std::wstring value;
  JSIdentifierLiteral() : JSNode(JSNodeType::LITERAL_IDENTITY, -2) {}
};

struct JSTemplateLiteral : public JSNode {
  common::AutoPtr<JSNode> tag;
  std::vector<std::wstring> quasis;
  JSNodeArray expressions;
  JSTemplateLiteral() : JSNode(JSNodeType::LITERAL_TEMPLATE, -2) {}
};

struct JSPrivateName : public JSNode {
  std::wstring value;
  JSPrivateName() : JSNode(JSNodeType::PRIVATE_NAME) {}
};

struct JSComment : public JSNode {
  JSComment() : JSNode(JSNodeType::LITERAL_COMMENT, -2) {}
};

struct JSInterpreterDirective : public JSNode {
  JSInterpreterDirective() : JSNode(JSNodeType::INTERPRETER_DIRECTIVE) {}
};

struct JSDirective : public JSNode {
  JSDirective() : JSNode(JSNodeType::DIRECTIVE) {}
};

struct JSBlockStatement : public JSNode {
  JSNodeArray body;
  JSNodeArray directives;
  JSBlockStatement() : JSNode(JSNodeType::STATEMENT_BLOCK) {}
};

struct JSEmptyStatement : public JSNode {
  JSEmptyStatement() : JSNode(JSNodeType::STATEMENT_EMPTY) {}
};

struct JSDebuggerStatement : public JSNode {
  JSDebuggerStatement() : JSNode(JSNodeType::STATEMENT_DEBUGGER) {}
};

struct JSForStatement : public JSNode {
  common::AutoPtr<JSNode> init;
  common::AutoPtr<JSNode> condition;
  common::AutoPtr<JSNode> update;
  common::AutoPtr<JSNode> body;
  JSForStatement() : JSNode(JSNodeType::STATEMENT_FOR) {}
};

struct JSForInStatement : public JSNode {
  JSDeclarationKind kind;
  common::AutoPtr<JSNode> declaration;
  common::AutoPtr<JSNode> expression;
  common::AutoPtr<JSNode> body;
  JSForInStatement() : JSNode(JSNodeType::STATEMENT_FOR_IN) {}
};

struct JSForOfStatement : public JSNode {
  JSDeclarationKind kind;
  common::AutoPtr<JSNode> declaration;
  common::AutoPtr<JSNode> expression;
  common::AutoPtr<JSNode> body;
  JSForOfStatement() : JSNode(JSNodeType::STATEMENT_FOR_OF) {}
};

struct JSForAwaitOfStatement : public JSForOfStatement {
  JSForAwaitOfStatement() : JSForOfStatement() {
    type = JSNodeType::STATEMENT_FOR_AWAIT_OF;
  }
};

struct JSWhileStatement : public JSNode {
  common::AutoPtr<JSNode> condition;
  common::AutoPtr<JSNode> body;
  JSWhileStatement() : JSNode(JSNodeType::STATEMENT_WHILE) {}
};

struct JSDoWhileStatement : public JSWhileStatement {
  JSDoWhileStatement() { type = JSNodeType::STATEMENT_DO_WHILE; }
};

struct JSYieldExpression : public JSNode {
  common::AutoPtr<JSNode> value;
  JSYieldExpression() : JSNode(JSNodeType::EXPRESSION_YIELD) {}
};

struct JSYieldDelegateExpression : public JSYieldExpression {
  JSYieldDelegateExpression() { type = JSNodeType::EXPRESSION_YIELD_DELEGATE; }
};

struct JSReturnStatement : public JSNode {
  common::AutoPtr<JSNode> value;
  JSReturnStatement() : JSNode(JSNodeType::STATEMENT_RETURN) {}
};

struct JSThrowStatement : public JSNode {
  common::AutoPtr<JSNode> value;
  JSThrowStatement() : JSNode(JSNodeType::STATEMENT_THROW) {}
};

struct JSTryCatchStatement : public JSNode {
  common::AutoPtr<JSNode> binding;
  common::AutoPtr<JSNode> statement;
  JSTryCatchStatement() : JSNode(JSNodeType::STATEMENT_TRY_CATCH) {}
};

struct JSTryStatement : public JSNode {
  common::AutoPtr<JSNode> try_;
  common::AutoPtr<JSNode> catch_;
  common::AutoPtr<JSNode> finally;
  JSTryStatement() : JSNode(JSNodeType::STATEMENT_TRY) {}
};

struct JSLabelStatement : public JSNode {
  common::AutoPtr<JSNode> label;
  common::AutoPtr<JSNode> statement;
  JSLabelStatement() : JSNode(JSNodeType::STATEMENT_LABEL) {}
};

struct JSBreakStatement : public JSNode {
  common::AutoPtr<JSNode> label;
  JSBreakStatement() : JSNode(JSNodeType::STATEMENT_BREAK) {}
};

struct JSContinueStatement : public JSNode {
  common::AutoPtr<JSNode> label;
  JSContinueStatement() : JSNode(JSNodeType::STATEMENT_CONTINUE) {}
};

struct JSIfStatement : public JSNode {
  common::AutoPtr<JSNode> condition;
  common::AutoPtr<JSNode> alternate;
  common::AutoPtr<JSNode> consequent;
  JSIfStatement() : JSNode(JSNodeType::STATEMENT_IF) {}
};

struct JSSwitchCaseStatement : public JSNode {
  common::AutoPtr<JSNode> match;
  JSNodeArray statements;
  JSSwitchCaseStatement() : JSNode(JSNodeType::STATEMENT_SWITCH_CASE) {}
};

struct JSSwitchStatement : public JSNode {
  common::AutoPtr<JSNode> expression;
  JSNodeArray cases;
  JSSwitchStatement() : JSNode(JSNodeType::STATEMENT_SWITCH) {}
};

struct JSDecorator : public JSNode {
  common::AutoPtr<JSNode> expression;
  JSDecorator() : JSNode(JSNodeType::DECORATOR) {}
};

struct JSBinaryExpression : public JSNode {
  common::AutoPtr<JSNode> left;
  common::AutoPtr<JSNode> right;
  std::wstring opt;
  JSBinaryExpression() : JSNode(JSNodeType::EXPRESSION_BINARY) {}
};

struct JSUpdateExpression : public JSBinaryExpression {
  JSUpdateExpression() {
    type = JSNodeType::EXPRESSION_UPDATE;
    level = -2;
  }
};

struct JSUnaryExpression : public JSBinaryExpression {
  JSUnaryExpression() {
    type = JSNodeType::EXPRESSION_UNARY;
    level = 4;
  }
};

struct JSGroupExpression : public JSNode {
  common::AutoPtr<JSNode> expression;
  JSGroupExpression() : JSNode(JSNodeType::EXPRESSION_GROUP, -2) {}
};

struct JSConditionExpression : public JSBinaryExpression {
  JSConditionExpression() {
    type = JSNodeType::EXPRESSION_CONDITION;
    level = 16;
  }
};

struct JSMemberExpression : public JSBinaryExpression {
  JSMemberExpression() {
    type = JSNodeType::EXPRESSION_MEMBER;
    level = 1;
  }
};

struct JSOptionalMemberExpression : public JSBinaryExpression {
  JSOptionalMemberExpression() {
    type = JSNodeType::EXPRESSION_OPTIONAL_MEMBER;
    level = 1;
  }
};

struct JSComputedMemberExpression : public JSBinaryExpression {
  JSComputedMemberExpression() {
    type = JSNodeType::EXPRESSION_COMPUTED_MEMBER;
    level = 1;
  }
};

struct JSOptionalComputedMemberExpression : public JSBinaryExpression {
  JSOptionalComputedMemberExpression() {
    type = JSNodeType::EXPRESSION_OPTIONAL_COMPUTED_MEMBER;
    level = 1;
  }
};

struct JSCallExpression : public JSBinaryExpression {
  JSNodeArray arguments;
  JSCallExpression() {
    type = JSNodeType::EXPRESSION_CALL;
    level = 1;
  }
};

struct JSOptionalCallExpression : public JSCallExpression {
  JSOptionalCallExpression() {
    type = JSNodeType::EXPRESSION_OPTIONAL_CALL;
    level = 1;
  }
};

struct JSAwaitExpression : public JSBinaryExpression {
  JSAwaitExpression() {
    type = JSNodeType::EXPRESSION_AWAIT;
    level = 4;
  }
};

struct JSTypeofExpression : public JSBinaryExpression {
  JSTypeofExpression() {
    type = JSNodeType::EXPRESSION_TYPEOF;
    level = 4;
  }
};

struct JSVoidExpression : public JSBinaryExpression {
  JSVoidExpression() {
    type = JSNodeType::EXPRESSION_VOID;
    level = 4;
  }
};

struct JSNewExpression : public JSBinaryExpression {
  JSNewExpression() {
    type = JSNodeType::EXPRESSION_NEW;
    level = 2;
  }
};

struct JSDeleteExpression : public JSBinaryExpression {
  JSDeleteExpression() {
    type = JSNodeType::EXPRESSION_DELETE;
    level = 4;
  }
};

struct JSParameterDeclaration : public JSNode {
  common::AutoPtr<JSNode> identifier;
  common::AutoPtr<JSNode> value;
  JSParameterDeclaration() : JSNode(JSNodeType::DECLARATION_PARAMETER, 0) {}
};

struct JSArrowFunctionDeclaration : public JSNode {
  std::wstring name;
  std::string sourceFile;
  bool async;
  JSNodeArray arguments;
  common::AutoPtr<JSNode> body;
  JSNodeArray directives;
  JSArrowFunctionDeclaration()
      : JSNode(JSNodeType::DECLARATION_ARROW_FUNCTION, -2) {}
};

struct JSFunctionDeclaration : public JSNode {
  JSNodeArray arguments;
  common::AutoPtr<JSNode> identifier;
  common::AutoPtr<JSNode> body;
  bool async;
  bool generator;
  JSFunctionDeclaration() : JSNode(JSNodeType::DECLARATION_FUNCTION, -2) {}
};

struct JSFunctionBodyDeclaration : public JSNode {
  JSNodeArray statements;
  JSNodeArray directives;
  JSFunctionBodyDeclaration()
      : JSNode(JSNodeType::DECLARATION_FUNCTION_BODY, -2) {}
};

struct JSObjectAccessor : public JSFunctionDeclaration {
  JSAccessorKind kind;
  JSObjectAccessor() : JSFunctionDeclaration() {
    type = JSNodeType::OBJECT_ACCESSOR;
  }
};

struct JSObjectMethod : public JSFunctionDeclaration {
  JSObjectMethod() { type = JSNodeType::OBJECT_METHOD; }
};

struct JSObjectProperty : public JSNode {
  common::AutoPtr<JSNode> identifier;
  common::AutoPtr<JSNode> implement;
  JSObjectProperty() : JSNode(JSNodeType::OBJECT_PROPERTY) {}
};

struct JSObjectDeclaration : public JSNode {
  JSNodeArray properties;
  JSObjectDeclaration() : JSNode(JSNodeType::DECLARATION_OBJECT, -2) {}
};

struct JSClassProperty : public JSNode {
  common::AutoPtr<JSNode> identifier;
  common::AutoPtr<JSNode> value;
  JSNodeArray decorators;
  bool static_;
  JSClassProperty() : JSNode(JSNodeType::CLASS_PROPERTY, 0) {}
};

struct JSClassMethod : public JSFunctionDeclaration {
  bool static_;
  JSNodeArray decorators;
  JSClassMethod() {
    type = JSNodeType::CLASS_METHOD;
    level = 0;
  }
};

struct JSClassAccessor : public JSFunctionDeclaration {
  bool static_;
  JSAccessorKind kind;
  JSNodeArray decorators;
  JSClassAccessor() {
    type = JSNodeType::CLASS_ACCESSOR;
    level = 0;
  }
};

struct JSClassDeclaration : public JSNode {
  common::AutoPtr<JSNode> identifier;
  common::AutoPtr<JSNode> extends;
  JSNodeArray decorators;
  JSNodeArray properties;
  JSClassDeclaration() : JSNode(JSNodeType::DECLARATION_CLASS, -2) {}
};

struct JSRestPatternItem : public JSNode {
  common::AutoPtr<JSNode> identifier;
  JSRestPatternItem() : JSNode(JSNodeType::PATTERN_REST_ITEM) {}
};

struct JSObjectPatternItem : public JSNode {
  common::AutoPtr<JSNode> identifier;
  common::AutoPtr<JSNode> match;
  common::AutoPtr<JSNode> value;
  JSObjectPatternItem() : JSNode(JSNodeType::PATTERN_OBJECT_ITEM) {}
};

struct JSObjectPattern : public JSNode {
  JSNodeArray items;
  JSObjectPattern() : JSNode(JSNodeType::PATTERN_OBJECT, -2) {}
};

struct JSArrayPatternItem : public JSNode {
  common::AutoPtr<JSNode> identifier;
  common::AutoPtr<JSNode> value;
  JSArrayPatternItem() : JSNode(JSNodeType::PATTERN_ARRAY_ITEM) {}
};

struct JSArrayPattern : public JSNode {
  JSNodeArray items;
  JSArrayPattern() : JSNode(JSNodeType::PATTERN_ARRAY, -2) {}
};

struct JSArrayDeclaration : public JSNode {
  JSNodeArray items;
  JSArrayDeclaration() : JSNode(JSNodeType::DECLARATION_ARRAY, -2) {}
};

struct JSVariableDeclarator : public JSNode {
  common::AutoPtr<JSNode> identifier;
  common::AutoPtr<JSNode> value;
  JSVariableDeclarator() : JSNode(JSNodeType::VARIABLE_DECLARATOR) {}
};

struct JSVariableDeclaration : public JSNode {
  JSDeclarationKind kind;
  JSNodeArray declarations;
  JSVariableDeclaration() : JSNode(JSNodeType::VARIABLE_DECLARATION) {}
};

struct JSImportSpecifier : public JSNode {
  common::AutoPtr<JSNode> identifier;
  common::AutoPtr<JSNode> alias;
  JSImportSpecifier() : JSNode(JSNodeType::IMPORT_SPECIFIER) {}
};

struct JSImportDefaultSpecifier : public JSNode {
  common::AutoPtr<JSNode> identifier;
  JSImportDefaultSpecifier() : JSNode(JSNodeType::IMPORT_DEFAULT) {}
};

struct JSImportNamespaceSpecifier : public JSNode {
  common::AutoPtr<JSNode> alias;
  JSImportNamespaceSpecifier() : JSNode(JSNodeType::IMPORT_NAMESPACE) {}
};

struct JSImportAttribute : public JSNode {
  common::AutoPtr<JSNode> key;
  common::AutoPtr<JSNode> value;
  JSImportAttribute() : JSNode(JSNodeType::IMPORT_ATTARTUBE) {}
};

struct JSImportDeclaration : public JSNode {
  common::AutoPtr<JSNode> source;
  JSNodeArray items;
  JSNodeArray attributes;
  JSImportDeclaration() : JSNode(JSNodeType::IMPORT_DECLARATION) {}
};

struct JSExportDefaultSpecifier : public JSNode {
  common::AutoPtr<JSNode> value;
  JSExportDefaultSpecifier() : JSNode(JSNodeType::EXPORT_DEFAULT) {}
};

struct JSExportSpecifier : public JSNode {
  common::AutoPtr<JSNode> identifier;
  common::AutoPtr<JSNode> alias;
  JSExportSpecifier() : JSNode(JSNodeType::EXPORT_SPECIFIER) {}
};

struct JSExportAllSpecifier : public JSNode {
  common::AutoPtr<JSNode> alias;
  JSExportAllSpecifier() : JSNode(JSNodeType::EXPORT_ALL) {}
};

struct JSExportDeclaration : public JSNode {
  JSNodeArray items;
  common::AutoPtr<JSNode> source;
  JSNodeArray attributes;
  JSExportDeclaration() : JSNode(JSNodeType::EXPORT_DECLARATION) {}
};

}; // namespace spark::compiler