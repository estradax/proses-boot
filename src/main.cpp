#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <functional>
#include <thread>
#include <chrono>

class Shell;

class Argument {
public:
  bool HasParameters() const;

  std::string ProgramName() const;
  std::vector<std::string> Parameters() const;

  void SetProgramName(const std::string &);
  void SetParameters(const std::vector<std::string> &);
  void SetOptions(const std::vector<std::string> &);

private:
  std::string program_name_;
  std::vector<std::string> options_;
  std::vector<std::string> parameters_;
};

bool Argument::HasParameters() const {
  return parameters_.size() > 0;
}

std::string Argument::ProgramName() const {
  return program_name_;
}

std::vector<std::string> Argument::Parameters() const {
  return parameters_;
}

void Argument::SetProgramName(const std::string &program_name) {
  program_name_ = program_name;
}

void Argument::SetOptions(const std::vector<std::string> &options) {
  options_ = options;
}

void Argument::SetParameters(const std::vector<std::string> &parameters) {
  parameters_ = parameters;
}

class FileOrDirectory {
public:
  static FileOrDirectory CreateDirectory(const std::string &);

  static FileOrDirectory CreateFile(const std::string &);

  void Add(const FileOrDirectory &);

  std::string Name() const;

  bool IsDirectory() const;

  std::shared_ptr<std::vector<FileOrDirectory>> Files() const;

private:
  FileOrDirectory(const std::string &, bool, const std::shared_ptr<std::vector<FileOrDirectory>> &);

private:
  std::shared_ptr<std::vector<FileOrDirectory>> files_;

  std::string name_;
  bool is_directory_;
};

FileOrDirectory::FileOrDirectory(const std::string &name, bool is_directory, const std::shared_ptr<std::vector<FileOrDirectory>> &files)
  : name_{name}, is_directory_{is_directory}, files_{files} {}

FileOrDirectory FileOrDirectory::CreateDirectory(const std::string &name) {
  return {name, true, std::make_shared<std::vector<FileOrDirectory>>()};
}

FileOrDirectory FileOrDirectory::CreateFile(const std::string &name) {
  return {name, false, std::make_shared<std::vector<FileOrDirectory>>()};
}

void FileOrDirectory::Add(const FileOrDirectory &file) {
  files_->push_back(file);
}

std::string FileOrDirectory::Name() const {
  return name_;
}

bool FileOrDirectory::IsDirectory() const {
  return is_directory_;
}

std::shared_ptr<std::vector<FileOrDirectory>> FileOrDirectory::Files() const {
  return files_;
}

class FileSystem {
public:
  void Populate();

  void Add(const FileOrDirectory &);

  void TraverseDirectory(const std::vector<std::string> &, const std::function<void(std::shared_ptr<std::vector<FileOrDirectory>>)> &func);

  std::shared_ptr<std::vector<FileOrDirectory>> Root() {
    return root_;
  }

private:
  std::shared_ptr<std::vector<FileOrDirectory>> root_;
};

void FileSystem::Populate() {
  auto tmp = FileOrDirectory::CreateDirectory("tmp");
  tmp.Add(FileOrDirectory::CreateFile("file.txt"));
  tmp.Add(FileOrDirectory::CreateFile("file2.txt"));

  auto sys = FileOrDirectory::CreateDirectory("sys");

  auto usr = FileOrDirectory::CreateDirectory("usr");
  usr.Add(FileOrDirectory::CreateDirectory("bin"));

  root_ = std::make_shared<std::vector<FileOrDirectory>>();
  root_->push_back(tmp);
  root_->push_back(sys);
  root_->push_back(usr);
}

void FileSystem::Add(const FileOrDirectory &file) {
  root_->push_back(file);
}

void FileSystem::TraverseDirectory(const std::vector<std::string> &cwd, const std::function<void(std::shared_ptr<std::vector<FileOrDirectory>>)> &func) {
  if (cwd.size() == 1) {
    func(root_);
    return;
  }

  auto files = root_;

  for (std::size_t i = 1; i < cwd.size(); i++) {
    for (const auto &f : *files) {
      if (f.IsDirectory() && f.Name() == cwd[i]) {
        files = f.Files();
      }
    }
  }

  func(files);
}

class Command {
public:
  virtual void Execute(Shell&) = 0;
};

