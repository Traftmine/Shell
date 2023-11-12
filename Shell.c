#include "Shell.h"
#include "Display.h"
#include "Evaluation.h"

#include <stdio.h>
#include <unistd.h>
#include <limits.h>  // For PATH_MAX

#include <readline/history.h>
#include <readline/readline.h>

Shell shell;
bool interactiveMode;
int shellStatus = 0;

// Fonction appelée lorsque l'utilisateur tape Ctrl+D
void EndOfFile(void) {
  printf("\n");
  exit(shellStatus);
}

// Appelée par yyparse() sur erreur syntaxique
void yyerror(char const *str) { fprintf(stderr, "%s\n", str); }

// Function to get the current working directory
char* getCurrentWorkingDirectory() {
  static char cwd[PATH_MAX];
  if (getcwd(cwd, sizeof(cwd)) != NULL) {
    return cwd;
  } else {
    perror("getcwd");
    return NULL;
  }
}

// Extracts the last component of the path (directory or file name)
const char* getLastPathComponent(const char* path) {
  const char* lastSlash = strrchr(path, '/');
  return (lastSlash != NULL) ? (lastSlash + 1) : path;
}

int parseLine(void) {
  if (interactiveMode) {
    char computerName[256];
    if (gethostname(computerName, sizeof(computerName)) != 0) {
      perror("Error getting computer name");
    }

    char *dotLocal = strstr(computerName, ".local");
    if (dotLocal != NULL) {
      *dotLocal = '\0'; // Remplace le point avec un caractère null
    }

    char computerName2[256];
    strcpy(computerName2, computerName); // Copie le contenu de computerName dans computerName2

    char *lastDash = strrchr(computerName2, '-'); // Recherche du dernier "-"
    if (lastDash != NULL) {
      lastDash++; // Avance d'une position pour obtenir le mot après le dernier "-"
    }
    for (int i = 0; lastDash[i] != '\0'; i++) {
      if (lastDash[i] >= 'A' && lastDash[i] <= 'Z') {
        lastDash[i] = lastDash[i] + 32; // Convertit le caractère en minuscule
      }
    }
    char prompt[256]; // Increased buffer size to accommodate the path
    snprintf(prompt, sizeof(prompt),
             "\33[1m%s@%s %s(\33[3%dm%d\33[0;1m) %%\33[0m ", lastDash, computerName,
              getLastPathComponent(getCurrentWorkingDirectory()),shellStatus == EXIT_SUCCESS ? 2 /* vert */ : 1 /* rouge */,
             shellStatus);

    char *line = readline(prompt);
    if (line != NULL) {
      int ret;
      size_t len = strlen(line);
      if (len > 0) {
        add_history(line); // Enregistre la ligne non vide dans l'historique
      }
      line = realloc(line, len + 2);
      strcpy(line + len, "\n"); // Ajoute \n à la ligne pour qu'elle puisse être
                                // traitée par l'analyseur
      ret = yyparse_string(line);
      free(line);
      return ret;
    } else {
      EndOfFile();
      return -1;
    }
  } else {
    return yyparse();
  }
}

void commandExecution(int parsingResult) {
  if (parsingResult == 0) { // L'analyse a abouti
    if (shell.showExprTree) {
      printExpr(shell.parsedExpr);
    }
    shellStatus = evaluateExpr(shell.parsedExpr);
    freeExpression(shell.parsedExpr);
  } else { // L'analyse de la ligne de commande a donné une erreur
    shellStatus = 2;
  }
}

int main(int argc, char *argv[]) {
  shell.args = (ProgramArgs){argc, argv};
  interactiveMode = isatty(STDIN_FILENO);
  if (interactiveMode) {
    using_history();
  }
  shell.showExprTree = interactiveMode;

  while (true) {
    commandExecution(parseLine());
  }

  printf("\n"); // Comme dans EndOfFile()
  return shellStatus;
}
