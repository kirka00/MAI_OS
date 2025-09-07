#include <iostream>
#include "DagScheduler.h"

using namespace std;

int main()
{
    DAGScheduler scheduler;
    cout << "Загрузка DAG из файла config.yaml..." << endl;
    if (!scheduler.load_from_yaml("config.yaml"))
    {
        cerr << "Не удалось загрузить или провалидировать DAG. Выход." << endl;
        return 1;
    }
    cout << "\n--- ЗАПУСК ВЫПОЛНЕНИЯ DAG ---\n"
         << endl;
    scheduler.run();
    return 0;
}
