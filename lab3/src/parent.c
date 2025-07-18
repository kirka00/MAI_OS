#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <semaphore.h>

#define SHM_NAME "/my_shared_memory"
#define SEM_WRITE_NAME "/my_sem_write"
#define SEM_READ_NAME "/my_sem_read"

#include "shared_struct.h"

int main()
{
    char filename[256];

    printf("Введите имя файла для вывода: ");
    if (!fgets(filename, sizeof(filename), stdin))
    {
        perror("fgets");
        exit(EXIT_FAILURE);
    }
    filename[strcspn(filename, "\n")] = 0;

    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1)
    {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }
    if (ftruncate(shm_fd, sizeof(struct shared_data)) == -1)
    {
        perror("ftruncate");
        exit(EXIT_FAILURE);
    }
    struct shared_data *shared_mem = mmap(NULL, sizeof(struct shared_data),
                                          PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_mem == MAP_FAILED)
    {
        perror("mmap");
        exit(EXIT_FAILURE);
    }
    close(shm_fd);

    // Удаляем старые семафоры, если они остались от предыдущего запуска
    sem_unlink(SEM_WRITE_NAME);
    sem_unlink(SEM_READ_NAME);

    // Создаем семафор для записи
    sem_t *sem_write = sem_open(SEM_WRITE_NAME, O_CREAT, 0666, 1);
    if (sem_write == SEM_FAILED)
    {
        perror("sem_open write");
        exit(EXIT_FAILURE);
    }

    // Создаем семафор для чтения
    sem_t *sem_read = sem_open(SEM_READ_NAME, O_CREAT, 0666, 0);
    if (sem_read == SEM_FAILED)
    {
        perror("sem_open read");
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();
    if (pid < 0)
    {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0)
    {
        execlp("./child", "child", filename, NULL);
        perror("execlp");
        exit(EXIT_FAILURE);
    }
    else
    {
        printf("Введите строки с числами (float). Пустая строка — завершение.\n");
        while (1)
        {
            char buffer[BUFFER_SIZE];
            if (!fgets(buffer, sizeof(buffer), stdin) || buffer[0] == '\n')
            {
                sem_wait(sem_write);
                shared_mem->buffer[0] = '\0';
                sem_post(sem_read);
                break;
            }

            sem_wait(sem_write);
            strncpy(shared_mem->buffer, buffer, BUFFER_SIZE);
            sem_post(sem_read);
        }
        wait(NULL);
    }

    sem_close(sem_write);
    sem_close(sem_read);
    sem_unlink(SEM_WRITE_NAME);
    sem_unlink(SEM_READ_NAME);

    munmap(shared_mem, sizeof(struct shared_data));
    shm_unlink(SHM_NAME);

    return 0;
}