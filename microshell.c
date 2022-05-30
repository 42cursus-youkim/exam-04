
#define _POSIX_SOURCE  // linux only. to suppress warnings for kill()
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

typedef enum { PipeRead = 0, PipeWrite = 1 } dir_t;
typedef enum { Ok = 0, Error = -1 } res_t;

bool is_str_equal(char* str1, char* str2) {
  return strcmp(str1, str2) == 0;
}

void ft_write(int fd, char* msg, int size);

void ft_putstr_fd(char* str, int fd) {
  if (str == NULL) {
    ft_write(2, "(NULL)\n", 7);
    return;
  }
  while (*str)
    ft_write(fd, str++, 1);
}

void ft_perror(char* msg, char* arg) {
  ft_putstr_fd(msg, 2);
  if (arg)
    ft_putstr_fd(arg, 2);
  ft_putstr_fd("\n", 2);
}

void syscall_error_check(res_t ret) {
  if (ret == Error) {
    ft_perror("error: fatal", NULL);
    kill(0, SIGINT);
    exit(1);
  }
}

void ft_write(int fd, char* msg, int size) {
  res_t ret = write(fd, msg, size);
  syscall_error_check(ret);
}

void ft_dup2(int srcfd, int dstfd) {
  res_t ret = dup2(srcfd, dstfd);
  syscall_error_check(ret);
}

void ft_close(int fd) {
  res_t ret = close(fd);
  syscall_error_check(ret);
}

void ft_pipe(int pipefd[2]) {
  res_t ret = pipe(pipefd);
  syscall_error_check(ret);
}

void ft_waitpid(pid_t pid, int* ws, int opt) {
  res_t ret = waitpid(pid, ws, opt);
  syscall_error_check(ret);
}

int ft_fork() {
  pid_t pid = fork();
  syscall_error_check(pid);
  return (pid);
}

void exec_builtin(char* args[], char* envp[]) {
  (void)envp;
  if (!args[1] || args[2]) {
    ft_perror("error: cd: bad arguments", NULL);
    return;
  }
  if (chdir(args[1]) == Error)
    ft_perror("error: cd: cannot change directory to ", args[1]);
}

void exec_cmd(char* args[], char* envp[], int infd, int outfd) {
  int ws;
  res_t ret;

  (void)envp;
  if (*args == NULL)
    return;

  pid_t pid;
  pid = ft_fork();
  if (pid)
    ft_waitpid(pid, &ws, 0);
  else {
    ft_dup2(infd, 0);
    ft_dup2(outfd, 1);
    ret = execve(args[0], args, NULL);
    if (ret == Error) {
      ft_perror("error: cannot execute ", args[0]);
      kill(0, SIGINT);
      exit(1);
    }
  }
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

void exec_pipelines(char** args, char** envp) {
  int start;
  int prev[2];
  int now[2];

  start = 0;
  prev[0] = 0;
  for (int i = 0; args[i]; i++) {
    if (is_str_equal(args[i], "|")) {
      ft_pipe(now);
      args[i] = 0;
      exec_cmd(args + start, envp, prev[PipeRead], now[PipeWrite]);
      if (prev[PipeRead] != 0)
        ft_close(prev[PipeRead]);
      ft_close(now[PipeWrite]);
      swap_pipe(prev, now);
      start = i + 1;
    }
  }
  now[PipeWrite] = 1;
  exec_cmd(args + start, envp, prev[PipeRead], now[PipeWrite]);
}

void exec_cmds(char* args[], char* envp[]) {
  if (*args == 0)
    return;
  if (is_str_equal(*args, "cd"))
    exec_builtin(args, envp);
  else
    exec_pipelines(args, envp);
}

int main(int argc, char* argv[], char* envp[]) {
  int start;

  start = 1;
  for (int i = 1; i < argc; i++) {
    if (is_str_equal(argv[i], ";")) {
      argv[i] = 0;
      exec_cmds(argv + start, envp);
      start = i + 1;
    }
  }
  exec_cmds(argv + start, envp);
}
