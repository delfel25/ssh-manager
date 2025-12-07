#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <filesystem>
#include <unistd.h>
#include <termios.h>
#include <iomanip>

namespace fs = std::filesystem;

const std::string CONFIG_DIR = std::string(getenv("HOME")) + "/.sshmanager";
const std::string HOSTS_FILE = CONFIG_DIR + "/hosts";
const std::string RESET   = "\033[0m";
const std::string RED     = "\033[31m";
const std::string GREEN   = "\033[32m";
const std::string YELLOW  = "\033[33m";
const std::string BLUE    = "\033[34m";
const std::string MAGENTA = "\033[35m";
const std::string CYAN    = "\033[36m";
const std::string BOLD    = "\033[1m";

struct SSHHost {
    std::string name;
    std::string hostname;
    std::string username;
    int port;
    std::string key_path;
    std::string tags;
};

class SSHManager {
private:
    std::vector<SSHHost> hosts;
    
    void load_hosts() {
        hosts.clear();
        std::ifstream file(HOSTS_FILE);
        if (!file.is_open()) return;
        
        std::string line;
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '#') continue;
            
            SSHHost host;
            size_t pos1 = line.find('|');
            size_t pos2 = line.find('|', pos1 + 1);
            size_t pos3 = line.find('|', pos2 + 1);
            size_t pos4 = line.find('|', pos3 + 1);
            size_t pos5 = line.find('|', pos4 + 1);
            
            if (pos1 != std::string::npos) {
                host.name = line.substr(0, pos1);
                host.hostname = line.substr(pos1 + 1, pos2 - pos1 - 1);
                host.username = line.substr(pos2 + 1, pos3 - pos2 - 1);
                host.port = std::stoi(line.substr(pos3 + 1, pos4 - pos3 - 1));
                host.key_path = line.substr(pos4 + 1, pos5 - pos4 - 1);
                if (pos5 != std::string::npos) {
                    host.tags = line.substr(pos5 + 1);
                }
                
                hosts.push_back(host);
            }
        }
        file.close();
    }
    
    void save_hosts() {
        std::ofstream file(HOSTS_FILE);
        file << "# SSH Manager Hosts Database\n";
        file << "# name|hostname|username|port|key_path|tags\n";
        
        for (const auto& host : hosts) {
            file << host.name << "|"
                 << host.hostname << "|"
                 << host.username << "|"
                 << host.port << "|"
                 << host.key_path << "|"
                 << host.tags << "\n";
        }
        file.close();
    }
    
    std::string get_input(const std::string& prompt) {
        std::cout << prompt;
        std::string input;
        std::getline(std::cin, input);
        return input;
    }
    
    void print_header(const std::string& title) {
        std::cout << BOLD << CYAN 
                  << "===========================================\n"
                  << " " << title << "\n"
                  << "===========================================\n"
                  << RESET;
    }
    
    bool quick_connect(const std::string& host_name) {
        for (auto& host : hosts) {
            if (host.name == host_name) {
                std::string ssh_cmd = "ssh ";
                if (!host.key_path.empty() && host.key_path != "~/.ssh/id_rsa") {
                    ssh_cmd += "-i " + host.key_path + " ";
                }
                if (host.port != 22) {
                    ssh_cmd += "-p " + std::to_string(host.port) + " ";
                }
                ssh_cmd += host.username + "@" + host.hostname;
                
                std::cout << GREEN << "Connecting to " << host.hostname << "...\n" << RESET;
                std::cout << YELLOW << "Command: " << ssh_cmd << "\n" << RESET;
                
                system(ssh_cmd.c_str());
                return true;
            }
        }
        return false;
    }

