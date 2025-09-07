#include "DagScheduler.h"
using namespace std;

bool DAGScheduler::load_from_yaml(const string &filename)
{
    try
    {
        YAML::Node config = YAML::LoadFile(filename);
        if (!config["jobs"])
        {
            cerr << "Ошибка: ключ 'jobs' не найден в файле " << filename << endl;
            return false;
        }
        for (const auto &node : config["jobs"])
        {
            Job job;
            job.id = node["id"].as<int>();
            job.name = node["name"].as<string>();
            if (node["dependencies"])
            {
                job.dependencies = node["dependencies"].as<vector<int>>();
            }
            if (node["fail"])
            {
                job.should_fail = node["fail"].as<bool>();
            }
            m_jobs[job.id] = job;
        }

        return validate_dag();
    }
    catch (const YAML::Exception &e)
    {
        cerr << "Ошибка парсинга YAML: " << e.what() << endl;
        return false;
    }
    catch (const exception &e)
    {
        cerr << "Ошибка: " << e.what() << endl;
        return false;
    }
}

void DAGScheduler::run()
{
    m_should_stop = false;
    vector<thread> worker_threads;
    // Основной цикл: продолжается, пока есть незавершенные задачи
    while (!all_jobs_done())
    {
        // Если задача не выполнилась, нам нужно прекратить планирование новых задач
        if (m_should_stop)
        {
            cout << "\n--- ОБНАРУЖЕНА ОШИБКА! ЗАПРОС НА ОСТАНОВКУ ---" << endl;
            break;
        }
        vector<int> ready_jobs;
        {
            // Находим все задачи, которые готовы к запуску (в ожидании и зависимости удовлетворены)
            // Этот блок защищен мьютексом для предотвращения гонки за m_jobs
            lock_guard<mutex> lock(m_mutex);
            for (auto &[id, job] : m_jobs)
            {
                if (job.status == JobStatus::PENDING && are_dependencies_met(job))
                {
                    ready_jobs.push_back(id);
                }
            }
        }
        // Запускаем новый поток для каждой готовой задачи
        for (int id : ready_jobs)
        {
            m_jobs[id].status = JobStatus::RUNNING;
            worker_threads.emplace_back(&DAGScheduler::execute_job, this, id);
        }
        // Небольшая пауза, чтобы предотвратить активное ожидание и высокую загрузку процессора
        this_thread::sleep_for(chrono::milliseconds(100));
    }
    // Ждем, пока все рабочие потоки завершат свое выполнение
    for (auto &th : worker_threads)
    {
        if (th.joinable())
        {
            th.join();
        }
    }
    cout << "\n--- ВЫПОЛНЕНИЕ DAG ЗАВЕРШЕНО ---" << endl;
    print_summary();
}

bool DAGScheduler::validate_dag()
{
    if (m_jobs.empty())
    {
        cerr << "Ошибка валидации: нет джобов для выполнения." << endl;
        return false;
    }
    if (!check_for_unknown_dependencies())
        return false;
    if (!check_for_cycles())
        return false;
    if (!check_for_single_component())
        return false;
    if (!check_for_start_and_end_jobs())
        return false;
    cout << "Валидация DAG прошла успешно." << endl;
    return true;
}

bool DAGScheduler::check_for_unknown_dependencies() const
{
    for (const auto &[id, job] : m_jobs)
    {
        for (int dep_id : job.dependencies)
        {
            if (m_jobs.find(dep_id) == m_jobs.end())
            {
                cerr << "Ошибка валидации: джоб " << id << " имеет неизвестную зависимость " << dep_id << endl;
                return false;
            }
        }
    }
    return true;
}

bool DAGScheduler::check_for_cycles()
{
    set<int> visited;         // Узлы, которые были полностью исследованы
    set<int> recursion_stack; // Узлы, находящиеся в данный момент в стеке рекурсии для текущего пути DFS
    for (const auto &[id, job] : m_jobs)
    {
        if (visited.find(id) == visited.end())
        {
            // Если, начиная с этого узла, обнаруживается цикл, весь граф недействителен
            if (is_cyclic_util(id, visited, recursion_stack))
            {
                cerr << "Ошибка валидации: обнаружен цикл в графе!" << endl;
                return false;
            }
        }
    }
    return true;
}

// Рекурсивный помощник для обнаружения циклов на основе DFS
bool DAGScheduler::is_cyclic_util(int id, set<int> &visited, set<int> &recursion_stack)
{
    visited.insert(id);
    recursion_stack.insert(id);
    // Находим всех соседей (задачи, которые зависят от текущей задачи)
    for (const auto &[job_id, job_node] : m_jobs)
    {
        for (int dep_id : job_node.dependencies)
        {
            if (dep_id == id)
            { // job_node зависит от id
                // Если сосед уже находится в стеке рекурсии, значит, у нас есть цикл
                if (recursion_stack.count(job_id))
                {
                    return true; // Цикл найден
                }
                // Если сосед еще не посещен, рекурсивно вызываем для него
                if (!visited.count(job_id))
                {
                    if (is_cyclic_util(job_id, visited, recursion_stack))
                    {
                        return true;
                    }
                }
            }
        }
    }
    recursion_stack.erase(id);
    return false;
}

