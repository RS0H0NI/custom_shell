// Author:  Vojtech Aschenbrenner <asch@cs.wisc.edu>, Fall 2023
// Revised: John Shawger <shawgerj@cs.wisc.edu>, Spring 2024
// Revised: Vojtech Aschenbrenner <asch@cs.wisc.edu>, Fall 2024
// Revised: Leshna Balara <lbalara@cs.wisc.edu>, Spring 2025
// Revised: Pavan Thodima <thodima@cs.wisc.edu>, Spring 2026

#include "parser.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

extern char **environ;

// TODO: Parser hook for variable substitution.
char *get_variable(const char *var) {
  (void)var;
  char *val = getenv(var);
  if(val==NULL){
    return "";
  }

  return val;
}

//Finding the executable file for given command (eg: ls)
char *find_exec(char *fpath, char *cmd_name){
  char *path_env = strdup(fpath);
  char *dir = strtok(path_env,":");
  if(dir==NULL){
    free(path_env);
    path_env = NULL;
    return NULL;
  }
  size_t len = 0;
  char *fullpath = malloc(2);
  
  //Finding if the path exists
  while(dir!=NULL){

    //Allocating full path
    free(fullpath);
    len = strlen(dir) + strlen(cmd_name) + 2;
    fullpath = malloc(len);
    strcpy(fullpath, dir);
    strcat(fullpath, "/");
    strcat(fullpath, cmd_name);

    //Checking if the path works
    int ok = access(fullpath,X_OK);
    if(ok==0){
      free(path_env);
      path_env = NULL;
      return fullpath;
    }
    dir = strtok(NULL,":");
  }
  free(fullpath);
  fullpath = NULL;
  free(path_env);
  path_env = NULL;
  return NULL;
}

int handle_builtins(char **command){
  //Coding exit
  if(strcmp(command[0],"exit")==0){
    exit(0);
  }

  //Coding cd [dir]
  if(strcmp(command[0],"cd")==0){
    //printf("here\n");
    //int cmd_length = find_cmd_length(command);
    //printf("%i\n",cmd_length);
    if(command[1]==NULL){
      char *home = getenv("HOME");
      if(home==NULL){
        fprintf(stderr,"cd: HOME not set\n");
        return 1;
      }
      int ch_code = chdir(home);
      if(ch_code!=0){
        perror("cd");
        return 1;
      }
    }
    else{
      //printf("here\n");
      
      int ch_code = chdir(command[1]);
      
      //printf("here too\n");
      if(ch_code!=0){
        perror("cd");
        return 1;
      }
    }
    //What to do in case of more arguments? Nothing
    return 0;
  }

  //Coding env [VAR=val]
  if(strcmp(command[0],"env")==0){
    
    if(command[1]!=NULL){
      char *var = strdup(command[1]);
      char *env_var = strtok(var,"=");
      char *set_var = strtok(NULL,"=");
      if(set_var==NULL){
        int env_rc = setenv(env_var,"",1);
        if(env_rc!=0){
          perror("setenv");
          exit(1);
        }
      }
      else{
        int env_rc = setenv(env_var,set_var,1);
        if(env_rc!=0){
          perror("setenv");
          exit(1);
        }
      }
      free(var);
    }
    else{
      int len = 0;
      while(environ[len]!=NULL){
        //printf("here\n");
        printf("%s\n",environ[len]);
        len = len+1;
      }
    }
    return 0;
  }
  return 2;
}

void exec_command(char **command){
  int built_in = handle_builtins(command);
  if(built_in == 0 || built_in==1){
    fflush(stdout);
    fflush(stderr);
    _exit(built_in);
  }

  //printf("here\n");
  if(command[0][0]=='/'){
    //printf("%s Command:",*command);
    execv(command[0],command);
    fprintf(stderr,"%s: Command not found\n", command[0]);
    fclose(stdin);
    fflush(stdout);
    fflush(stderr);
    _exit(1);
  }
  else{
    char *path = getenv("PATH");
    if(path==NULL){
      setenv("PATH","/bin",1);
      path = getenv("PATH");
    }
    char *ac_path = find_exec(path,command[0]);
    if(ac_path!=NULL){
      execv(ac_path,command);
      fprintf(stderr,"%s: Command not found\n", command[0]);
      fclose(stdin);
      fflush(stdout);
      fflush(stderr);
      _exit(1);
    }
    else{
      fprintf(stderr,"%s: Command not found\n", command[0]);
      fflush(stdout);
      fflush(stderr);
      _exit(1);
    }
  }
  
  
}

