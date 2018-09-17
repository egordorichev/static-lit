#include <iostream>
#include <string>
#include <fstream>

#include "lit_context.hpp"

void execute_file(const std::string& file_path);
void execute_in_repl(LitContext& context, const std::string& code);

int main(int argc, char** argv) {
  LitContext repl_context;

  if (argc == 2 && std::string(argv[1]).find('.') != std::string::npos) {
    std::string file_path = std::string(argv[1]);

    if (file_path.size() > 3 && file_path.rfind(".lit") != std::string::npos) {
      execute_file(file_path);
    } else {
      printf("Error: %s is not a Lit file\n", file_path.c_str());
    }
  } else if (argc == 1) {
    for(;;) {
      std::string input;

      printf("> ");

      std::cin >> input;

      if (input == ".exit") {
        break;
      }

      execute_in_repl(repl_context, input);
    }
  } else {
    printf("Usage: lit [path]\n");
  }

  return 0;
}

void execute_file(const std::string& file_path) {
  LitContext context;
  std::ifstream file_stream(file_path);

  if (!file_stream.good()) {
    printf("File %s doesn't exist or can't be read\n", file_path.c_str());

    return;
  }

  std::string code((std::istreambuf_iterator<char>(file_stream)), std::istreambuf_iterator<char>());

  context.execute(code.c_str());
}

void execute_in_repl(LitContext& context, const std::string& code) {
  context.execute(code.c_str());
}