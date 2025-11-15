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
    
    char mkdir_op[256];
    char cmd_copy[256];
    char folder[256];
    char cmd[256];
    int job_num;
    
    // Check for fg/bg command
    if (parse_job_command(input, cmd, &job_num)) {
        if (strcmp(cmd, "fg") == 0) {
            fg_job(job_num);
            return "";
        }
        if (strcmp(cmd, "bg") == 0) {
            bg_job(job_num);
            return "";
        }
    }
    
    // Check cd
    char dir[256];
    if (TAGCD(input, dir, sizeof(dir))) {
        char *out = cdCMD(dir, mkdir_op, sizeof(mkdir_op));
        strcpy(result, out);
        return result;
    }

    // Check echo
    char text[256];
    if (TAGECHO(input, text, sizeof(text))) {
        char *out = echoCMD(text, mkdir_op, sizeof(mkdir_op));
        strcpy(result, out);
        return result;
    }

    // Check mkdir
    strncpy(cmd_copy, input, sizeof(cmd_copy) - 1);
    cmd_copy[255] = '\0';
    
    if (TAGMKDIR(cmd_copy, folder, sizeof(folder))) {
        char *out = mkdirCMD(folder, mkdir_op, sizeof(mkdir_op));
        strcpy(result, out);
        return result;
    }
    
    // Check rmdir
    strncpy(cmd_copy, input, sizeof(cmd_copy) - 1);
    cmd_copy[255] = '\0';
    
    if (TAGRMDIR(cmd_copy, folder, sizeof(folder))) {
        char *out = rmdirCMD(folder, mkdir_op, sizeof(mkdir_op));
        strcpy(result, out);
        return result;
    }
    
    // Check cat
    char filename[256];
    strncpy(cmd_copy, input, sizeof(cmd_copy) - 1);
    cmd_copy[255] = '\0';

    if (TAGCAT(cmd_copy, filename, sizeof(filename))) {
        char *out = catCMD(filename, result, sizeof(result));
        return out;
    }

    // Check touch
    if (TAGTOUCH(input, filename, sizeof(filename))) {
        char *out = touchCMD(filename, mkdir_op, sizeof(mkdir_op));
        strcpy(result, out);
        return result;
    }

    // Check cp
    char src[256], dest[256];
    strncpy(cmd_copy, input, sizeof(cmd_copy) - 1);
    cmd_copy[255] = '\0';

    if (TAGCP(cmd_copy, src, dest, sizeof(src))) {
        char *out = cpCMD(src, dest, mkdir_op, sizeof(mkdir_op));
        strcpy(result, out);
        return result;
    }

    // Check mv
    strncpy(cmd_copy, input, sizeof(cmd_copy) - 1);
    cmd_copy[255] = '\0';

    if (TAGMV(cmd_copy, src, dest, sizeof(src))) {
        char *out = mvCMD(src, dest, mkdir_op, sizeof(mkdir_op));
        strcpy(result, out);
        return result;
    }

    // Check grep
    char pattern[256];
    strncpy(cmd_copy, input, sizeof(cmd_copy) - 1);
    cmd_copy[255] = '\0';

    if (TAGGREP(cmd_copy, pattern, filename, sizeof(pattern))) {
        char *out = grepCMD(pattern, filename, result, sizeof(result));
        return out;
    }

    // Check find
    strncpy(cmd_copy, input, sizeof(cmd_copy) - 1);
    cmd_copy[255] = '\0';

    if (TAGFIND(cmd_copy, filename, sizeof(filename))) {
        char *out = findCMD(filename, result, sizeof(result));
        return out;
    }

    // Check chmod
    char mode[256];
    strncpy(cmd_copy, input, sizeof(cmd_copy) - 1);
    cmd_copy[255] = '\0';

    if (TAGCHMOD(cmd_copy, mode, filename, sizeof(mode))) {
        char *out = chmodCMD(mode, filename, mkdir_op, sizeof(mkdir_op));
        strcpy(result, out);
        return result;
    }

    // After chmod check
char owner[256];
strncpy(cmd_copy, input, sizeof(cmd_copy) - 1);
cmd_copy[255] = '\0';

if (TAGCHOWN(cmd_copy, owner, filename, sizeof(owner))) {
    char *out = chownCMD(owner, filename, mkdir_op, sizeof(mkdir_op));
    strcpy(result, out);
    return result;
}

// Check ps
if (strcmp(input, "ps") == 0) {
    char *out = psCMD(result, sizeof(result));
    return out;
}

// Check top
if (strcmp(input, "top") == 0) {
    char *out = topCMD(result, sizeof(result));
    return out;
}

    // Check sleep
    char *argv[32];
    strncpy(cmd_copy, input, sizeof(cmd_copy) - 1);
    cmd_copy[255] = '\0';
    seconds = TAGS(cmd_copy, argv, &is_sleep);
    
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

    if (strcmp(input, "clear") == 0) {
        clear();
        move(0,0);
        refresh();
        return "";
    }

    if (strcmp(input, "exit") == 0) {
        endwin();
        exit(0);
    }

    return "command not found\n";
}


int main(void) {
    char username[255];
    strcpy(username, getun());

    initscr();
    cbreak();

    start_color();
    use_default_colors();
    if (!has_colors) {
        printw("error: terminal does not support colors.");
        return EXIT_FAILURE;
    }
    printw("                                                    WELCOME %s\n\n", username);
    printw("                                                    MXJESTICAL SHELL\n\n");
    printw("                                                    GNU GENERAL PUBLIC LICENSE 3\n\n");

    char input[256];
    while (1) {
        char* directory = pwd();
        printw("\n<%s %s>: ", username, directory);
        refresh();
        getstr(input);

        char *output = shell_execute(input);
        printw("%s", output);
        refresh();
    }

    return 0;
}