#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <set>
#include <thread>
#include <chrono>
#include <mutex>
#include <atomic>
#include <fstream>
#include <stdexcept>
#include <algorithm>

#include <yaml-cpp/yaml.h>
#include <random>

using namespace std;

enum class JobStatus
{
    PENDING,
    RUNNING,
    COMPLETED,
    FAILED,
    CANCELLED
};

struct Job
{
    int id;
    string name;
    vector<int> dependencies; // Список идентификаторов задач, от которых зависит эта задача
    JobStatus status = JobStatus::PENDING;
    bool should_fail = false; // Флаг для имитации сбоя задачи для тестирования
};

// Управляет выполнением DAG
class DAGScheduler
{
public:
    bool load_from_yaml(const string &filename);
    void run();

private:
    // Выполняет все проверки валидации для DAG
    bool validate_dag();
    // Проверяет, что все зависимости задач ссылаются на существующие задачи
    bool check_for_unknown_dependencies() const;
    // Проверяет наличие циклов в DAG с помощью поиска в глубину
    bool check_for_cycles();
    // Вспомогательная функция для обнаружения циклов
    bool is_cyclic_util(int id, set<int> &visited, set<int> &recursion_stack);
    // Проверяет, является ли DAG единой компонентой связности
    bool check_for_single_component();
    // Вспомогательная функция для проверки связности
    void dfs_connectivity(int start_node, set<int> &visited);
    // Проверяет наличие хотя бы одной начальной и одной конечной задачи
    bool check_for_start_and_end_jobs();

    // Функция, которая выполняется каждым рабочим потоком для одной задачи
    void execute_job(int id);
    // Проверяет, все ли зависимости для данной задачи были выполнены
    bool are_dependencies_met(const Job &job) const;
    // Проверяет, все ли задачи в DAG завершены (успешно, с ошибкой или отменены)
    bool all_jobs_done() const;
    // Выводит сводку по конечному статусу всех задач
    void print_summary() const;

private:
    // Хранит все задачи, сопоставленные по их ID
    map<int, Job> m_jobs;
    // Мьютекс для защиты общего доступа к карте m_jobs
    mutex m_mutex;
    // Флаг для сигнализации всем запущенным задачам об остановке в случае сбоя
    atomic<bool> m_should_stop{false};
};
