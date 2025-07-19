#include "../contracts/contracts.h"
#include <iostream>
using namespace std;

void print_usage()
{
    cout << "Использование программы:" << endl;
    cout << "1 <A> <B>   - Подсчёт простых чисел в [A, B]" << endl;
    cout << "2 <X>       - Вычислить e с точностью X" << endl;
    cout << "exit        - Выход" << endl;
}

int main()
{
    cout << "Программа 1 (Статическая компоновка с реализацией 1)" << endl;
    print_usage();

    int command;
    while (cin >> command)
    {
        if (command == 1)
        {
            int a, b;
            cin >> a >> b;
            cout << "Результат: " << PrimeCount(a, b) << endl;
        }
        else if (command == 2)
        {
            int x;
            cin >> x;
            cout << "Результат: " << E(x) << endl;
        }
        else
        {
            cout << "Неизвестная команда" << endl;
            print_usage();
        }
    }
    return 0;
}