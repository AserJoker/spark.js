#pragma once
#include "common/Array.hpp"
#include "common/AutoPtr.hpp"
#include "common/Object.hpp"
#include <cstdint>
#include <fmt/format.h>
#include <string>
namespace spark::compiler {
class JSParser : public common::Object {
public:
  enum class NodeType {
    PRIVATE_NAME,

    LITERAL_REGEX,
    LITERAL_NULL,
    LITERAL_STRING,
    LITERAL_BOOLEAN,
    LITERAL_NUMBER,
    LITERAL_COMMENT,
    LITERAL_MULTILINE_COMMENT,
    LITERAL_UNDEFINED,
    LITERAL_IDENTITY,
    LITERAL_TEMPLATE,

    LITERAL_BIGINT,
    // LITERAL_DECIMAL,

    THIS,

    SUPER,

    PROGRAM,

    STATEMENT_EMPTY,
    STATEMENT_BLOCK,
    STATEMENT_DEBUGGER,
    STATEMENT_RETURN,
    STATEMENT_YIELD,
    STATEMENT_LABEL,
    STATEMENT_BREAK,
    STATEMENT_CONTINUE,
    STATEMENT_IF,
    STATEMENT_SWITCH,
    STATEMENT_SWITCH_CASE,
    STATEMENT_THROW,
    STATEMENT_TRY,
    STATEMENT_TRY_CATCH,
    STATEMENT_WHILE,
    STATEMENT_DO_WHILE,
    STATEMENT_FOR,
    STATEMENT_FOR_IN,
    STATEMENT_FOR_OF,
    STATEMENT_FOR_AWAIT_OF,

    VARIABLE_DECLARATION,
    VARIABLE_DECLARATOR,

    DECORATOR,

    DIRECTIVE,
    DIRECTIVE_LITERAL,
    INTERPRETER_DIRECTIVE,

    OBJECT_PROPERTY,
    OBJECT_METHOD,
    OBJECT_ACCESSOR,

    // EXPRESSION_RECORD,
    // EXPRESSION_TUPLE,

    EXPRESSION_UNARY,
    EXPRESSION_UPDATE,
    EXPRESSION_BINARY,
    EXPRESSION_MEMBER,
    EXPRESSION_OPTIONAL_MEMBER,
    EXPRESSION_COMPUTED_MEMBER,
    EXPRESSION_OPTIONAL_COMPUTED_MEMBER,
    EXPRESSION_CONDITION,
    EXPRESSION_CALL,
    EXPRESSION_OPTIONAL_CALL,
    EXPRESSION_NEW,
    EXPRESSION_DELETE,
    EXPRESSION_AWAIT,
    EXPRESSION_VOID,
    EXPRESSION_TYPEOF,
    EXPRESSION_GROUP,
    EXPRESSION_ASSIGMENT,

    EXPRESSION_REST,

    PATTERN_REST_ITEM,
    PATTERN_OBJECT,
    PATTERN_OBJECT_ITEM,
    PATTERN_ARRAY,
    PATTERN_ARRAY_ITEM,

    CLASS_METHOD,
    CLASS_PROPERTY,
    CLASS_ACCESSOR,
    STATIC_BLOCK,

    IMPORT_DECLARATION,
    IMPORT_SPECIFIER,
    IMPORT_DEFAULT,
    IMPORT_NAMESPACE,
    IMPORT_ATTARTUBE,
    EXPORT_DECLARATION,
    EXPORT_DEFAULT,
    EXPORT_SPECIFIER,
    EXPORT_ALL,

    DECLARATION_ARROW_FUNCTION,
    DECLARATION_FUNCTION,
    DECLARATION_PARAMETER,
    DECLARATION_REST_PARAMETER,
    DECLARATION_OBJECT,
    DECLARATION_ARRAY,
    DECLARATION_CLASS,
  };

  enum class AccessorKind { GET, SET };

  enum class DeclarationKind { CONST, LET, VAR, UNKNOWN };

  struct Position {
    std::uint32_t line;
    std::uint32_t column;
    std::uint32_t offset;
  };

