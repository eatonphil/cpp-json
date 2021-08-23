#include "json.hpp"

#include <iostream>

int main(int argc, char *argv[]) {
  if (argc == 1) {
    std::cerr << "Expected JSON input argument to parse" << std::endl;
    return 1;
  }

  std::string in{argv[1]};

  auto [ast, error] = json::parse(in);
  if (error.size()) {
    std::cerr << error << std::endl;
    return 1;
  }

  std::cout << json::deparse(ast);
}