class User {
public:
  User() = default;

  static User Create(const std::string &, const std::string &);
  static User CreateSuperuser(const std::string &, const std::string &);

  std::string Login() const;
  std::string Password() const;
  bool IsSuperuser() const;

private:
  User(const std::string &, const std::string &, bool);

private:
  std::string login_;
  std::string password_;

  bool is_superuser_;
};

User::User(const std::string &login, const std::string &password, bool is_superuser)
  : login_{login}, password_{password}, is_superuser_{is_superuser} {}

User User::Create(const std::string &login, const std::string &password) {
  return {login, password, false};
}

User User::CreateSuperuser(const std::string &login, const std::string &password) {
  return {login, password, true};
}

std::string User::Login() const {
  return login_;
}

std::string User::Password() const {
  return password_;
}

bool User::IsSuperuser() const {
  return is_superuser_;
}

class Shell {
public:
  static void MainLoop();

  void DisplayPrompt();

  bool IsAuthenticating();

  void Shutdown();

  std::vector<std::string> Cwd() {
    return cwd_;
  }

  Argument Arg() const;
  User CurrentUser() const;

private:
  Shell();

  std::vector<std::string> Tokenize(std::string);
  void ParseArgs(const std::vector<std::string> &);
  bool ContainsCommand(const std::string &);

  bool IsRunning() const;

private:
  std::vector<User> users_;
  User current_user_;

  std::vector<std::string> cwd_;

  bool is_running_;
  std::unordered_map<std::string, std::unique_ptr<Command>> commands_;
  Argument arg_;
};

void Shell::DisplayPrompt() {
  std::cout << current_user_.Login() << '@' << "desktop:";
  for (std::size_t i = 0; i < cwd_.size(); i++) {
    if (i == 0 || i == cwd_.size() - 1) {
      std::cout << cwd_[i];
    } else {
      std::cout << cwd_[i] << '/';
    }
  }

  std::cout << "$ ";
}

User Shell::CurrentUser() const {
  return current_user_;
}

bool Shell::IsAuthenticating() {
  std::string login;
  std::string password;

  std::cout << "login: ";
  std::getline(std::cin, login);

  std::cout << "password: ";
  std::getline(std::cin, password);

  for (const auto &user : users_) {
    if (user.Login() == login && user.Password() == password) {
      current_user_ = user;
      return true;
    }
  }

  std::cout << "invalid login\n";
  return false;
}

class ExitCommand : public Command {
public:
  virtual void Execute(Shell &);
};

class ListCommand : public Command {
public:
  ListCommand(const std::shared_ptr<FileSystem> &);

  virtual void Execute(Shell &);

private:
  std::shared_ptr<FileSystem> fs_;
};

class RemoveCommand : public Command {
public:
  RemoveCommand(const std::shared_ptr<FileSystem> &);

  virtual void Execute(Shell &);

private:
  std::shared_ptr<FileSystem> fs_;
};

RemoveCommand::RemoveCommand(const std::shared_ptr<FileSystem> &fs) : fs_{fs} {}

ListCommand::ListCommand(const std::shared_ptr<FileSystem> &fs) : fs_{fs} {}

class MakeDirectoryCommand : public Command {
public:
  MakeDirectoryCommand(const std::shared_ptr<FileSystem> &);

  virtual void Execute(Shell &);

private:
  std::shared_ptr<FileSystem> fs_;
};

MakeDirectoryCommand::MakeDirectoryCommand(const std::shared_ptr<FileSystem> &fs) : fs_{fs} {}

class ClearCommand : public Command {
public:
  virtual void Execute(Shell &);
};

void ExitCommand::Execute(Shell &shell) {
  shell.Shutdown();
}

void ListCommand::Execute(Shell &shell) {
  fs_->TraverseDirectory(shell.Cwd(), [&](std::shared_ptr<std::vector<FileOrDirectory>> files) {
    for (const auto &f : *files) {
      std::cout << f.Name() << ' ';
    }

    std::cout << '\n';
  });
}

