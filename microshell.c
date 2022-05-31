
#define _POSIX_SOURCE  // linux only. to suppress warnings for kill()
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
// clang-format off
// utils
typedef enum { In = 0, Out = 1 } dir_t;
typedef enum { Ok = 0, Error = -1 } res_t;

void copy_pipe(int from[2], int to[2]) { to[In] = from[In]; to[Out] = from[Out]; }
void swap_pipe(int a[2], int b[2]) { int tmp_a[2]; copy_pipe(a, tmp_a); copy_pipe(b, a); copy_pipe(tmp_a, b); }
bool str_eq(char *a, char *b) { return strcmp(a, b) == 0; }

int str_len(char *s) { int i = 0; while(s[i]) i++; return i; }
int ft_write(int fd, char *s) { return write(fd, s, str_len(s)); }
void ft_perror(char *s, char *a) { ft_write(2, s); if (a) ft_write(2, a); ft_write(2, "\n"); }
int chk(int ret) {
  if (ret == Error) {
    ft_perror("error: fatal", NULL);
    kill(0, SIGINT);
    exit(1);
  }
  return ret;
}

// clang-format on
// logic
void run_builtin(char* av[]) {
  if (!av[1] || av[2])
    return ft_perror("error: cd: bad arguments", NULL);
  else if (chdir(av[1]) == Error)
    ft_perror("error: cd: cannot change directory to ", av[1]);
}

void run_cmd(char* av[], char* ev[], int in, int out) {
  if (!av[0])
    return;
  int ws;
  pid_t pid = chk(fork());
  if (pid)
    chk(waitpid(pid, &ws, 0));
  else {
    chk(dup2(in, STDIN_FILENO));
    chk(dup2(out, STDOUT_FILENO));
    if (execve(av[0], av, ev) == Error) {
      ft_perror("error: cannot execute ", av[0]);
      kill(0, SIGINT);
      exit(1);
    }
  }
}

void run_pipe(char* av[], char* ev[]) {
  int in[2], out[2];
  int start = 0;
  in[In] = STDIN_FILENO;
  for (int i = 0; av[i]; i++) {
    if (str_eq(av[i], "|")) {
      chk(pipe(out));
      av[i] = NULL;
      run_cmd(av + start, ev, in[In], out[Out]);
      if (in[In] != STDIN_FILENO)
        chk(close(in[In]));
      chk(close(out[Out]));
      swap_pipe(in, out);
      start = i + 1;
    }
  }
  out[Out] = STDOUT_FILENO;
  run_cmd(av + start, ev, in[In], out[Out]);
}

void run_cmds(char* av[], char* ev[]) {
  if (!av[0])
    return;
  if (str_eq(av[0], "cd"))
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
  return 0;
}
