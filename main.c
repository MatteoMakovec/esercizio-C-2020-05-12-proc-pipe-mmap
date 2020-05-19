//il processo padre per comunicare con il processo figlio prepara:
//- una pipe
//- una memory map condivisa
//
//il processo padre manda i dati al processo figlio attraverso la pipe
//
//il processo figlio restituisce il risultato attraverso la memory map convidisa (che può essere anonima o basata su file).
//esempio:
//https://github.com/marcotessarotto/exOpSys/blob/7ce5b8f75782f2de0cb7f65bb7ce62dd143220e6/010files/mmap_anon.c#L18
//
//il processo padre prende come argomento a linea di comando un nome di file.
//il processo padre legge il file e manda i contenuti attraverso la pipe al processo figlio.
//
//il processo figlio riceve attraverso la pipe i contenuti del file e calcola SHA3_512.
//
//quando la pipe raggiunge EOF, il processo figlio produce il digest di SHA3_512 e lo scrive nella memory map condivisa,
//poi il processo figlio termina.
//
//quando al processo padre viene notificato che il processo figlio ha terminato, prende il digest dalla memory map condivisa
//e lo scrive a video ("SHA3_512 del file %s è il seguente: " <segue digest in formato esadecimale>).

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>


#define FILE_SIZE 2048
#define BUFFER_SIZE 1024


int main(int argc, char * argv[]) {
	//Dato che SHA3_512 non funziona sul mio computer, svolgo il programma senza eseguire il digest
	int pipe_fd[2];
	int fd;
	int res;
	char * buffer;
	char * file_content;
	char * file_name;
	int bytes_read = 0;

	if (argc == 1) {
		printf("specificare come parametro il nome del file da leggere\n");
		exit(EXIT_FAILURE);
	}

	char * memory = mmap(NULL, 64, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

	if (memory == MAP_FAILED) {
		perror("mmap()");
		exit(EXIT_FAILURE);
	}

	if (pipe(pipe_fd) == -1) {
		perror("pipe()");

		exit(EXIT_FAILURE);
	}

	switch (fork()) {
		case 0:
			close(pipe_fd[1]);

			buffer = malloc(FILE_SIZE);
			if (buffer == NULL) {
				perror("malloc()");
				exit(EXIT_FAILURE);
			}

			file_content = malloc(1);
			file_content[0] = '\0';
			while ((res = read(pipe_fd[0], buffer, FILE_SIZE)) > 0) {
				bytes_read += res;
				file_content = realloc(file_content, bytes_read);
				strcat(file_content, buffer);
			}

			memcpy(memory, file_content, strlen(file_content));

			close(pipe_fd[0]);
			exit(EXIT_SUCCESS);

		case -1:
			perror("problema con fork");
			exit(EXIT_FAILURE);

		default:
			close(pipe_fd[0]);

			buffer = malloc(BUFFER_SIZE);
			memset(buffer, 0, BUFFER_SIZE);

			file_name = argv[1];
			if ((fd = open(file_name, O_RDONLY)) == -1) {
				perror("open()");
				exit(EXIT_FAILURE);
			}
			while ((bytes_read = read(fd, buffer, BUFFER_SIZE)) > 0) {
				res = write(pipe_fd[1], buffer, BUFFER_SIZE);
				if (res == -1) {
					perror("write()");
				}
			}

			close(fd);
			close(pipe_fd[1]);

			wait(NULL);

			printf("Il contenuto del file_content è il seguente: \n", file_name);
			for (int i = 0; i < FILE_SIZE / 8; i++) {
				printf("%c", memory[i]);
			}


			exit(EXIT_SUCCESS);
	}
}
