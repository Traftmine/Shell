#include "Evaluation.h"
#include "Shell.h"

#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

// ------ FUNCTIONS ------ //

int valeurStatus(int status);

// ---- MAIN FUNCTION ---- //

int evaluateExpr(Expression *expr) {

  if (expr->type == ET_EMPTY) { // Commande vide
    fprintf(stderr, "Expression is empty\n");
    return EXIT_SUCCESS;
  }

  int status;

  // Extraction de la commande et de ses arguments de l'expression //
  char *command = expr->argsList.args[0]; // Commande principale

  char *args[64];
  int arg_count = 0;
  char *token = strtok(command, " ");

  while (token != NULL) {
      args[arg_count] = token;
      arg_count++;
      token = strtok(NULL, " ");
  }
  args[arg_count] = NULL;

  pid_t pid;

  switch (expr->type) {
    case ET_SIMPLE: // Commande simple
      pid = fork();
      if (pid == -1) {
        // Erreur lors de la création du processus fils
        perror("fork");
        return -1;
      }
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

      break;
    case ET_REDIRECT: // Redirection
      switch (expr->redirect.type) {
        case REDIR_OUT: // Sortie (> ou >&)
          pid = fork();
          if (pid == 0) {
            int out = open(expr->redirect.fileName, O_CREAT | O_WRONLY | O_TRUNC, 0644);
            if (out == -1) {
              perror("open");
              exit(1);
            }
            dup2(out, expr->redirect.fd); // Redirige stdout vers le fichier
            close(out);
            execvp(command, args);
            perror("execvp");
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
            dup2(in, expr->redirect.fd); // Redirige stdin depuis le fichier
            close(in);
            execvp(command, args);
            perror("execvp");
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
            dup2(out, STDOUT_FILENO); // Redirige stdout vers le fichier en mode prolongation (>>)
            close(out);
            execvp(command, args);
            perror("execvp");
            exit(127);
          }
          waitpid(pid, NULL, 0);
          break;
      }
    case ET_SEQUENCE: // Séquence (;)
      break;
    case ET_SEQUENCE_AND: // Séquence conditionnelle (&&)
      break;
    case ET_SEQUENCE_OR: // Séquence conditionnelle (||)
      break;
    case ET_BG: // Tâche en arrière-plan (&)
      break;
    case ET_PIPE: // Tube (|)
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
