
#define _POSIX_SOURCE  // linux only. to suppress warnings for kill()
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

// utils
typedef enum { PipeRead = 0, PipeWrite = 1 } dir_t;
typedef enum { Ok = 0, Error = -1 } res_t;

int ft_strlen(char* str) {
  int i = 0;
  while (str[i])
    i++;
  return i;
}

bool str_eq(char* str1, char* str2) {
  return strcmp(str1, str2) == 0;
}

void ft_write(int fd, char* msg, int size);

void ft_putstr_fd(int fd, char* str) {
  if (str)
    ft_write(fd, str, ft_strlen(str));
  else
    ft_write(2, "(NULL)\n", ft_strlen("(NULL)\n"));
}

void ft_perror(char* msg, char* arg) {
  ft_putstr_fd(2, msg);
  if (arg)
    ft_putstr_fd(2, arg);
  ft_putstr_fd(2, "\n");
}

int chk(int ret) {
  if (ret == Error) {
    ft_perror("error: fatal", NULL);
    kill(0, SIGINT);
    exit(1);
  }
  return ret;
}

void copy_pipe(int from[2], int to[2]) {
  to[PipeRead] = from[PipeRead];
  to[PipeWrite] = from[PipeWrite];
}

void swap_pipe(int left[2], int right[2]) {
  int tmp[2];

  copy_pipe(left, tmp);
  copy_pipe(right, left);
  copy_pipe(tmp, right);
}

void ft_write(int fd, char* msg, int size) {
  chk(write(fd, msg, size));
}

// logic
void run_builtin(char* av[]) {
  if (!av[1] || av[2])
    return ft_perror("error: cd: bad arguments", NULL);
  if (chdir(av[1]) == Error)
    ft_perror("error: cd: cannot change directory to ", av[1]);
}

void exec_cmd(char* av[], char* ev[], int infd, int outfd) {
  if (*av == NULL)
    return;
  int ws;
  pid_t pid = chk(fork());
  if (pid)
    chk(waitpid(pid, &ws, 0));
  else {
    chk(dup2(infd, STDIN_FILENO));
    chk(dup2(outfd, STDOUT_FILENO));
    if (execve(av[0], av, ev) == Error) {
      ft_perror("error: cannot execute ", av[0]);
      kill(0, SIGINT);
      exit(1);
    }
  }
}

void run_pipe(char* av[], char* ev[]) {
  int prev[2];
  int now[2];

  int start = 0;
  prev[PipeRead] = STDIN_FILENO;
  for (int i = 0; av[i]; i++) {
    if (str_eq(av[i], "|")) {
      chk(pipe(now));
      av[i] = NULL;
      exec_cmd(av + start, ev, prev[PipeRead], now[PipeWrite]);
      if (prev[PipeRead] != STDIN_FILENO)
        chk(close(prev[PipeRead]));
      chk(close(now[PipeWrite]));
      swap_pipe(prev, now);
      start = i + 1;
    }
  }
  now[PipeWrite] = STDOUT_FILENO;
  exec_cmd(av + start, ev, prev[PipeRead], now[PipeWrite]);
}

void run_cmds(char* av[], char* ev[]) {
  if (*av == NULL)
    return;
  if (str_eq(*av, "cd"))
    run_builtin(av);
  else
    run_pipe(av, ev);
}

int main(int ac, char* av[], char* ev[]) {
  int start = 1;
  for (int i = 1; i < ac; i++) {
    if (str_eq(av[i], ";")) {
      av[i] = NULL;
      run_cmds(av + start, ev);
      start = i + 1;
    }
  }
  run_cmds(av + start, ev);
}
