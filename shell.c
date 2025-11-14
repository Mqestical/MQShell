#define _GNU_SOURCE
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <time.h>
#include <ncurses.h>
#include <signal.h>
#include "shell.h"

int job_id = 0;
volatile sig_atomic_t sigchld_flag = 0;
Job *head = NULL;
static char output[8192];

// ----- Internal command: pwd -----
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
    remove_done_jobs(&head);
    return output;
}


int ends_with_ampersand(const char *input) {
    int len = strlen(input);
    if (len == 0) return 0;

    int i = len - 1;

    // skip spaces
    while (i >= 0 && isspace((unsigned char)input[i]))
        i--;

    if (i < 0) return 0;

    return input[i] == '&';
}


// ----- Sleep helper -----
void clock_nsleep(int seconds, long nanoseconds) {
    struct timespec ts = {seconds, nanoseconds};
    long ret = syscall(SYS_clock_nanosleep, CLOCK_REALTIME, 0, &ts, NULL);
    if (ret != 0) perror("clock_nanosleep");
}

// ----- Add job -----
void add_job(Job **head, const char *cmd) {
    job_id++;
    Job *new_job = malloc(sizeof(Job));
    new_job->job_id = job_id;
    new_job->status = RUNNING;
    new_job->pid = 0; // will set after fork
    strncpy(new_job->cmd, cmd, 255);
    new_job->cmd[255] = '\0';
    new_job->next = NULL;

    if (*head == NULL) {
        *head = new_job;
    } else {
        Job *temp = *head;
        while (temp->next != NULL)
            temp = temp->next;
        temp->next = new_job;
    }
}

// ----- Remove finished jobs -----
void remove_done_jobs(Job **head) {
    Job *current = *head;
    Job *prev = NULL;

    while (current != NULL) {
        if (current->status == DONE) {
            if (prev == NULL) *head = current->next;
            else prev->next = current->next;

            Job *to_free = current;
            current = current->next;
            free(to_free);
        } else {
            prev = current;
            current = current->next;
        }
    }
}

// ----- Print jobs -----
void print_jobs(Job *head) {
    Job *temp = head;
    while (temp != NULL) {
        printw("[%d] %s (%s)\n", temp->job_id, temp->cmd,
               temp->status == RUNNING ? "RUNNING" :
               temp->status == STOPPED ? "STOPPED" : "DONE");
        temp = temp->next;
        refresh();
    }
}

// ----- Handle SIGCHLD -----
void handle_sigchld() {
    int status;
    pid_t w;
    while ((w = waitpid(-1, &status, WNOHANG | WUNTRACED | WCONTINUED)) > 0) {
        Job *temp = head;
        while (temp != NULL) {
            if (temp->pid == w) {
                if (WIFEXITED(status) || WIFSIGNALED(status))
                    temp->status = DONE;
                else if (WIFSTOPPED(status))
                    temp->status = STOPPED;
                else if (WIFCONTINUED(status))
                    temp->status = RUNNING;
            }
            temp = temp->next;
        }
    }
    sigchld_flag = 0;
}

// ----- Background process -----
void BG_process(const char *input) {
    if (input == NULL || strlen(input) == 0) {
        printw("error: empty command\n");
        return;
    }

    // Copy input so we can modify it
    char cmd[256];
    strncpy(cmd, input, 255);
    cmd[255] = '\0';

    // Remove trailing &
    int len = strlen(cmd);
    if (len > 0 && cmd[len-1] == '&') {
        cmd[len-1] = '\0';
    }

    // Trim spaces
    trim_whitespace(cmd);

    if (strlen(cmd) == 0) {
        printw("error: empty command after removing &\n");
        return;
    }

    // Add job to linked list
    add_job(&head, cmd);

    // Get the last added job
    Job *new_job = head;
    while (new_job->next != NULL) new_job = new_job->next;

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return;
    }

  if (pid == 0) {
    Job *job;
    // child process
    if (strcmp(cmd, "ls") == 0) {
        printw("%s", ls());
        fflush(stdout);
    } else if (strcmp(cmd, "pwd") == 0) {
        printw("%s", pwd());
        fflush(stdout);
    } else if (strcmp(cmd, "sleep") == 0) {
        clock_nsleep(5, 0); // gon' change this, just a placeholder. 
    } else if (strcmp(cmd, "joblist") == 0) {
        print_jobs(head);
    } else {
        printw("command not found\n");
    }
    fflush(stdout);
    _exit(0); // important to exit the child
}else {
        // Parent
        new_job->pid = pid;
        printw("[%d] %d\n", new_job->job_id, pid);
        refresh();
    }

    return; // leave it for later (placeholder)

}

// ----- Signal handlers -----
void sigchld(int signal) { sigchld_flag = 1; }
void sigint(int signal) { write(STDOUT_FILENO, "Ctrl+C detected\n", 16); }
void sigtstp(int signal) { write(STDOUT_FILENO, "Ctrl+Z detected\n", 16); }

void sighandler(int signal) {
    struct sigaction sa;

    sa.sa_handler = &sigchld;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGCHLD, &sa, NULL);

    sa.sa_handler = &sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

    sa.sa_handler = &sigtstp;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGTSTP, &sa, NULL);
}

// ----- Trim leading and trailing whitespace -----
static void trim_whitespace(char *str) {
    char *end;

    // Trim leading
    while(isspace((unsigned char)*str)) str++;

    if(*str == 0) return;

    // Trim trailing
    end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) end--;

    // Write new null terminator
    *(end + 1) = '\0';
}