# SSH Manager 

A simple, fast SSH connection manager for Linux terminals.

## Features
-  **Quick SSH connections** by host name
-  **Interactive menu** for easy management
-  **Import from existing SSH config**
-  **Tagging system** for organizing hosts
-  **Host statistics** and connection history
-  **Backup and restore** functionality

## Requirements
- Linux or macOS
- Bash shell
- SSH client
- GCC (for C++ version)


# Install
- git clone https://github.com/delfel25/ssh-manager.git
- cd ssh-manager
- g++ -std=c++17 -o sshman sshman.cpp
- sudo cp sshman /usr/local/bin/
- sshman

# sshman	Interactive mode

- **name**     Quick connect to host
- **list**     List all hosts
- **add**      Add new host
- **import**   Import from ~/.ssh/config
- **delete**   Delete host
- **backup**   Backup hosts database
- **restore**  Restore from backup
- **help**     Show help