void RemoveCommand::Execute(Shell &shell) {
  auto arg = shell.Arg();

  if (!arg.HasParameters()) {
    std::cout << arg.ProgramName() << ": missing operand\n";
    return;
  }

  fs_->TraverseDirectory(shell.Cwd(), [&](std::shared_ptr<std::vector<FileOrDirectory>> files) {
    bool is_exists = false;

    for (const auto &parameter : arg.Parameters()) {
      auto it = files->begin();
      for (; it != files->end(); it++) {
        if (!it->IsDirectory() && it->Name() == parameter) {
          is_exists = true; 
          break;
        }
      }
      
      if (is_exists) {
        files->erase(it);
        is_exists = false;
      }
    }
  });
}

void MakeDirectoryCommand::Execute(Shell &shell) {
  auto arg = shell.Arg();

  if (!arg.HasParameters()) {
    std::cout << arg.ProgramName() << ": missing operand\n";
    return;
  }

  fs_->TraverseDirectory(shell.Cwd(), [&](std::shared_ptr<std::vector<FileOrDirectory>> files) {
    bool is_exists = false;

    for (const auto &parameter : arg.Parameters()) {
      for(const auto &file : *files) {
        if (file.IsDirectory() && file.Name() == parameter) {
          std::cout << arg.ProgramName() << ": directory exists\n";
          is_exists = true;
        }
      }

      if (!is_exists) {
        files->push_back(FileOrDirectory::CreateDirectory(parameter));
      }
      is_exists = false;
    }
  });
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

  while (!shell.IsAuthenticating()) {}

  std::string input;

  while (shell.IsRunning()) {
    shell.DisplayPrompt();
    std::getline(std::cin, input);
    
    auto args = shell.Tokenize(input);
    shell.ParseArgs(args);
  }
}

void Shell::Shutdown() {
  is_running_ = false;
}

Shell::Shell() : is_running_{true}, cwd_{"/", "tmp"} {
  auto fs = std::make_shared<FileSystem>();
  fs->Populate();
  users_ = {
    User::CreateSuperuser("root", "12345678"),
    User::Create("user", "12345678")
  };

  commands_.insert({"exit", std::make_unique<ExitCommand>()});
  commands_.insert({"ls", std::make_unique<ListCommand>(fs)});
  commands_.insert({"mkdir", std::make_unique<MakeDirectoryCommand>(fs)});
  commands_.insert({"clear", std::make_unique<ClearCommand>()});
  commands_.insert({"rm", std::make_unique<RemoveCommand>(fs)});
}

bool Shell::IsRunning() const {
  return is_running_;
}

Argument Shell::Arg() const {
  return arg_;
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

  Argument argument{};
  argument.SetProgramName(args[0]);

  std::vector<std::string> parameters{};
  std::vector<std::string> options{};
  if (args.size() > 1) {
    for (std::size_t i = 1; i < args.size(); i++) {
      if (args[i].find('-') == 0) {
        options.push_back(args[i]);
      } else {
        parameters.push_back(args[i]);
      }
    }
  }

  argument.SetParameters(parameters);
  argument.SetOptions(options);

  arg_ = argument;

  auto cmd = arg_.ProgramName();
  if (ContainsCommand(cmd)) {
    commands_[cmd]->Execute(*this);
    return;
  }

  std::cout << "command not found: " << cmd << '\n';
}

bool Shell::ContainsCommand(const std::string &cmd) {
  return commands_.find(cmd) != commands_.end();
}

void Boot() {
  std::cout << "Finding bios...\n";
  std::cout << "Executing bios...\n";
  std::cout << "POST\n";
  std::cout << "Test block memory a...\n";
  std::cout << "Test block memory b...\n";
  std::cout << "Test block memory c...\n";
  std::cout << "Test block memory d...\n";
  std::cout << "Test block memory e...\n";
  std::cout << "Finding graphic cards...\n";
  std::cout << "Finding operating system...\n";

  float progress = 0.0;
  while (progress < 1.0) {
    int bar_width = 70;

    std::cout << "[";
    int pos = bar_width * progress;
    for (int i = 0; i < bar_width; ++i) {
        if (i < pos) std::cout << "=";
        else if (i == pos) std::cout << ">";
        else std::cout << " ";
    }
    std::cout << "] " << int(progress * 100.0) << " %\r";
    std::cout.flush();

    progress += 0.16; // for demonstration only

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }

  std::cout << std::endl;
}

int main() {
  Boot();

  Shell::MainLoop();

  return 0;
}
