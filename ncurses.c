#include <ncurses.h>
#include <string.h>
#include <stdlib.h>
#include "UN.h"
#include "shell.h"

char *shell_execute(const char *input);
char result[8192];
int main() {
    char username[255];
    strcpy(username, getun());

    initscr();
    cbreak();

    printw("                             WELCOME %s\n\n", username);
    printw("                            MXJESTICAL SHELL\n\n");
    printw("                        GNU GENERAL PUBLIC LICENSE 3\n\n");

    char input[256];

    while (1) {
        printw("\n<%s>: ", username);
        refresh();
        getstr(input);

        char *output = shell_execute(input);

        printw("%s", output);
        refresh();
    }
    getnstr(input, sizeof(input)-1);
    return 0;
}

char *shell_execute(const char *input) {
    result[0] = '\0';
    
    if(ends_with_ampersand(input)) {

        BG_process(input);
    }

    if (strcmp(input, "pwd") == 0) {
        strcpy(result, pwd());
        return result;
    }

    if (strcmp(input, "ls") == 0) {
        strcpy(result, ls());
        return result;
    }

    if (strcmp(input, "exit") == 0) {
        endwin(); 
        exit(0);
    }

    return "command not found\n";
}

/*
char *shell_execute(const char *input) {
    
    result[0] = '\0';

    if (strcmp(input, "pwd") == 0) {
        strcpy(result, pwd());
        return result;
    }

    if (strcmp(input, "ls") == 0) {
        strcpy(result, ls());
        return result;
    }

    if (strncmp(input, "cd", 2) == 0) {
        const char *arg = input + 2;
        while (*arg == ' ') arg++;
        strcpy(result, cd(NULL));
        return result;
    }

    if (strcmp(input, "exit") == 0) {
        endwin(); 
        exit(0);
    }

    return "command not found\n";
}
*/