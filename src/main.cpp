#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <algorithm>
#include <unordered_map>

class Shell;

class Command {
public:
  virtual void Execute(Shell&) = 0;
};

class Shell {
public:
  static void MainLoop();

  void Shutdown();

private:
  Shell();

  bool IsRunning() const;
  std::vector<std::string> Tokenize(std::string);
  void ParseArgs(const std::vector<std::string> &);
  bool ContainsCommand(const std::string &);

private:
  bool is_running_;
  std::unordered_map<std::string, std::unique_ptr<Command>> commands_;
};

class ExitCommand : public Command {
public:
  virtual void Execute(Shell &);
};

class ListCommand : public Command {
public:
  virtual void Execute(Shell &);
};

class ClearCommand : public Command {
public:
  virtual void Execute(Shell &);
};

void ExitCommand::Execute(Shell &shell) {
  shell.Shutdown();
}

void ListCommand::Execute(Shell &shell) {
  std::cout << "unimplemented\n";
}

void ClearCommand::Execute(Shell &shell) {
  std::system("clear");
}

void Shell::MainLoop() {
  Shell shell;

  std::string input;

  while (shell.IsRunning()) {
    std::cout << ">> ";
    std::getline(std::cin, input);
    
    auto args = shell.Tokenize(input);
    shell.ParseArgs(args);
  }
}

void Shell::Shutdown() {
  is_running_ = false;
}

Shell::Shell() : is_running_{true} {
  commands_.insert({"exit", std::make_unique<ExitCommand>()});
  commands_.insert({"ls", std::make_unique<ListCommand>()});
  commands_.insert({"clear", std::make_unique<ClearCommand>()});
}

bool Shell::IsRunning() const {
  return is_running_;
}

static inline void rtrim(std::string &s) {
  s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
    return !std::isspace(ch);
  }).base(), s.end());
}

std::vector<std::string> Shell::Tokenize(std::string s) {
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

void Shell::ParseArgs(const std::vector<std::string> &args) {
  if (args.size() < 1) {
    return;
  }

  auto cmd = args[0];
  if (ContainsCommand(cmd)) {
    commands_[cmd]->Execute(*this);
    return;
  }

  std::cout << "command not found: " << cmd << '\n';
}

bool Shell::ContainsCommand(const std::string &cmd) {
  return commands_.find(cmd) != commands_.end();
}

int main() {
  Shell::MainLoop();

  return 0;
}
