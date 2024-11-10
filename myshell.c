#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <dirent.h>

#define RED "\033[1;31m"
#define GREEN "\033[1;32m"
#define YELLOW "\033[1;33m"
#define CYAN "\033[1;36m"
#define RESET "\033[0m"

// Function declarations
void shell_loop();
void execute_command(char **args);
char *read_command();
char **parse_command(char *input);
void cd_command(char *path);
void help_command();
void exit_command();
void delete_command(char *path);
void create_file_command(char *path);
void list_files_command(char *path);
int copy_file(const char *src, const char *dest);
int move_file(const char *src, const char *dest);
int get_cpu_usage();
void display_welcome();

void shell_loop() {
    char *input;
    char **args;

    while (1) {
        printf(CYAN "\nmyshell> " RESET);
        input = read_command();
        args = parse_command(input);

        if (args[0] != NULL) {
            execute_command(args);
        }

        free(input);
        free(args);
    }
}

void display_welcome() {
    printf(GREEN "\n======================================================================\n" RESET);
    printf(YELLOW "             Welcome to Kritika's Custom Shell\n" RESET);
    printf(GREEN "======================================================================\n" RESET);
    printf(CYAN "Type 'help' to see available commands.\n" RESET);
}

char *read_command() {
    char *input = NULL;
    size_t len = 0;
    getline(&input, &len, stdin);
    return input;
}

char **parse_command(char *input) {
    int bufsize = 64, position = 0;
    char **tokens = malloc(bufsize * sizeof(char*));
    char *token;

    if (!tokens) {
        fprintf(stderr, RED "myshell: allocation error\n" RESET);
        exit(EXIT_FAILURE);
    }

    token = strtok(input, " \t\r\n\a");
    while (token != NULL) {
        tokens[position] = token;
        position++;

        if (position >= bufsize) {
            bufsize += 64;
            tokens = realloc(tokens, bufsize * sizeof(char*));
            if (!tokens) {
                fprintf(stderr, RED "myshell: allocation error\n" RESET);
                exit(EXIT_FAILURE);
            }
        }

        token = strtok(NULL, " \t\r\n\a");
    }
    tokens[position] = NULL;
    return tokens;
}

void cd_command(char *path) {
    if (chdir(path) != 0) {
        perror(RED "myshell: cd" RESET);
    }
}

void help_command() {
    printf(YELLOW "\n------------------------------------------------\n" RESET);
    printf(CYAN "      Kritika's Custom Shell Commands:\n" RESET);
    printf(YELLOW "------------------------------------------------\n" RESET);
    printf(GREEN "cd <directory>" RESET " - Change directory\n");
    printf(GREEN "exit" RESET " - Exit the shell\n");
    printf(GREEN "copy <src> <dest>" RESET " - Copy files or folders\n");
    printf(GREEN "cut <src> <dest>" RESET " - Move files or folders\n");
    printf(GREEN "delete <file>" RESET " - Delete file with confirmation\n");
    printf(GREEN "cpu" RESET " - Display CPU usage\n");
    printf(GREEN "create <file>" RESET " - Create a new file\n");
    printf(GREEN "list <directory>" RESET " - List all files in a directory\n");
}

void exit_command() {
    printf(YELLOW "Exiting the shell...\n" RESET);
    exit(0);
}

void delete_command(char *path) {
    char response;
    printf(YELLOW "Are you sure you want to delete %s? (y/n): " RESET, path);
    response = getchar();
    getchar(); // Consume newline

    if (response == 'y' || response == 'Y') {
        if (unlink(path) == 0) {
            printf(GREEN "Deleted Successfully\n" RESET);
        } else {
            perror(RED "myshell: delete" RESET);
        }
    } else {
        printf(CYAN "Deletion canceled\n" RESET);
    }
}

void create_file_command(char *path) {
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror(RED "myshell: create" RESET);
    } else {
        printf(GREEN "File created successfully: %s\n" RESET, path);
        close(fd);
    }
}

