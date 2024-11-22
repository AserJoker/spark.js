#include "compiler/JSParser.hpp"
#include "common/AutoPtr.hpp"
#include "error/JSSyntaxError.hpp"
#include <algorithm>
#include <array>
#include <codecvt>
#include <fmt/xchar.h>
#include <locale>
#include <string>
#include <vector>

using namespace spark;
using namespace spark::compiler;

static std::array WSC = {0x9, 0xb, 0xc, 0x20, 0xa0, 0xfeff};
static std::array LTC = {0xa, 0xd, 0x2028, 0x2029};
static std::array KEYWORLDS = {
    L"break",      L"case",       L"catch",     L"class",  L"const",
    L"continue",   L"debugger",   L"default",   L"delete", L"do",
    L"else",       L"export",     L"extends",   L"false",  L"finally",
    L"for",        L"function",   L"if",        L"import", L"in",
    L"instanceof", L"new",        L"null",      L"return", L"super",
    L"switch",     L"this",       L"throw",     L"true",   L"try",
    L"typeof",     L"var",        L"void",      L"while",  L"with",
    L"enum",       L"implements", L"interface", L"let",    L"package",
    L"private",    L"protected",  L"public",    L"static", L"await",
    L"yield"};

void JSParser::bindDeclaration(common::AutoPtr<IdentifierLiteral> identifier) {
  auto name = identifier->value;
  Declaration *declar = nullptr;
  auto scope = _currentScope;
  while (!declar && scope) {
    for (auto &dec : scope->declarations) {
      if (dec.name == name) {
        declar = &dec;
        break;
      }
    }
    if (declar != nullptr) {
      break;
    }
    scope = scope->parent;
  }
  Binding *binding = nullptr;
  for (auto &b : _currentScope->bindings) {
    if (b.declaration == declar) {
      binding = &b;
      break;
    }
  }
  if (!binding) {
    _currentScope->bindings.push_back({});
    binding = &*_currentScope->bindings.rbegin();
    binding->declaration = declar;
  }
  binding->references.push_back(identifier.getRawPointer());
}
void JSParser::bindScope(common::AutoPtr<Node> node) {
  auto scope = _currentScope;
  if (node->scope != nullptr) {
    _currentScope = node->scope.getRawPointer();
  }
  switch (node->type) {
  case NodeType::LITERAL_IDENTITY:
    bindDeclaration(node.cast<IdentifierLiteral>());
    break;
  case NodeType::DECLARATION_ARROW_FUNCTION: {
    auto afunc = node.cast<ArrowFunctionDeclaration>();
    for (auto &arg : afunc->arguments) {
      bindScope(arg);
    }
    bindScope(afunc->body);
  } break;
  case NodeType::DECLARATION_FUNCTION: {
    auto afunc = node.cast<FunctionDeclaration>();
    for (auto &arg : afunc->arguments) {
      bindScope(arg);
    }
    bindScope(afunc->body);
  } break;

  case NodeType::STATEMENT_FOR_IN: {
    auto forin = node.cast<ForInStatement>();
    bindScope(forin->expression);
    bindScope(forin->body);
    if (forin->declaration->type != NodeType::LITERAL_IDENTITY ||
        forin->kind == DeclarationKind::UNKNOWN) {
      bindScope(forin->declaration);
    }
  } break;
  case NodeType::STATEMENT_FOR_OF: {
    auto forof = node.cast<ForOfStatement>();
    bindScope(forof->expression);
    bindScope(forof->body);
    if (forof->declaration->type != NodeType::LITERAL_IDENTITY ||
        forof->kind == DeclarationKind::UNKNOWN) {
      bindScope(forof->declaration);
    }
  } break;
  case NodeType::STATEMENT_FOR_AWAIT_OF: {
    auto forof = node.cast<ForAwaitOfStatement>();
    bindScope(forof->expression);
    bindScope(forof->body);
    if (forof->declaration->type != NodeType::LITERAL_IDENTITY ||
        forof->kind == DeclarationKind::UNKNOWN) {
      bindScope(forof->declaration);
    }
  } break;
  case NodeType::OBJECT_PROPERTY: {
    auto prop = node.cast<ObjectProperty>();
    if (prop->implement != nullptr) {
      bindScope(prop->implement);
    }
    if (prop->identifier->type != NodeType::LITERAL_IDENTITY ||
        !prop->implement) {
      bindScope(prop->identifier);
    }
  } break;
  case NodeType::OBJECT_METHOD: {
    auto prop = node.cast<ObjectMethod>();
    for (auto &arg : prop->arguments) {
      bindScope(arg);
    }
    bindScope(prop->body);
    if (prop->identifier->type != NodeType::LITERAL_IDENTITY) {
      bindScope(prop->identifier);
    }
  } break;
  case NodeType::OBJECT_ACCESSOR: {
    auto prop = node.cast<ObjectMethod>();
    for (auto &arg : prop->arguments) {
      bindScope(arg);
    }
    bindScope(prop->body);
    if (prop->identifier->type != NodeType::LITERAL_IDENTITY) {
      bindScope(prop->identifier);
    }
  } break;
  case NodeType::EXPRESSION_MEMBER: {
    auto member = node.cast<MemberExpression>();
    bindScope(member->left);
  } break;
  case NodeType::EXPRESSION_OPTIONAL_MEMBER: {
    auto member = node.cast<OptionalMemberExpression>();
    bindScope(member->left);
  } break;
  case NodeType::PATTERN_OBJECT_ITEM: {
    auto objitem = node.cast<ObjectPatternItem>();
    if (objitem->value != nullptr) {
      bindScope(objitem->value);
    }
    if (objitem->identifier->type != NodeType::LITERAL_IDENTITY) {
      bindScope(objitem->identifier);
    }
  } break;
  case NodeType::PATTERN_ARRAY_ITEM: {
    auto arritem = node.cast<ArrayPatternItem>();
    if (arritem->value != nullptr) {
      bindScope(arritem->value);
    }
  } break;
  case NodeType::CLASS_METHOD: {
    auto method = node.cast<ClassMethod>();
    for (auto &deco : method->decorators) {
      bindScope(deco);
    }
    if (method->identifier->type != NodeType::LITERAL_IDENTITY) {
      bindScope(method->identifier);
    }
    for (auto &arg : method->arguments) {
      bindScope(arg);
    }
    bindScope(method->body);
  } break;
  case NodeType::CLASS_PROPERTY: {
    auto prop = node.cast<ClassProperty>();
    for (auto &deco : prop->decorators) {
      bindScope(deco);
    }
    if (prop->identifier->type != NodeType::LITERAL_IDENTITY) {
      bindScope(prop->identifier);
    }
    if (prop->value != nullptr) {
      bindScope(prop->value);
    }
  } break;
  case NodeType::CLASS_ACCESSOR: {
    auto method = node.cast<ClassAccessor>();
    for (auto &deco : method->decorators) {
      bindScope(deco);
    }
    if (method->identifier->type != NodeType::LITERAL_IDENTITY) {
      bindScope(method->identifier);
    }
    for (auto &arg : method->arguments) {
      bindScope(arg);
    }
    bindScope(method->body);
  } break;
  case NodeType::DECLARATION_PARAMETER: {
    auto param = node.cast<Parameter>();
    if (param->identifier->type != NodeType::LITERAL_IDENTITY) {
      bindScope(param);
    }
    if (param->value != nullptr) {
      bindScope(param->value);
    }
  } break;
  case NodeType::DECLARATION_CLASS: {
    auto clazz = node.cast<ClassDeclaration>();
    if (clazz->extends != nullptr) {
      bindScope(clazz->extends);
    }
    for (auto &dec : clazz->decorators) {
      bindScope(dec);
    }
    for (auto &prop : clazz->properties) {
      bindScope(prop);
    }
  } break;
  case NodeType::PATTERN_REST_ITEM: {
    auto rest = node.cast<RestPatternItem>();
    if (rest->identifier->type != NodeType::LITERAL_IDENTITY) {
      bindScope(rest->identifier);
    }
  } break;
  case NodeType::EXPORT_SPECIFIER: {
    auto specifier = node.cast<ExportSpecifier>();
    bindScope(specifier->identifier);
  } break;
  case NodeType::VARIABLE_DECLARATOR: {
    auto dec = node.cast<VariableDeclarator>();
    if (dec->value != nullptr) {
      bindScope(dec->value);
    }
    if (dec->identifier->type != NodeType::LITERAL_IDENTITY) {
      bindScope(dec->identifier);
    }
  } break;
  case NodeType::EXPORT_DEFAULT:
  case NodeType::EXPORT_DECLARATION:
  case NodeType::EXPRESSION_REST:
  case NodeType::EXPRESSION_CALL:
  case NodeType::EXPRESSION_OPTIONAL_CALL:
  case NodeType::EXPRESSION_NEW:
  case NodeType::EXPRESSION_DELETE:
  case NodeType::EXPRESSION_AWAIT:
  case NodeType::EXPRESSION_VOID:
  case NodeType::EXPRESSION_TYPEOF:
  case NodeType::EXPRESSION_GROUP:
  case NodeType::EXPRESSION_ASSIGMENT:
  case NodeType::PATTERN_OBJECT:
  case NodeType::PATTERN_ARRAY:
  case NodeType::STATIC_BLOCK:
  case NodeType::DECLARATION_OBJECT:
  case NodeType::DECLARATION_ARRAY:
  case NodeType::PROGRAM:
  case NodeType::STATEMENT_EMPTY:
  case NodeType::STATEMENT_BLOCK:
  case NodeType::STATEMENT_DEBUGGER:
  case NodeType::STATEMENT_RETURN:
  case NodeType::STATEMENT_YIELD:
  case NodeType::STATEMENT_LABEL:
  case NodeType::STATEMENT_BREAK:
  case NodeType::STATEMENT_CONTINUE:
  case NodeType::STATEMENT_IF:
  case NodeType::STATEMENT_SWITCH:
  case NodeType::STATEMENT_SWITCH_CASE:
  case NodeType::STATEMENT_THROW:
  case NodeType::STATEMENT_TRY:
  case NodeType::STATEMENT_TRY_CATCH:
  case NodeType::STATEMENT_WHILE:
  case NodeType::STATEMENT_DO_WHILE:
  case NodeType::STATEMENT_FOR:
  case NodeType::VARIABLE_DECLARATION:
  case NodeType::DECORATOR:
  case NodeType::EXPRESSION_UNARY:
  case NodeType::EXPRESSION_UPDATE:
  case NodeType::EXPRESSION_BINARY:
  case NodeType::EXPRESSION_COMPUTED_MEMBER:
  case NodeType::EXPRESSION_OPTIONAL_COMPUTED_MEMBER:
  case NodeType::EXPRESSION_CONDITION: {
    for (auto &child : node->children) {
      bindScope(child);
    }
  } break;
  case NodeType::THIS:
  case NodeType::SUPER:
  case NodeType::DIRECTIVE:
  case NodeType::INTERPRETER_DIRECTIVE:
  case NodeType::IMPORT_DECLARATION:
  case NodeType::IMPORT_SPECIFIER:
  case NodeType::IMPORT_DEFAULT:
  case NodeType::IMPORT_NAMESPACE:
  case NodeType::IMPORT_ATTARTUBE:
  case NodeType::EXPORT_ALL:
  case NodeType::PRIVATE_NAME:
  case NodeType::LITERAL_REGEX:
  case NodeType::LITERAL_NULL:
  case NodeType::LITERAL_STRING:
  case NodeType::LITERAL_BOOLEAN:
  case NodeType::LITERAL_NUMBER:
  case NodeType::LITERAL_COMMENT:
  case NodeType::LITERAL_MULTILINE_COMMENT:
  case NodeType::LITERAL_UNDEFINED:
  case NodeType::LITERAL_TEMPLATE:
  case NodeType::LITERAL_BIGINT:
    return;
  }
  _currentScope = scope;
}

common::AutoPtr<JSParser::Node> JSParser::parse(uint32_t filename,
                                                const std::wstring &source) {
  Position position = {0, 0, 0};
  auto root = readProgram(filename, source, position);
  if (root != nullptr) {
    bindScope(root);
  }
  return root;
}

std::wstring JSParser::formatException(const std::wstring &message,
                                       uint32_t filename,
                                       const std::wstring &source,
                                       Position position) {
  static std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
  std::wstring line;
  std::wstring spaceLine;
  auto index = position.offset - position.column;
  while (source[index] != '\n' && source[index] != '\0') {
    if (source[index] != '\r') {
      line += source[index];
      if (index == position.offset) {
        spaceLine += L'^';
      } else {
        spaceLine += L' ';
      }
    }
    index++;
  }
  if (source[position.offset] == '\r' || source[position.offset] == '\n' ||
      source[position.offset] == '\0') {
    spaceLine += L'^';
  }

  return fmt::format(L"{}: \n{}\n{}\n", message, line, spaceLine);
}

JSParser::Location JSParser::getLocation(const std::wstring &source,
                                         Position &start, Position &end) {
  Location loc;
  loc.start = start;
  loc.end = end;
  loc.end.offset--;
  loc.end.column--;
  return loc;
}

void JSParser::declareVariable(common::AutoPtr<Node> declarator,
                               common::AutoPtr<Node> identifier,
                               Declaration::TYPE type, bool isConst) {
  if (identifier->type == NodeType::PATTERN_OBJECT) {
    auto obj = identifier.cast<ObjectPattern>();
    for (auto &item : obj->items) {
      if (item->type == NodeType::PATTERN_OBJECT_ITEM) {
        auto objitem = item.cast<ObjectPatternItem>();
        if (!objitem->match) {
          return declareVariable(declarator, objitem->identifier, type,
                                 isConst);
        } else {
          return declareVariable(declarator, objitem->match, type, isConst);
        }
      } else if (item->type == NodeType::PATTERN_REST_ITEM) {
        return declareVariable(declarator,
                               item.cast<RestPatternItem>()->identifier, type,
                               isConst);
      }
    }
  }
  if (identifier->type == NodeType::PATTERN_ARRAY) {
    auto arr = identifier.cast<ArrayPattern>();
    for (auto &item : arr->items) {
      if (item->type == NodeType::PATTERN_ARRAY_ITEM) {
        auto arritem = item.cast<ArrayPatternItem>();
        return declareVariable(declarator, arritem->identifier, type, isConst);
      } else if (item->type == NodeType::PATTERN_REST_ITEM) {
        return declareVariable(declarator,
                               item.cast<RestPatternItem>()->identifier, type,
                               isConst);
      }
    }
  }
  Declaration declar;
  declar.isConst = isConst;
  declar.type = type;
  declar.scope = _currentScope;
  declar.node = declarator.getRawPointer();
  if (identifier->type == NodeType::LITERAL_STRING) {
    declar.name = identifier.cast<StringLiteral>()->value;
  } else if (identifier->type == NodeType::LITERAL_IDENTITY) {
    declar.name = identifier.cast<IdentifierLiteral>()->value;
  }
  _currentScope->declarations.push_back(declar);
}

bool JSParser::skipWhiteSpace(uint32_t filename, const std::wstring &source,
                              Position &position) {
  auto &chr = source[position.offset];
  if (std::find(WSC.begin(), WSC.end(), chr) == WSC.end()) {
    return false;
  }
  for (;;) {
    auto &chr = source[position.offset];
    if (std::find(WSC.begin(), WSC.end(), chr) != WSC.end()) {
      position.offset++;
      position.column++;
    } else {
      break;
    }
  }
  return true;
}

bool JSParser::skipComment(uint32_t filename, const std::wstring &source,
                           Position &position) {
  auto node = readComment(filename, source, position);
  if (node != nullptr) {
    position = node->location.end;
    position.offset++;
    position.column++;

    return true;
  }
  return false;
}

bool JSParser::skipLineTerminatior(uint32_t filename,
                                   const std::wstring &source,
                                   Position &position) {
  auto &chr = source[position.offset];
  if (std::find(LTC.begin(), LTC.end(), chr) == LTC.end()) {
    return false;
  }
  for (;;) {
    auto &chr = source[position.offset];
    if (source[position.offset] == '\r' &&
        source[position.offset + 1] == '\n') {
      position.offset += 2;
      position.line++;
      position.column = 0;
    } else if (std::find(LTC.begin(), LTC.end(), chr) != LTC.end()) {
      position.offset++;
      position.line++;
      position.column = 0;
    } else {
      break;
    }
  }
  return true;
}

bool JSParser::skipSemi(uint32_t filename, const std::wstring &source,
                        Position &position) {
  auto &chr = source[position.offset];
  if (chr == ';') {
    position.offset++;
    position.column++;
    return true;
  }
  return false;
}

void JSParser::skipInvisible(uint32_t filename, const std::wstring &source,
                             Position &position, bool *isNewline) {
  auto current = position;
  while (skipLineTerminatior(filename, source, position) ||
         skipComment(filename, source, position) ||
         skipWhiteSpace(filename, source, position)) {
    ;
  }
  if (isNewline) {
    *isNewline = position.line != current.line;
  }
}

void JSParser::skipNewLine(uint32_t filename, const std::wstring &source,
                           Position &position, bool *isNewline) {
  auto current = position;
  while (skipComment(filename, source, position) ||
         skipWhiteSpace(filename, source, position) ||
         skipLineTerminatior(filename, source, position) ||
         skipSemi(filename, source, position)) {
  }
  if (isNewline) {
    *isNewline = position.line != current.line;
  }
}

common::AutoPtr<JSParser::Token>
JSParser::readStringToken(uint32_t filename, const std::wstring &source,
                          Position &position) {
  auto current = position;
  auto &chr = source[current.offset];
  if (chr == '\"' || chr == L'\'') {
    common::AutoPtr token = new Token;
    auto start = chr;
    auto mask = false;
    for (;;) {
      auto &chr = source[current.offset];
      if (chr == '\0' || std::find(LTC.begin(), LTC.end(), chr) != LTC.end()) {
        current.column += current.offset - position.offset;
        throw error::JSSyntaxError(
            formatException(L"Incomplete string", filename, source, current),
            {filename, current.line, current.column});
      }
      if (chr == '\\') {
        mask = !mask;
      }
      if (chr == start && current.offset != position.offset && !mask) {
        break;
      }
      if (mask && chr != '\\') {
        mask = false;
      }

      current.offset++;
    }
    current.column += current.offset - position.offset;
    current.offset++;
    current.column++;
    token->location = getLocation(source, position, current);
    position = current;
    return token;
  }
  return nullptr;
}

common::AutoPtr<JSParser::Token>
JSParser::readCommentToken(uint32_t filename, const std::wstring &source,
                           Position &position) {
  auto current = position;
  auto &chr = source[current.offset];
  if (chr == '/') {
    current.offset++;
    if (source[current.offset] == '/') {
      for (;;) {
        if (source[current.offset] == '\0' ||
            std::find(LTC.begin(), LTC.end(), source[current.offset]) !=
                LTC.end()) {
          current.offset--;
          break;
        }
        current.offset++;
      }
      current.column += current.offset - position.offset;
      common::AutoPtr token = new Token();
      current.offset++;
      current.column++;
      token->location = getLocation(source, position, current);
      position = current;
      return token;
    }
    if (source[current.offset] == '*') {
      current.offset++;
      for (;;) {
        auto &chr = source[current.offset];
        if (chr == '\0') {
          throw error::JSSyntaxError(
              formatException(L"Incomplete comment", filename, source, current),
              {filename, current.line, current.column});
        }
        if (chr == '/' && source[current.offset - 1] == '*') {
          current.column++;
          current.offset++;
          break;
        }
        if (std::find(LTC.begin(), LTC.end(), chr) != LTC.end()) {
          current.line++;
          current.column = 0;
        } else {
          current.column++;
        }
        current.offset++;
      }
      common::AutoPtr token = new Token();
      current.offset++;
      current.column++;
      token->location = getLocation(source, position, current);
      position = current;
      return token;
    }
  }
  return nullptr;
}

common::AutoPtr<JSParser::Token>
JSParser::readNumberToken(uint32_t filename, const std::wstring &source,
                          Position &position) {

  auto current = position;
  auto &chr = source[current.offset];
  if ((chr >= '0' && chr <= '9') ||
      (chr == '.' && source[current.offset + 1] >= '0' &&
       source[current.offset + 1] <= '9')) {
    if (chr == '0' && (source[current.offset + 1] == 'x' ||
                       source[current.offset + 1] == 'X')) {
      current.offset += 2;
      for (;;) {
        auto &chr = source[current.offset];
        if ((chr >= '0' && chr <= '9') || (chr >= 'a' && chr <= 'f') ||
            (chr >= 'A' && chr <= 'F')) {
          current.offset++;
        } else {
          break;
        }
      }
    } else if (chr == '0' && (source[current.offset + 1] == 'o' ||
                              source[current.offset + 1] == 'O')) {
      current.offset += 2;
      for (;;) {
        auto &chr = source[current.offset];
        if (chr >= '0' && chr <= '7') {
          current.offset++;
        } else {
          break;
        }
      }
    } else {
      bool dec = false;
      for (;;) {
        auto &chr = source[current.offset];
        if (chr == '.' && !dec) {
          dec = true;
          current.offset++;
        } else if (chr >= '0' && chr <= '9') {
          current.offset++;
        } else {
          break;
        }
      }
    }
    auto &chr = source[current.offset];
    if (chr == 'e' || chr == 'E') {
      current.offset++;
      auto &chr = source[current.offset];
      if (chr == '+' || chr == '-') {
        current.offset++;
      }
      for (;;) {
        auto &chr = source[current.offset];
        if (chr >= '0' && chr <= '9') {
          current.offset++;
        } else {
          break;
        }
      }
    }
    current.offset--;
    current.column += current.offset - position.offset;
    common::AutoPtr<Token> token = new Token;
    current.offset++;
    current.column++;
    token->location = getLocation(source, position, current);
    position = current;
    return token;
  }
  return nullptr;
}

