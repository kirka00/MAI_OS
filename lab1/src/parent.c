#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

int main()
{
    char filename[256], buffer[1024];
    int pipefd[2];

    printf("Введите имя файла для вывода: ");
    if (!fgets(filename, sizeof(filename), stdin))
    {
        perror("fgets");
        exit(1);
    }

    filename[strcspn(filename, "\n")] = 0;
    if (pipe(pipefd) == -1)
    {
        perror("pipe");
        exit(1);
    }

    pid_t pid = fork();
    if (pid < 0)
    {
        perror("fork");
        exit(1);
    }

    if (pid == 0)
    { // Дочерний процесс
        close(pipefd[1]);
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[0]);
        execlp("./child", "child", filename, NULL);
        perror("execlp");
        exit(1);
    }
    else
    { // Родительский процесс
        close(pipefd[0]);
        printf("Введите строки с числами (float). Пустая строка — завершение.\n");
        while (fgets(buffer, sizeof(buffer), stdin))
        {
            if (buffer[0] == '\n')
                break;
            if (write(pipefd[1], buffer, strlen(buffer)) == -1)
            {
                perror("write");
                break;
            }
        }
        close(pipefd[1]);
        wait(NULL);
    }

    return 0;
}
