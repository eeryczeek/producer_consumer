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

	int cons_index;
	while(1){
		float speed = ((float)rand()/(float)(RAND_MAX)+1) * SLEEP;
		sem_wait(Sc);

		sem_wait(Sf);
		cons_index = full[info[4]];
		info[4] = (info[4] + 1) % N;
		sem_post(Sf);

		printf("Reading: %d from buffer[%d]\n", buffer[cons_index], cons_index);
		buffer[cons_index] = value++;
		
		sleep(speed);
		
		sem_wait(Se);
		full[info[1]] = cons_index;
		info[1] = (info[1] + 1) % N;
		sem_post(Se);

		sem_post(Sp);
	}
	
	return 0;
}