common::AutoPtr<JSParser::Token>
JSParser::readBigIntToken(uint32_t filename, const std::wstring &source,
                          Position &position) {

  auto current = position;
  auto &chr = source[current.offset];
  if ((chr >= '0' && chr <= '9')) {
    if (chr == '0' && (source[current.offset + 1] == 'x' ||
                       source[current.offset + 1] == 'X')) {
      current.offset += 2;
      for (;;) {
        auto &chr = source[current.offset];
        if ((chr >= '0' && chr <= '9') || (chr >= 'a' && chr <= 'f') ||
            (chr >= 'A' && chr <= 'F')) {
          current.offset++;
        } else {
          break;
        }
      }
    } else if (chr == '0' && (source[current.offset + 1] == 'o' ||
                              source[current.offset + 1] == 'O')) {
      current.offset += 2;
      for (;;) {
        auto &chr = source[current.offset];
        if (chr >= '0' && chr <= '7') {
          current.offset++;
        } else {
          break;
        }
      }
    } else {
      for (;;) {
        auto &chr = source[current.offset];
        if (chr >= '0' && chr <= '9') {
          current.offset++;
        } else {
          break;
        }
      }
    }
    auto &chr = source[current.offset];
    if (chr == 'e' || chr == 'E') {
      current.offset++;
      for (;;) {
        auto &chr = source[current.offset];
        if (chr >= '0' && chr <= '9') {
          current.offset++;
        } else {
          break;
        }
      }
    }
    auto flag = source[current.offset];
    if (flag == 'n') {
      current.offset++;
    } else {
      return nullptr;
    }
    current.offset--;
    current.column += current.offset - position.offset;
    common::AutoPtr<Token> token = new Token;
    current.offset++;
    current.column++;
    token->location = getLocation(source, position, current);
    position = current;
    return token;
  }
  return nullptr;
}

common::AutoPtr<JSParser::Token>
JSParser::readRegexToken(uint32_t filename, const std::wstring &source,
                         Position &position) {
  static std::array regexFlags = {'d', 'g', 'i', 'm', 's', 'y'};
  auto current = position;
  auto &chr = source[current.offset];
  if (chr == '/') {
    current.offset++;
    auto group = false;
    for (;;) {
      auto &chr = source[current.offset];
      if (chr == '\\') {
        current.offset++;
        if (source[current.offset] == L'u') {
          current.offset += 5;
        } else {
          current.offset++;
        }
        continue;
      }
      if (chr == '[') {
        group = true;
      }
      if (chr == ']') {
        group = false;
      }
      if (chr == '/' && !group) {
        break;
      }
      if (chr == '\0' || std::find(LTC.begin(), LTC.end(), chr) != LTC.end()) {
        current.column += current.offset - position.offset;
        throw error::JSSyntaxError(
            formatException(L"Incomplete regex", filename, source, current),
            {filename, current.line, current.column});
      }
      current.offset++;
    }
    auto &chr = source[current.offset + 1];
    if (std::find(regexFlags.begin(), regexFlags.end(), chr) !=
        regexFlags.end()) {
      current.offset++;
      for (;;) {
        auto &chr = source[current.offset];
        if (std::find(regexFlags.begin(), regexFlags.end(), chr) ==
            regexFlags.end()) {
          current.offset--;
          break;
        }
        current.offset++;
      }
    }
    current.column += current.offset - position.offset;
    common::AutoPtr token = new Token();
    current.offset++;
    current.column++;
    token->location = getLocation(source, position, current);
    position = current;
    return token;
  }
  return nullptr;
}

common::AutoPtr<JSParser::Token>
JSParser::readBooleanToken(uint32_t filename, const std::wstring &source,
                           Position &position) {
  static wchar_t t[] = L"true";
  static wchar_t f[] = L"false";
  auto current = position;
  auto &chr = source[position.offset];
  wchar_t *p = nullptr;
  if (chr == 't') {
    p = &t[0];
  } else if (chr == 'f') {
    p = &f[0];
  } else {
    return nullptr;
  }
  while (*p) {
    auto &chr = source[current.offset];
    if (*p != chr) {
      return nullptr;
    }
    p++;
    current.offset++;
  }
  common::AutoPtr token = new Token();
  current.offset--;
  current.column += current.offset - position.offset;
  current.offset++;
  token->location = getLocation(source, position, current);
  position = current;
  return token;
}

common::AutoPtr<JSParser::Token>
JSParser::readNullToken(uint32_t filename, const std::wstring &source,
                        Position &position) {
  static wchar_t s[] = L"null";
  auto current = position;
  auto &chr = source[position.offset];
  if (chr != 'n') {
    return nullptr;
  }
  wchar_t *p = &s[0];
  while (*p) {
    auto &chr = source[current.offset];
    if (*p != chr) {
      return nullptr;
    }
    p++;
    current.offset++;
  }
  common::AutoPtr token = new Token();
  current.offset--;
  current.column += current.offset - position.offset;
  current.offset++;
  token->location = getLocation(source, position, current);
  position = current;
  return token;
}

common::AutoPtr<JSParser::Token>
JSParser::readUndefinedToken(uint32_t filename, const std::wstring &source,
                             Position &position) {
  static wchar_t s[] = L"undefined";
  auto current = position;
  auto &chr = source[position.offset];
  if (chr != 'u') {
    return nullptr;
  }
  wchar_t *p = &s[0];
  while (*p) {
    auto &chr = source[current.offset];
    if (*p != chr) {
      return nullptr;
    }
    p++;
    current.offset++;
  }
  common::AutoPtr token = new Token();
  current.offset--;
  current.column += current.offset - position.offset;
  current.offset++;
  token->location = getLocation(source, position, current);
  position = current;
  return token;
}
common::AutoPtr<JSParser::Token>
JSParser::readKeywordToken(uint32_t filename, const std::wstring &source,
                           Position &position) {
  auto current = position;
  auto &chr = source[current.offset];
  if ((chr >= 'a' && chr <= 'z')) {
    current.offset++;
    for (;;) {
      auto &chr = source[current.offset];
      if (chr >= 'a' && chr <= 'z') {
        current.offset++;
      } else {
        current.offset--;
        break;
      }
    }
    current.column += current.offset - position.offset;
    common::AutoPtr token = new Token();
    current.offset++;
    current.column++;
    token->location = getLocation(source, position, current);
    for (auto &keyword : KEYWORLDS) {
      if (token->location.isEqual(source, keyword)) {
        position = current;
        return token;
      }
    }
  }
  return nullptr;
}

common::AutoPtr<JSParser::Token>
JSParser::readIdentifierToken(uint32_t filename, const std::wstring &source,
                              Position &position, bool allowKeyword) {
  // TODO: unicode support
  auto current = position;
  auto &chr = source[current.offset];
  if ((chr >= 'a' && chr <= 'z') || (chr >= 'A' && chr <= 'Z') || chr == '_' ||
      chr == '$') {
    current.offset++;
    for (;;) {
      auto &chr = source[current.offset];
      if ((chr >= 'a' && chr <= 'z') || (chr >= 'A' && chr <= 'Z') ||
          chr == '_' || chr == '$' || (chr >= '0' && chr <= '9')) {
        current.offset++;
      } else {
        current.offset--;
        break;
      }
    }
    current.column += current.offset - position.offset;
    common::AutoPtr token = new Token();
    current.offset++;
    current.column++;
    token->location = getLocation(source, position, current);
    if (!allowKeyword) {
      for (auto &keyword : KEYWORLDS) {
        if (token->location.isEqual(source, keyword)) {
          return nullptr;
        }
      }
    }
    position = current;
    return token;
  }
  return nullptr;
}

common::AutoPtr<JSParser::Token>
JSParser::readSymbolToken(uint32_t filename, const std::wstring &source,
                          Position &position) {
  static const std::vector<std::wstring> operators = {
      L">>>=",   L"...", L"<<=", L">>>", L"===", L"!==", L"**=", L">>=", L"&&=",
      LR"(??=)", L"**",  L"==",  L"!=",  L"<<",  L">>",  L"<=",  L">=",  L"&&",
      L"||",     L"??",  L"++",  L"--",  L"+=",  L"-=",  L"*=",  L"/=",  L"%=",
      L"||=",    L"&=",  L"^=",  L"|=",  L"=>",  L"?.",  L"=",   L"*",   L"/",
      L"%",      L"+",   L"-",   L"<",   L">",   L"&",   L"^",   L"|",   L",",
      L"!",      L"~",   L"(",   L")",   L"[",   L"]",   L"{",   L"}",   L"@",
      L"#",      L".",   L"?",   L":",   L";",
  };
  auto current = position;
  for (auto &opt : operators) {
    bool found = true;
    auto offset = current.offset;
    for (auto &ch : opt) {
      auto &chr = source[offset];
      if (chr != ch) {
        found = false;
        break;
      }
      offset++;
    }
    offset--;
    if (found) {
      current.offset = offset;
      current.column += current.offset - position.offset;
      common::AutoPtr token = new Token();
      current.offset++;
      current.column++;
      token->location = getLocation(source, position, current);
      position = current;
      return token;
    }
  }
  return nullptr;
}

common::AutoPtr<JSParser::Token>
JSParser::readTemplateToken(uint32_t filename, const std::wstring &source,
                            Position &position) {
  auto current = position;
  auto &chr = source[current.offset];
  if (chr == '`') {
    current.offset++;
    int mask = 0;
    for (;;) {
      auto chr = source[current.offset];
      if (chr == '\\') {
        mask++;
      }
      if (chr == '`' && mask % 2 == 0) {
        break;
      }
      if (mask % 2 == 0 && chr == '{' && source[current.offset - 1] == '$') {
        return nullptr;
      }
      if (chr == '\0') {
        throw error::JSSyntaxError(
            formatException(L"Incomplete string", filename, source, current),
            {filename, current.line, current.column});
      }
      if (chr == '\r') {
        current.offset++;
        chr = source[current.offset];
        if (chr == '\n') {
          current.offset++;
        }
        current.line++;
        current.column = 0;
      } else if (chr == '\n') {
        current.offset++;
        current.line++;
        current.column = 0;
      } else {
        current.offset++;
        current.column++;
      }
    }
    common::AutoPtr token = new Token();
    current.offset++;
    current.column++;
    token->location = getLocation(source, position, current);
    position = current;
    return token;
  }
  return nullptr;
}

common::AutoPtr<JSParser::Token>
JSParser::readTemplateStartToken(uint32_t filename, const std::wstring &source,
                                 Position &position) {
  auto current = position;
  auto &chr = source[current.offset];
  if (chr == '`') {
    current.offset++;
    int mask = 0;
    for (;;) {
      auto chr = source[current.offset];
      if (chr == '\\') {
        mask++;
      }
      if (mask % 2 == 0 && chr == '{' && source[current.offset - 1] == '$') {
        break;
      }
      if (mask % 2 == 0 && chr == '`') {
        return nullptr;
      }
      if (chr == '\0') {
        throw error::JSSyntaxError(
            formatException(L"Incomplete string", filename, source, current),
            {filename, current.line, current.column});
      }
      if (chr == '\r') {
        current.offset++;
        chr = source[current.offset];
        if (chr == '\n') {
          current.offset++;
        }
        current.line++;
        current.column = 0;
      } else if (chr == '\n') {
        current.offset++;
        current.line++;
        current.column = 0;
      } else {
        current.offset++;
        current.column++;
      }
    }
    common::AutoPtr token = new Token();
    current.offset++;
    current.column++;
    token->location = getLocation(source, position, current);
    position = current;
    return token;
  }
  return nullptr;
}

common::AutoPtr<JSParser::Token> JSParser::readTemplatePatternToken(
    uint32_t filename, const std::wstring &source, Position &position) {

  auto current = position;
  auto &chr = source[current.offset];
  if (chr == '}') {
    current.offset++;
    int mask = 0;
    for (;;) {
      auto chr = source[current.offset];
      if (chr == '\\') {
        mask++;
      }
      if (mask % 2 == 0 && chr == '{' && source[current.offset - 1] == '$') {
        break;
      }
      if (mask % 2 == 0 && chr == '`') {
        return nullptr;
      }
      if (chr == '\0') {
        throw error::JSSyntaxError(
            formatException(L"Incomplete string", filename, source, current),
            {filename, current.line, current.column});
      }
      if (chr == '\r') {
        current.offset++;
        chr = source[current.offset];
        if (chr == '\n') {
          current.offset++;
        }
        current.line++;
        current.column = 0;
      } else if (chr == '\n') {
        current.offset++;
        current.line++;
        current.column = 0;
      } else {
        current.offset++;
        current.column++;
      }
    }
    common::AutoPtr token = new Token();
    current.offset++;
    current.column++;
    token->location = getLocation(source, position, current);
    position = current;
    return token;
  }
  return nullptr;
}

common::AutoPtr<JSParser::Token>
JSParser::readTemplateEndToken(uint32_t filename, const std::wstring &source,
                               Position &position) {
  auto current = position;
  auto &chr = source[current.offset];
  if (chr == '}') {
    current.offset++;
    int mask = 0;
    for (;;) {
      auto chr = source[current.offset];
      if (chr == '\\') {
        mask++;
      }
      if (mask % 2 == 0 && chr == '`') {
        break;
      }
      if (mask % 2 == 0 && chr == '{' && source[current.offset - 1] == '$') {
        return nullptr;
      }
      if (chr == '\0') {
        throw error::JSSyntaxError(
            formatException(L"Incomplete string", filename, source, current),
            {filename, current.line, current.column});
      }
      if (chr == '\r') {
        current.offset++;
        chr = source[current.offset];
        if (chr == '\n') {
          current.offset++;
        }
        current.line++;
        current.column = 0;
      } else if (chr == '\n') {
        current.offset++;
        current.line++;
        current.column = 0;
      } else {
        current.offset++;
        current.column++;
      }
    }
    common::AutoPtr token = new Token();
    current.offset++;
    current.column++;
    token->location = getLocation(source, position, current);
    position = current;
    return token;
  }
  return nullptr;
}

common::AutoPtr<JSParser::Node>
JSParser::readProgram(uint32_t filename, const std::wstring &source,
                      Position &position) {
  auto current = position;
  auto interpreter = readInterpreterDirective(filename, source, current);
  common::AutoPtr program = new JSParser::Program();
  program->type = NodeType::PROGRAM;
  program->interpreter = interpreter;
  program->scope = new Scope();
  program->scope->node = program.getRawPointer();
  _currentScope = program->scope.getRawPointer();
  if (interpreter != nullptr) {
    interpreter->addParent(program);
  }
  skipInvisible(filename, source, current);
  auto directive = readDirective(filename, source, current);
  while (directive != nullptr) {
    directive->addParent(program);
    program->directives.push_back(directive);
    skipInvisible(filename, source, current);
    directive = readDirective(filename, source, current);
  }
  skipNewLine(filename, source, current);
  while (source[current.offset] != '\0') {
    auto statement = readStatement(filename, source, current);
    if (!statement) {
      break;
    }
    statement->addParent(program);
    program->body.push_back(statement);
    skipNewLine(filename, source, current);
  }
  skipNewLine(filename, source, current);
  if (source[current.offset] != '\0') {
    throw error::JSSyntaxError(
        formatException(L"Unexcepted token", filename, source, current),
        {filename, current.line, current.column});
  }
  _currentScope = nullptr;
  program->location = getLocation(source, position, current);
  position = current;
  return program;
}

common::AutoPtr<JSParser::Node>
JSParser::readStatement(uint32_t filename, const std::wstring &source,
                        Position &position) {
  common::AutoPtr<Node> node = nullptr;
  auto current = position;
  if (!node) {
    node = readEmptyStatement(filename, source, current);
  }
  if (!node) {
    node = readLabelStatement(filename, source, current);
  }
  if (!node) {
    node = readBlockStatement(filename, source, current);
  }
  if (!node) {
    node = readImportDeclaration(filename, source, current);
  }
  if (!node) {
    node = readExportDeclaration(filename, source, current);
  }
  if (!node) {
    node = readDebuggerStatement(filename, source, current);
  }
  if (!node) {
    node = readWhileStatement(filename, source, current);
  }
  if (!node) {
    node = readDoWhileStatement(filename, source, current);
  }
  if (!node) {
    node = readYieldStatement(filename, source, current);
  }
  if (!node) {
    node = readReturnStatement(filename, source, current);
  }
  if (!node) {
    node = readBreakStatement(filename, source, current);
  }
  if (!node) {
    node = readContinueStatement(filename, source, current);
  }
  if (!node) {
    node = readThrowStatement(filename, source, current);
  }
  if (!node) {
    node = readIfStatement(filename, source, current);
  }
  if (!node) {
    node = readSwitchStatement(filename, source, current);
  }
  if (!node) {
    node = readTryStatement(filename, source, current);
  }
  if (!node) {
    node = readForOfStatement(filename, source, current);
  }

  if (!node) {
    node = readForInStatement(filename, source, current);
  }

  if (!node) {
    node = readForStatement(filename, source, current);
  }

  // if (!node) {
  //   node = readWithStatement(filename,source,position);
  // }
  if (node != nullptr) {
    position = current;
    return node;
  }
  common::AutoPtr<Node> check;
  if (!node) {
    node = readVariableDeclaration(filename, source, current);
  }
  if (node == nullptr) {
    node = readExpressionStatement(filename, source, current);
  }
  if (node != nullptr && node->type != NodeType::DECLARATION_CLASS &&
      node->type != NodeType::DECLARATION_FUNCTION) {
    bool isNewLine = false;
    auto backup = current;
    skipInvisible(filename, source, current, &isNewLine);
    if (!isNewLine && source[current.offset] != '\0') {
      auto token = readSymbolToken(filename, source, current);
      if (token == nullptr || (!token->location.isEqual(source, L";") &&
                               !token->location.isEqual(source, L"}"))) {
        throw error::JSSyntaxError(
            formatException(L"Unexcepted token", filename, source, current),
            {filename, current.line, current.column});
      } else {
        if (token != nullptr && !token->location.isEqual(source, L";")) {
          current = backup;
        }
      }
    }
  }
  position = current;
  return node;
}

common::AutoPtr<JSParser::Node>
JSParser::readDebuggerStatement(uint32_t filename, const std::wstring &source,
                                JSParser::Position &position) {
  auto current = position;
  skipInvisible(filename, source, current);
  auto identifier = readKeywordToken(filename, source, current);
  if (identifier != nullptr &&
      identifier->location.isEqual(source, L"debugger")) {
    common::AutoPtr node = new DebuggerStatement;
    node->location = getLocation(source, position, current);
    position = current;
    return node;
  }
  return nullptr;
}

common::AutoPtr<JSParser::Node>
JSParser::readWhileStatement(uint32_t filename, const std::wstring &source,
                             Position &position) {
  auto current = position;
  skipInvisible(filename, source, current);
  auto token = readKeywordToken(filename, source, current);
  if (token != nullptr && token->location.isEqual(source, L"while")) {
    common::AutoPtr node = new WhileStatement;
    skipInvisible(filename, source, current);
    token = readSymbolToken(filename, source, current);
    if (!token || !token->location.isEqual(source, L"(")) {
      throw error::JSSyntaxError(
          formatException(L"Unexcepted token", filename, source, current),
          {filename, current.line, current.column});
    }
    node->condition = readExpressions(filename, source, current);
    if (!node->condition) {
      throw error::JSSyntaxError(
          formatException(L"Unexcepted token", filename, source, current),
          {filename, current.line, current.column});
    }
    node->condition->addParent(node);
    skipInvisible(filename, source, current);
    token = readSymbolToken(filename, source, current);
    if (!token || !token->location.isEqual(source, L")")) {
      throw error::JSSyntaxError(
          formatException(L"Unexcepted token", filename, source, current),
          {filename, current.line, current.column});
    }
    node->body = readStatement(filename, source, current);
    if (!node->body) {
      throw error::JSSyntaxError(
          formatException(L"Unexcepted token", filename, source, current),
          {filename, current.line, current.column});
    }
    node->body->addParent(node);
    node->location = getLocation(source, position, current);
    position = current;
    return node;
  }
  return nullptr;
}

common::AutoPtr<JSParser::Node>
JSParser::readDoWhileStatement(uint32_t filename, const std::wstring &source,
                               Position &position) {
  auto current = position;
  skipInvisible(filename, source, current);
  auto token = readKeywordToken(filename, source, current);
  if (token != nullptr && token->location.isEqual(source, L"do")) {
    common::AutoPtr node = new DoWhileStatement;
    node->type = NodeType::STATEMENT_DO_WHILE;
    node->body = readStatement(filename, source, current);
    if (!node->body) {
      throw error::JSSyntaxError(
          formatException(L"Unexcepted token", filename, source, current),
          {filename, current.line, current.column});
    }
    node->body->addParent(node);
    skipInvisible(filename, source, current);
    token = readKeywordToken(filename, source, current);
    if (token == nullptr || !token->location.isEqual(source, L"while")) {
      throw error::JSSyntaxError(
          formatException(L"Unexcepted token", filename, source, current),
          {filename, current.line, current.column});
    }
    skipInvisible(filename, source, current);
    token = readSymbolToken(filename, source, current);
    if (!token || !token->location.isEqual(source, L"(")) {
      throw error::JSSyntaxError(
          formatException(L"Unexcepted token", filename, source, current),
          {filename, current.line, current.column});
    }
    node->condition = readExpressions(filename, source, current);
    if (!node->condition) {
      throw error::JSSyntaxError(
          formatException(L"Unexcepted token", filename, source, current),
          {filename, current.line, current.column});
    }
    node->condition->addParent(node);
    skipInvisible(filename, source, current);
    token = readSymbolToken(filename, source, current);
    if (!token || !token->location.isEqual(source, L")")) {
      throw error::JSSyntaxError(
          formatException(L"Unexcepted token", filename, source, current),
          {filename, current.line, current.column});
    }
    node->location = getLocation(source, position, current);
    position = current;
    return node;
  }
  return nullptr;
}

