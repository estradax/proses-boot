#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

static inline void rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

std::vector<std::string> ParseInput(std::string s) {
  std::vector<std::string> args;

  rtrim(s);

  std::size_t pos = 0;
  while ((pos = s.find(' ')) != std::string::npos) {
      args.push_back(s.substr(0, pos));
      s.erase(0, pos + 1);
  }
  args.push_back(s);

  return args;
}

bool ParseArgs(const std::vector<std::string> &args) {
  if (args.size() < 1) {
    return true;
  }

  auto cmd = args[0];
  if (cmd == "exit") {
    return false;
  } 

  std::cout << "command not found: " << cmd << '\n';
  return true;
}

int main() {
  std::string input;

  auto running = true;

  while (running) {
    std::cout << ">> ";
    std::getline(std::cin, input);
    
    auto args = ParseInput(input);
    running = ParseArgs(args);
  }

  return 0;
}