int exec_pipelines(struct pipeline* p){
  int num_cmd = p->num_commands;
  int wstatus;
  //Handles 1 command
  if(num_cmd==1){
    int built_in = handle_builtins(p->commands[0].argv);
    if (built_in == 0 || built_in == 1) return built_in;

    int rc=fork();
    //int child_rc = 0;
    if(rc<0){
      perror("fork");
      exit(1);
    }
    else if(rc==0){
      char** cmd = p->commands[0].argv;
      exec_command(cmd);
    }
    else{
      waitpid(rc,&wstatus,0);
    }
    return WIFEXITED(wstatus)?WEXITSTATUS(wstatus):1;
  }

  int pipe_fd[num_cmd-1][2];//Stores file descriptors for all pipes

  //Creating pipelines
  for(int i = 0; i<num_cmd-1; i++){
    if(pipe(pipe_fd[i]) == -1){
      perror("pipe");
      exit(1);
    }
  }

  pid_t pids[num_cmd];//to store pids
  for (int i = 0; i < num_cmd; i++) {
    pid_t rc = fork();
    if (rc < 0) {
      perror("fork");
      exit(1);
    }
    else if (rc == 0) {
      //read stdin from the previous pipe's read end if not first command
      if (i > 0) {
        if (dup2(pipe_fd[i - 1][0], STDIN_FILENO) == -1) {
          perror("dup2");
          fflush(stdout);
          fflush(stderr);
          _exit(1);
        }
      }

      //write stdout to the current pipe's write end, if not last command
      if (i < num_cmd - 1) {
        if (dup2(pipe_fd[i][1], STDOUT_FILENO) == -1) {
          perror("dup2");
          fflush(stdout);
          fflush(stderr);
          _exit(1);
        }
      }

      //Close all pipes
      for (int j = 0; j < num_cmd - 1; j++) {
        close(pipe_fd[j][0]);
        close(pipe_fd[j][1]);
      }
      exec_command(p->commands[i].argv);
      //exit(1);
    }
    else {
      pids[i] = rc;
    }
  }

    //Close pipes
    for (int i = 0; i < num_cmd - 1; i++) {
      close(pipe_fd[i][0]);
      close(pipe_fd[i][1]);
    }

    //Wait for all children
    int fin_status = 0;
    for (int i = 0; i < num_cmd; i++) {
      waitpid(pids[i], &fin_status, 0);
    }
    return fin_status;
  }


int main(int argc, char **argv) {
  (void)argc;
  (void)argv;

  //Incorrect number of arguments
  if(argc!=2 && argc!=1){
    fprintf(stderr,"Usage: %s [file]\n", argv[0]);
    exit(1);
  }

  //Batch Mode or Interactive
  int interactive=0;
  FILE *file_ptr = NULL;
  if(argc==1){
    interactive = isatty(STDIN_FILENO);
    file_ptr = stdin;
  }
  if(argc==2){
    file_ptr = fopen(argv[1],"r");
    if(file_ptr == NULL){
      perror("fopen");
      exit(1);
    }
  }

  //Execution
  char* line = NULL;
  size_t len = 0;
  int nread = 0;
  int fin_status = 0;
  while(1){
    
    if(interactive){
      printf("wsh> ");
      fflush(stdout);
    }
    nread = getline(&line,&len,file_ptr);
  
    if(nread==-1){
      free(line);
      line=NULL;
      if(file_ptr!=stdin){
        fclose(file_ptr);
      }
      
      exit(fin_status);
    }
    //dealt with EOF above
    
    //if(nread==-1) continue;

    struct command_line* cmd = parse_input(line);//Getting the list of commands
    if(cmd==NULL){
      continue;
    }
    
    for(int i = 0; i<cmd->num_pipelines; i++){
      fin_status = exec_pipelines(&cmd->pipelines[i]);
    }
    free_command_line(cmd);
  }
  free(line);
  if(file_ptr!=stdin) fclose(file_ptr);
  return 0;
}