common::AutoPtr<JSParser::Node>
JSParser::readForStatement(uint32_t filename, const std::wstring &source,
                           Position &position) {
  auto current = position;
  skipInvisible(filename, source, current);
  auto token = readKeywordToken(filename, source, current);
  if (token != nullptr && token->location.isEqual(source, L"for")) {
    common::AutoPtr node = new ForStatement;

    auto parentScope = _currentScope;
    node->scope = new Scope(parentScope);
    node->scope->node = node.getRawPointer();
    _currentScope = node->scope.getRawPointer();

    node->type = NodeType::STATEMENT_FOR;
    skipInvisible(filename, source, current);
    token = readSymbolToken(filename, source, current);
    if (!token || !token->location.isEqual(source, L"(")) {
      throw error::JSSyntaxError(
          formatException(L"Unexcepted token", filename, source, current),
          {filename, current.line, current.column});
    }
    node->init = readVariableDeclaration(filename, source, current);
    if (!node->init) {
      node->init = readExpressions(filename, source, current);
    }
    skipInvisible(filename, source, current);
    token = readSymbolToken(filename, source, current);
    if (!token || !token->location.isEqual(source, L";")) {
      throw error::JSSyntaxError(
          formatException(L"Unexcepted token", filename, source, current),
          {filename, current.line, current.column});
    }
    if (node->init != nullptr) {
      node->init->addParent(node);
    }
    node->condition = readExpressions(filename, source, current);
    skipInvisible(filename, source, current);
    token = readSymbolToken(filename, source, current);
    if (!token || !token->location.isEqual(source, L";")) {
      throw error::JSSyntaxError(
          formatException(L"Unexcepted token", filename, source, current),
          {filename, current.line, current.column});
    }
    if (node->condition != nullptr) {
      node->condition->addParent(node);
    }
    node->update = readExpressions(filename, source, current);
    skipInvisible(filename, source, current);
    token = readSymbolToken(filename, source, current);
    if (!token || !token->location.isEqual(source, L")")) {
      throw error::JSSyntaxError(
          formatException(L"Unexcepted token", filename, source, current),
          {filename, current.line, current.column});
    }
    if (node->update != nullptr) {
      node->update->addParent(node);
    }
    node->body = readStatement(filename, source, current);
    if (!node->body) {
      throw error::JSSyntaxError(
          formatException(L"Unexcepted token", filename, source, current),
          {filename, current.line, current.column});
    }
    if (node->body != nullptr) {
      node->body->addParent(node);
    }
    _currentScope = parentScope;
    node->location = getLocation(source, position, current);
    position = current;
    return node;
  }
  return nullptr;
}

common::AutoPtr<JSParser::Node>
JSParser::readForInStatement(uint32_t filename, const std::wstring &source,
                             Position &position) {
  auto current = position;
  skipInvisible(filename, source, current);
  auto token = readKeywordToken(filename, source, current);
  if (token != nullptr && token->location.isEqual(source, L"for")) {
    common::AutoPtr node = new ForInStatement;
    node->type = NodeType::STATEMENT_FOR_IN;
    skipInvisible(filename, source, current);
    token = readSymbolToken(filename, source, current);
    if (!token || !token->location.isEqual(source, L"(")) {
      throw error::JSSyntaxError(
          formatException(L"Unexcepted token", filename, source, current),
          {filename, current.line, current.column});
    }
    auto backup = current;
    skipInvisible(filename, source, current);
    token = readKeywordToken(filename, source, current);
    Declaration::TYPE type = Declaration::TYPE::UNINITIALIZED;
    bool isConst = false;
    if (token != nullptr) {
      if (token->location.isEqual(source, L"const")) {
        node->kind = DeclarationKind::CONST;
        isConst = true;
      } else if (token->location.isEqual(source, L"let")) {
        node->kind = DeclarationKind::LET;
      } else if (token->location.isEqual(source, L"var")) {
        node->kind = DeclarationKind::VAR;
        type = Declaration::TYPE::UNDEFINED;
      } else {
        node->kind = DeclarationKind::UNKNOWN;
        current = backup;
      }
    }
    node->declaration = readIdentifierLiteral(filename, source, current);
    if (!node->declaration) {
      node->declaration = readObjectPattern(filename, source, current);
    }
    if (!node->declaration) {
      node->declaration = readArrayDeclaration(filename, source, current);
    }
    if (node->declaration != nullptr) {
      node->declaration->addParent(node);
      skipInvisible(filename, source, current);
      token = readKeywordToken(filename, source, current);
      if (token != nullptr && token->location.isEqual(source, L"in")) {

        auto parentScope = _currentScope;
        node->scope = new Scope(parentScope);
        node->scope->node = node.getRawPointer();
        _currentScope = node->scope.getRawPointer();

        if (node->kind != DeclarationKind::UNKNOWN) {
          declareVariable(node, node->declaration, type, isConst);
        }

        node->expression = readExpressions(filename, source, current);
        if (!node->expression) {
          throw error::JSSyntaxError(
              formatException(L"Unexcepted token", filename, source, current),
              {filename, current.line, current.column});
        }
        node->expression->addParent(node);
        skipInvisible(filename, source, current);
        token = readSymbolToken(filename, source, current);
        if (!token || !token->location.isEqual(source, L")")) {
          throw error::JSSyntaxError(
              formatException(L"Unexcepted token", filename, source, current),
              {filename, current.line, current.column});
        }
        node->body = readStatement(filename, source, current);
        if (!node->body) {
          throw error::JSSyntaxError(
              formatException(L"Unexcepted token", filename, source, current),
              {filename, current.line, current.column});
        }
        node->body->addParent(node);
        _currentScope = parentScope;

        node->location = getLocation(source, position, current);
        position = current;
        return node;
      }
    }
  }
  return nullptr;
}

common::AutoPtr<JSParser::Node>
JSParser::readForOfStatement(uint32_t filename, const std::wstring &source,
                             Position &position) {
  auto current = position;
  skipInvisible(filename, source, current);
  auto token = readKeywordToken(filename, source, current);
  if (token != nullptr && token->location.isEqual(source, L"for")) {
    common::AutoPtr<ForOfStatement> node;
    auto backup = current;
    skipInvisible(filename, source, current);
    token = readKeywordToken(filename, source, current);
    if (token != nullptr && token->location.isEqual(source, L"await")) {
      node = new ForAwaitOfStatement;
      node->type = NodeType::STATEMENT_FOR_AWAIT_OF;
    } else {
      node = new ForOfStatement;
      node->type = NodeType::STATEMENT_FOR_OF;
      current = backup;
    }
    skipInvisible(filename, source, current);
    token = readSymbolToken(filename, source, current);
    if (!token || !token->location.isEqual(source, L"(")) {
      throw error::JSSyntaxError(
          formatException(L"Unexcepted token", filename, source, current),
          {filename, current.line, current.column});
    }
    backup = current;
    skipInvisible(filename, source, current);
    token = readKeywordToken(filename, source, current);
    Declaration::TYPE type = Declaration::TYPE::UNINITIALIZED;
    bool isConst = false;
    if (token != nullptr) {
      if (token->location.isEqual(source, L"const")) {
        node->kind = DeclarationKind::CONST;
        isConst = true;
      } else if (token->location.isEqual(source, L"let")) {
        node->kind = DeclarationKind::LET;
      } else if (token->location.isEqual(source, L"var")) {
        node->kind = DeclarationKind::VAR;
        type = Declaration::TYPE::UNDEFINED;
      } else {
        node->kind = DeclarationKind::UNKNOWN;
        current = backup;
      }
    }
    node->declaration = readIdentifierLiteral(filename, source, current);
    if (!node->declaration) {
      node->declaration = readObjectPattern(filename, source, current);
    }
    if (!node->declaration) {
      node->declaration = readArrayDeclaration(filename, source, current);
    }
    if (node->declaration != nullptr) {
      node->declaration->addParent(node);
      skipInvisible(filename, source, current);
      token = readIdentifierToken(filename, source, current);
      if (token != nullptr && token->location.isEqual(source, L"of")) {

        auto parentScope = _currentScope;
        node->scope = new Scope(parentScope);
        node->scope->node = node.getRawPointer();
        _currentScope = node->scope.getRawPointer();
        if (node->kind != DeclarationKind::UNKNOWN) {
          declareVariable(node, node->declaration, type, isConst);
        }
        node->expression = readExpressions(filename, source, current);
        if (!node->expression) {
          throw error::JSSyntaxError(
              formatException(L"Unexcepted token", filename, source, current),
              {filename, current.line, current.column});
        }
        node->expression->addParent(node);
        skipInvisible(filename, source, current);
        token = readSymbolToken(filename, source, current);
        if (!token || !token->location.isEqual(source, L")")) {
          throw error::JSSyntaxError(
              formatException(L"Unexcepted token", filename, source, current),
              {filename, current.line, current.column});
        }
        node->body = readStatement(filename, source, current);
        if (!node->body) {
          throw error::JSSyntaxError(
              formatException(L"Unexcepted token", filename, source, current),
              {filename, current.line, current.column});
        }
        node->body->addParent(node);
        _currentScope = parentScope;
        node->location = getLocation(source, position, current);
        position = current;
        return node;
      }
    }
  }
  return nullptr;
}

common::AutoPtr<JSParser::Node>
JSParser::readEmptyStatement(uint32_t filename, const std::wstring &source,
                             Position &position) {
  auto current = position;
  skipInvisible(filename, source, current);
  auto token = readSymbolToken(filename, source, current);
  if (token != nullptr && token->location.isEqual(source, L";")) {
    common::AutoPtr node = new EmptyStatement;
    node->type = NodeType::STATEMENT_EMPTY;
    node->location = getLocation(source, position, current);
    position = current;
    return node;
  }
  return nullptr;
}

common::AutoPtr<JSParser::Node>
JSParser::readYieldStatement(uint32_t filename, const std::wstring &source,
                             Position &position) {
  auto current = position;
  skipInvisible(filename, source, current);
  auto token = readIdentifierToken(filename, source, current);
  if (token != nullptr && token->location.isEqual(source, L"yield")) {
    common::AutoPtr node = new YieldStatement;
    auto isNewLine = false;
    skipInvisible(filename, source, current, &isNewLine);
    if (!isNewLine) {
      node->value = readExpressions(filename, source, current);
    }
    if (node->value != nullptr) {
      node->value->addParent(node);
    }
    node->location = getLocation(source, position, current);
    position = current;
    return node;
  }
  return nullptr;
}

common::AutoPtr<JSParser::Node>
JSParser::readReturnStatement(uint32_t filename, const std::wstring &source,
                              Position &position) {
  auto current = position;
  skipInvisible(filename, source, current);
  auto token = readKeywordToken(filename, source, current);
  if (token != nullptr && token->location.isEqual(source, L"return")) {
    common::AutoPtr node = new YieldStatement;
    auto isNewLine = false;
    skipInvisible(filename, source, current, &isNewLine);
    if (!isNewLine) {
      node->value = readExpressions(filename, source, current);
    }
    if (node->value != nullptr) {
      node->value->addParent(node);
    }
    node->location = getLocation(source, position, current);
    position = current;
    return node;
  }
  return nullptr;
}

common::AutoPtr<JSParser::Node>
JSParser::readThrowStatement(uint32_t filename, const std::wstring &source,
                             Position &position) {
  auto current = position;
  skipInvisible(filename, source, current);
  auto token = readKeywordToken(filename, source, current);
  if (token != nullptr && token->location.isEqual(source, L"throw")) {
    common::AutoPtr node = new ThrowStatement;
    auto isNewLine = false;
    skipInvisible(filename, source, current, &isNewLine);
    if (!isNewLine) {
      node->value = readExpressions(filename, source, current);
    }
    if (node->value != nullptr) {
      node->value->addParent(node);
    }
    node->location = getLocation(source, position, current);
    position = current;
    return node;
  }
  return nullptr;
}

common::AutoPtr<JSParser::Node>
JSParser::readBreakStatement(uint32_t filename, const std::wstring &source,
                             Position &position) {
  auto current = position;
  skipInvisible(filename, source, current);
  auto token = readKeywordToken(filename, source, current);
  if (token != nullptr && token->location.isEqual(source, L"break")) {
    common::AutoPtr node = new BreakStatement;
    auto isNewLine = false;
    skipInvisible(filename, source, current, &isNewLine);
    if (!isNewLine) {
      node->label = readIdentifierLiteral(filename, source, current);
    }
    if (node->label != nullptr) {
      node->label->addParent(node);
    }
    node->location = getLocation(source, position, current);
    position = current;
    return node;
  }
  return nullptr;
}

common::AutoPtr<JSParser::Node>
JSParser::readContinueStatement(uint32_t filename, const std::wstring &source,
                                Position &position) {
  auto current = position;
  skipInvisible(filename, source, current);
  auto token = readKeywordToken(filename, source, current);
  if (token != nullptr && token->location.isEqual(source, L"continue")) {
    common::AutoPtr node = new BreakStatement;
    auto isNewLine = false;
    skipInvisible(filename, source, current, &isNewLine);
    if (!isNewLine) {
      node->label = readIdentifierLiteral(filename, source, current);
    }
    if (node->label != nullptr) {
      node->label->addParent(node);
    }
    node->location = getLocation(source, position, current);
    position = current;
    return node;
  }
  return nullptr;
}

common::AutoPtr<JSParser::Node>
JSParser::readLabelStatement(uint32_t filename, const std::wstring &source,
                             Position &position) {
  auto current = position;
  skipInvisible(filename, source, current);
  auto label = readIdentifierLiteral(filename, source, current);
  if (label != nullptr) {
    auto token = readSymbolToken(filename, source, current);
    if (token != nullptr && token->location.isEqual(source, L":")) {
      common::AutoPtr node = new LabelStatement;
      node->type = NodeType::STATEMENT_LABEL;
      node->label = label;
      label->addParent(node);
      node->statement = readStatement(filename, source, current);
      if (!node->statement) {
        throw error::JSSyntaxError(
            formatException(L"Unexcepted token", filename, source, current),
            {filename, current.line, current.column});
      }
      node->statement->addParent(node);
      node->location = getLocation(source, position, current);
      position = current;
      return node;
    }
  }
  return nullptr;
}

common::AutoPtr<JSParser::Node>
JSParser::readIfStatement(uint32_t filename, const std::wstring &source,
                          Position &position) {
  auto current = position;
  skipInvisible(filename, source, current);
  auto token = readKeywordToken(filename, source, current);
  if (token != nullptr && token->location.isEqual(source, L"if")) {
    common::AutoPtr node = new IfStatement;
    skipInvisible(filename, source, current);
    token = readSymbolToken(filename, source, current);
    if (!token || !token->location.isEqual(source, L"(")) {
      throw error::JSSyntaxError(
          formatException(L"Unexcepted token", filename, source, current),
          {filename, current.line, current.column});
    }
    node->condition = readExpressions(filename, source, current);
    if (!node->condition) {
      throw error::JSSyntaxError(
          formatException(L"Unexcepted token", filename, source, current),
          {filename, current.line, current.column});
    }
    node->condition->addParent(node);
    skipInvisible(filename, source, current);
    token = readSymbolToken(filename, source, current);
    if (!token || !token->location.isEqual(source, L")")) {
      throw error::JSSyntaxError(
          formatException(L"Unexcepted token", filename, source, current),
          {filename, current.line, current.column});
    }
    node->alternate = readStatement(filename, source, current);
    node->alternate->addParent(node);
    if (!node->alternate) {
      throw error::JSSyntaxError(
          formatException(L"Unexcepted token", filename, source, current),
          {filename, current.line, current.column});
    }
    auto backup = current;
    skipInvisible(filename, source, current);
    token = readKeywordToken(filename, source, current);
    if (token != nullptr && token->location.isEqual(source, L"else")) {
      node->consequent = readStatement(filename, source, current);
      if (!node->consequent) {
        throw error::JSSyntaxError(
            formatException(L"Unexcepted token", filename, source, current),
            {filename, current.line, current.column});
      }
      node->consequent->addParent(node);
    } else {
      current = backup;
    }
    node->location = getLocation(source, position, current);
    position = current;
    return node;
  }
  return nullptr;
}

common::AutoPtr<JSParser::Node>
JSParser::readSwitchStatement(uint32_t filename, const std::wstring &source,
                              Position &position) {
  auto current = position;
  skipInvisible(filename, source, current);
  auto token = readKeywordToken(filename, source, current);
  if (token != nullptr && token->location.isEqual(source, L"switch")) {
    common::AutoPtr node = new SwitchStatement;
    skipInvisible(filename, source, current);
    token = readSymbolToken(filename, source, current);
    if (!token || !token->location.isEqual(source, L"(")) {
      throw error::JSSyntaxError(
          formatException(L"Unexcepted token", filename, source, current),
          {filename, current.line, current.column});
    }
    node->expression = readExpressions(filename, source, current);
    if (!node->expression) {
      throw error::JSSyntaxError(
          formatException(L"Unexcepted token", filename, source, current),
          {filename, current.line, current.column});
    }
    node->expression->addParent(node);
    skipInvisible(filename, source, current);
    token = readSymbolToken(filename, source, current);
    if (!token || !token->location.isEqual(source, L")")) {
      throw error::JSSyntaxError(
          formatException(L"Unexcepted token", filename, source, current),
          {filename, current.line, current.column});
    }
    skipInvisible(filename, source, current);
    token = readSymbolToken(filename, source, current);
    if (!token || !token->location.isEqual(source, L"{")) {
      throw error::JSSyntaxError(
          formatException(L"Unexcepted token", filename, source, current),
          {filename, current.line, current.column});
    }
    auto parentScope = _currentScope;
    node->scope = new Scope(parentScope);
    node->scope->node = node.getRawPointer();
    _currentScope = node->scope.getRawPointer();

    auto case_ = readSwitchCaseStatement(filename, source, current);
    while (case_ != nullptr) {
      case_->addParent(node);
      node->cases.push_back(case_);
      case_ = readSwitchCaseStatement(filename, source, current);
    }
    skipInvisible(filename, source, current);
    token = readSymbolToken(filename, source, current);
    if (!token || !token->location.isEqual(source, L"}")) {
      throw error::JSSyntaxError(
          formatException(L"Unexcepted token", filename, source, current),
          {filename, current.line, current.column});
    }
    _currentScope = parentScope;

    node->location = getLocation(source, position, current);
    position = current;
    return node;
  }
  return nullptr;
}

common::AutoPtr<JSParser::Node>
JSParser::readSwitchCaseStatement(uint32_t filename, const std::wstring &source,
                                  Position &position) {
  auto current = position;
  skipInvisible(filename, source, current);
  auto token = readKeywordToken(filename, source, current);
  if (token != nullptr) {
    common::AutoPtr node = new SwitchCaseStatement;
    if (token->location.isEqual(source, L"case")) {
      skipInvisible(filename, source, current);
      node->match = readExpressions(filename, source, current);
      if (!node->match) {
        throw error::JSSyntaxError(
            formatException(L"Unexcepted token", filename, source, current),
            {filename, current.line, current.column});
      }
      node->match->addParent(node);
    } else if (!token->location.isEqual(source, L"default")) {
      return nullptr;
    }
    skipInvisible(filename, source, current);
    token = readSymbolToken(filename, source, current);
    if (!token || !token->location.isEqual(source, L":")) {
      throw error::JSSyntaxError(
          formatException(L"Unexcepted token", filename, source, current),
          {filename, current.line, current.column});
    }
    auto backup = current;
    skipInvisible(filename, source, current);
    auto token = readKeywordToken(filename, source, current);
    if (token == nullptr || (!token->location.isEqual(source, L"case") &&
                             !token->location.isEqual(source, L"default"))) {
      current = backup;
      skipNewLine(filename, source, current);
      auto statement = readStatement(filename, source, current);
      while (statement != nullptr) {
        statement->addParent(node);
        node->statements.push_back(statement);
        skipNewLine(filename, source, current);
        backup = current;
        auto token = readKeywordToken(filename, source, current);
        if (token != nullptr && (token->location.isEqual(source, L"case") ||
                                 token->location.isEqual(source, L"default"))) {
          current = backup;
          break;
        } else {
          current = backup;
        }
        skipNewLine(filename, source, current);
        statement = readStatement(filename, source, current);
      }
    } else {
      current = backup;
    }
    node->location = getLocation(source, position, current);
    position = current;
    return node;
  }
  return nullptr;
}

common::AutoPtr<JSParser::Node>
JSParser::readTryStatement(uint32_t filename, const std::wstring &source,
                           Position &position) {

  auto current = position;
  skipInvisible(filename, source, current);
  auto token = readKeywordToken(filename, source, current);
  if (token != nullptr && token->location.isEqual(source, L"try")) {
    common::AutoPtr node = new TryStatement;
    node->type = NodeType::STATEMENT_TRY;
    node->try_ = readBlockStatement(filename, source, current);
    if (!node->try_) {
      throw error::JSSyntaxError(
          formatException(L"Unexcepted token", filename, source, current),
          {filename, current.line, current.column});
    }
    node->try_->addParent(node);
    node->catch_ = readTryCatchStatement(filename, source, current);
    auto backup = current;
    skipInvisible(filename, source, current);
    token = readKeywordToken(filename, source, current);
    if (token != nullptr && token->location.isEqual(source, L"finally")) {
      node->finally = readBlockStatement(filename, source, current);
      if (!node->finally) {
        throw error::JSSyntaxError(
            formatException(L"Unexcepted token", filename, source, current),
            {filename, current.line, current.column});
      }
    } else {
      current = backup;
    }
    if (!node->catch_ && !node->finally) {
      throw error::JSSyntaxError(
          formatException(L"Unexcepted token", filename, source, current),
          {filename, current.line, current.column});
    }
    if (node->catch_ != nullptr) {
      node->catch_->addParent(node);
    }
    if (node->finally != nullptr) {
      node->finally->addParent(node);
    }
    node->location = getLocation(source, position, current);
    position = current;
    return node;
  }
  return nullptr;
}

common::AutoPtr<JSParser::Node>
JSParser::readTryCatchStatement(uint32_t filename, const std::wstring &source,
                                Position &position) {
  auto current = position;
  skipInvisible(filename, source, current);
  auto token = readKeywordToken(filename, source, current);
  if (token != nullptr && token->location.isEqual(source, L"catch")) {
    common::AutoPtr node = new TryCatchStatement;

    auto parentScope = _currentScope;
    node->scope = new Scope(parentScope);
    node->scope->node = node.getRawPointer();
    _currentScope = node->scope.getRawPointer();

    auto backup = current;
    skipInvisible(filename, source, current);
    token = readSymbolToken(filename, source, current);
    if (!token) {
      throw error::JSSyntaxError(
          formatException(L"Unexcepted token", filename, source, current),
          {filename, current.line, current.column});
    }
    if (token->location.isEqual(source, L"(")) {
      node->binding = readIdentifierLiteral(filename, source, current);

      if (!node->binding) {
        node->binding = readObjectPattern(filename, source, current);
      }
      if (!node->binding) {
        node->binding = readArrayPattern(filename, source, current);
      }
      if (!node->binding) {
        throw error::JSSyntaxError(
            formatException(L"Unexcepted token", filename, source, current),
            {filename, current.line, current.column});
      }
      node->binding->addParent(node);
      skipInvisible(filename, source, current);
      token = readSymbolToken(filename, source, current);
      if (!token || !token->location.isEqual(source, L")")) {
        throw error::JSSyntaxError(
            formatException(L"Unexcepted token", filename, source, current),
            {filename, current.line, current.column});
      }
      declareVariable(node, node->binding, Declaration::TYPE::UNINITIALIZED,
                      false);
    } else {
      current = backup;
    }
    node->statement = readBlockStatement(filename, source, current);
    if (!node->statement) {
      throw error::JSSyntaxError(
          formatException(L"Unexcepted token", filename, source, current),
          {filename, current.line, current.column});
    }
    node->statement->addParent(node);
    _currentScope = parentScope;
    node->location = getLocation(source, position, current);
    position = current;
    return node;
  }
  return nullptr;
}

