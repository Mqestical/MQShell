#define _GNU_SOURCE
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <time.h>
#include <ncurses.h>
#include <sys/wait.h>
#include "shell.h"

#define MAX_JOB_COUNT 100

typedef enum {
    RUNNING,
    STOPPED,
    DONE
} job_status_t;


typedef struct {
    pid_t pid;
    char cmd[256];
    job_status_t status;
    int job_num;
} job_t;

job_t jobs[MAX_JOB_COUNT];
int job_count = 0;


struct linux_dirent64 {
    ino64_t        d_ino;    // inode number
    off64_t        d_off;    // offset to next dirent
    unsigned short d_reclen; // length of this record
    unsigned char  d_type;   // file type
    char           d_name[]; // filename (null-terminated)
};
// Static buffer for returned strings
static char output[8192];

// ----- Internal command: pwd -----=
char *pwd(void) {
    static char buf[512];
    if (getcwd(buf, sizeof(buf)) == NULL) {
        snprintf(output, sizeof(output), "error: getcwd failed\n");
        return output;
    }
    snprintf(output, sizeof(output), "%s\n", buf);
    return output;
}

// ----- Internal command: ls -----
char *ls(void) {
    char cwd[512];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        snprintf(output, sizeof(output), "error: getcwd failed\n");
        return output;
    }

    int dirfd = open(cwd, O_RDONLY | O_DIRECTORY);
    if (dirfd == -1) {
        snprintf(output, sizeof(output), "error: open failed\n");
        return output;
    }

    char buf[8192];
    int nread = syscall(SYS_getdents64, dirfd, buf, sizeof(buf));
    if (nread == -1) {
        snprintf(output, sizeof(output), "error: getdents64 failed\n");
        close(dirfd);
        return output;
    }

    output[0] = '\0';
    for (int bpos = 0; bpos < nread; ) {
        struct linux_dirent64 *d = (struct linux_dirent64 *)(buf + bpos);
        strcat(output, d->d_name);
        strcat(output, "\n");
        bpos += d->d_reclen;
    }

    close(dirfd);
    return output;
}


int ends_with_ampersand(char *input) {
    int len = strlen(input);
    if (len == 0) return 0;

    int i = len - 1;
    while (i >= 0 && isspace((unsigned char)input[i])) {
        i--;
    }

    if (i < 0) return 0; 

    int is_bg = (input[i] == '&');

    input[i + 1] = '\0';

    return is_bg;
}

void clock_nsleep(int seconds, long nanoseconds) {
    
    struct timespec ts = {seconds, nanoseconds};
    long ret = syscall(SYS_clock_nanosleep, CLOCK_REALTIME, 0, &ts, NULL);
    if (ret != 0) perror("clock_nanosleep");
}
void BG_process(char *ampsand_input) {
    // Trim trailing spaces and '&'
    int len = strlen(ampsand_input);
    while (len > 0 && isspace((unsigned char)ampsand_input[len - 1]))
        ampsand_input[--len] = '\0';
    if (len > 0 && ampsand_input[len - 1] == '&')
        ampsand_input[--len] = '\0';

    // If empty after trimming
    if (len == 0) {
        printw("error: empty command.\n");
        return;
    }

    // Tokenize command
    char *args[] = { ampsand_input, NULL };

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return;
    }

    if (pid == 0) {
        // child process
        execvp(ampsand_input, args);
        perror("execvp failed");
        _exit(1);
    } else {
        // parent: store in job table
        if (job_count < MAX_JOB_COUNT) {
            jobs[job_count].pid = pid;
            strncpy(jobs[job_count].cmd, ampsand_input, sizeof(jobs[job_count].cmd) - 1);
            jobs[job_count].cmd[sizeof(jobs[job_count].cmd) - 1] = '\0';
            jobs[job_count].status = RUNNING;
            jobs[job_count].job_num = job_count + 1;
            printw("[%d] %d %s\n", jobs[job_count].job_num, (int)pid, jobs[job_count].cmd);
            job_count++;
        } else {
            printw("error: job table full.\n");
        }
    }
}