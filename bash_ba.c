#include "apue.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>

/*
 * Make argv array (*argvp) for tokens in s which are separated by
 * delimiters.   Return -1 on error or the number of tokens otherwise.
 */
int makeargv(char *s, char *delimiters, char ***argvp);

// global variables 
static int fg = 1;   //for keeping track of whether running in bg or fg
static char *home = NULL;
char *cwd;
int cwd_size;

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
    //printf("fg: %d\n", fg);
    //fflush(stdout);
    if (fg) {
      do {
        // wait for child to finish (WUNTRACED returns and reports stopped processes)
        wpid = waitpid(pid, &status, WUNTRACED);
      } while (!WIFEXITED(status) && !WIFSIGNALED(status)); //check to see if child status returned
    }
  }
  // 0 if ok, -1 if error
  return wpid;
}

int spawn_children(int num_children, char ***cmds)
{
  //backup stdin
  int stdin_bak = dup(0);
  int child_std_in = 0;
  int final_stdout = 1; //where output goes once entire cmd (including pipes) has completed 
  int fd[2];
  char *token;

  // handle redirection in
  char redirection_detected = 0;
  for (int i=0; (token=cmds[0][i]) != NULL; i++) {
    if (!redirection_detected && !strcmp(token, "<")) {
      child_std_in = open(cmds[0][i+1], O_RDONLY);
      if (child_std_in < 0) {
        err_ret(cmds[0][i+1]); 
        return 1;
      }
      // remove the "<" token and the file name token that follows from list
      // of tokens for first cmd
      cmds[0][i] = NULL;
      cmds[0][++i] = NULL; // increment i, so that we skip the now null file name token
      redirection_detected = 1;
    }
    else if (redirection_detected) { // need to move tokens up 2 positions in arg list
      cmds[0][i-2] = cmds[0][i];
      cmds[0][i] = NULL;
    }
  }
  
  int fg_bak = fg; //save state for last child
  fg = 0; //all children run in background until last
  for (int child=0; child<num_children-1; child++) {
    pipe(fd);
    //fd[1] is now write end of pipe, fd[0] is read end of pipe
    spawn_child(child_std_in, fd[1], cmds[child]);
    close(fd[1]); //the child is done writing to the pipe, so we can close the write end
    child_std_in=fd[0]; //the next childs stdin should be the read end of the pipe that this child wrote
  }

  // handle redirection out
  int last_child = num_children-1;
  if (child_std_in != 0) dup2(child_std_in, 0);
  mode_t final_stdout_mode = O_RDWR | O_CREAT;
  for (int i=0; (token=cmds[last_child][i]) != NULL; i++) {
    if (!strcmp(token, ">")) {
      final_stdout = open(cmds[last_child][i+1], final_stdout_mode, 0644);
      if (child_std_in < 0) {
        err_ret(cmds[last_child][i+1]); 
        return 1;
      }
      // remove the redirection token and the file name token that follows from list
      // of tokens for first cmd
      cmds[last_child][i] = NULL;
      cmds[last_child][i+1] = NULL; // increment i, so that we skip the now null file name token
      break;
    }
    if (!strcmp(token, ">>")) {
      final_stdout = open(cmds[last_child][i+1], final_stdout_mode|O_APPEND, 0644);
      if (child_std_in < 0) {
        err_ret(cmds[last_child][i+1]); 
        return 1;
      }
      // remove the redirection token and the file name token that follows from list
      // of tokens for first cmd
      cmds[last_child][i] = NULL;
      cmds[last_child][i+1] = NULL; // increment i, so that we skip the now null file name token
      break;
    }
  }

  fg = fg_bak; //restore fg so that we are only running last child process in background if we were told to
  spawn_child(child_std_in, final_stdout, cmds[last_child]);
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
  char cwd_reset = 0;
  if (path == NULL && chdir(home) == 0) cwd_reset = 1;
  else if (path != NULL && chdir(path) == 0) cwd_reset = 1;
  else err_ret(path);
  
  if (cwd_reset) {
    cwd = path_alloc(cwd, &cwd_size);
    if (getcwd(cwd, cwd_size) == NULL) err_sys("getcwd fail");
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
  // initialize vars
  char **children, *line, *line_no_leading_spaces;
  home = getenv("HOME");
  int child, num_children;
  cwd = path_alloc(cwd, &cwd_size);
  getcwd(cwd, cwd_size);

  // main loop
  do {
    // get cwd and write it to beginning of cmd prompt
    write(STDOUT_FILENO, "\n", 1);
    if (cwd) write(STDOUT_FILENO, cwd, strlen(cwd));
    write(STDOUT_FILENO, "> ", 2);

    // read input from cmd prompt and skip commands that start with white space
    line = get_line_from_stdin();
    line_no_leading_spaces = line + strspn(line, " "); 
    if (!strcmp(line_no_leading_spaces, "\n")) continue;

    // children is a char**. each char* points to a child.
    // I use the term child to describe a section of input that will be forked
    // into its own process
    //
    // mapping input to corresponding char **children would look like
    //    ls                => [["ls"]];                   (child 0 = ["ls"]
    //    ls -la            => [["ls -la"]];               (child 0 = ["ls -la"]
    //    ls -la | grep dup => [["ls -la"]["grep dup"]]    (child 0 = ["ls -la"], child 1 = ["grep dup"])
    num_children = makeargv(line, "|\n", &children);

    // cmds is a char***. each char* points to a token within a child.
    //
    // a similar mapping would look like
    //    ls                => [[["ls"]]];                   
    //    ls -la            => [ [ ["ls"], ["-la"] ] ];      (child 0 = [["ls"], ["-la"]], token 0 = ["ls"]
    //    ls -la | grep dup => [ [ ["ls"],   ["-la"] ], 
    //                           [ ["grep"], ["dup"] ] ]     
    char ***cmds = malloc(num_children*sizeof(char *));
    for (child=0; child<num_children-1; child++) {
      makeargv(children[child], " ", &cmds[child]);
    }

    // the last child is special.
    // if last child ends with &, then we have to wait for it
    // we also have to remove the & from the cmds char*** so that it doesn't
    // throw errors when we exec.
    int last_token = makeargv(children[child], " ", &cmds[child]) -1;
    unsigned long last_token_idx = strlen(cmds[child][last_token]) -1;
    fg = strcmp(cmds[child][last_token] + last_token_idx, "&"); //if ends in &, sets fg (foreground) to 0
    if (!fg && last_token_idx) *(cmds[child][last_token]+last_token_idx) = '\0'; //removes & in the case that  
                                                      //there are no spaces between last child and cmd (e.g., ls&)
    else if (!fg) cmds[child][last_token] = NULL; //removes & in the case that there are spaces (e.g., ls &)

    // if no pipes try to satisfy child with builtin
    // if child satisfied, then restart loop
    // else spawn child processes 0x605100
    if (num_children == 1) {
      int try = try_exec_builtin(cmds[0][0], cmds[0][1]);
      if (try == 1) goto reentry;
    }
    spawn_children(num_children, cmds);

reentry:
    for(child=0; child<num_children; child++) {
      free(cmds[child][0]);
      free(cmds[child]);
    }
    free(children[0]);
    free(children);
    free(cmds);
  } while (1);

  exit(0);
}