common::AutoPtr<JSParser::Node>
JSParser::readExpressionStatement(uint32_t filename, const std::wstring &source,
                                  Position &position) {
  return readExpressions(filename, source, position);
}

common::AutoPtr<JSParser::Node> JSParser::readValue(uint32_t filename,
                                                    const std::wstring &source,
                                                    Position &position) {
  auto current = position;
  skipInvisible(filename, source, current);
  auto node = readGroupExpression(filename, source, current);
  if (!node) {
    node = readBooleanLiteral(filename, source, current);
  }
  if (!node) {
    node = readStringLiteral(filename, source, current);
  }
  if (!node) {
    node = readBigIntLiteral(filename, source, current);
  }
  if (!node) {
    node = readNumberLiteral(filename, source, current);
  }
  if (!node) {
    node = readNullLiteral(filename, source, current);
  }
  if (!node) {
    node = readUndefinedLiteral(filename, source, current);
  }
  if (!node) {
    node = readRegexLiteral(filename, source, current);
  }
  if (!node) {
    node = readTemplateLiteral(filename, source, current);
  }
  if (!node) {
    node = readNewExpression(filename, source, current);
  }
  if (!node) {
    node = readFunctionDeclaration(filename, source, current);
  }
  if (!node) {
    node = readClassDeclaration(filename, source, current);
  }
  if (!node) {
    node = readIdentifierLiteral(filename, source, current);
  }
  if (!node) {
    node = readArrayDeclaration(filename, source, current);
  }
  if (!node) {
    node = readObjectDeclaration(filename, source, current);
  }
  if (node != nullptr) {
    for (;;) {
      auto expr = readMemberExpression(filename, source, current);
      if (!expr) {
        expr = readCallExpression(filename, source, current);
      }
      if (expr != nullptr) {
        expr.cast<BinaryExpression>()->left = node;
        node->addParent(expr);
        node = expr;
        node->location = getLocation(source, position, current);
      } else {
        break;
      }
    }
    node->location = getLocation(source, position, current);
    position = current;
    return node;
  }
  return nullptr;
}

common::AutoPtr<JSParser::Node> JSParser::readRValue(uint32_t filename,
                                                     const std::wstring &source,
                                                     Position &position,
                                                     int level) {
  auto current = position;
  skipInvisible(filename, source, current);
  common::AutoPtr<Node> node = nullptr;
  if (!node) {
    node = readUnaryExpression(filename, source, current);
  }
  if (!node) {
    node = readVoidExpression(filename, source, current);
  }
  if (!node) {
    node = readDeleteExpression(filename, source, current);
  }
  if (!node) {
    node = readTypeofExpression(filename, source, current);
  }
  if (!node) {
    node = readAwaitExpression(filename, source, current);
  }
  if (!node) {
    node = readValue(filename, source, current);
  }
  if (node != nullptr) {
    for (;;) {
      if (level < 3) {
        break;
      }
      auto expr = readUpdateExpression(filename, source, current);
      if (level >= 17 && expr == nullptr) {
        expr = readAssigmentExpression(filename, source, current);
      }
      if (level >= 9 && expr == nullptr) {
        expr = readInstanceOfExpression(filename, source, current);
      }
      if (level >= 9 && expr == nullptr) {
        expr = readInExpression(filename, source, current);
      }
      if (level >= 4 && expr == nullptr) {
        auto tmp = current;
        expr = readBinaryExpression(filename, source, current);
        if (expr != nullptr && expr->level > level) {
          current = tmp;
          expr = nullptr;
        }
      }
      if (level >= 16 && expr == nullptr) {
        expr = readConditionExpression(filename, source, current);
      }
      if (expr != nullptr) {
        expr.cast<BinaryExpression>()->left = node;
        node->addParent(expr);
        node = expr;
        node->location = getLocation(source, position, current);
      } else {
        break;
      }
    }
    node->location = getLocation(source, position, current);
    position = current;
    return node;
  }
  return nullptr;
}

common::AutoPtr<JSParser::Node>
JSParser::readDecorator(uint32_t filename, const std::wstring &source,
                        Position &position) {
  auto current = position;
  skipInvisible(filename, source, current);
  auto token = readSymbolToken(filename, source, current);
  if (token != nullptr && token->location.isEqual(source, L"@")) {
    common::AutoPtr node = new Decorator;
    node->type = NodeType::DECORATOR;
    node->expression = readValue(filename, source, current);
    if (!node->expression) {
      throw error::JSSyntaxError(
          formatException(L"Unexcepted token", filename, source, current),
          {filename, current.line, current.column});
    }
    node->expression->addParent(node);
    node->location = getLocation(source, position, current);
    position = current;
    return node;
  }
  return nullptr;
}

common::AutoPtr<JSParser::Node>
JSParser::readExpression(uint32_t filename, const std::wstring &source,
                         Position &position) {
  return readRValue(filename, source, position, 999);
}

common::AutoPtr<JSParser::Node>
JSParser::readExpressions(uint32_t filename, const std::wstring &source,
                          Position &position) {
  auto current = position;
  skipInvisible(filename, source, current);
  auto expr = readExpression(filename, source, current);
  if (!expr) {
    return nullptr;
  }
  auto next = current;
  skipInvisible(filename, source, current);
  auto token = readSymbolToken(filename, source, current);
  if (token != nullptr && token->location.isEqual(source, L",")) {
    auto right = readExpressions(filename, source, current);
    if (!right) {
      throw error::JSSyntaxError(
          formatException(L"Unexcepted token", filename, source, current),
          {filename, current.line, current.column});
    }
    common::AutoPtr node = new BinaryExpression;
    node->opt = L",";
    node->level = 18;
    node->left = expr;
    node->right = right;
    expr->addParent(node);
    right->addParent(node);
    node->location = getLocation(source, position, current);
    expr = node;
    next = current;
  }
  current = next;
  position = current;
  return expr;
}

common::AutoPtr<JSParser::Node>
JSParser::readBlockStatement(uint32_t filename, const std::wstring &source,
                             Position &position) {
  auto current = position;
  skipNewLine(filename, source, current);
  auto token = readSymbolToken(filename, source, current);
  if (token != nullptr && token->location.isEqual(source, L"{")) {
    common::AutoPtr node = new BlockStatement;
    auto parentScope = _currentScope;
    node->scope = new Scope(parentScope);
    node->scope->node = node.getRawPointer();
    _currentScope = node->scope.getRawPointer();
    skipNewLine(filename, source, current);
    auto statement = readStatement(filename, source, current);
    while (statement != nullptr) {
      statement->addParent(node);
      node->body.push_back(statement);
      skipNewLine(filename, source, current);
      statement = readStatement(filename, source, current);
    }
    skipNewLine(filename, source, current);
    token = readSymbolToken(filename, source, current);
    if (!token || !token->location.isEqual(source, L"}")) {
      throw error::JSSyntaxError(
          formatException(L"Unexcepted token", filename, source, current),
          {filename, current.line, current.column});
    }
    _currentScope = parentScope;
    node->location = getLocation(source, position, current);
    position = current;
    return node;
  }
  return nullptr;
}

common::AutoPtr<JSParser::Node> JSParser::readInterpreterDirective(
    uint32_t filename, const std::wstring &source, Position &position) {
  auto current = position;
  skipNewLine(filename, source, current);
  auto &chr = source[current.offset];
  if (chr == '#' && source[position.offset + 1] == '!') {
    for (;;) {
      auto &chr = source[current.offset];
      if (std::find(LTC.begin(), LTC.end(), chr) != LTC.end()) {
        break;
      }
      current.offset++;
    }
    current.column += current.offset - position.offset;
    common::AutoPtr node = new InterpreterDirective();
    node->location = getLocation(source, position, current);
    position = current;
    return node;
  }
  return nullptr;
}

common::AutoPtr<JSParser::Node>
JSParser::readDirective(uint32_t filename, const std::wstring &source,
                        Position &position) {
  auto current = position;
  skipNewLine(filename, source, current);
  auto value = readStringToken(filename, source, current);
  if (value != nullptr) {
    common::AutoPtr node = new Directive();
    node->location = value->location;
    auto backup = current;
    auto token = readSymbolToken(filename, source, current);
    if (!token || !token->location.isEqual(source, L";")) {
      current = backup;
    }
    position = current;
    return node;
  }
  return nullptr;
}

common::AutoPtr<JSParser::Node>
JSParser::readStringLiteral(uint32_t filename, const std::wstring &source,
                            Position &position) {
  auto current = position;
  skipInvisible(filename, source, current);
  auto token = readStringToken(filename, source, current);
  if (token != nullptr) {
    common::AutoPtr node = new StringLiteral;
    auto src = source.substr(token->location.start.offset,
                             token->location.end.offset -
                                 token->location.start.offset + 1);
    node->value = src.substr(1, src.length() - 2);
    node->location = token->location;
    position = current;
    return node;
  }
  return nullptr;
}

common::AutoPtr<JSParser::Node>
JSParser::readTemplateLiteral(uint32_t filename, const std::wstring &source,
                              Position &position) {
  auto current = position;
  skipInvisible(filename, source, current);
  auto tag = readIdentifierToken(filename, source, current);
  auto temp = readTemplateToken(filename, source, current);
  common::AutoPtr node = new TemplateLiteral;
  if (tag != nullptr) {
    node->tag = source.substr(tag->location.start.offset,
                              tag->location.end.offset -
                                  tag->location.start.offset + 1);
  }
  if (temp != nullptr) {
    auto src = source.substr(temp->location.start.offset,
                             temp->location.end.offset -
                                 temp->location.start.offset + 1);
    node->quasis.push_back(src.substr(1, src.length() - 2));
    node->location = getLocation(source, position, current);
    position = current;
    return node;
  }
  temp = readTemplateStartToken(filename, source, current);
  if (temp != nullptr) {
    auto src = temp->location.getSource(source);
    node->quasis.push_back(src.substr(1, src.length() - 3));
    for (;;) {
      auto exp = readExpressions(filename, source, current);
      if (!exp) {
        throw error::JSSyntaxError(
            formatException(L"Unexcepted token", filename, source, current),
            {filename, current.line, current.column});
      }
      exp->addParent(node);
      node->expressions.push_back(exp);
      auto part = readTemplatePatternToken(filename, source, current);
      if (part != nullptr) {
        auto src = part->location.getSource(source);
        node->quasis.push_back(src.substr(1, src.length() - 3));
      } else {
        part = readTemplateEndToken(filename, source, current);
        if (!part) {
          throw error::JSSyntaxError(
              formatException(L"Unexcepted token", filename, source, current),
              {filename, current.line, current.column});
        }
        auto src = part->location.getSource(source);
        node->quasis.push_back(src.substr(1, src.length() - 2));
        node->location = getLocation(source, position, current);
        position = current;
        return node;
      }
    }
  }
  return nullptr;
}

common::AutoPtr<JSParser::Node>
JSParser::readNumberLiteral(uint32_t filename, const std::wstring &src,
                            Position &position) {
  auto current = position;
  skipInvisible(filename, src, current);
  auto token = readNumberToken(filename, src, current);
  if (token != nullptr) {
    common::AutoPtr node = new NumberLiteral;
    auto source = token->location.getSource(src);
    if (source[0] == '0' && (source[1] == 'x' || source[1] == 'X')) {
      node->value = (double)std::stol(source, nullptr, 16);
    } else if (source[0] == '0' && (source[1] == 'o' && source[1] == 'O')) {
      node->value = (double)std::stol(source, nullptr, 8);
    } else {
      node->value = std::stold(source);
    }
    node->location = token->location;
    position = current;
    return node;
  }
  return nullptr;
}
common::AutoPtr<JSParser::Node>
JSParser::readBigIntLiteral(uint32_t filename, const std::wstring &source,
                            Position &position) {
  auto current = position;
  skipInvisible(filename, source, current);
  auto token = readBigIntToken(filename, source, current);
  if (token != nullptr) {
    common::AutoPtr node = new BigIntLiteral;
    node->location = token->location;
    node->value = source.substr(token->location.start.offset,
                                token->location.end.offset -
                                    token->location.start.offset);
    position = current;
    return node;
  }
  return nullptr;
}

common::AutoPtr<JSParser::Node>
JSParser::readRegexLiteral(uint32_t filename, const std::wstring &source,
                           Position &position) {
  auto current = position;
  skipInvisible(filename, source, current);
  auto token = readRegexToken(filename, source, current);
  if (token != nullptr) {
    common::AutoPtr node = new RegexLiteral;
    auto src = token->location.getSource(source);
    auto end = src.find_last_of('/');
    node->value = src.substr(1, end - 1);
    auto flags = src.substr(end + 1);
    for (auto &ch : flags) {
      if (ch == 'd')
        node->hasIndices = true;
      else if (ch == 'g')
        node->global = true;
      else if (ch == 'i')
        node->ignoreCase = true;
      else if (ch == 'm')
        node->multiline = true;
      else if (ch == 's')
        node->dotAll = true;
      else if (ch == 'y')
        node->sticky = true;
    }
    node->location = token->location;
    position = current;
    return node;
  }
  return nullptr;
}

common::AutoPtr<JSParser::Node>
JSParser::readBooleanLiteral(uint32_t filename, const std::wstring &source,
                             Position &position) {
  auto current = position;
  skipInvisible(filename, source, current);
  auto token = readBooleanToken(filename, source, current);
  if (!token) {
    return nullptr;
  }
  common::AutoPtr node = new BooleanLiteral();
  node->value = token->location.isEqual(source, L"true");
  node->location = token->location;
  position = current;
  return node;
}

common::AutoPtr<JSParser::Node>
JSParser::readUndefinedLiteral(uint32_t filename, const std::wstring &source,
                               Position &position) {
  auto current = position;
  skipInvisible(filename, source, current);
  auto token = readUndefinedToken(filename, source, current);
  if (!token) {
    return nullptr;
  }
  common::AutoPtr node = new UndefinedLiteral();
  node->location = token->location;
  position = current;
  return node;
}

common::AutoPtr<JSParser::Node>
JSParser::readNullLiteral(uint32_t filename, const std::wstring &source,
                          Position &position) {
  auto current = position;
  skipInvisible(filename, source, current);
  auto token = readNullToken(filename, source, current);
  if (!token) {
    return nullptr;
  }
  common::AutoPtr node = new NullLiteral();
  node->location = token->location;
  position = current;
  return node;
}

common::AutoPtr<JSParser::Node>
JSParser::readIdentifierLiteral(uint32_t filename, const std::wstring &source,
                                Position &position) {
  auto current = position;
  skipInvisible(filename, source, current);
  auto token = readIdentifierToken(filename, source, current);
  if (!token) {
    token = readKeywordToken(filename, source, current);
    if (token != nullptr) {
      if (token->location.isEqual(source, L"this")) {
        common::AutoPtr node = new IdentifierLiteral();
        node->type = NodeType::THIS;
        node->value = token->location.getSource(source);
        node->location = token->location;
        position = current;
        return node;
      }
      if (token->location.isEqual(source, L"super")) {
        common::AutoPtr node = new IdentifierLiteral();
        node->type = NodeType::SUPER;
        node->value = token->location.getSource(source);
        node->location = token->location;
        position = current;
        return node;
      }
    }
    return nullptr;
  }
  common::AutoPtr node = new IdentifierLiteral();
  node->value = token->location.getSource(source);
  node->location = token->location;
  position = current;
  return node;
}

common::AutoPtr<JSParser::Node>
JSParser::readMemberLiteral(uint32_t filename, const std::wstring &source,
                            Position &position) {
  auto current = position;
  skipInvisible(filename, source, current);
  auto token = readIdentifierToken(filename, source, current, true);
  if (token == nullptr) {
    return nullptr;
  }
  common::AutoPtr node = new IdentifierLiteral();
  node->value = token->location.getSource(source);
  node->location = token->location;
  position = current;
  return node;
}

common::AutoPtr<JSParser::Node>
JSParser::readPrivateName(uint32_t filename, const std::wstring &source,
                          Position &position) {
  auto current = position;
  skipInvisible(filename, source, current);
  auto token = readSymbolToken(filename, source, current);
  if (token != nullptr && token->location.isEqual(source, L"#")) {
    token = readIdentifierToken(filename, source, current);
    if (!token) {
      return nullptr;
    }
    common::AutoPtr node = new PrivateName();
    node->value = token->location.getSource(source);
    node->location = getLocation(source, position, current);
    position = current;
    return node;
  }
  return nullptr;
}

common::AutoPtr<JSParser::Node>
JSParser::readBinaryExpression(uint32_t filename, const std::wstring &source,
                               JSParser::Position &position) {
  const std::vector<std::vector<std::wstring>> operators = {
      {},
      {},
      {},
      {},
      {L"**"},
      {L"*", L"/", L"%"},
      {L"+", L"-"},
      {L">>>", L"<<", L">>"},
      {L"<=", L">=", L"<", L">"},
      {L"===", L"!==", L"==", L"!="},
      {L"&"},
      {L"^"},
      {L"|"},
      {L"&&"},
      {L"||"},
      {L"??"},
      {},
      {},
      {}};
  auto current = position;
  skipInvisible(filename, source, current);
  auto token = readSymbolToken(filename, source, current);
  if (!token) {
    return nullptr;
  }
  for (size_t level = 0; level < operators.size(); level++) {
    for (auto &opt : operators[level]) {
      if (token->location.isEqual(source, opt)) {
        common::AutoPtr node = new BinaryExpression;
        node->location = token->location;
        node->level = (uint32_t)level;
        node->opt = opt;
        node->right = readRValue(filename, source, current, (uint32_t)level);
        if (!node->right) {
          throw error::JSSyntaxError(
              formatException(L"Unexcepted token", filename, source, current),
              {filename, current.line, current.column});
        }
        node->right->addParent(node);
        position = current;
        return node;
      }
    }
  }
  return nullptr;
}

common::AutoPtr<JSParser::Node>
JSParser::readAssigmentExpression(uint32_t filename, const std::wstring &source,
                                  JSParser::Position &position) {
  const std::vector<std::wstring> operators = {
      {L"=", L"+=", L"-=", L"**=", L"*=", L"/=", L"%=", L">>>=", L"<<=", L">>=",
       L"&&=", L"||=", L"&=", L"^=", L"|=", LR"(??=)"}};
  auto current = position;
  skipInvisible(filename, source, current);
  auto token = readSymbolToken(filename, source, current);
  if (!token) {
    return nullptr;
  }
  for (auto &opt : operators) {
    if (token->location.isEqual(source, opt)) {
      common::AutoPtr node = new BinaryExpression;
      node->level = 17;
      node->opt = opt;
      node->type = NodeType::EXPRESSION_ASSIGMENT;
      node->right = readRValue(filename, source, current, 17);
      if (!node->right) {
        throw error::JSSyntaxError(
            formatException(L"Unexcepted token", filename, source, current),
            {filename, current.line, current.column});
      }
      node->right->addParent(node);
      node->location = getLocation(source, position, current);
      position = current;
      return node;
    }
  }
  return nullptr;
}

common::AutoPtr<JSParser::Node>
JSParser::readConditionExpression(uint32_t filename, const std::wstring &source,
                                  Position &position) {
  auto current = position;
  skipInvisible(filename, source, current);
  auto token = readSymbolToken(filename, source, current);
  if (!token) {
    return nullptr;
  }
  if (token->location.isEqual(source, L"?")) {
    auto next = current;
    auto truly = readExpression(filename, source, current);
    if (!truly) {
      throw error::JSSyntaxError(
          formatException(L"Unexcepted token", filename, source, current),
          {filename, current.line, current.column});
    }
    skipInvisible(filename, source, current);
    token = readSymbolToken(filename, source, current);
    if (!token || !token->location.isEqual(source, L":")) {
      throw error::JSSyntaxError(
          formatException(L"Unexcepted token", filename, source, current),
          {filename, current.line, current.column});
    }
    auto falsely = readExpression(filename, source, current);
    if (!falsely) {
      throw error::JSSyntaxError(
          formatException(L"Unexcepted token", filename, source, current),
          {filename, current.line, current.column});
    }
    common::AutoPtr node = new ConditionExpression;

    common::AutoPtr vnode = new BinaryExpression;
    vnode->level = -2;

    vnode->location = getLocation(source, next, current);
    vnode->left = truly;
    truly->addParent(vnode);
    vnode->right = falsely;
    falsely->addParent(vnode);

    node->right = vnode;
    vnode->addParent(node);

    node->location = getLocation(source, position, current);
    position = current;
    return node;
  }
  return nullptr;
}

common::AutoPtr<JSParser::Node>
JSParser::readUpdateExpression(uint32_t filename, const std::wstring &source,
                               Position &position) {
  auto current = position;
  skipInvisible(filename, source, current);
  auto token = readSymbolToken(filename, source, current);
  if (token != nullptr) {
    if (token->location.isEqual(source, L"++") ||
        token->location.isEqual(source, L"--")) {
      common::AutoPtr node = new UpdateExpression;
      node->location = token->location;
      node->opt = token->location.getSource(source);
      position = current;
      return node;
    }
  }
  return nullptr;
}

common::AutoPtr<JSParser::Node>
JSParser::readUnaryExpression(uint32_t filename, const std::wstring &source,
                              Position &position) {
  static std::vector<std::wstring> unarys = {L"!", L"~",  L"+",
                                             L"-", L"++", L"--"};
  auto current = position;
  skipInvisible(filename, source, current);
  auto token = readSymbolToken(filename, source, current);
  if (token != nullptr) {
    auto src = token->location.getSource(source);
    auto it = std::find(unarys.begin(), unarys.end(), src);
    if (it != unarys.end()) {
      common::AutoPtr node = new UnaryExpression;
      node->location = token->location;
      node->opt = src;
      node->right = readRValue(filename, source, current, 4);
      if (!node->right) {
        throw error::JSSyntaxError(
            formatException(L"Unexcepted token", filename, source, current),
            {filename, current.line, current.column});
      }
      node->right->addParent(node);
      position = current;
      return node;
    }
  }
  return nullptr;
}

