#include "json.hpp"

#include <iostream>
#include <iterator>
#include <sstream>

namespace json {
std::ostream &operator<<(std::ostream &os, JSONTokenType jtt) {
  switch (jtt) {
  case JSONTokenType::String:
    return os << "String";
  case JSONTokenType::Number:
    return os << "Number";
  case JSONTokenType::Syntax:
    return os << "Syntax";
  case JSONTokenType::Boolean:
    return os << "Boolean";
  case JSONTokenType::Null:
    return os << "Null";
  }
}

std::string format_error(std::string base, std::string source, int index) {
  std::ostringstream s;
  int counter = 0;
  int line = 1;
  int column = 0;
  std::string lastline = "";
  std::string whitespace = "";
  for (auto c : source) {
    if (counter == index) {
      break;
    }

    if (c == '\n') {
      line++;
      column = 0;
      lastline = "";
      whitespace = "";
    } else if (c == '\t') {
      column++;
      lastline += "  ";
      whitespace += "  ";
    } else {
      column++;
      lastline += c;
      whitespace += " ";
    }

    counter++;
  }

  // Continue accumulating the lastline for debugging
  while (counter < source.size()) {
    auto c = source[counter];
    if (c == '\n') {
      break;
    }
    lastline += c;
    counter++;
  }

  s << base << " at line " << line << ", column " << column << std::endl;
  s << lastline << std::endl;
  s << whitespace << "^";

  return s.str();
}

int lex_whitespace(std::string raw_json, int index) {
  while (std::isspace(raw_json[index])) {
    if (index == raw_json.length()) {
      break;
    }

    index++;
  }

  return index;
}

std::tuple<JSONToken, int, std::string> lex_syntax(std::string raw_json,
                                                   int index) {
  JSONToken token{"", JSONTokenType::Syntax, index};
  std::string value = "";
  auto c = raw_json[index];
  if (c == '[' || c == ']' || c == '{' || c == '}' || c == ':' || c == ',') {
    token.value += c;
    index++;
  }

  return {token, index, ""};
}

std::tuple<JSONToken, int, std::string> lex_string(std::string raw_json,
                                                   int original_index) {
  int index = original_index;
  JSONToken token{"", JSONTokenType::String, index};
  std::string value = "";
  auto c = raw_json[index];
  if (c != '"') {
    return {token, original_index, ""};
  }
  index++;

  // TODO: handle nested quotes
  while (c = raw_json[index], c != '"') {
    if (index == raw_json.length()) {
      return {
          token, index,
          format_error("Unexpected EOF while lexing string", raw_json, index)};
    }

    token.value += c;
    index++;
  }
  index++;

  return {token, index, ""};
}

std::tuple<JSONToken, int, std::string> lex_number(std::string raw_json,
                                                   int original_index) {
  int index = original_index;
  JSONToken token = {"", JSONTokenType::Number, index};
  std::string value = "";
  // TODO: handle not just integers
  while (true) {
    if (index == raw_json.length()) {
      break;
    }

    auto c = raw_json[index];
    if (!(c >= '0' && c <= '9')) {
      break;
    }

    token.value += c;
    index++;
  }

  return {token, index, ""};
}

std::tuple<JSONToken, int, std::string> lex_keyword(std::string raw_json,
                                                    std::string keyword,
                                                    JSONTokenType type,
                                                    int original_index) {
  int index = original_index;
  JSONToken token{"", type, index};
  while (keyword[index - original_index] == raw_json[index]) {
    if (index == raw_json.length()) {
      break;
    }

    index++;
  }

  if (index - original_index == keyword.length()) {
    token.value = keyword;
  }
  return {token, index, ""};
}

std::tuple<JSONToken, int, std::string> lex_null(std::string raw_json,
                                                 int index) {
  return lex_keyword(raw_json, "null", JSONTokenType::Null, index);
}

std::tuple<JSONToken, int, std::string> lex_true(std::string raw_json,
                                                 int index) {
  return lex_keyword(raw_json, "true", JSONTokenType::Boolean, index);
}

std::tuple<JSONToken, int, std::string> lex_false(std::string raw_json,
                                                  int index) {
  return lex_keyword(raw_json, "false", JSONTokenType::Boolean, index);
}

std::tuple<std::vector<JSONToken>, std::string> lex(std::string raw_json) {
  std::vector<JSONToken> tokens;
  auto original_copy = std::make_shared<std::string>(raw_json);
  auto generic_lexers = {lex_syntax, lex_string, lex_number,
                         lex_null,   lex_true,   lex_false};
  for (int i = 0; i < raw_json.length(); i++) {
    if (auto new_index = lex_whitespace(raw_json, i); i != new_index) {
      i = new_index - 1;
      continue;
    }

    auto found = false;
    for (auto lexer : generic_lexers) {
      if (auto [token, new_index, error] = lexer(raw_json, i); i != new_index) {
        if (error.length()) {
          return {{}, error};
        }

        token.full_source = original_copy;
        tokens.push_back(token);
        i = new_index - 1;
        found = true;
        break;
      }
    }

    if (found) {
      continue;
    }

    return {{}, format_error("Unable to lex", raw_json, i)};
  }

  return {tokens, ""};
}

std::string format_parse_error(std::string base, JSONToken token) {
  std::ostringstream s;
  s << "Unexpected token '" << token.value << "', type '" << token.type
    << "', index ";
  s << std::endl << base;
  return format_error(s.str(), *token.full_source, token.location);
}

std::tuple<std::vector<JSONValue>, int, std::string>
parse_array(std::vector<JSONToken> tokens, int index) {
  std::vector<JSONValue> children = {};
  while (index < tokens.size()) {
    auto t = tokens[index];
    if (t.type == JSONTokenType::Syntax) {
      if (t.value == "]") {
        return {children, index + 1, ""};
      }

      if (t.value == ",") {
        index++;
        t = tokens[index];
      } else if (children.size() > 0) {
        return {{},
                index,
                format_parse_error("Expected comma after element in array", t)};
      }
    }

    auto [child, new_index, error] = parse(tokens, index);
    if (error.size()) {
      return {{}, index, error};
    }

    children.push_back(child);
    index = new_index;
  }

  return {
      {},
      index,
      format_parse_error("Unexpected EOF while parsing array", tokens[index])};
}

std::tuple<std::map<std::string, JSONValue>, int, std::string>
parse_object(std::vector<JSONToken> tokens, int index) {
  std::map<std::string, JSONValue> values = {};
  while (index < tokens.size()) {
    auto t = tokens[index];
    if (t.type == JSONTokenType::Syntax) {
      if (t.value == "}") {
        return {values, index + 1, ""};
      }

      if (t.value == ",") {
        index++;
        t = tokens[index];
      } else if (values.size() > 0) {
        return {
            {},
            index,
            format_parse_error("Expected comma after element in object", t)};
      } else {
        return {{},
                index,
                format_parse_error(
                    "Expected key-value pair or closing brace in object", t)};
      }
    }

    auto [key, new_index, error] = parse(tokens, index);
    if (error.size()) {
      return {{}, index, error};
    }

    if (key.type != JSONValueType::String) {
      return {
          {}, index, format_parse_error("Expected string key in object", t)};
    }
    index = new_index;
    t = tokens[index];

    if (!(t.type == JSONTokenType::Syntax && t.value == ":")) {
      return {{},
              index,
              format_parse_error("Expected colon after key in object", t)};
    }
    index++;
    t = tokens[index];

    auto [value, new_index1, error1] = parse(tokens, index);
    if (error1.size()) {
      return {{}, index, error1};
    }

    values[key.string.value()] = value;
    index = new_index1;
  }

  return {values, index + 1, ""};
}

std::tuple<JSONValue, int, std::string> parse(std::vector<JSONToken> tokens,
                                              int index) {
  auto token = tokens[index];
  switch (token.type) {
  case JSONTokenType::Number: {
    auto n = std::stod(token.value);
    return {JSONValue{.number = n, .type = JSONValueType::Number}, index + 1,
            ""};
  }
  case JSONTokenType::Boolean:
    return {JSONValue{.boolean = token.value == "true",
                      .type = JSONValueType::Boolean},
            index + 1, ""};
  case JSONTokenType::Null:
    return {JSONValue{.type = JSONValueType::Null}, index + 1, ""};
  case JSONTokenType::String:
    return {JSONValue{.string = token.value, .type = JSONValueType::String},
            index + 1, ""};
  case JSONTokenType::Syntax:
    if (token.value == "[") {
      auto [array, new_index, error] = parse_array(tokens, index + 1);
      return {JSONValue{.array = array, .type = JSONValueType::Array},
              new_index, error};
    }

    if (token.value == "{") {
      auto [object, new_index, error] = parse_object(tokens, index + 1);
      return {JSONValue{.object = std::optional(object),
                        .type = JSONValueType::Object},
              new_index, error};
    }
  }

  return {{}, index, format_parse_error("Failed to parse", token)};
}

std::string deparse(JSONValue v, std::string whitespace) {
  switch (v.type) {
  case JSONValueType::String:
    return "\"" + v.string.value() + "\"";
  case JSONValueType::Boolean:
    return (v.boolean.value() ? "true" : "false");
  case JSONValueType::Number:
    return std::to_string(v.number.value());
  case JSONValueType::Null:
    return "null";
  case JSONValueType::Array: {
    std::string s = "[\n";
    auto a = v.array.value();
    for (int i = 0; i < a.size(); i++) {
      auto value = a[i];
      s += whitespace + "  " + deparse(value, whitespace + "  ");
      if (i < a.size() - 1) {
        s += ",";
      }

      s += "\n";
    }

    return s + whitespace + "]";
  }
  case JSONValueType::Object: {
    std::string s = "{\n";
    auto values = v.object.value();
    auto i = 0;
    for (auto const &[key, value] : values) {
      s += whitespace + "  " + "\"" + key +
           "\": " + deparse(value, whitespace + "  ");

      if (i < values.size() - 1) {
        s += ",";
      }

      s += "\n";
      i++;
    }

    return s + whitespace + "}";
  }
  }
}
} // namespace json
