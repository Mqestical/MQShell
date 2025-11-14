#include <ncurses.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <stdbool.h>
#include "UN.h"
#include "shell.h"

bool is_sleep;
char result[8192];

char *shell_execute(const char *input) {
    result[0] = '\0';
    int seconds;

    // Run in background if ends with &
    if (ends_with_ampersand(input)) {
        BG_process(input);
        return "";
    }

    char *argv[32];
    seconds = TAGS(input, argv, &is_sleep);

    if (is_sleep) {
        clock_nsleep(seconds, 0);
        return "";
    }

    if (strcmp(input, "pwd") == 0) {
        strcpy(result, pwd());
        return result;
    }

    if (strcmp(input, "ls") == 0) {
        strcpy(result, ls());
        return result;
    }

    if (strcmp(input, "joblist") == 0) {
        print_jobs(head);
        return "";
    }

    if (strcmp(input, "exit") == 0) {
        endwin();
        exit(0);
    }

    return "command not found\n";
}

int main() {
    char username[255];
    strcpy(username, getun());

    initscr();
    cbreak();

    printw(" WELCOME %s\n\n", username);
    printw(" MXJESTICAL SHELL\n\n");
    printw(" GNU GENERAL PUBLIC LICENSE 3\n\n");

    char input[256];
    while (1) {
        printw("\n<%s>: ", username);
        refresh();
        getstr(input);

        char *output = shell_execute(input);
        printw("%s", output);
        refresh();
    }

    return 0;
}