  struct Location {
    Position start;
    Position end;
    std::wstring getSource(const std::wstring &source) {
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

  struct Token : public common::Object {
    Location location;
  };

  struct Node : public common::Object {
    NodeType type;
    Location location;
    int32_t level;
    Node(const NodeType &type, int32_t level = 0) : type(type), level(level) {}
  };

  struct NodeArray : public std::vector<common::AutoPtr<Node>> {};

  struct Program : public Node {
    common::AutoPtr<Node> interpreter;
    NodeArray directives;
    NodeArray body;
    Program() : Node(JSParser::NodeType::PROGRAM, 0) {}
  };

  struct StringLiteral : public Node {
    std::wstring value;
    StringLiteral() : Node(JSParser::NodeType::LITERAL_STRING, -2) {}
  };

  struct NumberLiteral : public Node {
    double value;
    NumberLiteral() : Node(JSParser::NodeType::LITERAL_NUMBER, -2) {}
  };

  struct BigIntLiteral : public Node {
    std::wstring value;
    BigIntLiteral() : Node(JSParser::NodeType::LITERAL_BIGINT, -2) {}
  };

  struct RegexLiteral : public Node {
    std::wstring value;
    bool hasIndices{};
    bool global{};
    bool ignoreCase{};
    bool multiline{};
    bool dotAll{};
    bool sticky{};
    RegexLiteral() : Node(JSParser::NodeType::LITERAL_REGEX, -2) {}
  };

  struct BooleanLiteral : public Node {
    bool value;
    BooleanLiteral() : Node(JSParser::NodeType::LITERAL_BOOLEAN, -2) {}
  };

  struct NullLiteral : public Node {
    NullLiteral() : Node(JSParser::NodeType::LITERAL_NULL, -2) {}
  };

  struct UndefinedLiteral : public Node {
    UndefinedLiteral() : Node(JSParser::NodeType::LITERAL_UNDEFINED, -2) {}
  };

  struct IdentifierLiteral : public Node {
    std::wstring value;
    IdentifierLiteral() : Node(JSParser::NodeType::LITERAL_IDENTITY, -2) {}
  };
  struct TemplateLiteral : public Node {
    std::wstring tag;
    std::vector<std::wstring> quasis;
    NodeArray expressions;
    TemplateLiteral() : Node(JSParser::NodeType::LITERAL_TEMPLATE, -2) {}
  };

  struct PrivateName : public Node {
    std::wstring value;
    PrivateName() : Node(JSParser::NodeType::PRIVATE_NAME) {}
  };

  struct Comment : public Node {
    Comment() : Node(JSParser::NodeType::LITERAL_COMMENT, -2) {}
  };

  struct InterpreterDirective : public Node {
    InterpreterDirective() : Node(JSParser::NodeType::INTERPRETER_DIRECTIVE) {}
  };

  struct Directive : public Node {
    Directive() : Node(JSParser::NodeType::DIRECTIVE) {}
  };

  struct BlockStatement : public Node {
    NodeArray body;
    NodeArray directives;
    BlockStatement() : Node(JSParser::NodeType::STATEMENT_BLOCK) {}
  };

  struct EmptyStatement : public Node {
    EmptyStatement() : Node(JSParser::NodeType::STATEMENT_EMPTY) {}
  };

  struct DebuggerStatement : public Node {
    DebuggerStatement() : Node(JSParser::NodeType::STATEMENT_DEBUGGER) {}
  };

  struct ForStatement : public Node {
    common::AutoPtr<Node> init;
    common::AutoPtr<Node> condition;
    common::AutoPtr<Node> update;
    common::AutoPtr<Node> body;
    ForStatement() : Node(JSParser::NodeType::STATEMENT_FOR) {}
  };

  struct ForInStatement : public Node {
    DeclarationKind kind;
    common::AutoPtr<Node> declaration;
    common::AutoPtr<Node> expression;
    common::AutoPtr<Node> body;
    ForInStatement() : Node(JSParser::NodeType::STATEMENT_FOR_IN) {}
  };

  struct ForOfStatement : public Node {
    DeclarationKind kind;
    common::AutoPtr<Node> declaration;
    common::AutoPtr<Node> expression;
    common::AutoPtr<Node> body;
    ForOfStatement() : Node(JSParser::NodeType::STATEMENT_FOR_OF) {}
  };

  struct ForAwaitOfStatement : public ForOfStatement {
    ForAwaitOfStatement() : ForOfStatement() {
      type = JSParser::NodeType::STATEMENT_FOR_AWAIT_OF;
    }
  };

  struct WhileStatement : public Node {
    common::AutoPtr<Node> condition;
    common::AutoPtr<Node> body;
    WhileStatement() : Node(JSParser::NodeType::STATEMENT_WHILE) {}
  };

  struct DoWhileStatement : public WhileStatement {
    DoWhileStatement() { type = NodeType::STATEMENT_DO_WHILE; }
  };

  struct YieldStatement : public Node {
    common::AutoPtr<Node> value;
    YieldStatement() : Node(JSParser::NodeType::STATEMENT_YIELD) {}
  };

  struct ReturnStatement : public Node {
    common::AutoPtr<Node> value;
    ReturnStatement() : Node(JSParser::NodeType::STATEMENT_RETURN) {}
  };

  struct ThrowStatement : public Node {
    common::AutoPtr<Node> value;
    ThrowStatement() : Node(JSParser::NodeType::STATEMENT_THROW) {}
  };

  struct TryCatchStatement : public Node {
    common::AutoPtr<Node> binding;
    common::AutoPtr<Node> statement;
    TryCatchStatement() : Node(JSParser::NodeType::STATEMENT_TRY_CATCH) {}
  };

  struct TryStatement : public Node {
    common::AutoPtr<Node> try_;
    common::AutoPtr<Node> catch_;
    common::AutoPtr<Node> finally;
    TryStatement() : Node(JSParser::NodeType::STATEMENT_TRY) {}
  };

  struct LabelStatement : public Node {
    common::AutoPtr<Node> label;
    common::AutoPtr<Node> statement;
    LabelStatement() : Node(JSParser::NodeType::STATEMENT_LABEL) {}
  };

  struct BreakStatement : public Node {
    common::AutoPtr<Node> label;
    BreakStatement() : Node(JSParser::NodeType::STATEMENT_BREAK) {}
  };

  struct ContinueStatement : public Node {
    common::AutoPtr<Node> label;
    ContinueStatement() : Node(JSParser::NodeType::STATEMENT_CONTINUE) {}
  };

  struct IfStatement : public Node {
    common::AutoPtr<Node> condition;
    common::AutoPtr<Node> alternate;
    common::AutoPtr<Node> consequent;
    IfStatement() : Node(JSParser::NodeType::STATEMENT_IF) {}
  };

  struct SwitchCaseStatement : public Node {
    common::AutoPtr<Node> match;
    NodeArray statements;
    SwitchCaseStatement() : Node(JSParser::NodeType::STATEMENT_SWITCH_CASE) {}
  };

  struct SwitchStatement : public Node {
    common::AutoPtr<Node> expression;
    NodeArray cases;
    SwitchStatement() : Node(JSParser::NodeType::STATEMENT_SWITCH) {}
  };

  struct Decorator : public Node {
    common::AutoPtr<Node> expression;
    Decorator() : Node(JSParser::NodeType::DECORATOR) {}
  };

  struct BinaryExpression : public Node {
    common::AutoPtr<Node> left;
    common::AutoPtr<Node> right;
    std::wstring opt;
    BinaryExpression() : Node(JSParser::NodeType::EXPRESSION_BINARY) {}
  };

  struct UpdateExpression : public BinaryExpression {
    UpdateExpression() {
      type = NodeType::EXPRESSION_UPDATE;
      level = -2;
    }
  };

  struct UnaryExpression : public BinaryExpression {
    UnaryExpression() {
      type = NodeType::EXPRESSION_UNARY;
      level = 4;
    }
  };

  struct GroupExpression : public Node {
    common::AutoPtr<Node> expression;
    GroupExpression() : Node(NodeType::EXPRESSION_GROUP, -2) {}
  };

  struct ConditionExpression : public BinaryExpression {
    ConditionExpression() {
      type = NodeType::EXPRESSION_CONDITION;
      level = 16;
    }
  };

  struct MemberExpression : public BinaryExpression {
    MemberExpression() {
      type = NodeType::EXPRESSION_MEMBER;
      level = 1;
    }
  };

  struct OptionalMemberExpression : public BinaryExpression {
    OptionalMemberExpression() {
      type = NodeType::EXPRESSION_OPTIONAL_MEMBER;
      level = 1;
    }
  };

  struct ComputedMemberExpression : public BinaryExpression {
    ComputedMemberExpression() {
      type = NodeType::EXPRESSION_COMPUTED_MEMBER;
      level = 1;
    }
  };

  struct OptionalComputedMemberExpression : public BinaryExpression {
    OptionalComputedMemberExpression() {
      type = NodeType::EXPRESSION_OPTIONAL_COMPUTED_MEMBER;
      level = 1;
    }
  };

  struct CallExpression : public BinaryExpression {
    NodeArray arguments;
    CallExpression() {
      type = NodeType::EXPRESSION_CALL;
      level = 1;
    }
  };

  struct OptionalCallExpression : public CallExpression {
    OptionalCallExpression() {
      type = NodeType::EXPRESSION_OPTIONAL_CALL;
      level = 1;
    }
  };

  struct AwaitExpression : public BinaryExpression {
    AwaitExpression() {
      type = NodeType::EXPRESSION_AWAIT;
      level = 4;
    }
  };

  struct TypeofExpression : public BinaryExpression {
    TypeofExpression() {
      type = NodeType::EXPRESSION_TYPEOF;
      level = 4;
    }
  };

  struct VoidExpression : public BinaryExpression {
    VoidExpression() {
      type = NodeType::EXPRESSION_VOID;
      level = 4;
    }
  };

  struct NewExpression : public BinaryExpression {
    NewExpression() {
      type = NodeType::EXPRESSION_NEW;
      level = 2;
    }
  };

  struct DeleteExpression : public BinaryExpression {
    DeleteExpression() {
      type = NodeType::EXPRESSION_DELETE;
      level = 4;
    }
  };

  struct Parameter : public Node {
    common::AutoPtr<Node> identifier;
    common::AutoPtr<Node> value;
    Parameter() : Node(NodeType::DECLARATION_PARAMETER, 0) {}
  };

  struct ArrowFunctionDeclaration : public Node {
    std::wstring name;

    std::string sourceFile;

    bool async;

    NodeArray arguments;

    common::AutoPtr<Node> body;
    ArrowFunctionDeclaration()
        : Node(NodeType::DECLARATION_ARROW_FUNCTION, -2) {}
  };

  struct FunctionDeclaration : public Node {
    NodeArray arguments;
    common::AutoPtr<Node> identifier;
    common::AutoPtr<Node> body;
    bool async;
    bool generator;
    FunctionDeclaration() : Node(NodeType::DECLARATION_FUNCTION, -2) {}
  };

  struct ObjectAccessor : public Node {
    AccessorKind kind;
    NodeArray arguments;
    common::AutoPtr<Node> identifier;
    common::AutoPtr<Node> body;
    ObjectAccessor() : Node(NodeType::OBJECT_ACCESSOR) {}
  };

  struct ObjectMethod : public FunctionDeclaration {
    ObjectMethod() { type = NodeType::OBJECT_METHOD; }
  };

  struct ObjectProperty : public Node {
    common::AutoPtr<Node> identifier;
    common::AutoPtr<Node> implement;
    ObjectProperty() : Node(NodeType::OBJECT_PROPERTY) {}
  };

  struct ObjectDeclaration : public Node {
    NodeArray properties;
    ObjectDeclaration() : Node(NodeType::DECLARATION_OBJECT, -2) {}
  };

  struct ClassProperty : public Node {
    common::AutoPtr<Node> identifier;
    common::AutoPtr<Node> value;
    NodeArray decorators;
    bool static_;
    ClassProperty() : Node(NodeType::CLASS_PROPERTY, 0) {}
  };

  struct ClassMethod : public FunctionDeclaration {
    bool static_;
    NodeArray decorators;
    ClassMethod() {
      type = NodeType::CLASS_METHOD;
      level = 0;
    }
  };

  struct ClassAccessor : public FunctionDeclaration {
    bool static_;
    AccessorKind kind;
    NodeArray decorators;
    ClassAccessor() {
      type = NodeType::CLASS_ACCESSOR;
      level = 0;
    }
  };

  struct ClassDeclaration : public Node {
    common::AutoPtr<Node> identifier;
    common::AutoPtr<Node> extends;
    NodeArray decorators;
    NodeArray properties;
    ClassDeclaration() : Node(NodeType::DECLARATION_CLASS, -2) {}
  };

  struct RestPatternItem : public Node {
    common::AutoPtr<Node> identifier;
    RestPatternItem() : Node(NodeType::PATTERN_REST_ITEM) {}
  };

  struct ObjectPatternItem : public Node {
    common::AutoPtr<Node> identifier;
    common::AutoPtr<Node> match;
    common::AutoPtr<Node> value;
    ObjectPatternItem() : Node(NodeType::PATTERN_OBJECT_ITEM) {}
  };

  struct ObjectPattern : public Node {
    NodeArray items;
    ObjectPattern() : Node(NodeType::PATTERN_OBJECT, -2) {}
  };

  struct ArrayPatternItem : public Node {
    common::AutoPtr<Node> identifier;
    common::AutoPtr<Node> value;
    ArrayPatternItem() : Node(NodeType::PATTERN_ARRAY_ITEM) {}
  };

  struct ArrayPattern : public Node {
    NodeArray items;
    ArrayPattern() : Node(NodeType::PATTERN_ARRAY, -2) {}
  };

  struct ArrayDeclaration : public Node {
    NodeArray items;
    ArrayDeclaration() : Node(NodeType::DECLARATION_ARRAY, -2) {}
  };

  struct VariableDeclarator : public Node {
    common::AutoPtr<Node> identifier;
    common::AutoPtr<Node> value;
    VariableDeclarator() : Node(NodeType::VARIABLE_DECLARATOR) {}
  };

  struct VariableDeclaration : public Node {
    DeclarationKind kind;
    NodeArray declarations;
    VariableDeclaration() : Node(NodeType::VARIABLE_DECLARATION) {}
  };

  struct ImportSpecifier : public Node {
    common::AutoPtr<Node> identifier;
    common::AutoPtr<Node> alias;
    ImportSpecifier() : Node(NodeType::IMPORT_SPECIFIER) {}
  };

  struct ImportDefaultSpecifier : public Node {
    common::AutoPtr<Node> identifier;
    ImportDefaultSpecifier() : Node(NodeType::IMPORT_DEFAULT) {}
  };

  struct ImportNamespaceSpecifier : public Node {
    common::AutoPtr<Node> alias;
    ImportNamespaceSpecifier() : Node(NodeType::IMPORT_NAMESPACE) {}
  };
  struct ImportAttribute : public Node {
    common::AutoPtr<Node> key;
    common::AutoPtr<Node> value;
    ImportAttribute() : Node(NodeType::IMPORT_ATTARTUBE) {}
  };

  struct ImportDeclaration : public Node {
    common::AutoPtr<Node> source;
    NodeArray items;
    NodeArray attributes;
    ImportDeclaration() : Node(NodeType::IMPORT_DECLARATION) {}
  };

  struct ExportDefaultSpecifier : public Node {
    common::AutoPtr<Node> value;
    ExportDefaultSpecifier() : Node(NodeType::EXPORT_DEFAULT) {}
  };

  struct ExportSpecifier : public Node {
    common::AutoPtr<Node> identifier;
    common::AutoPtr<Node> alias;
    ExportSpecifier() : Node(NodeType::EXPORT_SPECIFIER) {}
  };

  struct ExportAllSpecifier : public Node {
    common::AutoPtr<Node> alias;
    ExportAllSpecifier() : Node(NodeType::EXPORT_ALL) {}
  };

  struct ExportDeclaration : public Node {
    NodeArray items;
    common::AutoPtr<Node> source;
    NodeArray attributes;
    ExportDeclaration() : Node(NodeType::EXPORT_DECLARATION) {}
  };

private:
  std::wstring formatException(const std::wstring &message, uint32_t filename,
                               const std::wstring &source, Position position);

  Location getLocation(const std::wstring &source, Position &start,
                       Position &end);

private:
  bool skipWhiteSpace(uint32_t filename, const std::wstring &source,
                      Position &position);

  bool skipComment(uint32_t filename, const std::wstring &source,
                   Position &position);

  bool skipLineTerminatior(uint32_t filename, const std::wstring &source,
                           Position &position);

  bool skipSemi(uint32_t filename, const std::wstring &source,
                Position &position);

  void skipInvisible(uint32_t filename, const std::wstring &source,
                     Position &position, bool *isNewline = nullptr);

  void skipNewLine(uint32_t filename, const std::wstring &source,
                   Position &position, bool *isNewline = nullptr);

private:
  common::AutoPtr<Token> readStringToken(uint32_t filename,
                                         const std::wstring &source,
                                         Position &position);

  common::AutoPtr<Token> readNumberToken(uint32_t filename,
                                         const std::wstring &source,
                                         Position &position);

  common::AutoPtr<Token> readBigIntToken(uint32_t filename,
                                         const std::wstring &source,
                                         Position &position);

  common::AutoPtr<Token> readRegexToken(uint32_t filename,
                                        const std::wstring &source,
                                        Position &position);

  common::AutoPtr<Token> readCommentToken(uint32_t filename,
                                          const std::wstring &source,
                                          Position &position);

  common::AutoPtr<Token> readBooleanToken(uint32_t filename,
                                          const std::wstring &source,
                                          Position &position);

  common::AutoPtr<Token> readNullToken(uint32_t filename,
                                       const std::wstring &source,
                                       Position &position);

  common::AutoPtr<Token> readUndefinedToken(uint32_t filename,
                                            const std::wstring &source,
                                            Position &position);

  common::AutoPtr<Token> readKeywordToken(uint32_t filename,
                                          const std::wstring &source,
                                          Position &position);

  common::AutoPtr<Token> readIdentifierToken(uint32_t filename,
                                             const std::wstring &source,
                                             Position &position,
                                             bool allowKeyword = false);

  common::AutoPtr<Token> readSymbolToken(uint32_t filename,
                                         const std::wstring &source,
                                         Position &position);

  common::AutoPtr<Token> readTemplateToken(uint32_t filename,
                                           const std::wstring &source,
                                           Position &position);

  common::AutoPtr<Token> readTemplateStartToken(uint32_t filename,
                                                const std::wstring &source,
                                                Position &position);

  common::AutoPtr<Token> readTemplatePatternToken(uint32_t filename,
                                                  const std::wstring &source,
                                                  Position &position);

  common::AutoPtr<Token> readTemplateEndToken(uint32_t filename,
                                              const std::wstring &source,
                                              Position &position);

private:
  common::AutoPtr<Node> readProgram(uint32_t filename,
                                    const std::wstring &source,
                                    Position &position);

  common::AutoPtr<Node> readStatement(uint32_t filename,
                                      const std::wstring &source,
                                      Position &position);

  common::AutoPtr<Node> readEmptyStatement(uint32_t filename,
                                           const std::wstring &source,
                                           Position &position);

  NodeArray readStatements(uint32_t filename, const std::wstring &source,
                           Position &position);

  common::AutoPtr<Node> readDebuggerStatement(uint32_t filename,
                                              const std::wstring &source,
                                              Position &position);

  common::AutoPtr<Node> readWhileStatement(uint32_t filename,
                                           const std::wstring &source,
                                           Position &position);

  common::AutoPtr<Node> readDoWhileStatement(uint32_t filename,
                                             const std::wstring &source,
                                             Position &position);

  common::AutoPtr<Node> readForStatement(uint32_t filename,
                                         const std::wstring &source,
                                         Position &position);

  common::AutoPtr<Node> readForInStatement(uint32_t filename,
                                           const std::wstring &source,
                                           Position &position);

  common::AutoPtr<Node> readForOfStatement(uint32_t filename,
                                           const std::wstring &source,
                                           Position &position);

  common::AutoPtr<Node> readBlockStatement(uint32_t filename,
                                           const std::wstring &source,
                                           Position &position);

  common::AutoPtr<Node> readYieldStatement(uint32_t filename,
                                           const std::wstring &source,
                                           Position &position);

  common::AutoPtr<Node> readReturnStatement(uint32_t filename,
                                            const std::wstring &source,
                                            Position &position);

  common::AutoPtr<Node> readThrowStatement(uint32_t filename,
                                           const std::wstring &source,
                                           Position &position);

  common::AutoPtr<Node> readBreakStatement(uint32_t filename,
                                           const std::wstring &source,
                                           Position &position);

  common::AutoPtr<Node> readContinueStatement(uint32_t filename,
                                              const std::wstring &source,
                                              Position &position);

  common::AutoPtr<Node> readLabelStatement(uint32_t filename,
                                           const std::wstring &source,
                                           Position &position);

  common::AutoPtr<Node> readIfStatement(uint32_t filename,
                                        const std::wstring &source,
                                        Position &position);

  common::AutoPtr<Node> readSwitchStatement(uint32_t filename,
                                            const std::wstring &source,
                                            Position &position);

  common::AutoPtr<Node> readSwitchCaseStatement(uint32_t filename,
                                                const std::wstring &source,
                                                Position &position);

  common::AutoPtr<Node> readTryStatement(uint32_t filename,
                                         const std::wstring &source,
                                         Position &position);

  common::AutoPtr<Node> readTryCatchStatement(uint32_t filename,
                                              const std::wstring &source,
                                              Position &position);

  common::AutoPtr<Node> readExpressionStatement(uint32_t filename,
                                                const std::wstring &source,
                                                Position &position);

  common::AutoPtr<Node> readValue(uint32_t filename, const std::wstring &source,
                                  Position &position);

  common::AutoPtr<Node> readRValue(uint32_t filename,
                                   const std::wstring &source,
                                   Position &position, int level);

  common::AutoPtr<Node> readDecorator(uint32_t filename,
                                      const std::wstring &source,
                                      Position &position);

  common::AutoPtr<Node> readExpression(uint32_t filename,
                                       const std::wstring &source,
                                       Position &position);

  common::AutoPtr<Node> readExpressions(uint32_t filename,
                                        const std::wstring &source,
                                        Position &position);

  common::AutoPtr<Node> readInterpreterDirective(uint32_t filename,
                                                 const std::wstring &source,
                                                 Position &position);

  common::AutoPtr<Node> readDirective(uint32_t filename,
                                      const std::wstring &source,
                                      Position &position);

  common::AutoPtr<Node> readStringLiteral(uint32_t filename,
                                          const std::wstring &source,
                                          Position &position);

  common::AutoPtr<Node> readNumberLiteral(uint32_t filename,
                                          const std::wstring &source,
                                          Position &position);

  common::AutoPtr<Node> readBigIntLiteral(uint32_t filename,
                                          const std::wstring &source,
                                          Position &position);

  common::AutoPtr<Node> readBooleanLiteral(uint32_t filename,
                                           const std::wstring &source,
                                           Position &position);

  common::AutoPtr<Node> readUndefinedLiteral(uint32_t filename,
                                             const std::wstring &source,
                                             Position &position);

  common::AutoPtr<Node> readNullLiteral(uint32_t filename,
                                        const std::wstring &source,
                                        Position &position);

  common::AutoPtr<Node> readIdentifierLiteral(uint32_t filename,
                                              const std::wstring &source,
                                              Position &position);

  common::AutoPtr<Node> readMemberLiteral(uint32_t filename,
                                          const std::wstring &source,
                                          Position &position);

  common::AutoPtr<Node> readPrivateName(uint32_t filename,
                                        const std::wstring &source,
                                        Position &position);

  common::AutoPtr<Node> readRegexLiteral(uint32_t filename,
                                         const std::wstring &source,
                                         Position &position);

  common::AutoPtr<Node> readTemplateLiteral(uint32_t filename,
                                            const std::wstring &source,
                                            Position &position);

  common::AutoPtr<Node> readBinaryExpression(uint32_t filename,
                                             const std::wstring &source,
                                             Position &position);

  common::AutoPtr<Node> readAssigmentExpression(uint32_t filename,
                                                const std::wstring &source,
                                                Position &position);

  common::AutoPtr<Node> readConditionExpression(uint32_t filename,
                                                const std::wstring &source,
                                                Position &position);

  common::AutoPtr<Node> readUpdateExpression(uint32_t filename,
                                             const std::wstring &source,
                                             Position &position);

  common::AutoPtr<Node> readUnaryExpression(uint32_t filename,
                                            const std::wstring &source,
                                            Position &position);

  common::AutoPtr<Node> readGroupExpression(uint32_t filename,
                                            const std::wstring &source,
                                            Position &position);

  common::AutoPtr<Node> readMemberExpression(uint32_t filename,
                                             const std::wstring &source,
                                             Position &position);

  common::AutoPtr<Node> readCallExpression(uint32_t filename,
                                           const std::wstring &source,
                                           Position &position);

  common::AutoPtr<Node> readRestExpression(uint32_t filename,
                                           const std::wstring &source,
                                           Position &position);

  common::AutoPtr<Node> readAwaitExpression(uint32_t filename,
                                            const std::wstring &source,
                                            Position &position);

  common::AutoPtr<Node> readTypeofExpression(uint32_t filename,
                                             const std::wstring &source,
                                             Position &position);

  common::AutoPtr<Node> readVoidExpression(uint32_t filename,
                                           const std::wstring &source,
                                           Position &position);

  common::AutoPtr<Node> readDeleteExpression(uint32_t filename,
                                             const std::wstring &source,
                                             Position &position);

  common::AutoPtr<Node> readNewExpression(uint32_t filename,
                                          const std::wstring &source,
                                          Position &position);

  common::AutoPtr<Node> readInExpression(uint32_t filename,
                                         const std::wstring &source,
                                         Position &position);

  common::AutoPtr<Node> readInstanceOfExpression(uint32_t filename,
                                                 const std::wstring &source,
                                                 Position &position);

  common::AutoPtr<Node> readParameter(uint32_t filename,
                                      const std::wstring &source,
                                      Position &position);

  common::AutoPtr<Node> readArrowFunctionDeclaration(uint32_t filename,
                                                     const std::wstring &source,
                                                     Position &position);

  common::AutoPtr<Node> readFunctionDeclaration(uint32_t filename,
                                                const std::wstring &source,
                                                Position &position);

  common::AutoPtr<Node> readArrayDeclaration(uint32_t filename,
                                             const std::wstring &source,
                                             Position &position);

  common::AutoPtr<Node> readObjectDeclaration(uint32_t filename,
                                              const std::wstring &source,
                                              Position &position);

  common::AutoPtr<Node> readObjectProperty(uint32_t filename,
                                           const std::wstring &source,
                                           Position &position);

  common::AutoPtr<Node> readObjectMethod(uint32_t filename,
                                         const std::wstring &source,
                                         Position &position);

  common::AutoPtr<Node> readObjectAccessor(uint32_t filename,
                                           const std::wstring &source,
                                           Position &position);

  common::AutoPtr<Node> readClassDeclaration(uint32_t filename,
                                             const std::wstring &source,
                                             Position &position);

  common::AutoPtr<Node> readClassProperty(uint32_t filename,
                                          const std::wstring &source,
                                          Position &position);

  common::AutoPtr<Node> readClassMethod(uint32_t filename,
                                        const std::wstring &source,
                                        Position &position);

  common::AutoPtr<Node> readClassAccessor(uint32_t filename,
                                          const std::wstring &source,
                                          Position &position);

  common::AutoPtr<Node> readStaticBlock(uint32_t filename,
                                        const std::wstring &source,
                                        Position &position);

  common::AutoPtr<Node> readVariableDeclarator(uint32_t filename,
                                               const std::wstring &source,
                                               Position &position);

  common::AutoPtr<Node> readVariableDeclaration(uint32_t filename,
                                                const std::wstring &source,
                                                Position &position);

  common::AutoPtr<Node> readObjectPattern(uint32_t filename,
                                          const std::wstring &source,
                                          Position &position);

  common::AutoPtr<Node> readObjectPatternItem(uint32_t filename,
                                              const std::wstring &source,
                                              Position &position);

  common::AutoPtr<Node> readArrayPattern(uint32_t filename,
                                         const std::wstring &source,
                                         Position &position);

  common::AutoPtr<Node> readArrayPatternItem(uint32_t filename,
                                             const std::wstring &source,
                                             Position &position);

  common::AutoPtr<Node> readRestPattern(uint32_t filename,
                                        const std::wstring &source,
                                        Position &position);

  common::AutoPtr<Node> readComment(uint32_t filename,
                                    const std::wstring &source,
                                    Position &position);

  common::AutoPtr<Node> readImportSpecifier(uint32_t filename,
                                            const std::wstring &source,
                                            Position &position);

  common::AutoPtr<Node> readImportAttriabue(uint32_t filename,
                                            const std::wstring &source,
                                            Position &position);

  common::AutoPtr<Node> readImportDefaultSpecifier(uint32_t filename,
                                                   const std::wstring &source,
                                                   Position &position);

  common::AutoPtr<Node> readImportNamespaceSpecifier(uint32_t filename,
                                                     const std::wstring &source,
                                                     Position &position);

  common::AutoPtr<Node> readImportDeclaration(uint32_t filename,
                                              const std::wstring &source,
                                              Position &position);

  common::AutoPtr<Node> readExportSpecifier(uint32_t filename,
                                            const std::wstring &source,
                                            Position &position);

  common::AutoPtr<Node> readExportDefaultSpecifier(uint32_t filename,
                                                   const std::wstring &source,
                                                   Position &position);

  common::AutoPtr<Node> readExportAllSpecifier(uint32_t filename,
                                               const std::wstring &source,
                                               Position &position);

  common::AutoPtr<Node> readExportDeclaration(uint32_t filename,
                                              const std::wstring &source,
                                              Position &position);

public:
  common::AutoPtr<Node> parse(uint32_t filename, const std::string &source);
};
}; // namespace spark::compiler