bool DAGScheduler::check_for_single_component()
{
    if (m_jobs.empty())
        return true;
    set<int> visited;
    dfs_connectivity(m_jobs.begin()->first, visited);

    if (visited.size() != m_jobs.size())
    {
        cerr << "Ошибка валидации: граф имеет несколько компонент связности." << endl;
        return false;
    }
    return true;
}

void DAGScheduler::dfs_connectivity(int start_node, set<int> &visited)
{
    visited.insert(start_node);
    for (int dep_id : m_jobs.at(start_node).dependencies)
    {
        if (visited.find(dep_id) == visited.end())
        {
            dfs_connectivity(dep_id, visited);
        }
    }
    for (const auto &[id, job] : m_jobs)
    {
        for (int dep_id : job.dependencies)
        {
            if (dep_id == start_node && visited.find(id) == visited.end())
            {
                dfs_connectivity(id, visited);
            }
        }
    }
}

bool DAGScheduler::check_for_start_and_end_jobs()
{
    set<int> has_outgoing_edges;
    set<int> has_incoming_edges;
    for (const auto &[id, job] : m_jobs)
    {
        if (!job.dependencies.empty())
        {
            has_incoming_edges.insert(id);
        }
        for (int dep_id : job.dependencies)
        {
            has_outgoing_edges.insert(dep_id);
        }
    }
    bool has_start_job = false;
    for (const auto &[id, job] : m_jobs)
    {
        if (has_incoming_edges.find(id) == has_incoming_edges.end())
        {
            has_start_job = true;
            break;
        }
    }
    bool has_end_job = false;
    for (const auto &[id, job] : m_jobs)
    {
        if (has_outgoing_edges.find(id) == has_outgoing_edges.end())
        {
            has_end_job = true;
            break;
        }
    }
    if (!has_start_job)
    {
        cerr << "Ошибка валидации: нет стартовых джобов (без зависимостей)." << endl;
        return false;
    }
    if (!has_end_job)
    {
        cerr << "Ошибка валидации: нет завершающих джобов (от которых никто не зависит)." << endl;
        return false;
    }
    return true;
}

void DAGScheduler::execute_job(int id)
{
    Job &current_job = m_jobs.at(id);

    {
        lock_guard<mutex> lock(m_mutex);
        cout << "[ЗАПУСК] Джоб " << id << ": " << current_job.name << endl;
    }
    // Имитация работы со случайной задержкой
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> distrib(1000, 3000);
    this_thread::sleep_for(chrono::milliseconds(distrib(gen)));
    // Проверяем, не был ли получен сигнал остановки (например, из-за сбоя другой задачи)
    if (m_should_stop)
    {
        current_job.status = JobStatus::CANCELLED;
        lock_guard<mutex> lock(m_mutex);
        cout << "[ОТМЕНЕН] Джоб " << id << ": " << current_job.name << endl;
        return;
    }
    // Имитация сбоя задачи
    if (current_job.should_fail)
    {
        current_job.status = JobStatus::FAILED;
        // Посылаем сигнал всем остальным потокам на остановку
        m_should_stop = true;
        lock_guard<mutex> lock(m_mutex);
        cerr << "[ОШИБКА] Джоб " << id << ": " << current_job.name << " завершился сбоем!" << endl;
    }
    else
    {
        current_job.status = JobStatus::COMPLETED;
        lock_guard<mutex> lock(m_mutex);
        cout << "[УСПЕХ] Джоб " << id << ": " << current_job.name << endl;
    }
}

bool DAGScheduler::are_dependencies_met(const Job &job) const
{
    for (int dep_id : job.dependencies)
    {
        if (m_jobs.at(dep_id).status != JobStatus::COMPLETED)
        {
            return false;
        }
    }
    return true;
}

bool DAGScheduler::all_jobs_done() const
{
    for (const auto &[id, job] : m_jobs)
    {
        if (job.status == JobStatus::PENDING || job.status == JobStatus::RUNNING)
        {
            return false;
        }
    }
    return true;
}

void DAGScheduler::print_summary() const
{
    cout << "\n--- Итоги выполнения ---" << endl;
    for (const auto &[id, job] : m_jobs)
    {
        cout << "Джоб " << id << " (" << job.name << "): ";
        switch (job.status)
        {
        case JobStatus::COMPLETED:
            cout << "УСПЕШНО";
            break;
        case JobStatus::FAILED:
            cout << "ОШИБКА";
            break;
        case JobStatus::CANCELLED:
            cout << "ОТМЕНЕН";
            break;
        case JobStatus::PENDING:
            cout << "НЕ ЗАПУЩЕН";
            break;
        case JobStatus::RUNNING:
            cout << "ВЫПОЛНЯЕТСЯ (ошибка?)";
            break;
        }
        cout << endl;
    }
}
