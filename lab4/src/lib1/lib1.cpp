#include "../contracts/contracts.h"
#include <cmath>

// Является ли число простым
bool isPrime(int n)
{
    if (n <= 1)
    {
        return false;
    }
    for (int i = 2; i * i <= n; i++)
    {
        if (n % i == 0)
        {
            return false;
        }
    }
    return true;
}

// Наивный подсчет
int PrimeCount(int A, int B)
{
    int count = 0;
    for (int i = A; i <= B; ++i)
    {
        if (isPrime(i))
        {
            count++;
        }
    }
    return count;
}

// Вычисление e через предел
float E(int x)
{
    if (x <= 0)
    {
        return 1.0f;
    }
    return pow(1.0f + 1.0f / x, x);
}