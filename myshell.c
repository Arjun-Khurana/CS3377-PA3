#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

//this is a new comment!

#define DELIMS " \t\n"

void execute(char** cmd);
int parseline(char* line, char** tokens);
int findPipeRedirect(int argc, char** tokens, char** cmd1, char** cmd2);
void execute_pipe(char** cmd1, char** cmd2);
void execute_redirect(int type, char** cmd, char** file);

int main(int argc, char* argv[])
{
  //printf("Hello World\n");
  while (1)
  {
    char* line = NULL;
    ssize_t bufsize = 0;
    printf("myshell > ");
    getline (&line, &bufsize, stdin);
    char** cmd = malloc(sizeof(char*));
    int argc = parseline(line, cmd);
    char** cmd1 = malloc(sizeof(cmd));
    char** cmd2 = malloc(sizeof(cmd));

    int pr = findPipeRedirect(argc, cmd, cmd1, cmd2);
    
    switch(pr)
    {
      case 0:
        if (!strcmp(cmd[0], "exit"))
        {
          return 0;
        }
        else
        {
          execute(cmd);
        }
      break;
      case 1:
        //printf("pipe found\n");
        execute_pipe(cmd1, cmd2);
      break;
      case 2:
      case 3:
      case 4:
        //printf("redirect found\n");
        execute_redirect(pr, cmd1, cmd2);
      break;
      case 5:
        execute(cmd1);
        execute(cmd2);
      break;
      default:
        printf("BIG ERROR");
        exit(1);
      break;
    }
  }
  return 0;
}

void execute(char** cmd)
{
  pid_t pid = fork();
  if (pid < 0)
  {
    perror("FORK ERROR");
    exit(1);
  }
  else if (pid == 0)
  {
    if (execvp(cmd[0],cmd) < 0)
    {
      perror("EXECUTION ERROR");
      exit(1);
    } 
  }
  else
  {
    waitpid(pid, NULL, 0);
    return;
  }
}

void execute_pipe(char** cmd1, char** cmd2)
{
  int fds[2];
  if (pipe(fds) < 0)
  {
    perror("PIPE ERROR");
    exit(1);
  }
  pid_t pid1, pid2;

  if (pid1 = fork() == 0)
  {
    dup2(fds[1], 1);
    close(fds[0]);
    
    if (execvp(cmd1[0],cmd1) < 0)
    {
      perror("EXECUTION ERROR");
      exit(1);
    }
  }
  else if (pid2 = fork() == 0)
  {
    dup2(fds[0], 0);
    close(fds[1]);
    
    if (execvp(cmd2[0],cmd2) < 0)
    {
      perror("EXECUTION ERROR");
      exit(1);
    }
  }
  else
  {
    close(fds[0]);
    close(fds[1]);
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
  }
}

void execute_redirect(int type, char** cmd, char** file)
{
  int in, out;
  pid_t pid = fork();
  switch(type)
  {
    case 2:
      out = open(file[0], O_WRONLY | O_CREAT, 0666);
      if (out < 0)
      {
        perror("OPEN ERROR");
        exit(1);
      }
      if (pid == 0)
      {
        dup2(out, 1);
        close(out);
        if (execvp(cmd[0], cmd) < 0)
        {
          perror("EXECUTION ERROR");
          exit(1);
        }
      }
      else
      {
        waitpid(pid, NULL, 0);
      }
    break;
    case 3:
      in = open(file[0], O_RDONLY, 0666);
      if (in < 0)
      {
        perror("OPEN ERROR");
        exit(1);
      }
      if (pid == 0)
      {
        dup2(in, 0);
        close(in);
        if (execvp(cmd[0], cmd) < 0)
        {
          perror("EXECUTION ERROR");
          exit(1);
        }
      }
      else
      {
        waitpid(pid, NULL, 0);
      }
    break;
    case 4:
      out = open(file[0], O_WRONLY | O_APPEND, 0666);
      if (out < 0)
      {
        perror("OPEN ERROR");
        exit(1);
      }
      if (pid == 0)
      {
        dup2(out, 1);
        close(out);
        if (execvp(cmd[0], cmd) < 0)
        {
          perror("EXECUTION ERROR");
          exit(1);
        }
      }
      else
      {
        waitpid(pid, NULL, 0);
      }
    break;
    default:
      printf("BIG ERROR");
    break;
  }
}

int parseline(char* line, char** tokens)
{
  int pos = 0;
  char* token;
  token = strtok(line, DELIMS);
  while (token != NULL)
  {
    //printf("%s\n", token);
    tokens[pos] = token;
    pos++;
    tokens = realloc(tokens, (pos + 1) * sizeof(char*));
    if (!tokens)
    {
      perror("ALLOCATION ERROR");
    }
    token = strtok(NULL, DELIMS);
  }
  tokens[pos] = NULL;

  return pos;
}

int findPipeRedirect(int argc, char** tokens, char** cmd1, char** cmd2)
{
  int retval = 0;
  int pos = -1;
  
  for (int i = 0; i < argc; i++)
  {
    if (!strcmp(tokens[i], "|"))
    {
      retval = 1;
      pos = i;
      break;
    }
    else if (!strcmp(tokens[i], ">"))
    {
      retval = 2;
      pos = i;
      break;
    }
    else if (!strcmp(tokens[i], "<"))
    {
      retval = 3;
      pos = i;
      break;
    }
    else if(!strcmp(tokens[i], ">>"))
    {
      retval = 4;
      pos = i;
      break;
    }
    else if(index(tokens[i], ';'))
    {
      char* src = tokens[i];
      char* dest;
      memcpy(dest, src, strlen(src)-1);
      tokens[i] = dest;
      retval = 5;
      pos = i+1;
      break;
    }
  }

  switch(retval)
  {
    case 1:
    case 2:
    case 3:
    case 4:
      for (int i = 0; i < pos; i++)
      {
        cmd1[i] = tokens[i];
        //printf("%s\n", cmd1[i]);
      }
      int i = 0;
      for (int j = pos + 1; j < argc; j++)
      {
        cmd2[i] = tokens[j];
        i++;
        //printf("%s\n", cmd2[i]);
      }
      cmd1[pos] = NULL;
      cmd2[i] = NULL;
    break;
    case 5:
      //printf(cmd1[0]);
      //printf(cmd2[0]);
      for (int i = 0; i < pos; i++)
      {
        //k = pos + i;
        cmd1[i] = tokens[i];
        //cmd2[i] = tokens[pos + i];
      }
      int k = 0;
      for (int j = pos; j < argc; j++)
      {
        //printf("%s\n", tokens[j]);
        cmd2[k] = tokens[j];
        //k++;
      }
      //cmd1[pos] = NULL;
      //cmd2[k] = NULL;
    break;
    default:
    break;
  }

  return retval;
}
