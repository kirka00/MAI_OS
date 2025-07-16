#include <stdio.h>
#include <stdlib.h>

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
    char line[1024];
    while (fgets(line, sizeof(line), stdin))
    {
        double sum = 0.0f;
        char *ptr = line;
        char *endptr;
        while (*ptr)
        {
            // strtof преобразует строку в float и передвигает endptr.
            float num = strtof(ptr, &endptr);
            // Если strtof ничего не считал, указатели останутся равны.
            if (ptr == endptr)
            {
                if (*ptr == '\0' || *ptr == '\n')
                {
                    break;
                }
                ptr++;
            }
            else
            {
                sum += num;
                ptr = endptr;
            }
        }
        fprintf(outFile, "%f\n", sum);
    }
    fclose(outFile);
    return 0;
}