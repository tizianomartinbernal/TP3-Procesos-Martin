#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

/*
Obtiene los índices de lectura y escritura para un buffer circular.

Parámetros:
- n: Tamaño total del buffer circular.
- s: Índice de inicio del buffer circular.
- i: Índice actual en el buffer circular.
- indice_lectura: Puntero al índice de lectura que se actualizará.
- indice_escritura: Puntero al índice de escritura que se actualizará.
*/
void get_index(int n, int s, int i, int *indice_lectura, int *indice_escritura){
    if (i == s){
        *indice_lectura = 0;
        *indice_escritura = 1;
    }
    if (i > s){
        *indice_lectura = i - s;
        *indice_escritura = i - s + 1;
    }
    if (i < s){
        *indice_lectura = n - s + i;
        *indice_escritura = n - s + i + 1;
    }
}

/*
Calcula el módulo de un número.

Parámetros:
- a: El dividendo.
- b: El divisor.

Retorna:
El resultado de la operación (a % b + b) % b.
*/
int mod(int a, int b) {
    return (a % b + b) % b;
}

/*
Imprime un mensaje según el estado del proceso hijo.

Parámetros:
- i: El índice del proceso hijo.
- num: El número recibido por el proceso hijo.
- indice_lectura: El índice de lectura del buffer circular.
- indice_escritura: El índice de escritura del buffer circular.
- n: El tamaño total del buffer circular.
*/
void print_message(int i, int num, int indice_lectura, int indice_escritura, int n) {
    if (n == 1) {
        printf("soy el hijo %i, recibi un %i del padre, lo incremento a %i y se lo mando al padre\n", i, num, num + 1);
    } else if (indice_lectura == 0) {
        printf("soy el hijo %i, recibi un %i del padre, lo incremento a %i y se lo mando al hijo %i\n", i, num, num + 1, mod(i + 1, n));
    } else if (indice_escritura == n) {
        printf("soy el hijo %i, recibi un %i del hijo %i, lo incremento a %i y se lo mando al padre\n", i, num, mod(i - 1, n), num + 1);
    } else {
        printf("soy el hijo %i, recibi un %i del hijo %i, lo incremento a %i y se lo mando al hijo %i\n", i, num, mod(i - 1, n), num + 1, mod(i + 1, n));
    }
}

int main(int argc, char **argv)
{	
	int s, c, n, pid, status;

	if (argc != 4){
		printf("Uso: anillo <n> <c> <s>\n");
		exit(0);
	}
	else{
		n = atoi(argv[1]);
		c = atoi(argv[2]);
		s = atoi(argv[3]);
	}
	if (s >= n || s < 0){
		printf("s debe ser menor a n y mayor o igual a 0\n");
		exit(0);
	}

	printf("Se crearan %i procesos, se enviará el caracter %i desde proceso %i\n", n, c, s);

	int pipes[n + 1][2]; // El primer pipe comunica al padre con el primer hijo, el último pipe comunica al último hijo con el padre
	for (int i = 0; i <= n; i++){
		if (pipe(pipes[i]) == -1){
			perror("pipe");
			exit(EXIT_FAILURE);
		}
	}

	pid_t pids[n];

	for (int i = 0; i < n; i++){
		pid = fork();
		pids[i] = pid;

		if (pid == -1){
			perror("fork");
			exit(EXIT_FAILURE);
		}

		if (pid == 0){ // hijo
			int indice_lectura;
			int indice_escritura;
			get_index(n, s, i, &indice_lectura, &indice_escritura);

			int num;

			for (int j = 0; j <= n; j++){
				if (j != indice_lectura && j != indice_escritura){
					close(pipes[j][0]);
					close(pipes[j][1]);
				}
			}

			close(pipes[indice_lectura][1]);
			close(pipes[indice_escritura][0]);

			read(pipes[indice_lectura][0], &num, sizeof(int));
			print_message(i, num, indice_lectura, indice_escritura, n);
			num++;
			write(pipes[indice_escritura][1], &num, sizeof(int));

			close(pipes[indice_lectura][0]);
			close(pipes[indice_escritura][1]);
			return 0;
		}
		else{ // padre
			if (i == 0){
				write(pipes[0][1], &c, sizeof(int));
			}
		}
	}

	// cierro los pipes en el padre
	for (int i = 0; i <= n; i++){
		if (i == n){
			close(pipes[i][1]);
		}
		else{
			close(pipes[i][0]);
			close(pipes[i][1]);
		}
	}

	// espero a que terminen los hijos
	for (int i = 0; i < n; i++){
		if (waitpid(pids[i], &status, 0) == -1){
			perror("waitpid");
			exit(EXIT_FAILURE);
		}
	}

	int num;
	read(pipes[n][0], &num, sizeof(int));
	printf("soy el padre, recibi un %i\n", num);
	close(pipes[n][0]);

	return 0;
}