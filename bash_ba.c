#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "apue.h"
#include <sys/wait.h>
#include <signal.h>

/*
 * Make argv array (*argvp) for tokens in s which are separated by
 * delimiters.   Return -1 on error or the number of tokens otherwise.
 */
int makeargv(char *s, char *delimiters, char ***argvp);
char *get_line_from_stdin(void)
{
  char *line = NULL;
  size_t bufsize = 0;
  getline(&line, &bufsize, stdin);
  return line;
}

int spawn_child(int my_std_in, int my_std_out, char **args) 
{
  pid_t pid; 
  pid_t wpid = 0;
  int status;

  // creates new proc without copying parent's address space
  pid = vfork();

  if (pid == 0) { //child
    // make stdin the stdin that was passed 
    if (my_std_in != 0) {
      dup2(my_std_in, 0);
      close(my_std_in);
    }
    if (my_std_out != 1) {
      // make stdout the stdin that was passed 
      dup2(my_std_out, 1);
      close(my_std_out);
    }
    if (execvp(args[0], args) == -1) {
      err_sys("bash_ba");
    }
  } else if (pid < 0) {
    err_sys("Failed fork");
  } 
  else { //parent
    do {
      // wait for child to finish (WUNTRACED returns and reports stopped processes)
      wpid = waitpid(pid, &status, WUNTRACED);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status)); //check to see if child status returned
  }

  // 0 if ok, -1 if error
  return wpid;
}

int spawn_children(int num_children, char ***cmds)
{
  //backup stdin
  int stdin_bak = dup(0);
  int child_std_in = 0;
  int fd[2];

  for (int child=0; child<num_children-1; child++) {
    pipe(fd);
    //fd[1] is now write end of pipe, fd[0] is read end of pipe
    spawn_child(child_std_in, fd[1], cmds[child]);
    close(fd[1]); //the child is done writing to the pipe, so we can close the write end
    child_std_in=fd[0]; //the next childs stdin should be the read end of the pipe that this child wrote
  }
  int last_child = num_children-1;
  if (child_std_in != 0) dup2(child_std_in, 0);
  spawn_child(child_std_in, 1, cmds[last_child]);
  dup2(stdin_bak, 0);
  close(stdin_bak);
  return 0;
}

// built-ins
char *builtins[] = {
  "cd",
  "exit",
  "kill",
};

int my_cd(char *path) {
  if (path == NULL) {
    write(STDOUT_FILENO, "bash_ba: cd expects full path argument\n", 39); //temporary
  } else {
    if (chdir(path) != 0) {
      err_ret("bash_ba");
    }
  }
  return 1;
}

int my_kill(char *child_str) {
  int child = atoi(child_str);
  kill(child, SIGQUIT);
  return 1;
}

int my_exit(char *args) {
  exit(0);
  return 1;
}

int (*ptrs_to_builtin_funcs[]) (char *) = {
  &my_cd,
  &my_exit,
  &my_kill,
};

int try_exec_builtin(char *builtin, char *arg) 
{
  int num_builtins = sizeof(builtins) / sizeof(char*);
  for (int i=0; i<num_builtins; i++) {
    if (strcmp(builtin, builtins[i]) == 0) {
      return (*ptrs_to_builtin_funcs[i])(arg);
    }
  }
  return 0;
}

int main() 
{
  char **children;
  char *line;
  int num_children;
  do {
    write(STDOUT_FILENO, "\n> ", 3);
    line = get_line_from_stdin();
    num_children = makeargv(line, "|\n", &children);
    char ***cmds = malloc(num_children*sizeof(char *));
    for (int child=0; child<num_children; child++) {
      makeargv(children[child], " ", &cmds[child]); 
    }
    if (num_children == 1) {
      int try = try_exec_builtin(cmds[0][0], cmds[0][1]);
      if (try == 1) continue;
    }
    spawn_children(num_children, cmds);
    free(cmds);
  } while (1);

}