common::AutoPtr<JSParser::Node>
JSParser::readGroupExpression(uint32_t filename, const std::wstring &source,
                              Position &position) {
  auto current = position;
  skipInvisible(filename, source, current);
  auto token = readSymbolToken(filename, source, current);
  if (token != nullptr && token->location.isEqual(source, L"(")) {
    auto expr = readExpressions(filename, source, current);
    if (!expr) {
      throw error::JSSyntaxError(
          formatException(L"Unexcepted token", filename, source, current),
          {filename, current.line, current.column});
    }
    token = readSymbolToken(filename, source, current);
    if (!token || !token->location.isEqual(source, L")")) {
      throw error::JSSyntaxError(
          formatException(L"Unexcepted token", filename, source, current),
          {filename, current.line, current.column});
    }
    common::AutoPtr node = new GroupExpression;
    node->expression = expr;
    expr->addParent(node);
    node->location = getLocation(source, position, current);
    position = current;
    return node;
  }
  return nullptr;
}

common::AutoPtr<JSParser::Node>
JSParser::readMemberExpression(uint32_t filename, const std::wstring &source,
                               Position &position) {
  auto current = position;
  skipInvisible(filename, source, current);
  auto token = readSymbolToken(filename, source, current);
  if (!token) {
    return nullptr;
  }

  if (token->location.isEqual(source, L".")) {
    auto identifier = readIdentifierLiteral(filename, source, current);
    if (!identifier) {
      identifier = readMemberLiteral(filename, source, current);
    }
    if (!identifier) {
      throw error::JSSyntaxError(
          formatException(L"Unexcepted token", filename, source, current),
          {filename, current.line, current.column});
    }
    common::AutoPtr node = new MemberExpression;
    node->location = getLocation(source, position, current);
    node->right = identifier;
    identifier->addParent(node);
    position = current;
    return node;
  }

  if (token->location.isEqual(source, L"?.")) {
    auto next = current;
    skipInvisible(filename, source, current);
    token = readSymbolToken(filename, source, current);
    if (token != nullptr) {
      if (token->location.isEqual(source, L"(")) {
        current = next;
        auto field = readCallExpression(filename, source, current)
                         .cast<CallExpression>();
        if (!field) {
          throw error::JSSyntaxError(
              formatException(L"Unexcepted token", filename, source, current),
              {filename, current.line, current.column});
        }
        common::AutoPtr node = new OptionalCallExpression;
        node->level = field->level;
        node->type = NodeType::EXPRESSION_OPTIONAL_CALL;
        node->arguments = field->arguments;
        for (auto &arg : node->arguments) {
          arg->addParent(node);
        }
        node->location = getLocation(source, position, current);
        position = current;
        return node;
      } else if (token->location.isEqual(source, L"[")) {
        current = next;
        auto field = readMemberExpression(filename, source, current)
                         .cast<ComputedMemberExpression>();
        if (!field) {
          throw error::JSSyntaxError(
              formatException(L"Unexcepted token", filename, source, current),
              {filename, current.line, current.column});
        }
        common::AutoPtr node = new OptionalComputedMemberExpression;
        node->right = field->right;
        node->right->addParent(node);
        node->location = getLocation(source, position, current);
        position = current;
        return node;
      }
    }
    common::AutoPtr node = new OptionalMemberExpression;
    node->location = getLocation(source, position, current);
    auto identifier = readIdentifierLiteral(filename, source, current);
    if (!identifier) {
      throw error::JSSyntaxError(
          formatException(L"Unexcepted token", filename, source, current),
          {filename, current.line, current.column});
    }
    node->right = identifier;
    identifier->addParent(node);
    position = current;
    return node;
  }
  if (token->location.isEqual(source, L"[")) {
    auto field = readExpressions(filename, source, current);
    if (!field) {
      throw error::JSSyntaxError(
          formatException(L"Unexcepted token", filename, source, current),
          {filename, current.line, current.column});
    }
    skipInvisible(filename, source, current);
    token = readSymbolToken(filename, source, current);
    if (!token || !token->location.isEqual(source, L"]")) {
      throw error::JSSyntaxError(
          formatException(L"Unexcepted token", filename, source, current),
          {filename, current.line, current.column});
    }
    common::AutoPtr node = new ComputedMemberExpression;
    node->right = field;
    field->addParent(node);
    node->location = getLocation(source, position, current);
    position = current;
    return node;
  }
  return nullptr;
}

common::AutoPtr<JSParser::Node>
JSParser::readCallExpression(uint32_t filename, const std::wstring &source,
                             Position &position) {
  auto current = position;
  skipInvisible(filename, source, current);
  auto token = readSymbolToken(filename, source, current);
  if (token != nullptr && token->location.isEqual(source, L"(")) {
    common::AutoPtr node = new CallExpression;
    auto arg = readRestExpression(filename, source, current);
    if (!arg) {
      arg = readExpression(filename, source, current);
    }
    while (arg != nullptr) {
      arg->addParent(node);
      node->arguments.push_back(arg);
      auto next = current;
      skipInvisible(filename, source, current);
      token = readSymbolToken(filename, source, current);
      if (!token) {
        throw error::JSSyntaxError(
            formatException(L"Unexcepted token", filename, source, current),
            {filename, current.line, current.column});
      }
      if (token->location.isEqual(source, L")")) {
        current = next;
        break;
      } else if (token->location.isEqual(source, L",")) {
        arg = readRestExpression(filename, source, current);
        if (!arg) {
          arg = readExpression(filename, source, current);
        }
      } else {
        throw error::JSSyntaxError(
            formatException(L"Unexcepted token", filename, source, current),
            {filename, current.line, current.column});
      }
    }
    skipInvisible(filename, source, current);
    token = readSymbolToken(filename, source, current);
    if (!token || !token->location.isEqual(source, L")")) {
      throw error::JSSyntaxError(
          formatException(L"Unexcepted token", filename, source, current),
          {filename, current.line, current.column});
    }
    node->location = getLocation(source, position, current);
    position = current;
    return node;
  }
  return nullptr;
}

common::AutoPtr<JSParser::Node>
JSParser::readRestExpression(uint32_t filename, const std::wstring &source,
                             Position &position) {
  auto current = position;
  skipInvisible(filename, source, current);
  auto token = readSymbolToken(filename, source, current);
  if (!token) {
    return nullptr;
  }
  if (token->location.isEqual(source, L"...")) {
    common::AutoPtr node = new BinaryExpression;
    node->level = 19;
    node->opt = L"...";
    node->type = NodeType::EXPRESSION_BINARY;
    node->right = readRValue(filename, source, current, 999);
    if (!node->right) {
      throw error::JSSyntaxError(
          formatException(L"Unexcepted token", filename, source, current),
          {filename, current.line, current.column});
    }
    node->right->addParent(node);
    node->location = getLocation(source, position, current);
    position = current;
    return node;
  }
  return nullptr;
}

common::AutoPtr<JSParser::Node>
JSParser::readAwaitExpression(uint32_t filename, const std::wstring &source,
                              Position &position) {
  auto current = position;
  skipInvisible(filename, source, current);
  auto token = readIdentifierToken(filename, source, current);
  if (token != nullptr && token->location.isEqual(source, L"await")) {
    common::AutoPtr node = new AwaitExpression;
    node->location = getLocation(source, position, current);
    node->right = readRValue(filename, source, current, 4);
    if (!node->right) {
      throw error::JSSyntaxError(
          formatException(L"Unexcepted token", filename, source, current),
          {filename, current.line, current.column});
    }
    node->right->addParent(node);
    position = current;
    return node;
  }
  return nullptr;
}

common::AutoPtr<JSParser::Node>
JSParser::readTypeofExpression(uint32_t filename, const std::wstring &source,
                               Position &position) {
  auto current = position;
  skipInvisible(filename, source, current);
  auto token = readKeywordToken(filename, source, current);
  if (token != nullptr && token->location.isEqual(source, L"typeof")) {
    common::AutoPtr node = new TypeofExpression;
    node->location = getLocation(source, position, current);
    node->right = readRValue(filename, source, current, 4);
    if (!node->right) {
      throw error::JSSyntaxError(
          formatException(L"Unexcepted token", filename, source, current),
          {filename, current.line, current.column});
    }
    node->right->addParent(node);
    position = current;
    return node;
  }
  return nullptr;
}

common::AutoPtr<JSParser::Node>
JSParser::readVoidExpression(uint32_t filename, const std::wstring &source,
                             Position &position) {
  auto current = position;
  skipInvisible(filename, source, current);
  auto token = readKeywordToken(filename, source, current);
  if (token != nullptr && token->location.isEqual(source, L"void")) {
    common::AutoPtr node = new VoidExpression;
    node->location = getLocation(source, position, current);
    node->right = readRValue(filename, source, current, 4);
    if (!node->right) {
      throw error::JSSyntaxError(
          formatException(L"Unexcepted token", filename, source, current),
          {filename, current.line, current.column});
    }
    node->right->addParent(node);
    position = current;
    return node;
  }
  return nullptr;
}

common::AutoPtr<JSParser::Node>
JSParser::readNewExpression(uint32_t filename, const std::wstring &source,
                            Position &position) {
  auto current = position;
  skipInvisible(filename, source, current);
  auto token = readKeywordToken(filename, source, current);
  if (token != nullptr && token->location.isEqual(source, L"new")) {
    common::AutoPtr node = new NewExpression;
    node->location = getLocation(source, position, current);
    node->right = readRValue(filename, source, current, 2);
    if (!node->right) {
      throw error::JSSyntaxError(
          formatException(L"Unexcepted token", filename, source, current),
          {filename, current.line, current.column});
    }
    node->right->addParent(node);
    position = current;
    return node;
  }
  return nullptr;
}

common::AutoPtr<JSParser::Node>
JSParser::readDeleteExpression(uint32_t filename, const std::wstring &source,
                               Position &position) {
  auto current = position;
  skipInvisible(filename, source, current);
  auto token = readKeywordToken(filename, source, current);
  if (token != nullptr && token->location.isEqual(source, L"delete")) {
    common::AutoPtr node = new DeleteExpression;
    node->location = getLocation(source, position, current);
    node->right = readRValue(filename, source, current, 4);
    if (!node->right) {
      throw error::JSSyntaxError(
          formatException(L"Unexcepted token", filename, source, current),
          {filename, current.line, current.column});
    }
    node->right->addParent(node);
    position = current;
    return node;
  }
  return nullptr;
}

common::AutoPtr<JSParser::Node>
JSParser::readInExpression(uint32_t filename, const std::wstring &source,
                           Position &position) {
  auto current = position;
  skipInvisible(filename, source, current);
  auto token = readKeywordToken(filename, source, current);
  if (token != nullptr && token->location.isEqual(source, L"in")) {
    common::AutoPtr node = new BinaryExpression;
    node->location = token->location;
    node->level = 9;
    node->opt = L"in";
    node->type = NodeType::EXPRESSION_BINARY;
    node->right = readRValue(filename, source, current, 9);
    if (!node->right) {
      throw error::JSSyntaxError(
          formatException(L"Unexcepted token", filename, source, current),
          {filename, current.line, current.column});
    }
    node->right->addParent(node);
    position = current;
    return node;
  }
  return nullptr;
}

common::AutoPtr<JSParser::Node> JSParser::readInstanceOfExpression(
    uint32_t filename, const std::wstring &source, Position &position) {
  auto current = position;
  skipInvisible(filename, source, current);
  auto token = readKeywordToken(filename, source, current);
  if (token != nullptr && token->location.isEqual(source, L"instanceof")) {
    common::AutoPtr node = new BinaryExpression;
    node->location = token->location;
    node->level = 9;
    node->opt = L"instanceof";
    node->type = NodeType::EXPRESSION_BINARY;
    node->right = readRValue(filename, source, current, 9);
    if (!node->right) {
      throw error::JSSyntaxError(
          formatException(L"Unexcepted token", filename, source, current),
          {filename, current.line, current.column});
    }
    node->right->addParent(node);
    position = current;
    return node;
  }
  return nullptr;
}

common::AutoPtr<JSParser::Node>
JSParser::readParameter(uint32_t filename, const std::wstring &source,
                        Position &position) {
  common::AutoPtr rest = readRestPattern(filename, source, position);
  if (rest != nullptr) {
    return rest;
  }
  auto current = position;
  skipInvisible(filename, source, current);
  common::AutoPtr node = new Parameter;
  auto identifier = readIdentifierLiteral(filename, source, current);
  if (!identifier) {
    identifier = readObjectPattern(filename, source, current);
  }
  if (!identifier) {
    identifier = readArrayPattern(filename, source, current);
  }
  if (identifier != nullptr) {
    node->identifier = identifier;
    identifier->addParent(node);
  } else {
    return nullptr;
  }
  auto next = current;
  skipInvisible(filename, source, current);
  auto token = readSymbolToken(filename, source, current);
  if (!token) {
    return nullptr;
  }
  if (token->location.isEqual(source, L"=")) {
    auto value = readExpression(filename, source, current);
    if (!value) {
      throw error::JSSyntaxError(
          formatException(L"Unexcepted token", filename, source, current),
          {filename, current.line, current.column});
    }
    node->value = value;
    value->addParent(node);
    next = current;
    skipInvisible(filename, source, current);
    token = readSymbolToken(filename, source, current);
  }
  if (token->location.isEqual(source, L",") ||
      token->location.isEqual(source, L")")) {
    current = next;
    node->location = getLocation(source, position, current);
    position = current;
    return node;
  }
  return nullptr;
}

common::AutoPtr<JSParser::Node> JSParser::readArrowFunctionDeclaration(
    uint32_t filename, const std::wstring &source, Position &position) {
  auto current = position;
  skipInvisible(filename, source, current);
  auto async = readIdentifierToken(filename, source, current);
  common::AutoPtr node = new ArrowFunctionDeclaration();
  if (async == nullptr || !async->location.isEqual(source, L"async")) {
    async = nullptr;
    current = position;
    node->async = false;
  } else {
    node->async = true;
  }
  skipInvisible(filename, source, current);
  auto token = readSymbolToken(filename, source, current);
  if (token != nullptr && token->location.isEqual(source, L"(")) {
    auto param = readParameter(filename, source, current);
    while (param != nullptr) {
      param->addParent(node);

      node->arguments.push_back(param);
      skipInvisible(filename, source, current);
      auto next = current;
      token = readSymbolToken(filename, source, current);
      if (!token) {
        return nullptr;
      }
      if (!token->location.isEqual(source, L",")) {
        if (token->location.isEqual(source, L")")) {
          current = next;
          break;
        } else {
          return nullptr;
        }
      }
      param = readParameter(filename, source, current);
      if (!param) {
        break;
      }
    }
    skipInvisible(filename, source, current);
    token = readSymbolToken(filename, source, current);
    if (!token || !token->location.isEqual(source, L")")) {
      return nullptr;
    }
    skipInvisible(filename, source, current);
    token = readSymbolToken(filename, source, current);
    if (!token || !token->location.isEqual(source, L"=>")) {
      return nullptr;
    }
    auto next = current;
    skipInvisible(filename, source, current);

    auto parentScope = _currentScope;
    node->scope = new Scope(parentScope);
    node->scope->node = node.getRawPointer();
    _currentScope = node->scope.getRawPointer();

    for (auto &param : node->arguments) {
      if (param->type == NodeType::PATTERN_REST_ITEM) {
        declareVariable(node, param.cast<RestPatternItem>()->identifier,
                        Declaration::TYPE::ARGUMENT, false);
      } else {
        declareVariable(node, param.cast<Parameter>()->identifier,
                        Declaration::TYPE::ARGUMENT, false);
      }
    }

    token = readSymbolToken(filename, source, current);
    if (token != nullptr) {
      current = next;
      if (token->location.isEqual(source, L"{")) {
        node->body = readBlockStatement(filename, source, current);
      } else {
        node->body = readExpression(filename, source, current);
      }
    } else {
      node->body = readExpression(filename, source, current);
    }
    if (!node->body) {
      throw error::JSSyntaxError(
          formatException(L"Unexcepted token", filename, source, current),
          {filename, current.line, current.column});
    }
    node->body->addParent(node);
    _currentScope = parentScope;
    node->location = getLocation(source, position, current);
    position = current;
    return node;
  }
  // aaa=>{...}
  auto param = readIdentifierLiteral(filename, source, current);
  if (param != nullptr) {
    skipInvisible(filename, source, current);
    token = readSymbolToken(filename, source, current);
    if (token != nullptr && token->location.isEqual(source, L"=>")) {

      auto parentScope = _currentScope;
      node->scope = new Scope(parentScope);
      node->scope->node = node.getRawPointer();
      _currentScope = node->scope.getRawPointer();

      declareVariable(node, param.cast<Parameter>()->identifier,
                      Declaration::TYPE::ARGUMENT, false);

      param->addParent(node);
      node->arguments.push_back(param);
      auto next = current;
      skipInvisible(filename, source, current);
      token = readSymbolToken(filename, source, current);
      if (token != nullptr) {
        current = next;
        if (token->location.isEqual(source, L"{")) {
          node->body = readBlockStatement(filename, source, current);
        } else {
          node->body = readExpression(filename, source, current);
        }
      } else {
        node->body = readExpression(filename, source, current);
      }
      if (!node->body) {
        throw error::JSSyntaxError(
            formatException(L"Unexcepted token", filename, source, current),
            {filename, current.line, current.column});
      }
      node->body->addParent(node);
      _currentScope = parentScope;
      node->location = getLocation(source, position, current);
      position = current;
      return node;
    }
  }
  return nullptr;
}

common::AutoPtr<JSParser::Node>
JSParser::readFunctionDeclaration(uint32_t filename, const std::wstring &source,
                                  Position &position) {
  auto current = position;
  skipInvisible(filename, source, current);
  auto next = current;
  auto async = readIdentifierToken(filename, source, current);
  if (async == nullptr || !async->location.isEqual(source, L"async")) {
    async = nullptr;
    current = next;
  }
  skipInvisible(filename, source, current);
  auto token = readKeywordToken(filename, source, current);
  if (token != nullptr && token->location.isEqual(source, L"function")) {
    skipInvisible(filename, source, current);
    auto next = current;
    auto generator = readSymbolToken(filename, source, current);
    if (generator == nullptr || !generator->location.isEqual(source, L"*")) {
      generator = nullptr;
      current = next;
    }
    common::AutoPtr node = new FunctionDeclaration;
    if (async != nullptr) {
      node->async = true;
    } else {
      node->async = false;
    }
    if (generator != nullptr) {
      node->generator = true;
    } else {
      node->generator = false;
    }
    node->identifier = readIdentifierLiteral(filename, source, current);
    if (node->identifier != nullptr) {
      node->identifier->addParent(node);
      declareVariable(node, node->identifier, Declaration::TYPE::FUNCTION,
                      false);
    }
    skipInvisible(filename, source, current);
    auto token = readSymbolToken(filename, source, current);
    if (token != nullptr && token->location.isEqual(source, L"(")) {

      auto parentScope = _currentScope;
      node->scope = new Scope(parentScope);
      node->scope->node = node.getRawPointer();
      _currentScope = node->scope.getRawPointer();

      auto param = readParameter(filename, source, current);
      while (param != nullptr) {
        param->addParent(node);

        if (param->type == NodeType::PATTERN_REST_ITEM) {
          declareVariable(node, param.cast<RestPatternItem>()->identifier,
                          Declaration::TYPE::ARGUMENT, false);
        } else {
          declareVariable(node, param.cast<Parameter>()->identifier,
                          Declaration::TYPE::ARGUMENT, false);
        }

        node->arguments.push_back(param);
        skipInvisible(filename, source, current);
        auto next = current;
        token = readSymbolToken(filename, source, current);
        if (!token) {
          throw error::JSSyntaxError(
              formatException(L"Unexcepted token", filename, source, current),
              {filename, current.line, current.column});
        }
        if (!token->location.isEqual(source, L",")) {
          if (token->location.isEqual(source, L")")) {
            current = next;
            break;
          } else {
            throw error::JSSyntaxError(
                formatException(L"Unexcepted token", filename, source, current),
                {filename, current.line, current.column});
          }
        }
        param = readParameter(filename, source, current);
        if (!param) {
          break;
        }
      }
      skipInvisible(filename, source, current);
      token = readSymbolToken(filename, source, current);
      if (!token || !token->location.isEqual(source, L")")) {
        throw error::JSSyntaxError(
            formatException(L"Unexcepted token", filename, source, current),
            {filename, current.line, current.column});
      }
      skipInvisible(filename, source, current);
      node->body = readBlockStatement(filename, source, current);
      if (!node->body) {
        throw error::JSSyntaxError(
            formatException(L"Unexcepted token", filename, source, current),
            {filename, current.line, current.column});
      }
      node->body->addParent(node);
      _currentScope = parentScope;
      node->location = getLocation(source, position, current);
      position = current;
      return node;
    }
  }
  return nullptr;
}

