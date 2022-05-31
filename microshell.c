
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

bool is_str_equal(char* str1, char* str2) {
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

// syscall
void ft_write(int fd, char* msg, int size) {
  chk(write(fd, msg, size));
}

// logic
void exec_builtin(char* args[]) {
  if (!args[1] || args[2])
    return ft_perror("error: cd: bad arguments", NULL);
  if (chdir(args[1]) == Error)
    ft_perror("error: cd: cannot change directory to ", args[1]);
}

void exec_cmd(char* args[], char* envp[], int infd, int outfd) {
  if (*args == NULL)
    return;
  int ws;
  pid_t pid = chk(fork());
  if (pid)
    chk(waitpid(pid, &ws, 0));
  else {
    chk(dup2(infd, STDIN_FILENO));
    chk(dup2(outfd, STDOUT_FILENO));
    if (execve(args[0], args, envp) == Error) {
      ft_perror("error: cannot execute ", args[0]);
      kill(0, SIGINT);
      exit(1);
    }
  }
}

void exec_pipelines(char* args[], char* envp[]) {
  int prev[2];
  int now[2];

  int start = 0;
  prev[PipeRead] = STDIN_FILENO;
  for (int i = 0; args[i]; i++) {
    if (is_str_equal(args[i], "|")) {
      chk(pipe(now));
      args[i] = NULL;
      exec_cmd(args + start, envp, prev[PipeRead], now[PipeWrite]);
      if (prev[PipeRead] != STDIN_FILENO)
        chk(close(prev[PipeRead]));
      chk(close(now[PipeWrite]));
      swap_pipe(prev, now);
      start = i + 1;
    }
  }
  now[PipeWrite] = STDOUT_FILENO;
  exec_cmd(args + start, envp, prev[PipeRead], now[PipeWrite]);
}

void exec_cmds(char* args[], char* envp[]) {
  if (*args == NULL)
    return;
  if (is_str_equal(*args, "cd"))
    exec_builtin(args);
  else
    exec_pipelines(args, envp);
}

int main(int argc, char* argv[], char* envp[]) {
  int start = 1;
  for (int i = 1; i < argc; i++) {
    if (is_str_equal(argv[i], ";")) {
      argv[i] = NULL;
      exec_cmds(argv + start, envp);
      start = i + 1;
    }
  }
  exec_cmds(argv + start, envp);
}
