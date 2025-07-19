#include "../contracts/contracts.h"
#include <vector>

// Подсчет простых чисел с помощью Решета Эратосфена
int PrimeCount(int A, int B)
{
    if (B < 2)
    {
        return 0;
    }
    std::vector<bool> is_prime(B + 1, true);
    is_prime[0] = is_prime[1] = false;
    for (int p = 2; p * p <= B; p++)
    {
        if (is_prime[p])
        {
            for (int i = p * p; i <= B; i += p)
                is_prime[i] = false;
        }
    }

    int count = 0;
    for (int i = A; i <= B; i++)
    {
        if (i >= 0 && i <= B && is_prime[i])
        {
            count++;
        }
    }
    return count;
}

// Вычисление e через сумму ряда 1/n!
float E(int x)
{
    float sum = 0.0f;
    long double factorial = 1.0;
    for (int i = 0; i <= x; i++)
    {
        if (i == 0)
        {
            factorial = 1.0;
        }
        else
        {
            factorial *= i;
        }
        sum += 1.0f / factorial;
    }
    return sum;
}