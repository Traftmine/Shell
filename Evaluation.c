#include "Evaluation.h"
#include "Shell.h"

#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

// ------ FUNCTIONS ------ //

int valeurStatus(int status);
int simpleCommand(Expression *expr);
int RedirectCommand(Expression *expr);
int pipeCommand(Expression *expr);
int backgroundCommand(Expression *expr);
void sigintHandler(int signum);

// ---- MAIN FUNCTION ---- //

int evaluateExpr(Expression *expr) {

  switch (expr->type) {
    case ET_EMPTY:
      fprintf(stderr, "Expression is empty\n");
      return 1;
      break;
    case ET_SIMPLE: // Commande simple
      return simpleCommand(expr);
      break;
    case ET_REDIRECT: // Redirection
      return RedirectCommand(expr);
      break;
    case ET_SEQUENCE: // Séquence (;)
      if (evaluateExpr(expr->left) == 1 || evaluateExpr(expr->right) == 1) { return 1; }
      return shellStatus;
      break;
    case ET_SEQUENCE_AND: // Séquence conditionnelle (&&)
      if (evaluateExpr(expr->left) == 0) {
        if(evaluateExpr(expr->right) == 0) {return 0;};
      }
      return 1;
      break;
    case ET_SEQUENCE_OR: // Séquence conditionnelle (||)
      if (evaluateExpr(expr->left) == 0) { return 0;}
      else {return evaluateExpr(expr->right);}
      return 1;
      break;
    case ET_BG: // Tâche en arrière-plan (&)
      return backgroundCommand(expr);
      break;
    case ET_PIPE: // Tube (|)
      pipeCommand(expr);
      break;
    default:
      fprintf(stderr, "Match with no type of expression");
      break;
  }
  return shellStatus;
}

// ------ FUNCTIONS ------ //

int valeurStatus(int status) {
  if (WIFEXITED(status)) {
    return WEXITSTATUS(status);
  }
  return 128 + WTERMSIG(status);
}

// --- Simple Command ---//

int simpleCommand(Expression *expr) {
  // Extraction de la commande et de ses arguments de l'expression //
  char *command = expr->argsList.args[0]; // Commande principale
  // Handle the "cd" command separately
  if (strcmp(command, "cd") == 0) {
    if (expr->argc > 1) {
      if (chdir(expr->argv[1]) == 0) {
        printf("Changed directory to %s\n", expr->argv[1]);
      } else { perror("cd"); }
    } else { fprintf(stderr, "cd: missing argument\n"); }
    return 0; // success for "cd"
  }

  int status;
  char *args[64];
  for (int i = 0; i < expr->argc; i++) {
    args[i] = expr->argv[i]; }
  args[expr->argc] = NULL;

  pid_t pid = fork();
  if (pid == -1) {
    // Erreur lors de la création du processus fils
    perror("fork");
    return -1; }
  if (pid == 0) {
    // Code du processus fils
    execvp(command, args);
    // Si execvp renvoie, il y a eu une erreur
    perror("execvp");
    exit(1); // Termine le processus fils en cas d'échec
  } else {
    // Code du processus parent
    if (waitpid(pid, &status, 0) == -1) {
      perror("waitpid");
      return -1;
    }
    return valeurStatus(status);
  }
}

// --- Redirect Command ---//

int RedirectCommand(Expression *expr){
  pid_t pid;

  switch (expr->redirect.type) {
    case REDIR_OUT: // Sortie (> ou >&)
      pid = fork();
      if (pid == 0) {
        int out = open(expr->redirect.fileName, O_CREAT | O_WRONLY | O_TRUNC, 0644);
        if (out == -1) {
          perror("open");
          exit(1);
        }
        if (expr->redirect.fd == -1){
          dup2(out, STDERR_FILENO);
          dup2(out, STDOUT_FILENO); // Redirige stdout vers le fichier
        }
        else {
          dup2(out, STDOUT_FILENO);
        }
        evaluateExpr(expr->left);
        close(out);
        exit(127);
      }
      waitpid(pid, NULL, 0);
      break;
    case REDIR_IN: // Entrée (< ou <&)
      pid = fork();
      if (pid == 0) {
        int in = open(expr->redirect.fileName, O_RDONLY);
        if (in == -1) {
          perror("open");
          exit(1);
        }
        if (expr->redirect.fd == -1) {
          dup2(in, STDIN_FILENO); // Redirige stdin depuis le fichier
          dup2(in, STDERR_FILENO);
        }
        else {
          dup2(in, expr->redirect.fd);
        }
        evaluateExpr(expr->left);
        close(in);
        exit(127);
      }
      waitpid(pid, NULL, 0);
      break;
    case REDIR_APP: // Sortie, mode ajout (>>)
      pid = fork();
      if (pid == 0) {
        int out = open(expr->redirect.fileName, O_CREAT | O_WRONLY | O_APPEND, 0644);
        if (out == -1) {
          perror("open");
          exit(1);
        }
        dup2(out, STDOUT_FILENO);
        evaluateExpr(expr->left);
        close(out);
        exit(127);
      }
      waitpid(pid, NULL, 0);
      break;
    default:
      fprintf(stderr,"No redirect command valid");
      return 1;
      break;
  }
  return shellStatus;
}

// ----Pipe Command ----//

int pipeCommand(Expression *expr) {
  int tube[2];
  if (pipe(tube) == -1) {
    perror("pipe");
    return 1;
  }
  pid_t pid1, pid2;
  if ((pid1 = fork()) == -1) {
    perror("fork");
    return 1;
  }
  if (pid1 == 0) {
    close(tube[0]);
    dup2(tube[1], STDOUT_FILENO);
    close(tube[1]);
    evaluateExpr(expr->left);
    exit(127);
  }
  if ((pid2 = fork()) == -1) {
    perror("fork");
    return 1;
  }
  if (pid2 == 0) {
    close(tube[1]);
    dup2(tube[0], STDIN_FILENO);
    close(tube[0]);
    evaluateExpr(expr->right);
    exit(127);
  }
  close(tube[0]);
  close(tube[1]);

  int status;
  waitpid(pid1, &status, 0);
  waitpid(pid2, &status, 0);

  return valeurStatus(status);
}

// --- Background Command ---//

int backgroundCommand(Expression *expr){
  pid_t child_pid;
  child_pid = fork();

  if (child_pid == 0) {
    pid_t child_child_pid;
    child_child_pid = fork();

    if (child_child_pid == 0) {
      evaluateExpr(expr->left); }
    else if (child_child_pid == -1) { // Si problème de fork avec child_child_pid
      perror("fork");
      exit(1); }

    exit(0); // Si aucun problème
  } else if(child_pid == -1) { // Si problème de fork avec child_pid
    perror("fork");
    exit(1); }

  int status;
  waitpid(child_pid,&status,0);
  return valeurStatus(status);
}