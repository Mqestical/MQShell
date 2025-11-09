#ifndef SHELL_H
#define SHELL_H

// exec one shell cmd and return output as a string
char *shell_execute(const char *input);

char *pwd(void);
char *ls(void);

#endif
