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

class User {
public:
  User() = default;

  static User for_dev_create(const std::string &, bool);

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

User User::for_dev_create(const std::string &login, bool is_superuser) {
  return User{login, "", is_superuser};
}

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

class Argument {
public:
  bool HasParameters() const;
  bool HasOptions() const;

  std::string ProgramName() const;
  std::vector<std::string> Parameters() const;
  std::vector<std::string> Options() const;

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

bool Argument::HasOptions() const {
  return options_.size() > 0;
}

std::string Argument::ProgramName() const {
  return program_name_;
}

std::vector<std::string> Argument::Parameters() const {
  return parameters_;
}

std::vector<std::string> Argument::Options() const {
  return options_;
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

#define READ_FLAG 4
#define WRITE_FLAG 2
#define EXECUTE_FLAG 1

class FileOrDirectory {
public:

  static FileOrDirectory CreateDirectory(const std::string &);

  static FileOrDirectory CreateFile(const std::string &);

  void Add(const FileOrDirectory &);
  void SetPermission(unsigned char);

  std::string Name() const;
  bool Readable() const;
  bool Writeable() const;
  bool Executable() const;

  bool IsDirectory() const;

  std::shared_ptr<std::vector<FileOrDirectory>> Files() const;

private:
  FileOrDirectory(const std::string &, bool, const std::shared_ptr<std::vector<FileOrDirectory>> &);

private:
  std::shared_ptr<std::vector<FileOrDirectory>> files_;

  std::string name_;
  bool is_directory_;

  unsigned char permission_{0};
};

FileOrDirectory::FileOrDirectory(const std::string &name, bool is_directory, const std::shared_ptr<std::vector<FileOrDirectory>> &files)
  : name_{name}, is_directory_{is_directory}, files_{files} {}

FileOrDirectory FileOrDirectory::CreateDirectory(const std::string &name) {
  return {name, true, std::make_shared<std::vector<FileOrDirectory>>()};
}

FileOrDirectory FileOrDirectory::CreateFile(const std::string &name) {
  FileOrDirectory file{name, false, std::make_shared<std::vector<FileOrDirectory>>()};
  file.SetPermission(READ_FLAG | WRITE_FLAG);

  return file;
}

void FileOrDirectory::Add(const FileOrDirectory &file) {
  files_->push_back(file);
}

void FileOrDirectory::SetPermission(unsigned char p) {
  permission_ |= p;
}

std::string FileOrDirectory::Name() const {
  return name_;
}

bool FileOrDirectory::Readable() const {
  return (permission_ & READ_FLAG) == READ_FLAG;
}

bool FileOrDirectory::Writeable() const {
  return (permission_ & WRITE_FLAG) == WRITE_FLAG;
}

bool FileOrDirectory::Executable() const {
  return (permission_ & EXECUTE_FLAG) == EXECUTE_FLAG;
}

bool FileOrDirectory::IsDirectory() const {
  return is_directory_;
}

std::shared_ptr<std::vector<FileOrDirectory>> FileOrDirectory::Files() const {
  return files_;
}

class FileSystem {
public:
  void for_dev_populate();

  void Add(const FileOrDirectory &);

  void TraverseDirectory(const std::vector<std::string> &, const std::function<void(std::shared_ptr<std::vector<FileOrDirectory>>)> &func);

  std::shared_ptr<std::vector<FileOrDirectory>> Root() {
    return root_;
  }

private:
  std::shared_ptr<std::vector<FileOrDirectory>> root_;
};

void FileSystem::for_dev_populate() {
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
  root_->push_back(FileOrDirectory::CreateFile("log.txt"));
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
  auto arg = shell.Arg();

  bool should_detail = false;
  if (arg.HasOptions()) {
    auto options = arg.Options();

    for (const auto &option : options) {
      if (option == "-l") {
        should_detail = true;
      }
    }
  }

  fs_->TraverseDirectory(shell.Cwd(), [&](std::shared_ptr<std::vector<FileOrDirectory>> files) {
    if (should_detail) {
      std::cout << "total " << files->size() << '\n';
      for (const auto &f : *files) {
        if (f.IsDirectory()) {
          std::cout << 'd';
        } else {
          std::cout << '-';
        }

        if (f.Readable()) {
          std::cout << 'r';
        } else {
          std::cout << '-';
        }

        if (f.Writeable()) {
          std::cout << 'w';
        } else {
          std::cout << '-';
        }

        if (f.Executable()) {
          std::cout << 'x';
        } else {
          std::cout << '-';
        }

        std::cout << " " << f.Name() << '\n';
      }     
    } else {
      for (const auto &f : *files) {
        std::cout << f.Name() << ' ';
      }
    }

    if (!should_detail) {
      std::cout << '\n';
    }
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

Shell::Shell() : is_running_{true}, cwd_{"/"} {
  auto fs = std::make_shared<FileSystem>();
  fs->for_dev_populate();

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

class Motherboard {
public:
  struct CPU {
    std::string name;
    std::size_t bit;
  };

  struct RAM {
    std::string name;
    std::size_t capacity;
  };

  struct Storage {
    std::string name;
    std::size_t capacity;
  };

  struct VGA {
    std::string name;
    std::size_t capacity;
  };

  struct PowerSupply {
    std::string name;
  };

  Motherboard(const std::string &, const CPU &, const std::vector<RAM> &,
      const std::vector<Storage> &, const std::vector<VGA> &, const PowerSupply &);

  std::vector<VGA> VGAList() const; 

private:
  std::string name_;

  CPU cpu_;
  std::vector<RAM> ram_list_;
  std::vector<Storage> storages_;
  std::vector<VGA> vga_list_;
  PowerSupply power_supply_;
};

Motherboard::Motherboard(const std::string &name, const CPU &cpu, const std::vector<RAM> &ram_list,
    const std::vector<Storage> &storages, const std::vector<VGA> &vga_list, const PowerSupply &power_supply) :
    name_{name}, cpu_{cpu}, ram_list_{ram_list}, storages_{storages}, vga_list_{vga_list}, power_supply_{power_supply} {}

std::vector<Motherboard::VGA> Motherboard::VGAList() const {
  return vga_list_;
}

class Computer {
public:
  static Computer Boot();

private:
  Computer(const Motherboard &);

  Motherboard motherboard_;
};

Computer::Computer(const Motherboard &motherboard) : motherboard_{motherboard} {}

Computer Computer::Boot() {
  Motherboard::CPU cpu{.name = "AMD Ryzen 7 2700X", .bit = 64};

  Motherboard::RAM ram{.name = "Corsair Vengeance DDR4", .capacity = 8};
  std::vector<Motherboard::RAM> ram_list{ram, ram};

  Motherboard::Storage storage{.name = "Samsung SSD 870 EVO", .capacity = 1024};
  std::vector<Motherboard::Storage> storages{storage};

  Motherboard::VGA vga{.name = "Asus ROG Strix RTX 2080", .capacity = 8};
  std::vector<Motherboard::VGA> vga_list{vga};

  Motherboard::PowerSupply power_supply{.name = "Asus ROG Thor"};

  Motherboard motherboard{"AMD x570", cpu, ram_list, storages, vga_list, power_supply};

  std::cout << "Finding bios...\n";
  std::this_thread::sleep_for(std::chrono::milliseconds(200));
  std::cout << "BIOS found\n";

  std::cout << "Executing bios...\n";
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  std::cout << "POST\n";
  std::cout << "  Test block memory a...\n";
  std::cout << "  Test block memory b...\n";
  std::this_thread::sleep_for(std::chrono::milliseconds(300));

  std::cout << "  Test block memory c...\n";
  std::this_thread::sleep_for(std::chrono::milliseconds(300));

  std::cout << "  Test block memory d...\n";
  std::cout << "  Test block memory e...\n";
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  std::cout << "Checking graphic cards...\n";
  std::this_thread::sleep_for(std::chrono::milliseconds(400));

  std::cout << "Graphic card found: \n";
  for (const auto &v : motherboard.VGAList()) {
    std::cout << "  " << v.name << '\n';
  }

  std::cout << "Finding operating system...\n";
  std::this_thread::sleep_for(std::chrono::milliseconds(300));
  std::cout << "OS found\n";

  std::cout << "Delivering to OS...\n";
  std::this_thread::sleep_for(std::chrono::milliseconds(300));

  std::cout << "Booting...\n";

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

    std::this_thread::sleep_for(std::chrono::milliseconds(300));
  }

  std::cout << "\n\n";

  return Computer{motherboard};
}

int main() {
  auto computer = Computer::Boot();

  Shell::MainLoop();

  return 0;
}