void list_files_command(char *path) {
    DIR *dir = opendir(path);
    struct dirent *entry;

    if (!dir) {
        perror(RED "myshell: list" RESET);
        return;
    }

    printf(CYAN "Files in directory %s:\n" RESET, path);
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {
            printf(GREEN "  %s\n" RESET, entry->d_name);
        } else if (entry->d_type == DT_DIR) {
            printf(YELLOW "  %s/\n" RESET, entry->d_name);
        }
    }
    closedir(dir);
}

int copy_file(const char *src, const char *dest) {
    int src_fd, dest_fd;
    char buffer[4096];
    ssize_t bytes;

    src_fd = open(src, O_RDONLY);
    if (src_fd < 0) {
        perror(RED "myshell: copy - opening source file" RESET);
        return -1;
    }

    dest_fd = open(dest, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (dest_fd < 0) {
        perror(RED "myshell: copy - opening/creating destination file" RESET);
        close(src_fd);
        return -1;
    }

    while ((bytes = read(src_fd, buffer, sizeof(buffer))) > 0) {
        if (write(dest_fd, buffer, bytes) != bytes) {
            perror(RED "myshell: copy - writing to destination file" RESET);
            close(src_fd);
            close(dest_fd);
            return -1;
        }
    }

    close(src_fd);
    close(dest_fd);
    printf(GREEN "File copied successfully from %s to %s\n" RESET, src, dest);
    return 0;
}

int move_file(const char *src, const char *dest) {
    if (rename(src, dest) == 0) {
        printf(GREEN "File moved successfully from %s to %s\n" RESET, src, dest);
        return 0;
    } else {
        perror(RED "myshell: cut - moving file" RESET);
        return -1;
    }
}

int get_cpu_usage() {
    FILE *fp = fopen("/proc/stat", "r");
    if (!fp) {
        perror(RED "myshell: get_cpu_usage" RESET);
        return -1;
    }

    char buffer[256];
    fgets(buffer, sizeof(buffer), fp);
    fclose(fp);

    int user, nice, system, idle;
    sscanf(buffer, "cpu %d %d %d %d", &user, &nice, &system, &idle);

    int total = user + nice + system + idle;
    int usage = 100 * (total - idle) / total;
    return usage;
}

void execute_command(char **args) {
    if (strcmp(args[0], "cd") == 0) {
        if (args[1]) {
            cd_command(args[1]);
        } else {
            fprintf(stderr, RED "myshell: cd requires a directory\n" RESET);
        }
    } else if (strcmp(args[0], "help") == 0) {
        help_command();
    } else if (strcmp(args[0], "exit") == 0) {
        exit_command();
    } else if (strcmp(args[0], "delete") == 0) {
        if (args[1]) {
            delete_command(args[1]);
        } else {
            fprintf(stderr, RED "myshell: delete requires a file path\n" RESET);
        }
    } else if (strcmp(args[0], "cpu") == 0) {
        int usage = get_cpu_usage();
        if (usage != -1) {
            printf(YELLOW "Current CPU Usage: %d%%\n" RESET, usage);
        }
    } else if (strcmp(args[0], "copy") == 0) {
        if (args[1] && args[2]) {
            copy_file(args[1], args[2]);
        } else {
            fprintf(stderr, RED "myshell: copy requires source and destination paths\n" RESET);
        }
    } else if (strcmp(args[0], "cut") == 0) {
        if (args[1] && args[2]) {
            move_file(args[1], args[2]);
        } else {
            fprintf(stderr, RED "myshell: cut requires source and destination paths\n" RESET);
        }
    } else if (strcmp(args[0], "create") == 0) {
        if (args[1]) {
            create_file_command(args[1]);
        } else {
            fprintf(stderr, RED "myshell: create requires a file path\n" RESET);
        }
    } else if (strcmp(args[0], "list") == 0) {
        if (args[1]) {
            list_files_command(args[1]);
        } else {
            fprintf(stderr, RED "myshell: list requires a directory path\n" RESET);
        }
    } else {
        fprintf(stderr, RED "myshell: command not found\n" RESET);
    }
}

int main() {
    display_welcome();
    shell_loop();
    return 0;
}