public:
    SSHManager() {
        fs::create_directories(CONFIG_DIR);
        load_hosts();
    }
    
    void add_host() {
        print_header("Add New SSH Host");
        
        SSHHost host;
        
        host.name = get_input("Name (alias): ");
        if (host.name.empty()) {
            std::cout << RED << "Name cannot be empty!\n" << RESET;
            return;
        }
        
        for (const auto& h : hosts) {
            if (h.name == host.name) {
                std::cout << RED << "Host already exists!\n" << RESET;
                return;
            }
        }
        
        host.hostname = get_input("Hostname/IP: ");
        host.username = get_input("Username [" + std::string(getenv("USER")) + "]: ");
        if (host.username.empty()) host.username = getenv("USER");
        
        std::string port_str = get_input("Port [22]: ");
        host.port = port_str.empty() ? 22 : std::stoi(port_str);
        
        host.key_path = get_input("SSH key path [~/.ssh/id_rsa]: ");
        if (host.key_path.empty()) host.key_path = "~/.ssh/id_rsa";
        
        host.tags = get_input("Tags (comma separated): ");
        
        hosts.push_back(host);
        save_hosts();
        
        std::cout << GREEN << "Host added: " << host.name << "\n" << RESET;
    }
    
    void list_hosts() {
        print_header("SSH Hosts List");
        
        if (hosts.empty()) {
            std::cout << YELLOW << "No hosts configured. Use 'add' first.\n" << RESET;
            return;
        }
        
        std::cout << BOLD
                  << std::left << std::setw(15) << "NAME"
                  << std::setw(20) << "HOSTNAME"
                  << std::setw(10) << "USER"
                  << std::setw(6) << "PORT"
                  << "TAGS\n" << RESET;
        
        std::cout << "------------------------------------------------------------\n";
        
        for (const auto& host : hosts) {
            std::cout << std::left << std::setw(15) << host.name
                      << std::setw(20) << host.hostname
                      << std::setw(10) << host.username
                      << std::setw(6) << host.port
                      << host.tags << "\n";
        }
        
        std::cout << "\n" << CYAN << "Total: " << hosts.size() << " hosts\n" << RESET;
    }
    
    void connect_host() {
        if (hosts.empty()) {
            std::cout << YELLOW << "No hosts. Add one first.\n" << RESET;
            return;
        }
        
        std::cout << BOLD << "Select host:\n" << RESET;
        for (size_t i = 0; i < hosts.size(); i++) {
            std::cout << "  " << i + 1 << ". " << hosts[i].name 
                      << " (" << hosts[i].hostname << ")\n";
        }
        std::cout << "  0. Cancel\n";
        
        std::string choice = get_input("\nYour choice: ");
        int index = std::stoi(choice) - 1;
        
        if (index < 0 || index >= (int)hosts.size()) {
            std::cout << YELLOW << "Cancelled.\n" << RESET;
            return;
        }
        
        SSHHost& host = hosts[index];
        
        std::string cmd = "ssh ";
        if (!host.key_path.empty() && host.key_path != "~/.ssh/id_rsa") {
            cmd += "-i " + host.key_path + " ";
        }
        if (host.port != 22) {
            cmd += "-p " + std::to_string(host.port) + " ";
        }
        cmd += host.username + "@" + host.hostname;
        
        std::cout << GREEN << "Connecting to " << host.hostname << "...\n" << RESET;
        std::cout << YELLOW << "Command: " << cmd << "\n" << RESET;
        
        system(cmd.c_str());
    }
    
    void delete_host() {
        if (hosts.empty()) {
            std::cout << YELLOW << "No hosts to delete.\n" << RESET;
            return;
        }
        
        list_hosts();
        std::string name = get_input("\nEnter host name to delete: ");
        
        auto it = std::remove_if(hosts.begin(), hosts.end(),
            [&name](const SSHHost& h) { return h.name == name; });
        
        if (it != hosts.end()) {
            hosts.erase(it, hosts.end());
            save_hosts();
            std::cout << GREEN << "Host deleted: " << name << "\n" << RESET;
        } else {
            std::cout << RED << "Host not found: " << name << "\n" << RESET;
        }
    }
    
    void import_ssh_config() {
        std::string config_path = std::string(getenv("HOME")) + "/.ssh/config";
        
        if (!fs::exists(config_path)) {
            std::cout << RED << "File not found: " << config_path << "\n" << RESET;
            return;
        }
        
        std::ifstream file(config_path);
        std::string line;
        SSHHost current;
        int imported = 0;
        
        while (std::getline(file, line)) {
            size_t pos = line.find('#');
            if (pos != std::string::npos) line = line.substr(0, pos);
            
            line.erase(0, line.find_first_not_of(" \t"));
            line.erase(line.find_last_not_of(" \t") + 1);
            
            if (line.empty()) continue;
            
            if (line.find("Host ") == 0) {
                if (!current.name.empty()) {
                    hosts.push_back(current);
                    imported++;
                    current = SSHHost();
                }
                current.name = line.substr(5);
                current.port = 22;
                current.username = getenv("USER");
            }
            else if (line.find("HostName ") == 0) {
                current.hostname = line.substr(9);
            }
            else if (line.find("User ") == 0) {
                current.username = line.substr(5);
            }
            else if (line.find("Port ") == 0) {
                current.port = std::stoi(line.substr(5));
            }
            else if (line.find("IdentityFile ") == 0) {
                current.key_path = line.substr(13);
            }
        }
        
        if (!current.name.empty()) {
            hosts.push_back(current);
            imported++;
        }
        
        file.close();
        save_hosts();
        
        std::cout << GREEN << "Imported " << imported << " hosts\n" << RESET;
    }
    
    void show_help() {
        print_header("SSH Manager Help");
        
        std::cout << BOLD << "Commands:\n" << RESET;
        std::cout << "  add     - Add new host\n";
        std::cout << "  list    - List all hosts\n";
        std::cout << "  connect - Connect to host\n";
        std::cout << "  delete  - Delete host\n";
        std::cout << "  import  - Import from ~/.ssh/config\n";
        std::cout << "  help    - Show this help\n";
        std::cout << "  exit    - Exit program\n";
        std::cout << "\n" << BOLD << "Quick connect:\n" << RESET;
        std::cout << "  Run: sshman <hostname>\n";
        std::cout << "  Example: sshman myserver\n";
    }
    
    void interactive_mode() {
        print_header("SSH Manager for Linux");
        
        std::cout << GREEN << "Welcome! Type 'help' for commands.\n\n" << RESET;
        
        while (true) {
            std::string prompt = "ssh> ";
            std::cout << prompt;
            std::string cmd;
            std::getline(std::cin, cmd);
            
            if (cmd == "exit" || cmd == "quit") {
                std::cout << GREEN << "Goodbye!\n" << RESET;
                break;
            }
            else if (cmd == "add") {
                add_host();
            }
            else if (cmd == "list" || cmd == "ls") {
                list_hosts();
            }
            else if (cmd == "connect" || cmd == "conn") {
                connect_host();
            }
            else if (cmd == "delete" || cmd == "rm") {
                delete_host();
            }
            else if (cmd == "import") {
                import_ssh_config();
            }
            else if (cmd == "help" || cmd == "?") {
                show_help();
            }
            else if (!cmd.empty()) {
                if (!quick_connect(cmd)) {
                    std::cout << RED << "Unknown command or host: " << cmd << "\n" << RESET;
                }
            }
            
            std::cout << "\n";
        }
    }
    
    bool quick_connect_mode(const std::string& host_name) {
        return quick_connect(host_name);
    }
    
    void show_hosts() {
        list_hosts();
    }
};

int main(int argc, char* argv[]) {
    SSHManager manager;
    
    if (argc == 2) {
        std::string arg = argv[1];
        
        if (arg == "--help" || arg == "-h") {
            std::cout << "SSH Manager - Simple SSH connection manager\n";
            std::cout << "Usage:\n";
            std::cout << "  sshman                    - Interactive mode\n";
            std::cout << "  sshman <hostname>         - Quick connect to host\n";
            std::cout << "  sshman list               - List all hosts\n";
            std::cout << "  sshman add                - Add new host\n";
            std::cout << "  sshman --help             - Show help\n";
            return 0;
        }
        else if (arg == "list" || arg == "ls") {
            manager.show_hosts();
            return 0;
        }
        else if (arg == "add") {
            manager.interactive_mode();
            return 0;
        }
        else {
            if (!manager.quick_connect_mode(arg)) {
                std::cout << RED << "Host not found: " << arg << "\n" << RESET;
                std::cout << "Use 'sshman list' to see available hosts\n";
                return 1;
            }
            return 0;
        }
    }
    
    manager.interactive_mode();
    return 0;
}
