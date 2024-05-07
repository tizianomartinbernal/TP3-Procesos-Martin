#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <string.h>
#include <stdbool.h>


#define MAX_COMMANDS 200
#define MAX_ARGS 256

/*
Removes double and single quotes from a string.

Parameters:
- string: The string from which quotes will be removed.
*/
void remove_quotes(char *string) {
    int i, j = 0;
    for (i = 0; string[i] != '\0'; i++) {
        if (string[i] != '"' && string[i] != '\'') {
            string[j++] = string[i];
        }
    }
    string[j] = '\0';
}

/*
Tokenizes a command into an array of commands.

Parameters:
- command: The command to be tokenized.
- commands: The array of commands.
- command_count: The number of commands in the array.
*/
void tokenize(char *command, char *commands[], int *command_count){
    char *token = strtok(command, "|");

    while (token != NULL){
        commands[(*command_count)++] = token;
        token = strtok(NULL, "|");
        }
}

/*
Creates pipes for the number of commands.

Parameters:
- pipes: The array of pipes.
- command_count: The number of commands.

Returns:
- 0 if the pipes were created successfully.
- -1 if an error occurred.
*/
int create_pipes(int pipes[][2], int command_count){
    for (int i = 0; i < command_count; i++){
        if (pipe(pipes[i]) == -1){
            return -1;
            }
        }
    return 0;
}

/*
Redirects the standard input and output for the child processes and closes the pipes.

Parameters:
- pipes: The array of pipes.
- command_count: The number of commands.
- i: The index of the current command.
*/
void redirect_and_close(int pipes[][2], int command_count, int i){
    if (i == 0 && command_count > 1){
        for (int j = 0; j < command_count; j++){
            if (j != 0){
                close(pipes[j][0]);
                close(pipes[j][1]);
                }
            }
        close(pipes[i][0]);
        dup2(pipes[i][1], STDOUT_FILENO);
        }
    else if (i == command_count - 1 && command_count > 1){
        for (int j = 0; j < command_count; j++){
            if (j != i - 1){
                close(pipes[j][0]);
                close(pipes[j][1]);
                }
            }
        close(pipes[i - 1][1]);
        dup2(pipes[i - 1][0], STDIN_FILENO);
        }
    else if (command_count > 1){
        for (int j = 0; j < command_count; j++){
            if (j != i - 1 && j != i){
                close(pipes[j][0]);
                close(pipes[j][1]);
                }
            }
        close(pipes[i - 1][1]);
        close(pipes[i][0]);
        dup2(pipes[i - 1][0], STDIN_FILENO);
        dup2(pipes[i][1], STDOUT_FILENO);
        close(pipes[i - 1][0]);
        close(pipes[i][1]);
        }
    else{
        close(pipes[i][0]);
        close(pipes[i][1]);
        }
}

int main() {
    char command[256];
    char *commands[MAX_COMMANDS];
    int command_count = 0;

    while (1) 
    {
        printf("Shell> ");
        fgets(command, sizeof(command), stdin);
        command[strcspn(command, "\n")] = '\0';

        tokenize(command, commands, &command_count);

        int pipes[command_count][2];

        if (create_pipes(pipes, command_count) == -1){
            perror("pipe");
            exit(EXIT_FAILURE);
            }

        pid_t pids[command_count];

        for (int i = 0; i < command_count; i++){
            pid_t pid = fork();
            pids[i] = pid;

            if (pid == -1){
                perror("fork");
                exit(EXIT_FAILURE);
                }

            else if (pid == 0){ // caso hijo

                redirect_and_close(pipes, command_count, i);

                // creo un arreglo para guardar el comando y los argumentos
                char *args[MAX_ARGS];
                char *command_copy = malloc(strlen(commands[i]) + 1);
                strcpy(command_copy, commands[i]);

                char *token = strtok(command_copy, " ");
                int j = 0;
                while (token != NULL){
                    args[j++] = token;
                    token = strtok(NULL, " ");
                    }
                args[j] = NULL;
                
                // quitar las comillas
                for (int k = 0; k < j; k++){
                    remove_quotes(args[k]);
                }
                
                // crear una variable que tenga el tamaño exacto
                char *args2[j + 1];
                for (int k = 0; k < j; k++){
                    args2[k] = args[k];
                    }
                args2[j] = NULL;

                if (execvp(args2[0], args2) == -1){
                    perror("execvp");
                    exit(EXIT_FAILURE);
                }
            }
        }

        // cerrar los pipes en el padre
        for (int i = 0; i < command_count; i++){
            close(pipes[i][0]);
            close(pipes[i][1]);
            }

        for (int i = 0; i < command_count; i++){
            int status;
            if (waitpid(pids[i], &status, 0) == -1){
                perror("waitpid");
                exit(EXIT_FAILURE);
                }
            // no hago nada si alguno de los procesos hijo fue interrumpido por una señal
            
            }

        command_count = 0;

    }
    return 0;
}