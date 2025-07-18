#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <string.h>

#define IDX(x, y, ncols) ((x) * (ncols) + (y))

typedef struct
{
    float *input, *output, *kernel;
    int rows, cols, ksize, start_row, end_row;
} thread_data_t;

void *convolve_part(void *arg)
{
    thread_data_t *data = (thread_data_t *)arg;
    int offset = data->ksize / 2;

    for (int i = data->start_row; i < data->end_row; i++)
    {
        for (int j = offset; j < data->cols - offset; j++)
        {
            float sum = 0.0f;
            for (int ki = -offset; ki <= offset; ki++)
            {
                for (int kj = -offset; kj <= offset; kj++)
                {
                    sum += data->input[IDX(i + ki, j + kj, data->cols)] *
                           data->kernel[IDX(ki + offset, kj + offset, data->ksize)];
                }
            }
            data->output[IDX(i, j, data->cols)] = sum;
        }
    }
    return NULL;
}

void apply_convolution(float *input, float *output, int rows, int cols, float *kernel, int ksize, int iterations, int num_threads)
{
    pthread_t threads[num_threads];
    thread_data_t thread_data[num_threads];

    int offset = ksize / 2;
    int work_rows = rows - 2 * offset;
    int rows_per_thread = work_rows / num_threads;

    float *current_input = input;
    float *current_output = output;

    for (int iter = 0; iter < iterations; iter++)
    {
        for (int t = 0; t < num_threads; t++)
        {
            int start = offset + t * rows_per_thread;
            int end = (t == num_threads - 1) ? (rows - offset) : (start + rows_per_thread);
            thread_data[t] = (thread_data_t){current_input, current_output, kernel, rows, cols, ksize, start, end};
            pthread_create(&threads[t], NULL, convolve_part, &thread_data[t]);
        }
        for (int t = 0; t < num_threads; t++)
        {
            pthread_join(threads[t], NULL);
        }
        float *tmp = current_input;
        current_input = current_output;
        current_output = tmp;
    }

    if (iterations % 2 == 0)
    {
        memcpy(output, current_input, rows * cols * sizeof(float));
    }
}

double get_time_ms()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec * 1000.0 + (double)tv.tv_usec / 1000.0;
}

int main(int argc, char *argv[])
{
    if (argc != 6)
    {
        printf("Usage: %s rows cols kernel_size iterations num_threads\n", argv[0]);
        return 1;
    }

    int rows = atoi(argv[1]);
    int cols = atoi(argv[2]);
    int ksize = atoi(argv[3]);
    int iterations = atoi(argv[4]);
    int num_threads = atoi(argv[5]);

    float *matrix = malloc(rows * cols * sizeof(float));
    float *result = malloc(rows * cols * sizeof(float));
    float *kernel = malloc(ksize * ksize * sizeof(float));

    if (!matrix || !result || !kernel)
    {
        perror("Failed to allocate memory");
        return 1;
    }

    for (int i = 0; i < rows * cols; i++)
        matrix[i] = (float)(rand() % 100) / 10.0f;
    for (int i = 0; i < ksize * ksize; i++)
        kernel[i] = 1.0f / (ksize * ksize);

    double start = get_time_ms();
    apply_convolution(matrix, result, rows, cols, kernel, ksize, iterations, num_threads);
    double end = get_time_ms();

    printf("Time taken: %.3f ms\n", end - start);

    free(matrix);
    free(result);
    free(kernel);
    return 0;
}