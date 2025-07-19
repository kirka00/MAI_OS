#include <iostream>
#include <dlfcn.h> // Для dlopen, dlsym, dlclose
using namespace std;

typedef int (*PrimeCount_t)(int, int);
typedef float (*E_t)(int);

void print_usage()
{
    cout << "Использование программы:" << endl;
    cout << "0           - Переключение реализации" << endl;
    cout << "1 <A> <B>   - Подсчёт простых чисел в [A, B]" << endl;
    cout << "2 <X>       - Вычислить e с точностью X" << endl;
    cout << "exit        - Выход" << endl;
}

int main()
{
    void *handle = nullptr;
    PrimeCount_t primeCountFunc = nullptr;
    E_t eFunc = nullptr;
    int current_lib = 1;

    #ifdef __APPLE__
        const char *lib_ext = ".dylib";
    #elif __linux__
        const char *lib_ext = ".so";
    #endif

    string lib1_name = "./libimpl1";
    lib1_name += lib_ext;

    handle = dlopen(lib1_name.c_str(), RTLD_LAZY);
    if (!handle)
    {
        cerr << "Не могу открыть библиотеку: " << dlerror() << endl;
        return 1;
    }

    primeCountFunc = (PrimeCount_t)dlsym(handle, "PrimeCount");
    eFunc = (E_t)dlsym(handle, "E");
    if (!primeCountFunc || !eFunc)
    {
        cerr << "Невозможно загрузить символы: " << dlerror() << endl;
        dlclose(handle);
        return 1;
    }

    cout << "Программа 2 (Динамическая загрузка). Текущая реализация: " << current_lib << endl;
    print_usage();

    int command;
    while (cin >> command)
    {
        if (command == 0)
        {
            dlclose(handle);
            current_lib = (current_lib == 1) ? 2 : 1;

            string next_lib_name = (current_lib == 1) ? "./libimpl1" : "./libimpl2";
            next_lib_name += lib_ext;

            handle = dlopen(next_lib_name.c_str(), RTLD_LAZY);
            if (!handle)
            {
                cerr << "Не могу открыть библиотеку: " << dlerror() << endl;
                return 1;
            }

            primeCountFunc = (PrimeCount_t)dlsym(handle, "PrimeCount");
            eFunc = (E_t)dlsym(handle, "E");
            if (!primeCountFunc || !eFunc)
            {
                cerr << "Невозможно загрузить символы: " << dlerror() << endl;
                dlclose(handle);
                return 1;
            }
            cout << "Перешёл к реализации " << current_lib << endl;
        }
        else if (command == 1)
        {
            int a, b;
            cin >> a >> b;
            if (primeCountFunc)
            {
                cout << "Результат: " << primeCountFunc(a, b) << endl;
            }
            else
            {
                cerr << "Функция PrimeCount не загружена." << endl;
            }
        }
        else if (command == 2)
        {
            int x;
            cin >> x;
            if (eFunc)
            {
                cout << "Результат: " << eFunc(x) << endl;
            }
            else
            {
                cerr << "Функция E не загружена." << endl;
            }
        }
        else if (cin.eof() || cin.fail())
        {
            cout << "Выход." << endl;
            break;
        }
        else
        {
            cout << "Неизвестная команда" << endl;
            print_usage();
            cin.clear();
        }
    }

    dlclose(handle);
    return 0;
}
