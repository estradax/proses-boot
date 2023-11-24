#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <algorithm>
#include <unordered_map>

class Shell;

class FileOrDirectory {
public:
  static FileOrDirectory CreateDirectory(const std::string &);

  static FileOrDirectory CreateFile(const std::string &);

  void Add(const FileOrDirectory &);

  std::string Name() const;

  bool IsDirectory() const;

  std::vector<FileOrDirectory> Files() const;

private:
  FileOrDirectory(const std::string &, bool);

private:
  std::vector<FileOrDirectory> files_;

  std::string name_;
  bool is_directory_;
};

FileOrDirectory::FileOrDirectory(const std::string &name, bool is_directory)
  : name_{name}, is_directory_{is_directory} {}

FileOrDirectory FileOrDirectory::CreateDirectory(const std::string &name) {
  return {name, true};
}

FileOrDirectory FileOrDirectory::CreateFile(const std::string &name) {
  return {name, false};
}

void FileOrDirectory::Add(const FileOrDirectory &file) {
  files_.push_back(file);
}

std::string FileOrDirectory::Name() const {
  return name_;
}

bool FileOrDirectory::IsDirectory() const {
  return is_directory_;
}

std::vector<FileOrDirectory> FileOrDirectory::Files() const {
  return files_;
}

class FileSystem {
public:
  static FileSystem Populate();

  void Add(const FileOrDirectory &);

  std::vector<FileOrDirectory> Root() {
    return root_;
  }

private:
  FileSystem(const std::vector<FileOrDirectory> &);

private:
  std::vector<FileOrDirectory> root_;
};

FileSystem::FileSystem(const std::vector<FileOrDirectory> &root)
  : root_{root} {}

FileSystem FileSystem::Populate() {
  auto tmp = FileOrDirectory::CreateDirectory("tmp");
  tmp.Add(FileOrDirectory::CreateFile("tmp_file.txt"));

  auto sys = FileOrDirectory::CreateDirectory("sys");

  std::vector<FileOrDirectory> root {
    tmp, sys
  };

  return {root};
}

void FileSystem::Add(const FileOrDirectory &file) {
  root_.push_back(file);
}

class Command {
public:
  virtual void Execute(Shell&) = 0;
};

class Shell {
public:
  static void MainLoop();

  void Shutdown();

  std::vector<std::string> Cwd() {
    return cwd_;
  }

private:
  Shell();

  bool IsRunning() const;
  std::vector<std::string> Tokenize(std::string);
  void ParseArgs(const std::vector<std::string> &);
  bool ContainsCommand(const std::string &);

private:
  std::vector<std::string> cwd_;

  bool is_running_;
  std::unordered_map<std::string, std::unique_ptr<Command>> commands_;
};

class ExitCommand : public Command {
public:
  virtual void Execute(Shell &);
};

class ListCommand : public Command {
public:
  ListCommand(const FileSystem &);

  virtual void Execute(Shell &);

private:
  void PrintList(const std::vector<FileOrDirectory> &);

private:
  FileSystem fs_;
};

ListCommand::ListCommand(const FileSystem &fs) : fs_{fs} {}

void ListCommand::PrintList(const std::vector<FileOrDirectory> &files) {
  for (const auto &f : files) {
    std::cout << f.Name() << ' ';
  }

  std::cout << '\n';
}

class ClearCommand : public Command {
public:
  virtual void Execute(Shell &);
};

void ExitCommand::Execute(Shell &shell) {
  shell.Shutdown();
}

void ListCommand::Execute(Shell &shell) {
  // you are on the root
  if (shell.Cwd().size() == 1) {
    PrintList(fs_.Root());
    return;
  }

  auto files = fs_.Root();
  auto cwd = shell.Cwd();

  for (std::size_t i = 1; i < cwd.size(); i++) {
    for (const auto &f : files) {
      if (f.IsDirectory() && f.Name() == cwd[i]) {
        files = f.Files();
      }
    }
  }

  PrintList(files);
}

void ClearCommand::Execute(Shell &shell) {
#if defined(_WIN32)
  std::system("cls");
#else
  std::system("clear");
#endif
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

Shell::Shell() : is_running_{true}, cwd_{"/"} {
  auto fs = FileSystem::Populate();

  commands_.insert({"exit", std::make_unique<ExitCommand>()});
  commands_.insert({"ls", std::make_unique<ListCommand>(fs)});
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
