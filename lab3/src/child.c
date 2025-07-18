#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>

#define SHM_NAME "/my_shared_memory"
#define SEM_WRITE_NAME "/my_sem_write"
#define SEM_READ_NAME "/my_sem_read"

#include "shared_struct.h"

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Использование: %s <имя_файла_для_вывода>\n", argv[0]);
        return 1;
    }
    FILE *outFile = fopen(argv[1], "w");
    if (!outFile)
    {
        perror("Ошибка открытия файла для записи");
        return 1;
    }

    int shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (shm_fd == -1)
    {
        perror("shm_open child");
        fclose(outFile);
        exit(EXIT_FAILURE);
    }

    struct shared_data *shared_mem = mmap(NULL, sizeof(struct shared_data),
                                          PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_mem == MAP_FAILED)
    {
        perror("mmap child");
        fclose(outFile);
        close(shm_fd);
        exit(EXIT_FAILURE);
    }
    close(shm_fd);

    sem_t *sem_write = sem_open(SEM_WRITE_NAME, 0);
    if (sem_write == SEM_FAILED)
    {
        perror("sem_open write child");
        exit(EXIT_FAILURE);
    }
    sem_t *sem_read = sem_open(SEM_READ_NAME, 0);
    if (sem_read == SEM_FAILED)
    {
        perror("sem_open read child");
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        sem_wait(sem_read);

        if (shared_mem->buffer[0] == '\0')
        {
            sem_post(sem_write);
            break;
        }

        double sum = 0.0f;
        char *ptr = shared_mem->buffer;
        char *endptr;
        while (*ptr)
        {
            float num = strtof(ptr, &endptr);
            if (ptr == endptr)
            {
                if (*ptr == '\0' || *ptr == '\n')
                    break;
                ptr++;
            }
            else
            {
                sum += num;
                ptr = endptr;
            }
        }
        fprintf(outFile, "%f\n", sum);
        fflush(outFile);

        sem_post(sem_write);
    }

    sem_close(sem_write);
    sem_close(sem_read);
    fclose(outFile);
    munmap(shared_mem, sizeof(struct shared_data));

    return 0;
}