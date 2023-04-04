#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>

int N = 4;

int main() {
	int fd_buff, fd_empty, fd_full, fd_info;
	char *buffer, *empty, *full, *info;
	sem_t *Sp, *Sc, *Se, *Sf;

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

    shm_unlink("/buffer");
    shm_unlink("/full");
    shm_unlink("/empty");
    shm_unlink("/info");

	Sp = sem_open("/Sp", O_CREAT, 0600, N);
	Sc = sem_open("/Sr", O_CREAT, 0600, 0);
	Se = sem_open("/Se", O_CREAT, 0600, 1);
	Sf = sem_open("/Sf", O_CREAT, 0600, 1);

	if (Sp == SEM_FAILED || Sc == SEM_FAILED || Se == SEM_FAILED || Sf == SEM_FAILED){
        perror("sem_open");
    }

	sem_close(Sp);
	sem_close(Sc);
	sem_close(Se);
    sem_close(Sf);
	sem_unlink("/Sp");
	sem_unlink("/Sc");
	sem_unlink("/Se");
    sem_unlink("/Sf");

	munmap(buffer, N*sizeof(int));
	munmap(empty, N*sizeof(int));
	munmap(full, N*sizeof(int));
    munmap(info, 5*sizeof(int));
	return 0;
}
