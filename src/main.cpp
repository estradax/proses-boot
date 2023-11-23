#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

static inline void rtrim(std::string &s) {
  s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
    return !std::isspace(ch);
  }).base(), s.end());
}

class Shell {
public:
  static void MainLoop() {
    Shell shell;

    std::string input;

    while (shell.IsRunning()) {
      std::cout << ">> ";
      std::getline(std::cin, input);
      
      auto args = shell.Tokenize(input);
      shell.ParseArgs(args);
    }
  }

private:
  Shell() : is_running_{true} {}

  bool IsRunning() const {
    return is_running_;
  }

  std::vector<std::string> Tokenize(std::string s) const {
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

  void ParseArgs(const std::vector<std::string> &args) {
    if (args.size() < 1) {
      return;
    }

    auto cmd = args[0];
    if (cmd == "exit") {
      is_running_ = false;
      return;
    } 

    std::cout << "command not found: " << cmd << '\n';
    return;
  }

private:
  bool is_running_;
};

int main() {
  Shell::MainLoop();

  return 0;
}
