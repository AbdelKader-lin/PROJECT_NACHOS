#include "syscall.h"
#define MAX_ARGS 10

int strcmp(const char *a, const char *b) {
    int i = 0;
    while (a[i] != '\0' && b[i] != '\0') {
        if (a[i] != b[i]) return a[i] - b[i];
        i++;
    }
    return a[i] - b[i];
}

void strcpy(char *dest, const char *src) {
    int i = 0;
    while (src[i]) {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

int split(char *line, char *argv[], int max) {
    int argc = 0, i = 0, start = 0;
    while (line[i] != '\0' && argc < max) {
        // skip spaces
        while (line[i] == ' ') i++;
        if (line[i] == '\0') break;
        start = i;
        while (line[i] != ' ' && line[i] != '\0') i++;
        line[i] = '\0';
        argv[argc++] = &line[start];
        i++;
    }
    return argc;
}

int main() {
    SpaceId newProc;
    OpenFileId input = ConsoleInput;
    OpenFileId output = ConsoleOutput;
    char prompt[] = "nachos> ";
    char buffer[80];
    char *argv[MAX_ARGS];
    int i, argc;

    while (1) {
        Write(prompt, sizeof(prompt) - 1, output);

        i = 0;
        do {
            Read(&buffer[i], 1, input);
        } while (buffer[i++] != '\n');
        buffer[--i] = '\0';

        if (i == 0) continue;

        argc = split(buffer, argv, MAX_ARGS);
        if (argc == 0) continue;
        
        if (strcmp(argv[0], "exit") == 0) {
            Halt();
        }

        if (strcmp(argv[0], "ls") == 0) {
            ListDirectory();
            continue;
        }

        if (strcmp(argv[0], "mkdir") == 0) {
            if (argc < 2) {
                Write("Usage: mkdir <dirname>\n", 24, output);
            } else {
                CreateDirectory(argv[1]);
            }
            continue;
        }

        if (strcmp(argv[0], "rmdir") == 0) {
            if (argc < 2) {
                Write("Usage: rmdir <dirname>\n", 24, output);
            } else {
                RemoveDirectory(argv[1]);
            }
            continue;
        }

        if (strcmp(argv[0], "cd") == 0) {
            if (argc < 2) {
                Write("Usage: cd <dirname>\n", 20, output);
            } else {
                ChangeDirectory(argv[1]);
            }
            continue;
        }

        newProc = Exec(argv[0]);
        Join(newProc);
    }
    return 0;
}