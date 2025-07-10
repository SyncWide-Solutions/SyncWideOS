void cmd_help(const char* args) {
    if (!args || !*args) {
        // General help
        terminal_writestring("SyncWideOS Command Help\n");
        terminal_writestring("=======================\n\n");
        
        terminal_writestring("File Operations:\n");
        terminal_writestring("  ls [path]           - List directory contents\n");
        terminal_writestring("  cd <path>           - Change directory\n");
        terminal_writestring("  pwd                 - Print working directory\n");
        terminal_writestring("  mkdir <name>        - Create directory\n");
        terminal_writestring("  read <file>         - Read file contents\n");
        terminal_writestring("  cat <file>          - Read file contents (alias for read)\n");
        terminal_writestring("  write <file> <text> - Write text to file\n");
        terminal_writestring("  touch <file>        - Create empty file\n");
        terminal_writestring("  cp <src> <dst>      - Copy file\n");
        terminal_writestring("  mv <src> <dst>      - Move/rename file\n");
        terminal_writestring("  rm <file>           - Delete file\n");
        terminal_writestring("  pia [file]          - Text editor\n\n");
        
        terminal_writestring("System Commands:\n");
        terminal_writestring("  help [command]      - Show help information\n");
        terminal_writestring("  clear               - Clear screen\n");
        terminal_writestring("  echo <text>         - Display text\n");
        terminal_writestring("  echo <text> > <file> - Write text to file\n");
        terminal_writestring("  echo <text> >> <file> - Append text to file\n");
        terminal_writestring("  system [info]       - Show system information\n");
        terminal_writestring("  mount               - Mount FAT32 filesystem\n");
        terminal_writestring("  unmount             - Unmount filesystem\n");
        terminal_writestring("  fsinfo              - Show filesystem information\n\n");
        
        terminal_writestring("Network Commands:\n");
        terminal_writestring("  ipconfig            - Show network configuration\n");
        terminal_writestring("  ping <ip>           - Ping an IP address\n");
        terminal_writestring("  dhcp                - Request DHCP configuration\n");
        terminal_writestring("  netstat             - Show network status\n");
        terminal_writestring("  install <package>   - Install network package\n\n");
        
        terminal_writestring("Type 'help <command>' for detailed information about a specific command.\n");
    } else {
        // Specific command help
        if (strcmp(args, "write") == 0) {
            terminal_writestring("write - Write text to a file\n\n");
            terminal_writestring("Usage:\n");
            terminal_writestring("  write <filename> <content>\n");
            terminal_writestring("  write <filename>           (interactive mode)\n\n");
            terminal_writestring("Examples:\n");
            terminal_writestring("  write hello.txt \"Hello, World!\"\n");
            terminal_writestring("  write config.txt \"debug=true\"\n");
        } else if (strcmp(args, "touch") == 0) {
            terminal_writestring("touch - Create an empty file\n\n");
            terminal_writestring("Usage:\n");
            terminal_writestring("  touch <filename>\n\n");
            terminal_writestring("Examples:\n");
            terminal_writestring("  touch newfile.txt\n");
            terminal_writestring("  touch data.log\n");
                } else if (strcmp(args, "cp") == 0) {
            terminal_writestring("cp - Copy files\n\n");
            terminal_writestring("Usage:\n");
            terminal_writestring("  cp <source> <destination>\n\n");
            terminal_writestring("Examples:\n");
            terminal_writestring("  cp file1.txt file2.txt\n");
            terminal_writestring("  cp data.log backup.log\n");
        } else if (strcmp(args, "mv") == 0) {
            terminal_writestring("mv - Move/rename files\n\n");
            terminal_writestring("Usage:\n");
            terminal_writestring("  mv <source> <destination>\n\n");
            terminal_writestring("Examples:\n");
            terminal_writestring("  mv oldname.txt newname.txt\n");
            terminal_writestring("  mv temp.log archive.log\n");
        } else if (strcmp(args, "rm") == 0) {
            terminal_writestring("rm - Remove files\n\n");
            terminal_writestring("Usage:\n");
            terminal_writestring("  rm <filename>\n\n");
            terminal_writestring("Examples:\n");
            terminal_writestring("  rm unwanted.txt\n");
            terminal_writestring("  rm temp.log\n\n");
            terminal_writestring("Warning: This permanently deletes the file!\n");
        } else if (strcmp(args, "echo") == 0) {
            terminal_writestring("echo - Display text or write to files\n\n");
            terminal_writestring("Usage:\n");
            terminal_writestring("  echo <text>              - Display text\n");
            terminal_writestring("  echo <text> > <file>     - Write text to file (overwrite)\n");
            terminal_writestring("  echo <text> >> <file>    - Append text to file\n\n");
            terminal_writestring("Examples:\n");
            terminal_writestring("  echo \"Hello World\"\n");
            terminal_writestring("  echo \"Log entry\" >> system.log\n");
            terminal_writestring("  echo \"New content\" > output.txt\n");
        } else if (strcmp(args, "pia") == 0) {
            terminal_writestring("pia - PIA Text Editor\n\n");
            terminal_writestring("Usage:\n");
            terminal_writestring("  pia [filename]          - Open editor (optionally with file)\n\n");
            terminal_writestring("Editor Controls:\n");
            terminal_writestring("  Ctrl+S                  - Save file\n");
            terminal_writestring("  Ctrl+Q                  - Quit editor\n");
            terminal_writestring("  F1                      - Toggle help menu\n");
            terminal_writestring("  Arrow keys              - Navigate\n");
            terminal_writestring("  Backspace               - Delete character\n");
            terminal_writestring("  Enter                   - New line\n\n");
            terminal_writestring("Examples:\n");
            terminal_writestring("  pia                     - Open empty editor\n");
            terminal_writestring("  pia myfile.txt          - Open/create myfile.txt\n");
        } else {
            terminal_writestring("No help available for command: ");
            terminal_writestring(args);
            terminal_writestring("\n");
            terminal_writestring("Type 'help' for a list of available commands.\n");
        }
    }
}
