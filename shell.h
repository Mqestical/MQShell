#ifndef SHELL_H
#define SHELL_H

#include <sys/types.h>
#include <signal.h>

typedef enum { RUNNING, STOPPED, DONE } JobStatus;

typedef struct Job {
    pid_t pid;
    int job_id;
    JobStatus status;
    char cmd[256];
    struct Job *next;
} Job;

struct linux_dirent64 {
    unsigned long  d_ino;    
    long           d_off;    
    unsigned short d_reclen; 
    unsigned char  d_type;   
    char           d_name[]; 
};

extern int job_id;
extern volatile sig_atomic_t sigchld_flag;
extern Job *head;

char *shell_execute(const char *input);
char *pwd(void);
char *ls(void);
int ends_with_ampersand(const char *input);
void BG_process(const char *input);
void clock_nsleep(int seconds, long nanoseconds);
void sigchld(int signal);
void sigint(int signal);
void sigtstp(int signal);
void sighandler(int signal);
void add_job(Job **head, const char *cmd);
void remove_done_jobs(Job **head);
void print_jobs(Job *head);
static void trim_whitespace(char *str);
int TAGS(char *input, char *argv[], bool *is_sleep);
#endif