common::AutoPtr<JSParser::Node>
JSParser::readArrayDeclaration(uint32_t filename, const std::wstring &source,
                               Position &position) {
  auto current = position;
  skipInvisible(filename, source, current);
  auto token = readSymbolToken(filename, source, current);
  if (token != nullptr && token->location.isEqual(source, L"[")) {
    common::AutoPtr node = new ArrayDeclaration;
    auto item = readRestExpression(filename, source, current);
    if (item == nullptr) {
      item = readExpression(filename, source, current);
    }
    for (;;) {
      auto next = current;
      skipInvisible(filename, source, current);
      token = readSymbolToken(filename, source, current);
      if (!token) {
        throw error::JSSyntaxError(
            formatException(L"Unexcepted token", filename, source, current),
            {filename, current.line, current.column});
      }
      if (token->location.isEqual(source, L"]")) {
        if (item != nullptr) {
          item->addParent(node);
          node->items.push_back(item);
        }
        current = next;
        break;
      }
      if (!token->location.isEqual(source, L",")) {
        throw error::JSSyntaxError(
            formatException(L"Unexcepted token", filename, source, current),
            {filename, current.line, current.column});
      }
      item->addParent(node);
      node->items.push_back(item);
      item = readRestExpression(filename, source, current);
      if (item == nullptr) {
        item = readExpression(filename, source, current);
      }
    }
    skipInvisible(filename, source, current);
    token = readSymbolToken(filename, source, current);
    if (!token) {
      throw error::JSSyntaxError(
          formatException(L"Unexcepted token", filename, source, current),
          {filename, current.line, current.column});
    }
    if (!token->location.isEqual(source, L"]")) {
      throw error::JSSyntaxError(
          formatException(L"Unexcepted token", filename, source, current),
          {filename, current.line, current.column});
    }
    node->location = getLocation(source, position, current);
    position = current;
    return node;
  }
  return nullptr;
}
common::AutoPtr<JSParser::Node>
JSParser::readObjectDeclaration(uint32_t filename, const std::wstring &source,
                                Position &position) {
  auto current = position;
  skipInvisible(filename, source, current);
  auto token = readSymbolToken(filename, source, current);
  if (token != nullptr && token->location.isEqual(source, L"{")) {
    common::AutoPtr node = new ObjectDeclaration;
    auto item = readObjectProperty(filename, source, current);
    for (;;) {
      auto next = current;
      skipInvisible(filename, source, current);
      token = readSymbolToken(filename, source, current);
      if (!token) {
        throw error::JSSyntaxError(
            formatException(L"Unexcepted token", filename, source, current),
            {filename, current.line, current.column});
      }
      if (token->location.isEqual(source, L"}")) {
        if (item != nullptr) {
          item->addParent(node);
          node->properties.push_back(item);
        }
        current = next;
        break;
      }
      if (!token->location.isEqual(source, L",") || !item) {
        current = next;
        throw error::JSSyntaxError(
            formatException(L"Unexcepted token", filename, source, current),
            {filename, current.line, current.column});
      }
      item->addParent(node);
      node->properties.push_back(item);
      item = readObjectProperty(filename, source, current);
    }
    skipInvisible(filename, source, current);
    token = readSymbolToken(filename, source, current);
    if (!token) {
      throw error::JSSyntaxError(
          formatException(L"Unexcepted token", filename, source, current),
          {filename, current.line, current.column});
    }
    if (!token->location.isEqual(source, L"}")) {
      throw error::JSSyntaxError(
          formatException(L"Unexcepted token", filename, source, current),
          {filename, current.line, current.column});
    }
    node->location = getLocation(source, position, current);
    position = current;
    return node;
  }
  return nullptr;
}

common::AutoPtr<JSParser::Node>
JSParser::readObjectProperty(uint32_t filename, const std::wstring &source,
                             Position &position) {
  auto rest = readRestExpression(filename, source, position);
  if (rest != nullptr) {
    return rest;
  }
  auto method = readObjectMethod(filename, source, position);
  if (method != nullptr) {
    return method;
  }
  method = readObjectAccessor(filename, source, position);
  if (method != nullptr) {
    return method;
  }
  auto current = position;
  auto identifier = readMemberLiteral(filename, source, current);
  if (!identifier) {
    identifier = readStringLiteral(filename, source, current);
  }
  if (!identifier) {
    identifier = readNumberLiteral(filename, source, current);
  }
  if (!identifier) {
    identifier = readMemberExpression(filename, source, current);
    if (identifier != nullptr &&
        identifier->type == NodeType::EXPRESSION_COMPUTED_MEMBER) {
      identifier = identifier.cast<ComputedMemberExpression>()->right;
    } else {
      current = position;
      identifier = nullptr;
    }
  }
  if (!identifier) {
    return nullptr;
  }
  common::AutoPtr node = new ObjectProperty;
  node->identifier = identifier;
  if (identifier != nullptr) {
    identifier->addParent(node);
  }
  auto next = current;
  skipInvisible(filename, source, current);
  auto token = readSymbolToken(filename, source, current);
  if (token != nullptr && token->location.isEqual(source, L":")) {
    auto impl = readExpression(filename, source, current);
    if (!impl) {
      throw error::JSSyntaxError(
          formatException(L"Unexcepted token", filename, source, current),
          {filename, current.line, current.column});
    }
    node->implement = impl;
    impl->addParent(node);
    next = current;
    skipInvisible(filename, source, current);
    token = readSymbolToken(filename, source, current);
  }
  if (token != nullptr) {
    if (token->location.isEqual(source, L",") ||
        token->location.isEqual(source, L"}")) {
      current = next;
      node->location = getLocation(source, position, current);
      position = current;
      return node;
    }
  }
  throw error::JSSyntaxError(
      formatException(L"Unexcepted token", filename, source, current),
      {filename, current.line, current.column});
}

common::AutoPtr<JSParser::Node>
JSParser::readObjectMethod(uint32_t filename, const std::wstring &source,
                           Position &position) {
  auto current = position;
  auto next = current;

  skipInvisible(filename, source, current);
  auto async = readIdentifierLiteral(filename, source, current);
  if (async != nullptr && async->location.isEqual(source, L"async")) {
    next = current;
  } else {
    current = next;
    async = nullptr;
  }

  skipInvisible(filename, source, current);
  auto generator = readSymbolToken(filename, source, current);
  if (!generator || !generator->location.isEqual(source, L"*")) {
    current = next;
    generator = nullptr;
  } else {
    next = current;
  }

  auto identifier = readMemberLiteral(filename, source, current);
  if (!identifier) {
    identifier = readStringLiteral(filename, source, current);
  }
  if (!identifier) {
    identifier = readNumberLiteral(filename, source, current);
  }

  if (!identifier) {
    identifier = readMemberExpression(filename, source, current);
    if (identifier != nullptr &&
        identifier->type == NodeType::EXPRESSION_COMPUTED_MEMBER) {
      identifier = identifier.cast<ComputedMemberExpression>()->right;
    } else {
      current = position;
      identifier = nullptr;
    }
  }
  if (!identifier) {
    if (!generator) {
      identifier = async;
      async = nullptr;
    }
    if (!identifier) {
      return nullptr;
    }
  }
  skipInvisible(filename, source, current);
  auto token = readSymbolToken(filename, source, current);
  if (token != nullptr && token->location.isEqual(source, L"(")) {
    common::AutoPtr node = new ObjectMethod;
    node->async = async != nullptr;
    node->generator = generator != nullptr;
    node->identifier = identifier;
    identifier->addParent(node);

    auto parentScope = _currentScope;
    node->scope = new Scope(parentScope);
    node->scope->node = node.getRawPointer();
    _currentScope = node->scope.getRawPointer();

    auto param = readParameter(filename, source, current);
    while (param != nullptr) {
      param->addParent(node);

      if (param->type == NodeType::PATTERN_REST_ITEM) {
        declareVariable(node, param.cast<RestPatternItem>()->identifier,
                        Declaration::TYPE::ARGUMENT, false);
      } else {
        declareVariable(node, param.cast<Parameter>()->identifier,
                        Declaration::TYPE::ARGUMENT, false);
      }

      node->arguments.push_back(param);
      skipInvisible(filename, source, current);
      auto next = current;
      token = readSymbolToken(filename, source, current);
      if (!token) {
        throw error::JSSyntaxError(
            formatException(L"Unexcepted token", filename, source, current),
            {filename, current.line, current.column});
      }
      if (!token->location.isEqual(source, L",")) {
        if (token->location.isEqual(source, L")")) {
          current = next;
          break;
        } else {
          throw error::JSSyntaxError(
              formatException(L"Unexcepted token", filename, source, current),
              {filename, current.line, current.column});
        }
      }
      param = readParameter(filename, source, current);
      if (!param) {
        break;
      }
    }
    skipInvisible(filename, source, current);
    token = readSymbolToken(filename, source, current);
    if (!token || !token->location.isEqual(source, L")")) {
      throw error::JSSyntaxError(
          formatException(L"Unexcepted token", filename, source, current),
          {filename, current.line, current.column});
    }
    skipInvisible(filename, source, current);
    node->body = readBlockStatement(filename, source, current);
    if (!node->body) {
      throw error::JSSyntaxError(
          formatException(L"Unexcepted token", filename, source, current),
          {filename, current.line, current.column});
    }
    node->body->addParent(node);
    _currentScope = parentScope;
    node->location = getLocation(source, position, current);
    position = current;
    return node;
  }
  return nullptr;
}

common::AutoPtr<JSParser::Node>
JSParser::readObjectAccessor(uint32_t filename, const std::wstring &source,
                             Position &position) {
  auto current = position;

  skipInvisible(filename, source, current);
  auto accessor = readIdentifierLiteral(filename, source, current);
  if (accessor == nullptr || (!accessor->location.isEqual(source, L"get") &&
                              !accessor->location.isEqual(source, L"set"))) {
    return nullptr;
  }

  auto identifier = readMemberLiteral(filename, source, current);
  if (!identifier) {
    identifier = readStringLiteral(filename, source, current);
  }
  if (!identifier) {
    identifier = readNumberLiteral(filename, source, current);
  }

  if (!identifier) {
    identifier = readMemberExpression(filename, source, current);
    if (identifier != nullptr &&
        identifier->type == NodeType::EXPRESSION_COMPUTED_MEMBER) {
      identifier = identifier.cast<ComputedMemberExpression>()->right;
    } else {
      current = position;
      identifier = nullptr;
    }
  }
  if (!identifier) {
    return nullptr;
  }
  skipInvisible(filename, source, current);
  auto token = readSymbolToken(filename, source, current);
  if (token != nullptr && token->location.isEqual(source, L"(")) {
    common::AutoPtr node = new ObjectAccessor;

    auto parentScope = _currentScope;
    node->scope = new Scope(parentScope);
    node->scope->node = node.getRawPointer();
    _currentScope = node->scope.getRawPointer();

    node->kind = accessor->location.isEqual(source, L"get") ? AccessorKind::GET
                                                            : AccessorKind::SET;
    node->identifier = identifier;
    identifier->addParent(node);
    auto param = readParameter(filename, source, current);
    while (param != nullptr) {
      if (param->type == NodeType::PATTERN_REST_ITEM) {
        declareVariable(node, param.cast<RestPatternItem>()->identifier,
                        Declaration::TYPE::ARGUMENT, false);
      } else {
        declareVariable(node, param.cast<Parameter>()->identifier,
                        Declaration::TYPE::ARGUMENT, false);
      }
      param->addParent(node);
      node->arguments.push_back(param);
      skipInvisible(filename, source, current);
      auto next = current;
      token = readSymbolToken(filename, source, current);
      if (!token) {
        throw error::JSSyntaxError(
            formatException(L"Unexcepted token", filename, source, current),
            {filename, current.line, current.column});
      }
      if (!token->location.isEqual(source, L",")) {
        if (token->location.isEqual(source, L")")) {
          current = next;
          break;
        } else {
          throw error::JSSyntaxError(
              formatException(L"Unexcepted token", filename, source, current),
              {filename, current.line, current.column});
        }
      }
      param = readParameter(filename, source, current);
      if (!param) {
        break;
      }
    }
    skipInvisible(filename, source, current);
    token = readSymbolToken(filename, source, current);
    if (!token || !token->location.isEqual(source, L")")) {
      throw error::JSSyntaxError(
          formatException(L"Unexcepted token", filename, source, current),
          {filename, current.line, current.column});
    }
    skipInvisible(filename, source, current);
    node->body = readBlockStatement(filename, source, current);
    if (!node->body) {
      throw error::JSSyntaxError(
          formatException(L"Unexcepted token", filename, source, current),
          {filename, current.line, current.column});
    }
    node->body->addParent(node);
    _currentScope = parentScope;
    node->location = getLocation(source, position, current);
    position = current;
    return node;
  }
  return nullptr;
}

common::AutoPtr<JSParser::Node>
JSParser::readClassDeclaration(uint32_t filename, const std::wstring &source,
                               Position &position) {
  auto current = position;
  common::AutoPtr node = new ClassDeclaration;
  auto decorator = readDecorator(filename, source, current);
  while (decorator != nullptr) {
    decorator->addParent(node);
    node->decorators.push_back(decorator);
    decorator = readDecorator(filename, source, current);
  }
  auto backup = current;
  skipInvisible(filename, source, current);
  auto token = readKeywordToken(filename, source, current);
  if (token != nullptr && token->location.isEqual(source, L"export")) {
    current = backup;
    auto exportNode = readExportDeclaration(filename, source, current)
                          .cast<ExportDeclaration>();
    if (!exportNode || exportNode->items.size() == 0 ||
        exportNode->items[0]->type != NodeType::DECLARATION_CLASS) {
      throw error::JSSyntaxError(
          formatException(L"Unexcepted token", filename, source, current),
          {filename, current.line, current.column});
    }
    auto clazz = exportNode->items[0].cast<ClassDeclaration>();
    clazz->decorators = node->decorators;
    for (auto &dec : node->decorators) {
      dec->addParent(clazz);
    }
    exportNode->location = getLocation(source, position, current);
    position = current;
    return exportNode;
  }
  if (token != nullptr && token->location.isEqual(source, L"class")) {
    node->identifier = readIdentifierLiteral(filename, source, current);
    if (node->identifier != nullptr) {
      node->identifier->addParent(node);
    }
    auto next = current;
    skipInvisible(filename, source, current);
    auto token = readKeywordToken(filename, source, current);
    if (token != nullptr && token->location.isEqual(source, L"extends")) {
      next = current;
      skipInvisible(filename, source, current);
      node->extends = readValue(filename, source, current);
      if (!node->extends) {
        throw error::JSSyntaxError(
            formatException(L"Unexcepted token", filename, source, current),
            {filename, current.line, current.column});
      }
      node->extends->addParent(node);
      next = current;
    } else {
      current = next;
    }
    skipInvisible(filename, source, current);
    token = readSymbolToken(filename, source, current);
    if (!token || !token->location.isEqual(source, L"{")) {
      throw error::JSSyntaxError(
          formatException(L"Unexcepted token", filename, source, current),
          {filename, current.line, current.column});
    }
    skipNewLine(filename, source, current);
    auto item = readClassProperty(filename, source, current);
    while (item != nullptr) {
      skipNewLine(filename, source, current);
      auto next = current;
      token = readSymbolToken(filename, source, current);
      if (token != nullptr && token->location.isEqual(source, L"}")) {
        if (item != nullptr) {
          item->addParent(node);
          node->properties.push_back(item);
        }
        current = next;
        break;
      } else {
        current = next;
      }
      node->properties.push_back(item);
      item = readClassProperty(filename, source, current);
    }
    skipNewLine(filename, source, current);
    token = readSymbolToken(filename, source, current);
    if (!token) {
      throw error::JSSyntaxError(
          formatException(L"Unexcepted token", filename, source, current),
          {filename, current.line, current.column});
    }
    if (!token->location.isEqual(source, L"}")) {
      skipInvisible(filename, source, current);
      throw error::JSSyntaxError(
          formatException(L"Unexcepted token", filename, source, current),
          {filename, current.line, current.column});
    }
    node->location = getLocation(source, position, current);
    position = current;
    return node;
  }
  return nullptr;
}

common::AutoPtr<JSParser::Node>
JSParser::readStaticBlock(uint32_t filename, const std::wstring &source,
                          Position &position) {
  auto current = position;
  skipNewLine(filename, source, current);
  auto token = readKeywordToken(filename, source, current);
  if (token != nullptr && token->location.isEqual(source, L"static")) {
    auto statement = readBlockStatement(filename, source, current);
    if (statement != nullptr) {
      statement->type = NodeType::STATIC_BLOCK;
      statement->location = getLocation(source, position, current);
      position = current;
      return statement;
    }
  }
  return nullptr;
}

common::AutoPtr<JSParser::Node>
JSParser::readClassMethod(uint32_t filename, const std::wstring &source,
                          Position &position) {
  auto current = position;
  common::AutoPtr node = new ClassMethod;
  auto decorator = readDecorator(filename, source, current);
  while (decorator != nullptr) {
    decorator->addParent(node);
    node->decorators.push_back(decorator);
    decorator = readDecorator(filename, source, current);
  }
  auto backup = current;
  skipInvisible(filename, source, current);
  auto static_ = readKeywordToken(filename, source, current);
  if (static_ != nullptr) {
    if (!static_->location.isEqual(source, L"static")) {
      static_ = nullptr;
      current = backup;
    }
  }
  backup = current;
  skipInvisible(filename, source, current);
  auto async = readIdentifierLiteral(filename, source, current);
  if (async != nullptr) {
    if (!async->location.isEqual(source, L"async")) {
      async = nullptr;
      current = backup;
    }
  }
  backup = current;
  skipInvisible(filename, source, current);
  auto generator = readSymbolToken(filename, source, current);
  if (generator != nullptr) {
    if (!generator->location.isEqual(source, L"*")) {
      generator = nullptr;
      current = backup;
    }
  }
  backup = current;
  auto identifier = readMemberLiteral(filename, source, current);
  if (!identifier) {
    identifier = readPrivateName(filename, source, current);
  }
  if (!identifier) {
    identifier = readStringLiteral(filename, source, current);
  }
  if (!identifier) {
    identifier = readNumberLiteral(filename, source, current);
  }
  if (!identifier) {
    backup = current;
    identifier = readMemberExpression(filename, source, current);
    if (identifier != nullptr &&
        identifier->type == NodeType::EXPRESSION_COMPUTED_MEMBER) {
      identifier = identifier.cast<ComputedMemberExpression>()->right;
    } else {
      identifier = nullptr;
      current = backup;
    }
  }
  if (!identifier) {
    if (!generator) {
      identifier = async;
      async = nullptr;
    }
  }
  if (identifier != nullptr) {
    identifier->addParent(node);
    skipInvisible(filename, source, current);
    auto token = readSymbolToken(filename, source, current);
    if (token != nullptr && token->location.isEqual(source, L"(")) {
      node->static_ = static_ != nullptr;
      node->async = async != nullptr;
      node->generator = generator != nullptr;
      node->identifier = identifier;

      auto parentScope = _currentScope;
      node->scope = new Scope(parentScope);
      node->scope->node = node.getRawPointer();
      _currentScope = node->scope.getRawPointer();

      auto param = readParameter(filename, source, current);
      while (param != nullptr) {
        if (param->type == NodeType::PATTERN_REST_ITEM) {
          declareVariable(node, param.cast<RestPatternItem>()->identifier,
                          Declaration::TYPE::ARGUMENT, false);
        } else {
          declareVariable(node, param.cast<Parameter>()->identifier,
                          Declaration::TYPE::ARGUMENT, false);
        }
        param->addParent(node);
        node->arguments.push_back(param);
        skipInvisible(filename, source, current);
        auto next = current;
        token = readSymbolToken(filename, source, current);
        if (!token) {
          throw error::JSSyntaxError(
              formatException(L"Unexcepted token", filename, source, current),
              {filename, current.line, current.column});
        }
        if (!token->location.isEqual(source, L",")) {
          if (token->location.isEqual(source, L")")) {
            current = next;
            break;
          } else {
            throw error::JSSyntaxError(
                formatException(L"Unexcepted token", filename, source, current),
                {filename, current.line, current.column});
          }
        }
        param = readParameter(filename, source, current);
        if (!param) {
          break;
        }
      }
      skipInvisible(filename, source, current);
      token = readSymbolToken(filename, source, current);
      if (!token || !token->location.isEqual(source, L")")) {
        throw error::JSSyntaxError(
            formatException(L"Unexcepted token", filename, source, current),
            {filename, current.line, current.column});
      }
      skipInvisible(filename, source, current);
      node->body = readBlockStatement(filename, source, current);
      if (!node->body) {
        throw error::JSSyntaxError(
            formatException(L"Unexcepted token", filename, source, current),
            {filename, current.line, current.column});
      }
      node->body->addParent(node);

      _currentScope = parentScope;

      node->location = getLocation(source, position, current);
      position = current;
      return node;
    }
  }

  return nullptr;
}

common::AutoPtr<JSParser::Node>
JSParser::readClassAccessor(uint32_t filename, const std::wstring &source,
                            Position &position) {
  auto current = position;
  common::AutoPtr node = new ClassAccessor;
  auto decorator = readDecorator(filename, source, current);
  while (decorator != nullptr) {
    decorator->addParent(node);
    node->decorators.push_back(decorator);
    decorator = readDecorator(filename, source, current);
  }
  skipInvisible(filename, source, current);
  auto static_ = readKeywordToken(filename, source, current);
  if (static_ != nullptr) {
    if (!static_->location.isEqual(source, L"static")) {
      static_ = nullptr;
      current = position;
    }
  }
  skipInvisible(filename, source, current);
  auto accessor = readIdentifierToken(filename, source, current);
  if (accessor != nullptr && (accessor->location.isEqual(source, L"get") ||
                              accessor->location.isEqual(source, L"set"))) {
    auto identifier = readMemberLiteral(filename, source, current);
    if (!identifier) {
      identifier = readPrivateName(filename, source, current);
    }
    if (!identifier) {
      identifier = readStringLiteral(filename, source, current);
    }
    if (!identifier) {
      identifier = readNumberLiteral(filename, source, current);
    }
    if (!identifier) {
      auto backup = current;
      identifier = readMemberExpression(filename, source, current);
      if (identifier != nullptr &&
          identifier->type == NodeType::EXPRESSION_COMPUTED_MEMBER) {
        identifier = identifier.cast<ComputedMemberExpression>()->right;
      } else {
        identifier = nullptr;
        current = backup;
      }
    }
    if (identifier != nullptr) {
      identifier->addParent(node);
      skipInvisible(filename, source, current);
      auto token = readSymbolToken(filename, source, current);
      if (token != nullptr && token->location.isEqual(source, L"(")) {

        auto parentScope = _currentScope;
        node->scope = new Scope(parentScope);
        node->scope->node = node.getRawPointer();
        _currentScope = node->scope.getRawPointer();

        node->static_ = static_ != nullptr;
        node->kind = accessor->location.isEqual(source, L"get")
                         ? AccessorKind::GET
                         : AccessorKind::SET;
        node->identifier = identifier;
        auto param = readParameter(filename, source, current);
        while (param != nullptr) {
          if (param->type == NodeType::PATTERN_REST_ITEM) {
            declareVariable(node, param.cast<RestPatternItem>()->identifier,
                            Declaration::TYPE::ARGUMENT, false);
          } else {
            declareVariable(node, param.cast<Parameter>()->identifier,
                            Declaration::TYPE::ARGUMENT, false);
          }
          param->addParent(node);
          node->arguments.push_back(param);
          skipInvisible(filename, source, current);
          auto next = current;
          token = readSymbolToken(filename, source, current);
          if (!token) {
            throw error::JSSyntaxError(
                formatException(L"Unexcepted token", filename, source, current),
                {filename, current.line, current.column});
          }
          if (!token->location.isEqual(source, L",")) {
            if (token->location.isEqual(source, L")")) {
              current = next;
              break;
            } else {
              throw error::JSSyntaxError(
                  formatException(L"Unexcepted token", filename, source,
                                  current),
                  {filename, current.line, current.column});
            }
          }
          param = readParameter(filename, source, current);
          if (!param) {
            break;
          }
        }
        skipInvisible(filename, source, current);
        token = readSymbolToken(filename, source, current);
        if (!token || !token->location.isEqual(source, L")")) {
          throw error::JSSyntaxError(
              formatException(L"Unexcepted token", filename, source, current),
              {filename, current.line, current.column});
        }
        skipInvisible(filename, source, current);
        node->body = readBlockStatement(filename, source, current);
        if (!node->body) {
          throw error::JSSyntaxError(
              formatException(L"Unexcepted token", filename, source, current),
              {filename, current.line, current.column});
        }
        node->body->addParent(node);
        _currentScope = parentScope;
        node->location = getLocation(source, position, current);
        position = current;
        return node;
      }
    }
  }
  return nullptr;
}

