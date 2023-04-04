#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <time.h>

int N = 4;
float SLEEP = 2.0;

int main() {
	srand(time(NULL));

	int fd_buff, fd_empty, fd_full, fd_info, value=1;
	char *buffer, *empty, *full;
	sem_t *Sp, *Sc, *Se, *Sf;
	int *info;

	fd_buff = shm_open("/buffer", O_CREAT|O_RDWR, 0600);
	ftruncate(fd_buff, N*sizeof(int));
	buffer = mmap(NULL, N*sizeof(int), PROT_READ|PROT_WRITE, MAP_SHARED, fd_buff, 0);

	fd_empty = shm_open("/empty", O_CREAT|O_RDWR, 0600);
	ftruncate(fd_empty, N*sizeof(int));
	empty = mmap(NULL, N*sizeof(int), PROT_READ|PROT_WRITE, MAP_SHARED, fd_empty, 0);

	fd_full = shm_open("/full", O_CREAT|O_RDWR, 0600);
	ftruncate(fd_full, N*sizeof(int));
	full = mmap(NULL, N*sizeof(int), PROT_READ|PROT_WRITE, MAP_SHARED, fd_full, 0);

	//info[0] = num_of_started_proc, indexes[1] = empty_add, indexes[2] = empty_rmv, indexes[3] = full_add, indexes[4] = full_rmv
	fd_info = shm_open("/info", O_CREAT|O_RDWR, 0600);
	ftruncate(fd_info, 5*sizeof(int));
	info = mmap(NULL, 5*sizeof(int), PROT_READ|PROT_WRITE, MAP_SHARED, fd_info, 0);

	if (buffer == MAP_FAILED || empty == MAP_FAILED || full == MAP_FAILED || info == MAP_FAILED){
		perror("mmap");
		exit(1);
	}

	if(info[0] == 0){
		info[0]++;
        for(int i = 0; i < N; i++){
            empty[i] = i;
            full[i] = -1;
        }
		for(int i = 1; i < 5; i++){
			info[i] = 0;
		}
	}

	Sp = sem_open("/Sp", O_CREAT, 0600, N);
	Sc = sem_open("/Sr", O_CREAT, 0600, 0);
	Se = sem_open("/Se", O_CREAT, 0600, 1);
	Sf = sem_open("/Sf", O_CREAT, 0600, 1);

	if (Sp == SEM_FAILED || Sc == SEM_FAILED || Se == SEM_FAILED || Sf == SEM_FAILED){
		perror("sem_open");
		exit(1);
	}

	int prod_index;
	while(1){
		float speed = ((float)rand()/(float)(RAND_MAX)+0.5) * SLEEP;
		sem_wait(Sp);

		sem_wait(Se);
		prod_index = empty[info[2]];
		info[2] = (info[2] + 1) % N;
		sem_post(Se);

		printf("Production: buffer[%d] = %d\n", prod_index, value);
		buffer[prod_index] = value++;
		sleep(speed);
		
		sem_wait(Sf);
		full[info[3]] = prod_index;
		info[3] = (info[3] + 1) % N;
		sem_post(Sf);

		sem_post(Sc);
	}

	return 0;
}