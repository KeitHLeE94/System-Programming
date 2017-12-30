#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "stack.h"

char current_directory[256];
char command[256];
stack* history_stack;

void command_executor(char* command);
void redirect(char* command);
void other_command(char* command);
void command_pipe(char* command1, char* command2);
void multi_pipe(char* command1, char* command2, char* command3);
void command_history(char* command);

int main(int argc, char* argv[]){
  history_stack = malloc(sizeof(node));
  history_stack->top = 0;
  history_stack->header = NULL;
  node* new_node;

  while(1){
    getcwd(current_directory, 256);
    printf("[%s]$ ", current_directory);
    fgets(command, 256, stdin);
    command[strlen(command) - 1] = '\0';
    if(strstr(command, "!") == NULL){
      new_node = create_node(command);
      push(history_stack, new_node);
    }

    int count = 0, i;
    char* command_list[100];

    if(strstr(command, "&") != NULL){
      pid_t pid;
      char* args[100];
      int i=1;
      int status;

      char* order = malloc(sizeof(char) * 1000);
      strcpy(order, command);
      order[strlen(order) - 2] = '\0';

      char* token = strtok(order, " ");
      args[0] = token;

      while(token != NULL){
        token = strtok(NULL, " ");
        args[i] = token;
        i++;
      }

      if((pid = fork()) < 0){
        perror("fork error: ");
        exit(-1);
      }
      else if(pid == 0){
        execvp(args[0], args);
        printf("[%d]+\tDone\t\t\t%s\n", getpid(), *args);
      }
      else{
        printf("\n[%d]\n", getpid());
        continue;
        waitpid(getpid(), &status, WNOHANG);
      }
      free(order);
    }

    if(strstr(command, "cd") != NULL){
      char* directory = strstr(command, " ") + 1;

      if(chdir(directory)){
        perror("cd error: ");
        exit(-1);
      }
      else{
        continue;
      }
    }

    if(strstr(command, ";") != NULL){
      char* token = strtok(command, ";");
      while(token != NULL){
        command_list[count] = token;
        token = strtok(NULL, ";");
        count++;
      }
      for(i=0; i<count; i++){
        if(command_list[i][0] == ' '){
          command_list[i] = strstr(command_list[i], " ") + 1;
        }
        if(command_list[i][strlen(command_list[i]) - 1] == ' '){
          command_list[i][strlen(command_list[i]) - 1] = '\0';
        }
      }
      for(i=0; i<count; i++){
        command_executor(command_list[i]);
      }
    }
    else{
      command_executor(command);
    }
  }
  return 0;
}

void command_executor(char* command){
  char menu;
  int pipe_count=0, i=0;
  while(command[i] != '\0'){
    if(command[i] == '|'){
      pipe_count++;
    }
    i++;
  }
  print_stack(history_stack);

  if((strstr(command, "<") != NULL) || (strstr(command, ">") != NULL)){
    redirect(command);
  }
  if(strstr(command, "!") != NULL){
    command_history(command);
  }
  if(pipe_count == 1){
    int count = 0, i;
    char* command_list[100];
    char* token = strtok(command, "|");
    while(token != NULL){
      command_list[count] = token;
      token = strtok(NULL, "|");
      count++;
    }
    for(i=0; i<count; i++){
      if(command_list[i][0] == ' '){
        command_list[i] = strstr(command_list[i], " ") + 1;
      }
      if(command_list[i][strlen(command_list[i]) - 1] == ' '){
        command_list[i][strlen(command_list[i]) - 1] = '\0';
      }
    }
    command_pipe(command_list[0], command_list[1]);
  }
  else if(pipe_count > 1){
    char* command_list[100];
    int count=0;

    char* token = strtok(command, "|");
    while(token != NULL){
      command_list[count] = token;
      token = strtok(NULL, "|");
      count++;
    }
    for(i=0; i<count; i++){
      if(command_list[i][0] == ' '){
        command_list[i] = strstr(command_list[i], " ") + 1;
      }
      if(command_list[i][strlen(command_list[i]) - 1] == ' '){
        command_list[i][strlen(command_list[i]) - 1] = '\0';
      }
    }
    multi_pipe(command_list[0], command_list[1], command_list[2]);
  }
  else{
    other_command(command);
  }
}