common::AutoPtr<JSParser::Node>
JSParser::readClassProperty(uint32_t filename, const std::wstring &source,
                            Position &position) {
  auto rest = readRestExpression(filename, source, position);
  if (rest != nullptr) {
    return rest;
  }
  auto sblock = readStaticBlock(filename, source, position);
  if (sblock != nullptr) {
    return sblock;
  }
  auto method = readClassMethod(filename, source, position);
  if (method != nullptr) {
    return method;
  }
  auto accessor = readClassAccessor(filename, source, position);
  if (accessor != nullptr) {
    return accessor;
  }
  auto current = position;
  common::AutoPtr node = new ClassProperty;
  auto decorator = readDecorator(filename, source, current);
  while (decorator != nullptr) {
    decorator->addParent(node);
    node->decorators.push_back(decorator);
    decorator = readDecorator(filename, source, current);
  }
  auto backup = current;
  skipNewLine(filename, source, current);
  auto staticFlag = readKeywordToken(filename, source, current);
  if (staticFlag != nullptr) {
    if (!staticFlag->location.isEqual(source, L"static")) {
      current = backup;
      staticFlag = nullptr;
    }
  }
  node->identifier = readMemberLiteral(filename, source, current);
  node->static_ = staticFlag != nullptr;
  if (!node->identifier) {
    node->identifier = readPrivateName(filename, source, current);
  }
  if (!node->identifier) {
    node->identifier = readStringLiteral(filename, source, current);
  }
  if (!node->identifier) {
    node->identifier = readNumberLiteral(filename, source, current);
  }
  if (!node->identifier) {
    auto backup = current;
    node->identifier = readMemberExpression(filename, source, current);
    if (node->identifier != nullptr &&
        node->identifier->type == NodeType::EXPRESSION_COMPUTED_MEMBER) {
      node->identifier =
          node->identifier.cast<ComputedMemberExpression>()->right;
    } else {
      node->identifier = nullptr;
      current = backup;
    }
  }
  if (!node->identifier) {
    return nullptr;
  }
  node->identifier->addParent(node);
  skipInvisible(filename, source, current);
  auto token = readSymbolToken(filename, source, current);
  if (token != nullptr && token->location.isEqual(source, L"=")) {
    node->value = readExpression(filename, source, current);
    if (!node->value) {
      throw error::JSSyntaxError(
          formatException(L"Unexcepted token", filename, source, current),
          {filename, current.line, current.column});
    }
    node->value->addParent(node);
  }
  node->location = getLocation(source, position, current);
  position = current;
  return node;
}

common::AutoPtr<JSParser::Node>
JSParser::readVariableDeclarator(uint32_t filename, const std::wstring &source,
                                 Position &position) {
  auto current = position;
  skipInvisible(filename, source, current);
  auto identifier = readIdentifierLiteral(filename, source, current);
  if (identifier == nullptr) {
    identifier = readArrayPattern(filename, source, current);
  }
  if (identifier == nullptr) {
    identifier = readObjectPattern(filename, source, current);
  }
  if (!identifier) {
    return nullptr;
  }
  common::AutoPtr node = new VariableDeclarator;
  node->identifier = identifier;
  identifier->addParent(node);
  auto next = current;
  skipInvisible(filename, source, current);
  auto token = readSymbolToken(filename, source, current);
  if (!token) {
    token = readIdentifierToken(filename, source, current);
  }
  if (token != nullptr && token->location.isEqual(source, L"=")) {
    auto value = readRValue(filename, source, current, 999);
    if (!value) {
      throw error::JSSyntaxError(
          formatException(L"Unexcepted token", filename, source, current),
          {filename, current.line, current.column});
    }
    node->value = value;
    value->addParent(node);
  } else {
    current = next;
  }
  node->location = getLocation(source, position, current);
  position = current;
  return node;
}

common::AutoPtr<JSParser::Node>
JSParser::readVariableDeclaration(uint32_t filename, const std::wstring &source,
                                  Position &position) {
  auto current = position;
  skipInvisible(filename, source, current);
  auto kind = readKeywordToken(filename, source, current);
  if (kind != nullptr) {
    common::AutoPtr node = new VariableDeclaration;
    Declaration::TYPE type = Declaration::TYPE::UNINITIALIZED;
    bool isConst = false;
    if (kind->location.isEqual(source, L"var")) {
      node->kind = DeclarationKind::VAR;
      type = Declaration::TYPE::UNDEFINED;
    } else if (kind->location.isEqual(source, L"let")) {
      node->kind = DeclarationKind::LET;
    } else if (kind->location.isEqual(source, L"const")) {
      node->kind = DeclarationKind::CONST;
      isConst = true;
    } else {
      return nullptr;
    }
    auto declarator = readVariableDeclarator(filename, source, current);
    if (!declarator) {
      throw error::JSSyntaxError(
          formatException(L"Unexcepted token", filename, source, current),
          {filename, current.line, current.column});
    }
    while (declarator != nullptr) {
      declarator->addParent(node);
      declareVariable(declarator,
                      declarator.cast<VariableDeclarator>()->identifier, type,
                      isConst);
      node->declarations.push_back(declarator);
      auto next = current;
      skipInvisible(filename, source, current);
      auto token = readSymbolToken(filename, source, current);
      if (token == nullptr) {
        current = next;
        break;
      }
      if (!token->location.isEqual(source, L",")) {
        current = next;
        break;
      }
      declarator = readVariableDeclarator(filename, source, current);
    }
    node->location = getLocation(source, position, current);
    position = current;
    return node;
  }
  return nullptr;
}

common::AutoPtr<JSParser::Node>
JSParser::readRestPattern(uint32_t filename, const std::wstring &source,
                          Position &position) {
  auto current = position;
  skipInvisible(filename, source, current);
  auto token = readSymbolToken(filename, source, current);
  if (token != nullptr && token->location.isEqual(source, L"...")) {
    auto identifier = readObjectPattern(filename, source, current);
    if (!identifier) {
      identifier = readArrayPattern(filename, source, current);
    }
    if (!identifier) {
      identifier = readIdentifierLiteral(filename, source, current);
    }
    if (!identifier) {
      throw error::JSSyntaxError(
          formatException(L"Unexcepted token", filename, source, current),
          {filename, current.line, current.column});
    }
    common::AutoPtr node = new RestPatternItem;
    node->identifier = identifier;
    identifier->addParent(node);
    node->location = getLocation(source, position, current);
    position = current;
    return node;
  }
  return nullptr;
}

common::AutoPtr<JSParser::Node>
JSParser::readObjectPatternItem(uint32_t filename, const std::wstring &source,
                                Position &position) {
  auto rest = readRestPattern(filename, source, position);
  if (rest != nullptr) {
    return rest;
  }
  auto current = position;
  skipInvisible(filename, source, current);
  auto next = current;
  auto identifier = readIdentifierLiteral(filename, source, current);
  common::AutoPtr node = new ObjectPatternItem;
  if (!identifier) {
    identifier = readStringLiteral(filename, source, current);
  }
  if (!identifier) {
    identifier = readMemberExpression(filename, source, current);
    if (!identifier ||
        identifier->type != NodeType::EXPRESSION_COMPUTED_MEMBER) {
      identifier = nullptr;
      current = next;
    } else {
      identifier = identifier.cast<ComputedMemberExpression>()->right;
    }
  }
  if (!identifier) {
    throw error::JSSyntaxError(
        formatException(L"Unexcepted token", filename, source, current),
        {filename, current.line, current.column});
  }
  node->identifier = identifier;
  identifier->addParent(node);
  next = current;
  skipInvisible(filename, source, current);
  auto token = readSymbolToken(filename, source, current);
  if (token != nullptr) {
    if (token->location.isEqual(source, L":")) {
      auto match = readIdentifierLiteral(filename, source, current);
      if (!match) {
        match = readObjectPattern(filename, source, current);
      }
      if (!match) {
        match = readArrayPattern(filename, source, current);
      }
      if (!match) {
        throw error::JSSyntaxError(
            formatException(L"Unexcepted token", filename, source, current),
            {filename, current.line, current.column});
      }
      node->match = match;
      match->addParent(node);
      next = current;
      skipInvisible(filename, source, current);
      token = readSymbolToken(filename, source, current);
    }
  }
  if (token != nullptr) {
    if (token->location.isEqual(source, L"=")) {
      auto value = readExpression(filename, source, current);
      if (!value) {
        throw error::JSSyntaxError(
            formatException(L"Unexcepted token", filename, source, current),
            {filename, current.line, current.column});
      }
      node->value = value;
      value->addParent(node);
      next = current;
      skipInvisible(filename, source, current);
      token = readSymbolToken(filename, source, current);
    }
  }
  if (token != nullptr) {
    if (token->location.isEqual(source, L",") ||
        token->location.isEqual(source, L"}")) {
      current = next;
      node->location = getLocation(source, position, current);
      position = current;
      return node;
    }
    current = next;
  }
  throw error::JSSyntaxError(
      formatException(L"Unexcepted token", filename, source, current),
      {filename, current.line, current.column});
}

common::AutoPtr<JSParser::Node>
JSParser::readObjectPattern(uint32_t filename, const std::wstring &source,
                            Position &position) {
  auto current = position;
  skipInvisible(filename, source, current);
  auto token = readSymbolToken(filename, source, current);
  if (token != nullptr && token->location.isEqual(source, L"{")) {
    common::AutoPtr node = new ObjectPattern;
    auto item = readObjectPatternItem(filename, source, current);
    while (item != nullptr) {
      item->addParent(node);
      node->items.push_back(item);
      auto next = current;
      skipInvisible(filename, source, current);
      token = readSymbolToken(filename, source, current);
      if (!token) {
        return nullptr;
      }
      if (token->location.isEqual(source, L"}")) {
        current = next;
        break;
      }
      if (!token->location.isEqual(source, L",")) {
        return nullptr;
      }
      item = readObjectPatternItem(filename, source, current);
    }
    skipInvisible(filename, source, current);
    token = readSymbolToken(filename, source, current);
    if (!token) {
      return nullptr;
    }
    if (!token->location.isEqual(source, L"}")) {
      return nullptr;
    }
    node->location = getLocation(source, position, current);
    position = current;
    return node;
  }
  return nullptr;
}

common::AutoPtr<JSParser::Node>
JSParser::readArrayPatternItem(uint32_t filename, const std::wstring &source,
                               Position &position) {
  auto rest = readRestPattern(filename, source, position);
  if (rest != nullptr) {
    return rest;
  }
  auto current = position;
  skipInvisible(filename, source, current);
  auto identifier = readObjectPattern(filename, source, current);
  if (!identifier) {
    identifier = readArrayPattern(filename, source, current);
  }
  if (!identifier) {
    identifier = readIdentifierLiteral(filename, source, current);
  }
  if (!identifier) {
    identifier = readStringLiteral(filename, source, current);
  }
  if (identifier != nullptr) {
    common::AutoPtr node = new ArrayPatternItem;
    node->identifier = identifier;
    identifier->addParent(node);
    auto next = current;
    skipInvisible(filename, source, current);
    auto token = readSymbolToken(filename, source, current);
    if (token != nullptr && token->location.isEqual(source, L"=")) {
      auto value = readExpression(filename, source, current);
      if (value == nullptr) {
        throw error::JSSyntaxError(
            formatException(L"Unexcepted token", filename, source, current),
            {filename, current.line, current.column});
      }
      node->value = value;
      value->addParent(node);
      next = current;
    }
    current = next;
    node->location = getLocation(source, position, current);
    position = current;
    return node;
  }
  return nullptr;
}

common::AutoPtr<JSParser::Node>
JSParser::readArrayPattern(uint32_t filename, const std::wstring &source,
                           Position &position) {
  auto current = position;
  skipInvisible(filename, source, current);
  auto token = readSymbolToken(filename, source, current);
  if (token != nullptr && token->location.isEqual(source, L"[")) {
    common::AutoPtr node = new ArrayPattern;
    auto item = readArrayPatternItem(filename, source, current);
    for (;;) {
      skipInvisible(filename, source, current);
      if (source[current.offset] == '\0') {
        throw error::JSSyntaxError(
            formatException(L"Unexcepted token", filename, source, current),
            {filename, current.line, current.column});
      }
      auto next = current;
      token = readSymbolToken(filename, source, current);
      if (!token) {
        return nullptr;
      }
      if (token->location.isEqual(source, L"]")) {
        if (item != nullptr) {
          item->addParent(node);
          node->items.push_back(item);
        }
        current = next;
        break;
      }
      if (!token->location.isEqual(source, L",")) {
        return nullptr;
      }
      item->addParent(node);
      node->items.push_back(item);
      item = readArrayPatternItem(filename, source, current);
    }
    skipInvisible(filename, source, current);
    token = readSymbolToken(filename, source, current);
    if (!token) {
      return nullptr;
    }
    if (!token->location.isEqual(source, L"]")) {
      return nullptr;
    }
    node->location = getLocation(source, position, current);
    position = current;
    return node;
  }
  return nullptr;
}

common::AutoPtr<JSParser::Node>
JSParser::readComment(uint32_t filename, const std::wstring &source,
                      Position &position) {
  auto current = position;
  while (skipWhiteSpace(filename, source, current) ||
         skipLineTerminatior(filename, source, current)) {
    ;
  }
  auto token = readCommentToken(filename, source, current);
  if (token != nullptr) {
    common::AutoPtr node = new Comment();
    node->location = token->location;
    if (source[node->location.start.offset + 1] == L'*') {
      node->type = NodeType::LITERAL_MULTILINE_COMMENT;
    }
    return node;
  }
  return nullptr;
}

common::AutoPtr<JSParser::Node>
JSParser::readImportSpecifier(uint32_t filename, const std::wstring &source,
                              Position &position) {
  auto current = position;
  skipInvisible(filename, source, current);
  auto identifier = readIdentifierLiteral(filename, source, current);
  if (identifier == nullptr) {
    identifier = readStringLiteral(filename, source, current);
  }
  if (identifier != nullptr) {
    common::AutoPtr node = new ImportSpecifier;
    node->identifier = identifier;
    identifier->addParent(node);
    auto backup = current;
    skipInvisible(filename, source, current);
    auto token = readIdentifierToken(filename, source, current);
    if (token != nullptr && token->location.isEqual(source, L"as")) {
      skipInvisible(filename, source, current);
      node->alias = readIdentifierLiteral(filename, source, current);

      if (!node->alias) {
        throw error::JSSyntaxError(
            formatException(L"Unexcepted token", filename, source, current),
            {filename, current.line, current.column});
      }
      node->alias->addParent(node);
    } else if (identifier->type == NodeType::LITERAL_STRING) {
      throw error::JSSyntaxError(
          formatException(L"Unexcepted token", filename, source, current),
          {filename, current.line, current.column});
    } else {
      current = backup;
    }
    node->location = getLocation(source, position, current);
    position = current;
    return node;
  }
  return nullptr;
}

common::AutoPtr<JSParser::Node> JSParser::readImportDefaultSpecifier(
    uint32_t filename, const std::wstring &source, Position &position) {
  auto current = position;
  skipInvisible(filename, source, current);
  auto identifier = readIdentifierLiteral(filename, source, current);
  if (identifier != nullptr) {
    common::AutoPtr node = new ImportDefaultSpecifier;
    node->identifier = identifier;
    identifier->addParent(node);
    node->location = getLocation(source, position, current);
    position = current;
    return node;
  }
  return nullptr;
}

common::AutoPtr<JSParser::Node> JSParser::readImportNamespaceSpecifier(
    uint32_t filename, const std::wstring &source, Position &position) {
  auto current = position;
  skipInvisible(filename, source, current);
  auto token = readSymbolToken(filename, source, current);
  if (token != nullptr && token->location.isEqual(source, L"*")) {
    skipInvisible(filename, source, current);
    token = readIdentifierToken(filename, source, current);
    if (!token || !token->location.isEqual(source, L"as")) {
      throw error::JSSyntaxError(
          formatException(L"Unexcepted token", filename, source, current),
          {filename, current.line, current.column});
    }
    skipInvisible(filename, source, current);
    auto identifier = readIdentifierLiteral(filename, source, current);
    if (!identifier) {
      throw error::JSSyntaxError(
          formatException(L"Unexcepted token", filename, source, current),
          {filename, current.line, current.column});
    }
    common::AutoPtr node = new ImportNamespaceSpecifier;
    node->alias = identifier;
    identifier->addParent(node);
    node->location = getLocation(source, position, current);
    position = current;
    return node;
  }
  return nullptr;
}
common::AutoPtr<JSParser::Node>
JSParser::readImportAttriabue(uint32_t filename, const std::wstring &source,
                              Position &position) {
  auto current = position;
  skipInvisible(filename, source, current);
  auto key = readIdentifierLiteral(filename, source, current);
  if (!key) {
    key = readStringLiteral(filename, source, current);
  }
  if (key != nullptr) {
    common::AutoPtr node = new ImportAttribute;
    node->type = NodeType::IMPORT_ATTARTUBE;
    node->key = key;
    key->addParent(node);
    skipInvisible(filename, source, current);
    auto token = readSymbolToken(filename, source, current);
    if (!token || !token->location.isEqual(source, L":")) {
      throw error::JSSyntaxError(
          formatException(L"Unexcepted token", filename, source, current),
          {filename, current.line, current.column});
    }
    node->value = readStringLiteral(filename, source, current);
    if (!node->value) {
      throw error::JSSyntaxError(
          formatException(L"Unexcepted token", filename, source, current),
          {filename, current.line, current.column});
    }
    node->value->addParent(node);
    node->location = getLocation(source, position, current);
    position = current;
    return node;
  }
  return nullptr;
}

common::AutoPtr<JSParser::Node>
JSParser::readImportDeclaration(uint32_t filename, const std::wstring &source,
                                Position &position) {
  auto current = position;
  skipInvisible(filename, source, current);
  auto token = readKeywordToken(filename, source, current);
  if (token != nullptr && token->location.isEqual(source, L"import")) {
    common::AutoPtr node = new ImportDeclaration;
    skipInvisible(filename, source, current);
    auto src = readStringLiteral(filename, source, current);
    if (src != nullptr) {
      node->source = src;
      src->addParent(node);
    } else {
      auto specifier = readImportNamespaceSpecifier(filename, source, current);
      if (specifier != nullptr) {
        specifier->addParent(node);
        declareVariable(specifier,
                        specifier.cast<ImportNamespaceSpecifier>()->alias,
                        Declaration::TYPE::UNINITIALIZED, true);
        node->items.push_back(specifier);
      } else {
        specifier = readImportDefaultSpecifier(filename, source, current);
        if (specifier != nullptr) {
          specifier->addParent(node);
          declareVariable(specifier,
                          specifier.cast<ImportDefaultSpecifier>()->identifier,
                          Declaration::TYPE::UNINITIALIZED, true);
          node->items.push_back(specifier);
          skipInvisible(filename, source, current);
          token = readSymbolToken(filename, source, current);
          if (token != nullptr && !token->location.isEqual(source, L",")) {
            throw error::JSSyntaxError(
                formatException(L"Unexcepted token", filename, source, current),
                {filename, current.line, current.column});
          }
        }
        auto backup = current;
        skipInvisible(filename, source, current);
        token = readSymbolToken(filename, source, current);
        if (token != nullptr && token->location.isEqual(source, L"{")) {
          auto specifier = readImportSpecifier(filename, source, current);
          for (;;) {
            skipInvisible(filename, source, current);
            auto token = readSymbolToken(filename, source, current);
            if (!token) {
              throw error::JSSyntaxError(
                  formatException(L"Unexcepted token", filename, source,
                                  current),
                  {filename, current.line, current.column});
            }
            if (token->location.isEqual(source, L",")) {
              if (!specifier) {
                throw error::JSSyntaxError(
                    formatException(L"Unexcepted token", filename, source,
                                    current),
                    {filename, current.line, current.column});
              }
              specifier->addParent(node);
              auto ispec = specifier.cast<ImportSpecifier>();
              if (ispec->alias != nullptr) {
                declareVariable(specifier, ispec->alias,
                                Declaration::TYPE::UNINITIALIZED, true);
              } else {
                declareVariable(specifier, ispec->identifier,
                                Declaration::TYPE::UNINITIALIZED, true);
              }
              node->items.push_back(specifier);
            } else if (token->location.isEqual(source, L"}")) {
              if (specifier != nullptr) {
                specifier->addParent(node);
                auto ispec = specifier.cast<ImportSpecifier>();
                if (ispec->alias != nullptr) {
                  declareVariable(specifier, ispec->alias,
                                  Declaration::TYPE::UNINITIALIZED, true);
                } else {
                  declareVariable(specifier, ispec->identifier,
                                  Declaration::TYPE::UNINITIALIZED, true);
                }
                node->items.push_back(specifier);
              }
              break;
            } else {
              throw error::JSSyntaxError(
                  formatException(L"Unexcepted token", filename, source,
                                  current),
                  {filename, current.line, current.column});
            }

            specifier = readImportSpecifier(filename, source, current);
          }
        } else {
          current = backup;
        }
        specifier = readImportNamespaceSpecifier(filename, source, current);
        if (specifier != nullptr) {
          declareVariable(specifier,
                          specifier.cast<ImportNamespaceSpecifier>()->alias,
                          Declaration::TYPE::UNINITIALIZED, true);
          specifier->addParent(node);
          node->items.push_back(specifier);
        }
      }
      skipInvisible(filename, source, current);
      token = readIdentifierToken(filename, source, current);
      if (!token || !token->location.isEqual(source, L"from")) {
        throw error::JSSyntaxError(
            formatException(L"Unexcepted token", filename, source, current),
            {filename, current.line, current.column});
      }
      src = readStringLiteral(filename, source, current);
      if (!src) {
        throw error::JSSyntaxError(
            formatException(L"Unexcepted token", filename, source, current),
            {filename, current.line, current.column});
      }
      src->addParent(node);
      node->source = src;
    }
    auto backup = current;
    skipInvisible(filename, source, current);
    token = readKeywordToken(filename, source, current);
    if (!token) {
      token = readIdentifierToken(filename, source, current);
    }
    if (token != nullptr && (token->location.isEqual(source, L"assert") ||
                             token->location.isEqual(source, L"with"))) {
      skipInvisible(filename, source, current);
      token = readSymbolToken(filename, source, current);
      if (!token || !token->location.isEqual(source, L"{")) {
        throw error::JSSyntaxError(
            formatException(L"Unexcepted token", filename, source, current),
            {filename, current.line, current.column});
      }
      auto attribute = readImportAttriabue(filename, source, current);
      while (attribute != nullptr) {
        attribute->addParent(node);
        node->attributes.push_back(attribute);
        auto backup = current;
        skipInvisible(filename, source, current);
        token = readSymbolToken(filename, source, current);
        if (token == nullptr) {
          throw error::JSSyntaxError(
              formatException(L"Unexcepted token", filename, source, current),
              {filename, current.line, current.column});
        }
        if (token->location.isEqual(source, L"}")) {
          current = backup;
          break;
        }
        if (!token->location.isEqual(source, L",")) {
          throw error::JSSyntaxError(
              formatException(L"Unexcepted token", filename, source, current),
              {filename, current.line, current.column});
        }
        attribute = readImportAttriabue(filename, source, current);
      }
      skipInvisible(filename, source, current);
      token = readSymbolToken(filename, source, current);
      if (!token || !token->location.isEqual(source, L"}")) {
        throw error::JSSyntaxError(
            formatException(L"Unexcepted token", filename, source, current),
            {filename, current.line, current.column});
      }
    } else {
      current = backup;
    }
    node->location = getLocation(source, position, current);
    position = current;
    return node;
  }
  return nullptr;
}

