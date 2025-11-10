#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>

#define BUFFER_SIZE 4096
#define MAX_ARGS 100

// ---------- BUILT-IN COMMANDS ----------

// date
void date_cmd(char *output, size_t size) {
    time_t t = time(NULL);
    snprintf(output, size, "Current Date & Time: %s", ctime(&t));
}

// echo
void echo_cmd(char *input, char *output, size_t size) {
    char *text = input + 5; // skip "echo "
    snprintf(output, size, "%s\n", text);
}

// pwd
void pwd_cmd(char *output, size_t size) {
    char cwd[BUFFER_SIZE];
    if (getcwd(cwd, sizeof(cwd)) != NULL)
        snprintf(output, size, "%s\n", cwd);
    else
        snprintf(output, size, "Error: could not get current directory\n");
}

// mkdir
void mkdir_cmd(char *input, char *output, size_t size) {
    char *dirname = input + 6;
    if (strlen(dirname) == 0)
        snprintf(output, size, "Usage: mkdir <directory_name>\n");
    else if (mkdir(dirname, 0755) == 0)
        snprintf(output, size, "Directory '%s' created successfully.\n", dirname);
    else
        snprintf(output, size, "Error: could not create directory '%s'.\n", dirname);
}

// touch
void touch_cmd(char *input, char *output, size_t size) {
    char *filename = input + 6;
    if (strlen(filename) == 0) {
        snprintf(output, size, "Usage: touch <file_name>\n");
        return;
    }

    int fd = open(filename, O_CREAT | O_WRONLY, 0644);
    if (fd == -1)
        snprintf(output, size, "Error: could not create file '%s'.\n", filename);
    else {
        close(fd);
        snprintf(output, size, "File '%s' created successfully.\n", filename);
    }
}

// cat
void cat_cmd(char *input, char *output, size_t size) {
    char *filename = input + 4;
    FILE *file = fopen(filename, "r");

    if (!file) {
        snprintf(output, size, "Error: could not open file '%s'.\n", filename);
        return;
    }

    output[0] = '\0';
    char line[256];
    while (fgets(line, sizeof(line), file)) {
        if (strlen(output) + strlen(line) < size)
            strcat(output, line);
        else
            break;
    }
    fclose(file);
}

// greet
void greet_cmd(char *input, char *output, size_t size) {
    char *name = strchr(input, ' ');
    if (name) name++;
    snprintf(output, size, "Hello, %s! Welcome to Mini Linux Shell!\n", name ? name : "User");
}

// roll
void roll_cmd(char *output, size_t size) {
    int roll = (rand() % 6) + 1;
    snprintf(output, size, "🎲 You rolled a %d!\n", roll);
}

// joke
void joke_cmd(char *output, size_t size) {
    const char *jokes[] = {
        "💻 Why did the computer get cold? Because it left its Windows open!",
        "😂 Debugging: Being the detective in a crime movie where you are also the murderer.",
        "🤣 Why do programmers prefer dark mode? Because light attracts bugs!",
        "😅 I would tell you a UDP joke... but you might not get it.",
        "😜 I told my computer I needed a break, and it said: 'You seem stressed, shall I crash?'"
    };
    int n = rand() % 5;
    snprintf(output, size, "%s\n", jokes[n]);
}

// ---------- CORE SHELL LOGIC ----------

// Split input into arguments
void parse_input(char *input, char **args) {
    for (int i = 0; i < MAX_ARGS; i++) args[i] = NULL;
    char *token = strtok(input, " \t\n");
    int i = 0;
    while (token && i < MAX_ARGS - 1)
        args[i++] = token, token = strtok(NULL, " \t\n");
    args[i] = NULL;
}

// Execute external Linux command and capture output
void execute_system_command(char **args, char *output, size_t size, int background) {
    int fd[2];
    pipe(fd);
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork failed");
        return;
    } 
    else if (pid == 0) {
        // Child process
        close(fd[0]);
        dup2(fd[1], STDOUT_FILENO);
        dup2(fd[1], STDERR_FILENO);
        close(fd[1]);
        execvp(args[0], args);
        perror("exec failed");
        exit(1);
    } 
    else {
        // Parent process
        close(fd[1]);
        int n = read(fd[0], output, size - 1);
        if (n > 0) output[n] = '\0';
        else snprintf(output, size, "Command executed successfully (no output).\n");
        close(fd[0]);
        if (!background)
            waitpid(pid, NULL, 0);
    }
}

// Handle piping or redirection
void handle_redirection_and_piping(char *input, char *output, size_t size) {
    int fd[2];
    pipe(fd);
    pid_t pid = fork();

    if (pid == 0) {
        close(fd[0]);
        dup2(fd[1], STDOUT_FILENO);
        dup2(fd[1], STDERR_FILENO);
        close(fd[1]);
        execl("/bin/sh", "sh", "-c", input, NULL);
        perror("exec failed");
        exit(1);
    } else {
        close(fd[1]);
        int n = read(fd[0], output, size - 1);
        if (n > 0)
            output[n] = '\0';
        else
            snprintf(output, size, "");
        close(fd[0]);
        waitpid(pid, NULL, 0);
    }
}

