#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
char *getun() {
    static char buf[128];
    int fd[2];
    pipe(fd);

    if (fork() == 0) {
        // Child: write to pipe
        close(fd[0]);
        dup2(fd[1], STDOUT_FILENO);
        execlp("whoami", "whoami", NULL);
        exit(1);
    } else {
        // Parent: read from pipe
        close(fd[1]);
        read(fd[0], buf, sizeof(buf));
        buf[strcspn(buf, "\n")] = '\0'; // remove nl
    }
    return buf;
}