common::AutoPtr<JSParser::Node>
JSParser::readExportSpecifier(uint32_t filename, const std::wstring &source,
                              Position &position) {
  auto current = position;
  skipInvisible(filename, source, current);
  auto identifier = readIdentifierLiteral(filename, source, current);
  if (identifier != nullptr) {
    common::AutoPtr node = new ExportSpecifier;
    node->identifier = identifier;
    identifier->addParent(node);
    auto backup = current;
    skipInvisible(filename, source, current);
    auto token = readIdentifierToken(filename, source, current);
    if (token != nullptr && token->location.isEqual(source, L"as")) {
      auto alias = readIdentifierLiteral(filename, source, current);
      if (!alias) {
        alias = readStringLiteral(filename, source, current);
      }
      if (!alias) {
        throw error::JSSyntaxError(
            formatException(L"Unexcepted token", filename, source, current),
            {filename, current.line, current.column});
      }
      node->alias = alias;
      alias->addParent(node);
    } else {
      current = backup;
    }
    node->location = getLocation(source, position, current);
    position = current;
    return node;
  }
  return nullptr;
}

common::AutoPtr<JSParser::Node> JSParser::readExportDefaultSpecifier(
    uint32_t filename, const std::wstring &source, Position &position) {
  auto current = position;
  skipInvisible(filename, source, current);
  auto token = readKeywordToken(filename, source, current);
  if (token != nullptr && token->location.isEqual(source, L"default")) {
    common::AutoPtr node = new ExportDefaultSpecifier;
    node->value = readExpression(filename, source, current);
    if (!node->value) {
      throw error::JSSyntaxError(
          formatException(L"Unexcepted token", filename, source, current),
          {filename, current.line, current.column});
    }
    node->value->addParent(node);
    node->location = getLocation(source, position, current);
    position = current;
    return node;
  }
  return nullptr;
}

common::AutoPtr<JSParser::Node>
JSParser::readExportAllSpecifier(uint32_t filename, const std::wstring &source,
                                 Position &position) {
  auto current = position;
  skipInvisible(filename, source, current);
  auto token = readSymbolToken(filename, source, current);
  if (token != nullptr && token->location.isEqual(source, L"*")) {
    common::AutoPtr node = new ExportAllSpecifier;
    auto backup = current;
    skipInvisible(filename, source, current);
    token = readIdentifierToken(filename, source, current);
    if (token != nullptr && token->location.isEqual(source, L"as")) {
      node->alias = readIdentifierLiteral(filename, source, current);
      if (!node->alias) {
        throw error::JSSyntaxError(
            formatException(L"Unexcepted token", filename, source, current),
            {filename, current.line, current.column});
      }
      node->alias->addParent(node);
    } else {
      current = backup;
    }
    node->location = getLocation(source, position, current);
    position = current;
    return node;
  }
  return nullptr;
}

common::AutoPtr<JSParser::Node>
JSParser::readExportDeclaration(uint32_t filename, const std::wstring &source,
                                Position &position) {
  auto current = position;
  skipInvisible(filename, source, current);
  auto token = readKeywordToken(filename, source, current);
  if (token != nullptr && token->location.isEqual(source, L"export")) {
    common::AutoPtr node = new ExportDeclaration;
    auto specifier = readExportDefaultSpecifier(filename, source, current);
    if (!specifier) {
      auto declaration = readClassDeclaration(filename, source, current)
                             .cast<ClassDeclaration>();
      if (declaration != nullptr && declaration->identifier == nullptr) {
        throw error::JSSyntaxError(
            formatException(L"Unexcepted token", filename, source, current),
            {filename, current.line, current.column});
      }
      specifier = declaration;
    }
    if (!specifier) {
      auto declaration = readFunctionDeclaration(filename, source, current)
                             .cast<FunctionDeclaration>();
      if (declaration != nullptr && declaration->identifier == nullptr) {
        throw error::JSSyntaxError(
            formatException(L"Unexcepted token", filename, source, current),
            {filename, current.line, current.column});
      }
      specifier = declaration;
    }
    if (!specifier) {
      specifier = readVariableDeclaration(filename, source, current);
    }
    if (specifier != nullptr) {
      specifier->addParent(node);
      node->items.push_back(specifier);
    } else {
      specifier = readExportAllSpecifier(filename, source, current);
      if (specifier != nullptr) {
        specifier->addParent(node);
        node->items.push_back(specifier);
      } else {
        skipInvisible(filename, source, current);
        auto token = readSymbolToken(filename, source, current);
        if (!token || !token->location.isEqual(source, L"{")) {
          throw error::JSSyntaxError(
              formatException(L"Unexcepted token", filename, source, current),
              {filename, current.line, current.column});
        }
        specifier = readExportSpecifier(filename, source, current);
        for (;;) {
          skipInvisible(filename, source, current);
          auto token = readSymbolToken(filename, source, current);
          if (!token) {
            throw error::JSSyntaxError(
                formatException(L"Unexcepted token", filename, source, current),
                {filename, current.line, current.column});
          }
          if (token->location.isEqual(source, L",")) {
            if (!specifier) {
              throw error::JSSyntaxError(
                  formatException(L"Unexcepted token", filename, source,
                                  current),
                  {filename, current.line, current.column});
            }
            specifier->addParent(node);
            node->items.push_back(specifier);
          } else if (token->location.isEqual(source, L"}")) {
            if (specifier != nullptr) {
              specifier->addParent(node);
              node->items.push_back(specifier);
            }
            break;
          } else {
            throw error::JSSyntaxError(
                formatException(L"Unexcepted token", filename, source, current),
                {filename, current.line, current.column});
          }
          specifier = readExportSpecifier(filename, source, current);
        }
      }
      auto backup = current;
      skipInvisible(filename, source, current);
      token = readIdentifierToken(filename, source, current);
      if (token != nullptr && token->location.isEqual(source, L"from")) {
        auto src = readStringLiteral(filename, source, current);
        if (!src) {
          throw error::JSSyntaxError(
              formatException(L"Unexcepted token", filename, source, current),
              {filename, current.line, current.column});
        }
        src->addParent(node);
        node->source = src;
        auto backup = current;
        skipInvisible(filename, source, current);
        token = readKeywordToken(filename, source, current);
        if (!token) {
          token = readIdentifierToken(filename, source, current);
        }
        if (token != nullptr && (token->location.isEqual(source, L"assert") ||
                                 token->location.isEqual(source, L"with"))) {
          skipInvisible(filename, source, current);
          token = readSymbolToken(filename, source, current);
          if (!token || !token->location.isEqual(source, L"{")) {
            throw error::JSSyntaxError(
                formatException(L"Unexcepted token", filename, source, current),
                {filename, current.line, current.column});
          }
          auto attribute = readImportAttriabue(filename, source, current);
          while (attribute != nullptr) {
            attribute->addParent(node);
            node->attributes.push_back(attribute);
            auto backup = current;
            skipInvisible(filename, source, current);
            token = readSymbolToken(filename, source, current);
            if (token == nullptr) {
              throw error::JSSyntaxError(
                  formatException(L"Unexcepted token", filename, source,
                                  current),
                  {filename, current.line, current.column});
            }
            if (token->location.isEqual(source, L"}")) {
              current = backup;
              break;
            }
            if (!token->location.isEqual(source, L",")) {
              throw error::JSSyntaxError(
                  formatException(L"Unexcepted token", filename, source,
                                  current),
                  {filename, current.line, current.column});
            }
            attribute = readImportAttriabue(filename, source, current);
          }
          skipInvisible(filename, source, current);
          token = readSymbolToken(filename, source, current);
          if (!token || !token->location.isEqual(source, L"}")) {
            throw error::JSSyntaxError(
                formatException(L"Unexcepted token", filename, source, current),
                {filename, current.line, current.column});
          }
        } else {
          current = backup;
        }
      } else if (node->items.size() &&
                 node->items[0]->type == NodeType::EXPORT_ALL) {
        throw error::JSSyntaxError(
            formatException(L"Unexcepted token", filename, source, current),
            {filename, current.line, current.column});
      } else {
        current = backup;
      }
    }
    node->location = getLocation(source, position, current);
    position = current;
    return node;
  }
  return nullptr;
}
std::wstring JSParser::toJSON(const std::wstring &filename,
                              const std::wstring &source,
                              common::AutoPtr<Node> node) {
  std::wstring type;
  switch (node->type) {
  case NodeType::PRIVATE_NAME:
    type = L"PRIVATE_NAME";
    break;
  case NodeType::LITERAL_REGEX:
    type = L"LITERAL_REGEX";
    break;
  case NodeType::LITERAL_NULL:
    type = L"LITERAL_NULL";
    break;
  case NodeType::LITERAL_STRING:
    type = L"LITERAL_STRING";
    break;
  case NodeType::LITERAL_BOOLEAN:
    type = L"LITERAL_BOOLEAN";
    break;
  case NodeType::LITERAL_NUMBER:
    type = L"LITERAL_NUMBER";
    break;
  case NodeType::LITERAL_COMMENT:
    type = L"LITERAL_COMMENT";
    break;
  case NodeType::LITERAL_MULTILINE_COMMENT:
    type = L"LITERAL_MULTILINE_COMMENT";
    break;
  case NodeType::LITERAL_UNDEFINED:
    type = L"LITERAL_UNDEFINED";
    break;
  case NodeType::LITERAL_IDENTITY:
    type = L"LITERAL_IDENTITY";
    break;
  case NodeType::LITERAL_TEMPLATE:
    type = L"LITERAL_TEMPLATE";
    break;
  case NodeType::LITERAL_BIGINT:
    type = L"LITERAL_BIGINT";
    break;
  case NodeType::THIS:
    type = L"THIS";
    break;
  case NodeType::SUPER:
    type = L"SUPER";
    break;
  case NodeType::PROGRAM:
    type = L"PROGRAM";
    break;
  case NodeType::STATEMENT_EMPTY:
    type = L"STATEMENT_EMPTY";
    break;
  case NodeType::STATEMENT_BLOCK:
    type = L"STATEMENT_BLOCK";
    break;
  case NodeType::STATEMENT_DEBUGGER:
    type = L"STATEMENT_DEBUGGER";
    break;
  case NodeType::STATEMENT_RETURN:
    type = L"STATEMENT_RETURN";
    break;
  case NodeType::STATEMENT_YIELD:
    type = L"STATEMENT_YIELD";
    break;
  case NodeType::STATEMENT_LABEL:
    type = L"STATEMENT_LABEL";
    break;
  case NodeType::STATEMENT_BREAK:
    type = L"STATEMENT_BREAK";
    break;
  case NodeType::STATEMENT_CONTINUE:
    type = L"STATEMENT_CONTINUE";
    break;
  case NodeType::STATEMENT_IF:
    type = L"STATEMENT_IF";
    break;
  case NodeType::STATEMENT_SWITCH:
    type = L"STATEMENT_SWITCH";
    break;
  case NodeType::STATEMENT_SWITCH_CASE:
    type = L"STATEMENT_SWITCH_CASE";
    break;
  case NodeType::STATEMENT_THROW:
    type = L"STATEMENT_THROW";
    break;
  case NodeType::STATEMENT_TRY:
    type = L"STATEMENT_TRY";
    break;
  case NodeType::STATEMENT_TRY_CATCH:
    type = L"STATEMENT_TRY_CATCH";
    break;
  case NodeType::STATEMENT_WHILE:
    type = L"STATEMENT_WHILE";
    break;
  case NodeType::STATEMENT_DO_WHILE:
    type = L"STATEMENT_DO_WHILE";
    break;
  case NodeType::STATEMENT_FOR:
    type = L"STATEMENT_FOR";
    break;
  case NodeType::STATEMENT_FOR_IN:
    type = L"STATEMENT_FOR_IN";
    break;
  case NodeType::STATEMENT_FOR_OF:
    type = L"STATEMENT_FOR_OF";
    break;
  case NodeType::STATEMENT_FOR_AWAIT_OF:
    type = L"STATEMENT_FOR_AWAIT_OF";
    break;
  case NodeType::VARIABLE_DECLARATION:
    type = L"VARIABLE_DECLARATION";
    break;
  case NodeType::VARIABLE_DECLARATOR:
    type = L"VARIABLE_DECLARATOR";
    break;
  case NodeType::DECORATOR:
    type = L"DECORATOR";
    break;
  case NodeType::DIRECTIVE:
    type = L"DIRECTIVE";
    break;
  case NodeType::INTERPRETER_DIRECTIVE:
    type = L"INTERPRETER_DIRECTIVE";
    break;
  case NodeType::OBJECT_PROPERTY:
    type = L"OBJECT_PROPERTY";
    break;
  case NodeType::OBJECT_METHOD:
    type = L"OBJECT_METHOD";
    break;
  case NodeType::OBJECT_ACCESSOR:
    type = L"OBJECT_ACCESSOR";
    break;
  case NodeType::EXPRESSION_UNARY:
    type = L"EXPRESSION_UNARY";
    break;
  case NodeType::EXPRESSION_UPDATE:
    type = L"EXPRESSION_UPDATE";
    break;
  case NodeType::EXPRESSION_BINARY:
    type = L"EXPRESSION_BINARY";
    break;
  case NodeType::EXPRESSION_MEMBER:
    type = L"EXPRESSION_MEMBER";
    break;
  case NodeType::EXPRESSION_OPTIONAL_MEMBER:
    type = L"EXPRESSION_OPTIONAL_MEMBER";
    break;
  case NodeType::EXPRESSION_COMPUTED_MEMBER:
    type = L"EXPRESSION_COMPUTED_MEMBER";
    break;
  case NodeType::EXPRESSION_OPTIONAL_COMPUTED_MEMBER:
    type = L"EXPRESSION_OPTIONAL_COMPUTED_MEMBER";
    break;
  case NodeType::EXPRESSION_CONDITION:
    type = L"EXPRESSION_CONDITION";
    break;
  case NodeType::EXPRESSION_CALL:
    type = L"EXPRESSION_CALL";
    break;
  case NodeType::EXPRESSION_OPTIONAL_CALL:
    type = L"EXPRESSION_OPTIONAL_CALL";
    break;
  case NodeType::EXPRESSION_NEW:
    type = L"EXPRESSION_NEW";
    break;
  case NodeType::EXPRESSION_DELETE:
    type = L"EXPRESSION_DELETE";
    break;
  case NodeType::EXPRESSION_AWAIT:
    type = L"EXPRESSION_AWAIT";
    break;
  case NodeType::EXPRESSION_VOID:
    type = L"EXPRESSION_VOID";
    break;
  case NodeType::EXPRESSION_TYPEOF:
    type = L"EXPRESSION_TYPEOF";
    break;
  case NodeType::EXPRESSION_GROUP:
    type = L"EXPRESSION_GROUP";
    break;
  case NodeType::EXPRESSION_ASSIGMENT:
    type = L"EXPRESSION_ASSIGMENT";
    break;
  case NodeType::EXPRESSION_REST:
    type = L"EXPRESSION_REST";
    break;
  case NodeType::PATTERN_REST_ITEM:
    type = L"PATTERN_REST_ITEM";
    break;
  case NodeType::PATTERN_OBJECT:
    type = L"PATTERN_OBJECT";
    break;
  case NodeType::PATTERN_OBJECT_ITEM:
    type = L"PATTERN_OBJECT_ITEM";
    break;
  case NodeType::PATTERN_ARRAY:
    type = L"PATTERN_ARRAY";
    break;
  case NodeType::PATTERN_ARRAY_ITEM:
    type = L"PATTERN_ARRAY_ITEM";
    break;
  case NodeType::CLASS_METHOD:
    type = L"CLASS_METHOD";
    break;
  case NodeType::CLASS_PROPERTY:
    type = L"CLASS_PROPERTY";
    break;
  case NodeType::CLASS_ACCESSOR:
    type = L"CLASS_ACCESSOR";
    break;
  case NodeType::STATIC_BLOCK:
    type = L"STATIC_BLOCK";
    break;
  case NodeType::IMPORT_DECLARATION:
    type = L"IMPORT_DECLARATION";
    break;
  case NodeType::IMPORT_SPECIFIER:
    type = L"IMPORT_SPECIFIER";
    break;
  case NodeType::IMPORT_DEFAULT:
    type = L"IMPORT_DEFAULT";
    break;
  case NodeType::IMPORT_NAMESPACE:
    type = L"IMPORT_NAMESPACE";
    break;
  case NodeType::IMPORT_ATTARTUBE:
    type = L"IMPORT_ATTARTUBE";
    break;
  case NodeType::EXPORT_DECLARATION:
    type = L"EXPORT_DECLARATION";
    break;
  case NodeType::EXPORT_DEFAULT:
    type = L"EXPORT_DEFAULT";
    break;
  case NodeType::EXPORT_SPECIFIER:
    type = L"EXPORT_SPECIFIER";
    break;
  case NodeType::EXPORT_ALL:
    type = L"EXPORT_ALL";
    break;
  case NodeType::DECLARATION_ARROW_FUNCTION:
    type = L"DECLARATION_ARROW_FUNCTION";
    break;
  case NodeType::DECLARATION_FUNCTION:
    type = L"DECLARATION_FUNCTION";
    break;
  case NodeType::DECLARATION_PARAMETER:
    type = L"DECLARATION_PARAMETER";
    break;
  case NodeType::DECLARATION_OBJECT:
    type = L"DECLARATION_OBJECT";
    break;
  case NodeType::DECLARATION_ARRAY:
    type = L"DECLARATION_ARRAY";
    break;
  case NodeType::DECLARATION_CLASS:
    type = L"DECLARATION_CLASS";
    break;
    break;
  }
  std::wstring result = L"{";
  result += fmt::format(L"\"type\":\"{}\"", type);
  result += L",\"children\":[";
  size_t index = 0;
  for (auto &child : node->children) {
    result += toJSON(filename, source, child);
    if (index != node->children.size() - 1) {
      result += L",";
    }
    index++;
  }
  result += L"]";
  result += fmt::format(L",\"id\":{}", node->id);
  if (node->scope != nullptr) {
    result += fmt::format(L",\"scope\":{{\"node\":{},\"declarations\":[",
                          node->scope->node->id);
    size_t index = 0;
    for (auto &declaration : node->scope->declarations) {
      result += L"{";
      std::wstring type;
      switch (declaration.type) {
      case Declaration::TYPE::UNDEFINED:
        type = L"undefined";
        break;
      case Declaration::TYPE::UNINITIALIZED:
        type = L"uninitialized";
        break;
      case Declaration::TYPE::FUNCTION:
        type = L"function";
        break;
      case Declaration::TYPE::ARGUMENT:
        type = L"argument";
        break;
      case Declaration::TYPE::CATCH:
        type = L"catch";
        break;
      }
      result += fmt::format(
          L"\"identifier\":\"{}\",\"isConst\":{},\"node\":{},\"type\":\"{}\"",
          declaration.name, declaration.isConst, declaration.node->id, type);
      result += L"}";
      if (index != node->scope->declarations.size() - 1) {
        result += L",";
      }
      index++;
    }
    result += L"]";
    result += L",\"bindings\":[";
    index = 0;
    for (auto &binding : node->scope->bindings) {
      result += L"{";
      if (binding.declaration) {
        result += fmt::format(L"\"name\":\"{}\"", binding.declaration->name);
        result += fmt::format(L",\"node\":{},", binding.declaration->node->id);
      }
      result += L"\"references\":[";
      size_t vindex = 0;
      for (auto &node : binding.references) {
        result += fmt::format(L"{}", node->id);
        if (vindex != binding.references.size() - 1) {
          result += L",";
        }
        vindex++;
      }
      result += L"]";
      result += L"}";
      if (index != node->scope->bindings.size() - 1) {
        result += L",";
      }
      index++;
    }
    result += L"]";
    result += L"}";
  }
  std::wstring src = node->location.getSource(source);
  std::wstring dst;
  for (auto &ch : src) {
    if (ch == L'\\') {
      dst += L"\\\\";
    } else if (ch == L'\"') {
      dst += L"\\\"";
    } else if (ch == L'\n') {
      dst += L"\\n";
    } else if (ch == L'\r') {
      dst += L"\\r";
    } else {
      dst += ch;
    }
  }
  result += fmt::format(L",\"location\":{{\"source\":\"{}\"}}", dst);
  result += L"}";
  return result;
}