// ---------- MAIN EXECUTION FUNCTION ----------

int execute_shell_command(char *input, char *output, size_t output_size) {
    memset(output, 0, output_size);
    srand(time(NULL));

    if (strlen(input) == 0) {
        snprintf(output, output_size, "No command entered.\n");
        return 0;
    }

    // Handle piping and redirection
    if (strchr(input, '|') || strchr(input, '>') || strchr(input, '<')) {
        handle_redirection_and_piping(input, output, output_size);
        return 0;
    }

    // BUILT-IN COMMANDS
    if (strcmp(input, "date") == 0) date_cmd(output, output_size);
    else if (strncmp(input, "echo ", 5) == 0) echo_cmd(input, output, output_size);
    else if (strcmp(input, "pwd") == 0) pwd_cmd(output, output_size);
    else if (strncmp(input, "mkdir ", 6) == 0) mkdir_cmd(input, output, output_size);
    else if (strncmp(input, "touch ", 6) == 0) touch_cmd(input, output, output_size);
    else if (strncmp(input, "cat ", 4) == 0) cat_cmd(input, output, output_size);
    else if (strncmp(input, "greet", 5) == 0) greet_cmd(input, output, output_size);
    else if (strcmp(input, "roll") == 0) roll_cmd(output, output_size);
    else if (strcmp(input, "joke") == 0) joke_cmd(output, output_size);
   else if (strcmp(input, "about") == 0) {
    snprintf(output, output_size,
        "=== 🐧 Mini Linux Shell ===\n"
        "---------------------------------------------\n"
        "👨‍💻 Developed By : Bhaumik Negi, Divyanshi Kaushik,\n"
        "                    Harshita Pant and Ansh Karki\n"
        "🏫 University     : Graphic Era Hill University\n"
        "📘 Course         : B.Tech - Operating Systems PBL Project\n"
        "---------------------------------------------\n"
        "⚙️  Features:\n"
        "   • Built-in commands (cd, mkdir, touch, cat, etc.)\n"
        "   • Process creation & execution using fork/exec\n"
        "   • Input/Output redirection (<, >)\n"
        "   • Command piping (|)\n"
        "   • Background execution (&)\n"
        "---------------------------------------------\n"
        "💡 Tip: Use 'help' to view all available commands.\n"
        "---------------------------------------------\n");
}

else if (strcmp(input, "help") == 0) {
    snprintf(output, output_size,
        "📘 HELP MENU — Mini Linux Shell Commands\n"
        "=================================================\n"
        "============== BUILT-IN COMMANDS ==============\n"
        " date               → Show current date and time.\n"
        " pwd                → Print current working directory.\n"
        " cd <dir>           → Change working directory.\n"
        " mkdir <dir>        → Create a new directory.\n"
        " touch <file>       → Create an empty file.\n"
        " cat <file>         → Display contents of a file.\n"
        " echo <msg>         → Print text to the screen.\n"
        " greet [name]       → Display a greeting message.\n"
        " roll               → Roll a dice (1–6).\n"
        " joke               → Tell a random programming joke.\n"
        " about              → Show project and developer info.\n"
        " help               → Display this help menu.\n"
        " exit               → Exit from the shell.\n"
        "-------------------------------------------------\n"
        "============ ⚙️ SYSTEM COMMANDS ============\n"
        " ls, whoami, ps, uname, df, cal, grep, wc, etc.\n"
        " → Executes real Linux system commands.\n"
        "-------------------------------------------------\n"
        "============ 🚀 ADVANCED FEATURES ============\n"
        " >  → Output Redirection  (e.g., echo Hello > out.txt)\n"
        " <  → Input Redirection   (e.g., cat < in.txt)\n"
        " |  → Piping              (e.g., ls | grep .c)\n"
        " &  → Background Execution (e.g., sleep 5 &)\n"
        "-------------------------------------------------\n"
        "💡 Tip: Combine commands like 'cat file.txt | wc -l'\n"
        "    for chaining and advanced command execution.\n"
        "=================================================\n");
}
    else if (strcmp(input, "exit") == 0) {
        snprintf(output, output_size, "Session closed.\n");
        exit(0);
    }
    else if (strncmp(input, "cd ", 3) == 0) {
        char *path = input + 3;
        while (*path == ' ') path++;
        if (chdir(path) == 0)
            snprintf(output, output_size, "Directory changed to: %s\n", path);
        else
            snprintf(output, output_size, "Error: No such directory: %s\n", path);
    }
    else {
        // External command (system)
        char *args[MAX_ARGS];
        char temp[BUFFER_SIZE];
        strcpy(temp, input);
        parse_input(temp, args);
        execute_system_command(args, output, output_size, 0);
    }

    return 0;
}
