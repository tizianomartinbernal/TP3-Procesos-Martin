#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

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

int mod (int a, int b){
	return (a % b + b) % b;
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

	int pipes[n + 1][2];
	for (int i = 0; i <= n; i++){
		pipe(pipes[i]);
	}

	for (int i = 0; i < n; i++){
		pid = fork();
		if (pid == 0){ // hijo
			int indice_lectura;
			int indice_escritura;
			get_index(n, s, i, &indice_lectura, &indice_escritura);

			int num;
			read(pipes[indice_lectura][0], &num, sizeof(int));

			if (indice_lectura == 0 && n != 1){
				printf("soy el hijo %i, recibi un %i del padre, lo incremento a %i y se lo mando al hijo %i\n", i, num, num + 1, mod(i + 1, n));
			}
			if (indice_escritura == n && n != 1){
				printf("soy el hijo %i, recibi un %i del hijo %i, lo incremento a %i y se lo mando al padre\n", i, num, mod(i - 1, n), num + 1);
			}
			if (indice_lectura != 0 && indice_escritura != n){
				printf("soy el hijo %i, recibi un %i del hijo %i, lo incremento a %i y se lo mando al hijo %i\n", i, num, mod(i - 1, n), num + 1, mod(i + 1, n));
			}
			if (n == 1){
				printf("soy el hijo %i, recibi un %i del padre, lo incremento a %i y se lo mando al padre\n", i, num, num + 1);
			}
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
	if (pid != 0){
		for (int i = 0; i <= n; i++){
			close(pipes[i][1]);
		}
		for (int i = 0; i < n; i++){
			wait(&status);
		}
		int num;
		read(pipes[n][0], &num, sizeof(int));
		printf("soy el padre, recibi un %i\n", num);
		for (int i = 0; i <= n; i++){
			close(pipes[i][0]);
		}
	}
	return 0;
}