void redirect(char* command_red){
  pid_t pid;
  char* args[100];
  int status;

  if(strstr(command_red, ">>") != NULL){
    char* dest = strstr(command_red, ">>") + 3;
    if(strstr(dest, "&") != NULL){
      dest[strlen(dest) - 1] = '\0';
    }
    int fd, n;
    int i=0;

    char* token = strtok(command_red, " ");

    while(strcmp(token, ">>") != 0){
      args[i] = token;
      token = strtok(NULL, " ");
      i++;
    }

    if((fd = open(dest, O_RDWR | O_APPEND | O_CREAT)) == -1){
      perror("file open error: ");
      exit(-1);
    }

    if((pid = fork()) < 0){
      perror("fork error: ");
      exit(-1);
    }
    else if(pid == 0){
      close(STDOUT_FILENO);
      dup(fd);
      close(fd);
      execvp(args[0], args);
      exit(1);
    }
    else{
      waitpid(pid, NULL, 0);
      dup(STDOUT_FILENO);
    }
  }

  else if(strstr(command_red, ">!") != NULL){
    char* dest = strstr(command_red, ">!") + 3;
    if(strstr(dest, "&") != NULL){
      dest[strlen(dest) - 1] = '\0';
    }
    int fd, n;
    int i=0;

    char* token = strtok(command_red, " ");

    while(strcmp(token, ">!") != 0){
      args[i] = token;
      token = strtok(NULL, " ");
      i++;
    }

    if((fd = open(dest, O_RDWR | O_CREAT)) == -1){
      perror("file open error: ");
      exit(-1);
    }

    if((pid = fork()) < 0){
      perror("fork error: ");
      exit(-1);
    }
    else if(pid == 0){
      close(STDOUT_FILENO);
      dup(fd);
      close(fd);
      execvp(args[0], args);
      exit(1);
    }
    else{
      waitpid(pid, NULL, 0);
      dup(STDOUT_FILENO);
    }
  }

  else if(strstr(command_red, ">") != NULL){
    char* dest = strstr(command_red, ">") + 2;
    if(strstr(dest, "&") != NULL){
      dest[strlen(dest) - 1] = '\0';
    }
    int fd, n;
    int i=0;

    char* token = strtok(command_red, " ");

    while(strcmp(token, ">") != 0){
      args[i] = token;
      token = strtok(NULL, " ");
      i++;
    }

    if((fd = open(dest, O_RDWR | O_TRUNC | O_CREAT)) == -1){
      perror("file open error: ");
      exit(-1);
    }

    if((pid = fork()) < 0){
      perror("fork error: ");
      exit(-1);
    }
    else if(pid == 0){
      close(STDOUT_FILENO);
      dup(fd);
      close(fd);
      execvp(args[0], args);
      exit(1);
    }
    else{
      waitpid(pid, NULL, 0);
      dup(STDOUT_FILENO);
    }
  }

  else if(strstr(command_red, "<") != NULL){
    char* input = strstr(command_red, "<") + 2;
    if(strstr(input, "&") != NULL){
      input[strlen(input) - 1] = '\0';
    }
    int fd, n;
    int i=0;

    char* token = strtok(command_red, " ");

    while(strcmp(token, "<") != 0){
      args[i] = token;
      token = strtok(NULL, " ");
      i++;
    }

    token = strtok(NULL, " ");

    while(token != NULL){
      args[i] = token;
      token = strtok(NULL, " ");
      i++;
    }

    if((fd = open(input, O_RDWR)) == -1){
      perror("file open error: ");
      exit(-1);
    }

    if((pid = fork()) < 0){
      perror("fork error: ");
      exit(-1);
    }
    else if(pid == 0){
      close(STDIN_FILENO);
      dup(fd);
      close(fd);
      execvp(args[0], args);
      exit(1);
    }
    else{
      waitpid(pid, NULL, 0);
      dup(STDIN_FILENO);
    }
  }
  return;
}

