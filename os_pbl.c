#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>

#define MAX_ARGS 100
#define BUFFER_SIZE 1024

// ---------------- Date Command ----------------
void date_cmd()
{
    printf("Date Command\n");

    char *args[] = {"date", NULL};

    pid_t pid = fork();

    if (pid < 0)
    {
        perror("fork failed");
        return;
    }

    if (pid == 0)
    {
        printf("Executing 'date' command...\n");
        execvp(args[0], args);
        perror("Execution failed");
        exit(1);
    }
    else
    {
        printf("Waiting for child process to finish...\n");
        waitpid(pid, NULL, 0);
        printf("Date command executed successfully!\n\n");
    }
}

// ---------------- Echo Command ----------------
void echo_cmd(char *input)
{
    char *args[MAX_ARGS];
    int count = 0;
    char temp[BUFFER_SIZE];
    strcpy(temp, input);

    char *token = strtok(temp, " ");
    while (token != NULL && count < MAX_ARGS)
    {
        args[count] = token;
        count++;
        token = strtok(NULL, " ");
    }
    args[count] = NULL;

    printf("Echo Command\n");

    printf("Printing each word separately:\n");
    for (int i = 1; i < count; i++)
    {
        printf(" Word[%d] = %s\n", i, args[i]);
    }

    int letters = 0;
    int digits = 0;
    int uppercase = 0;
    int lowercase = 0;

    for (int i = 1; i < count; i++)
    {
        for (int j = 0; args[i][j] != '\0'; j++)
        {
            char c = args[i][j];
            if (c >= 'A' && c <= 'Z')
            {
                uppercase++;
                letters++;
            }
            else if (c >= 'a' && c <= 'z')
            {
                lowercase++;
                letters++;
            }
            else if (c >= '0' && c <= '9')
            {
                digits++;
            }
        }
    }

    printf("Letters: %d, Uppercase: %d, Lowercase: %d, Digits: %d\n", letters, uppercase, lowercase, digits);

    int total_chars = 0;
    for (int i = 1; i < count; i++)
    {
        total_chars += strlen(args[i]);
    }
    printf("Total characters (excluding spaces) = %d\n", total_chars);

    printf("Echo Output: ");
    for (int i = 1; i < count; i++)
    {
        printf("%s ", args[i]);
    }
    printf("\n\n");
}

// ---------------- PWD Command ----------------
void pwd_cmd()
{
    char cwd[BUFFER_SIZE];
    if (getcwd(cwd, sizeof(cwd)) == NULL)
    {
        perror("pwd failed");
        return;
    }

    printf("PWD Command\n");
    printf("Current directory: %s\n", cwd);

    char path_copy[BUFFER_SIZE];
    strcpy(path_copy, cwd);

    printf("Folder levels:\n");
    char *level = strtok(path_copy, "/");
    int lvl = 1;
    while (level != NULL)
    {
        printf(" Level %d: %s\n", lvl, level);
        lvl++;
        level = strtok(NULL, "/");
    }

    printf("ASCII codes of each character:\n");
    for (int i = 0; cwd[i] != '\0'; i++)
    {
        printf(" Char[%d] = %c, ASCII=%d\n", i, cwd[i], (int)cwd[i]);
    }

    int sep_count = 0;
    for (int i = 0; cwd[i] != '\0'; i++)
    {
        if (cwd[i] == '/')
        {
            sep_count++;
        }
    }
    printf("Number of folder separators: %d\n", sep_count);
    printf("Total characters in path: %lu\n\n", strlen(cwd));
}

// ---------------- LS Command ----------------
void ls_cmd()
{
    char *args[] = {"ls", "-l", NULL};

    pid_t pid = fork();

    if (pid < 0)
    {
        perror("fork failed");
        return;
    }

    if (pid == 0)
    {
        printf("Executing 'ls -l' command...\n");
        execvp(args[0], args);
        perror("Execution failed");
        exit(1);
    }
    else
    {
        printf("Waiting for child process to finish...\n");
        waitpid(pid, NULL, 0);
        printf("LS command executed successfully!\n\n");
    }
}

// ---------------- Fun Commands ----------------
void greet_cmd(char *name)
{
    printf("Greet Command\n");
    if (name != NULL)
    {
        printf("Hello, %s! Welcome to Mini Linux Shell!\n\n", name);
    }
    else
    {
        printf("Hello! Welcome to Mini Linux Shell!\n\n");
    }
}

void roll_cmd()
{
    printf("Roll Dice Command\n");
    int roll = (rand() % 6) + 1;
    printf("You rolled a %d!\n\n", roll);
}

void joke_cmd()
{
    printf("Joke Command\n");
    const char *jokes[] =
        {
            "Why did the computer get cold? Because it left its Windows open!",
            "I told my computer I needed a break, and it said: 'You seem stressed, shall I crash?'",
            "Why do programmers prefer dark mode? Because light attracts bugs!",
            "Debugging: Being the detective in a crime movie where you are also the murderer.",
            "I would tell you a UDP joke... but you might not get it."};

    int n = rand() % 5;
    printf("%s\n\n", jokes[n]);
}

void about_cmd()
{
    printf("About Mini Linux Shell\n");
    printf("This shell demonstrates basic OS commands with fun extensions.\n\n");
    printf("Command Descriptions:\n");
    printf(" date   - Displays the current system date and time.\n");
    printf(" echo   - Prints user input text and shows character statistics.\n");
    printf(" pwd    - Shows current working directory and folder hierarchy.\n");
    printf(" ls     - Lists files and directories in the current folder.\n");
    printf(" greet  - Greets the user by name with a welcome message.\n");
    printf(" roll   - Simulates rolling a six-sided dice.\n");
    printf(" joke   - Displays a random programming-related joke.\n");
    printf(" about  - Shows information about the Mini Linux Shell.\n");
    printf(" exit   - Exits the shell program.\n\n");

}


// ---------------- Main Shell ----------------
int main()
{
    char line[BUFFER_SIZE];
    int command_no = 1;
    srand(time(NULL));

    printf("=== Mini Linux Shell ===\n");
    printf("Available commands: date, echo, pwd, ls, greet, roll, joke, about\n");
    printf("Type 'exit' to quit\n\n");

    while (1)
    {
        printf("Command [%d]> ", command_no);

        if (!fgets(line, sizeof(line), stdin))
        {
            break;
        }

        line[strcspn(line, "\n")] = 0;

        if (strlen(line) == 0)
        {
            command_no++;
            continue;
        }

        if (strcmp(line, "exit") == 0)
        {
            break;
        }

        if (strcmp(line, "date") == 0)
        {
            date_cmd();
        }
        else if (strncmp(line, "echo ", 5) == 0)
        {
            echo_cmd(line);
        }
        else if (strcmp(line, "pwd") == 0)
        {
            pwd_cmd();
        }
        else if (strcmp(line, "ls") == 0)
        {
            ls_cmd();
        }
        else if (strncmp(line, "greet", 5) == 0)
        {
            char *name = strchr(line, ' ');
            if (name != NULL)
            {
                name++;
            }
            greet_cmd(name);
        }
        else if (strcmp(line, "roll") == 0)
        {
            roll_cmd();
        }
        else if (strcmp(line, "joke") == 0)
        {
            joke_cmd();
        }
        else if (strcmp(line, "about") == 0)
        {
            about_cmd();
        }
        else
        {
            printf("Unknown command: %s\n\n", line);
        }

        command_no++;
    }

    printf("Mini Linux Shell terminated. Total commands entered: %d\n", command_no - 1);
    return 0;
}