void other_command(char* command_other){
  pid_t pid;
  char* args[100];
  int i=1;

  if(strstr(command_other, " ") == NULL){
    if((pid = fork()) < 0){
      perror("fork error: ");
      exit(-1);
    }
    else if(pid == 0){
      execlp(command_other, command_other, NULL);
    }
    else{
      waitpid(pid, NULL, 0);
    }
  }
  else{
    char* token = strtok(command_other, " ");
    args[0] = token;

    while(token != NULL){
      token = strtok(NULL, " ");
      args[i] = token;
      i++;
    }

    if((pid = fork()) < 0){
      perror("fork error: ");
      exit(-1);
    }
    else if(pid == 0){
      execvp(args[0], args);
      exit(1);
    }
    else{
      waitpid(pid, NULL, 0);
    }
  }
}

void command_pipe(char* command1, char* command2){
  pid_t pid;
  int p[2];
  char* args1[100];
  char* args2[100];
  int i=1, j=1, status, k;

  char* token1 = strtok(command1, " ");
  args1[0] = token1;

  while(token1 != NULL){
    token1 = strtok(NULL, " ");
    args1[i] = token1;
    i++;
  }

  char* token2 = strtok(command2, " ");
  args2[0] = token2;

  while(token2 != NULL){
    token2 = strtok(NULL, " ");
    args2[j] = token2;
    j++;
  }

  if(pipe(p) == -1){
    perror("pipe error: ");
    exit(-1);
  }

  if(fork() == 0){
    dup2(p[1], 1);
    close(p[0]);
    close(p[1]);
    execvp(args1[0], args1);
    exit(1);
  }
  if(fork() == 0){
    dup2(p[0], 0);
    close(p[0]);
    close(p[1]);
    execvp(args2[0], args2);
    exit(1);
  }
  close(p[0]);
  close(p[1]);
  dup(0);
  dup(1);

  for(k=0; k<2; k++){
    wait(&status);
  }
}

void multi_pipe(char* command1, char* command2, char* command3){
  pid_t pid;
  int p[4];
  char* args1[100];
  char* args2[100];
  char* args3[100];
  int i=1, j=1, k=1, l;
  int status;

  char* token1 = strtok(command1, " ");
  args1[0] = token1;

  while(token1 != NULL){
    token1 = strtok(NULL, " ");
    args1[i] = token1;
    i++;
  }

  char* token2 = strtok(command2, " ");
  args2[0] = token2;

  while(token2 != NULL){
    token2 = strtok(NULL, " ");
    args2[j] = token2;
    j++;
  }

  char* token3 = strtok(command3, " ");
  args3[0] = token3;

  while(token3 != NULL){
    token3 = strtok(NULL, " ");
    args3[k] = token3;
    k++;
  }

  if(pipe(p) == -1){
    perror("pipe error: ");
    exit(-1);
  }
  if(pipe(p+2) == -1){
    perror("pipe error: ");
    exit(-1);
  }

  if((pid = fork()) < 0){
    perror("fork error: ");
    exit(-1);
  }
  else if(pid == 0){
    dup2(p[1], 1);
    close(p[0]);
    close(p[1]);
    close(p[2]);
    close(p[3]);
    execvp(args1[0], args1);
  }
  else{
    if((pid = fork()) < 0){
      perror("fork error: ");
      exit(-1);
    }
    else if(pid == 0){
      dup2(p[0], 0);
      dup2(p[3], 1);
      close(p[0]);
      close(p[1]);
      close(p[2]);
      close(p[3]);
      execvp(args2[0], args2);
    }
    else{
      if((pid = fork()) < 0){
        perror("fork error: ");
        exit(-1);
      }
      else if(pid == 0){
        dup2(p[2], 0);
        close(p[0]);
        close(p[1]);
        close(p[2]);
        close(p[3]);
        execvp(args3[0], args3);
      }
    }
  }
  close(p[0]);
  close(p[1]);
  close(p[2]);
  close(p[3]);

  for(l=0; l<3; l++){
    wait(&status);
  }
}

void command_history(char* command){
  char* hist = search(history_stack, command);
  command_executor(hist);
}
