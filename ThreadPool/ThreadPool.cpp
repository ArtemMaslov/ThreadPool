///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///
// Модуль ThreadPool.
// 
// Версия: 1.0.0.1
// Дата последнего изменения: 30.03.2023
// 
// Автор: Маслов А.С. (https://github.com/ArtemMaslov).
///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///

#include <iostream>

#include "ThreadPool.h"

using namespace ThreadPoolModule;

///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///
///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///

TaskId   TaskBase::UniqueId      = 0;
ThreadId ThreadHandler::UniqueId = 0;

///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///
///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///

TaskBase::TaskBase() :
    Id(UniqueId++)
{
}

ThreadHandler::ThreadHandler(ThreadPoolBase& holder) :
    Id(UniqueId++),
    Holder(holder),
    Thread(&ThreadHandler::OnRunningThread, this)
{
    THREAD_POOL_PRINTF("Thread #%zd is constructed\n", Id);
}

ThreadHandler::~ThreadHandler()
{
    // Завершаем потоки в случае ошибки в конструкторе.
    if (!Holder.IsTerminating)
    {
        Holder.IsTerminating = true;
        Holder.NotifyThread.notify_all();
    }

    // Перед вызовом деструктора потока вызываем join(), если он ещё не был вызван.
    // Иначе программа будет завершена через terminate().
    if (Thread.joinable())
        Thread.join();
    
    THREAD_POOL_PRINTF("Thread #%zd is destructed\n", Id);
}

///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///
///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///

TaskBase* const ThreadPoolBase::GetTaskForHandler()
{
    // Удаляем задачу из очереди и добавляем её в таблицу завершённых/выполняемых задач.
    TaskBase* const task = TasksQueue.front();
    TasksQueue.pop_front();

    TasksInProgress.insert(std::pair<TaskId, TaskBase*>(task->Id, task));

    return task;
}

///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///
///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///

void ThreadHandler::OnRunningThread()
{
    THREAD_POOL_PRINTF("Thread #%zd is running\n", Id);

    while (!Holder.IsTerminating)
    {
        TaskBase* taskToDo = nullptr;
        {
            std::unique_lock<std::mutex> lock(Holder.ThreadPoolBaseAccess);
            
            // Ожидаем появления задач в очереди или завершения работы ThreadPool.
            while (Holder.TasksQueue.empty() && !Holder.NotifyThreadFlag && !Holder.IsTerminating)
                Holder.NotifyThread.wait(lock);
            
            if (Holder.IsTerminating)
                break;

            // Получаем задачу для выполнения.
            taskToDo = Holder.GetTaskForHandler();
            Holder.NotifyThreadFlag = false;
        }

        // Выполняем задачу.
        THREAD_POOL_PRINTF("Thread #%zd is starting task %zd\n", Id, taskToDo->Id);
        taskToDo->Execute();
        THREAD_POOL_PRINTF("Thread #%zd have done task %zd\n", Id, taskToDo->Id);
        
        {
            std::unique_lock<std::mutex> lock(Holder.ThreadPoolBaseAccess);

            taskToDo->IsDone = true;
            Holder.DoneTasksCount++;

            if (Holder.IsWaitingAllDone && Holder.DoneTasksCount == Holder.TasksCount)
            {
                // ThreadPool ожидает завершения всех задач, уведомляем его, что все задачи выполнены.
                Holder.IsWaitingAllDone = false;
                Holder.NotifyAllTasksDoneFlag = true;
                Holder.NotifyAllDone.notify_all();
            }

            if (!taskToDo->IsWaitable)
            {
                // Результат задания никто не будет ожидать и получать, освобождаем ресурсы.
                auto elemIter = Holder.TasksInProgress.erase(taskToDo->Id);
                delete taskToDo;
            }
            else if (taskToDo->IsWaiting)
            {
                // Результат задания ожидает ThreadPool, уведомляем его, что задание готово.
                Holder.NotifyOneTaskDoneFlag = true;
                Holder.NotifyOneTaskDone.notify_all();
            }
        }
    }

    THREAD_POOL_PRINTF("Thread #%zd is stopped\n", Id);
}

///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///
///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///

ThreadPool::ThreadPool(const size_t threadsCount) :
    Handlers()
{
    // В случае исключения созданные потоки будут корректно освобождены.
    Handlers.reserve(threadsCount);
    for (size_t st = 0; st < threadsCount; st++)
        Handlers.emplace_back(*this);
}

ThreadPool::~ThreadPool()
{
    IsTerminating = true;
    NotifyThread.notify_all();
    // В ThreadHandler вызывается std::thread.join().
}

///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///
///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///

void ThreadPool::Wait(TaskId id)
{
    {
        std::unique_lock<std::mutex> lock(ThreadPoolBaseAccess);

        // Пытаемся найти задачу среди выполненных или выполняемых.
        auto elemIter = TasksInProgress.find(id);

        if (elemIter != TasksInProgress.end())
        {
            // Задание найдено.
            TaskBase* const task = elemIter->second;
            // Задание было завершено, ожидание не нужно.
            if (task->IsDone)
                return;

            // Попытка ожидания задания, которое нельзя ожидать => ошибка.
            THREAD_POOL_ASSERT("Attempt to wait for not waitable task",
                               task->IsWaitable == true);
            task->IsWaiting = true;
            // Задание ещё выполняется, ожидаем его завершения.
        }
        else
        {
            TaskBase* task = nullptr;
            // Задание не найдено среди выполняющихся, ищем его в очереди на выполнение.
            for (TaskBase* elem: TasksQueue)
            {
                if (elem->Id == id)
                {
                    task = elem;
                    break;
                }
            }
            // Попытка ожидания не существующего задания => ошибка.
            THREAD_POOL_ASSERT("Attempt to wait for not existing task",
                               task != nullptr);
            
            // Попытка ожидания задания, которое нельзя ожидать => ошибка.
            THREAD_POOL_ASSERT("Attempt to wait for not waitable task",
                               task->IsWaitable == true);
            task->IsWaiting = true;
            // Ожидаем выполнения задания.
        }
    }

    while (true)
    {
        std::unique_lock<std::mutex> lock(ThreadPoolBaseAccess);

        while (!NotifyOneTaskDoneFlag)
            NotifyOneTaskDone.wait(lock);
        NotifyOneTaskDoneFlag = false;

        auto elemIter = TasksInProgress.find(id);
        // Было выполнено не наше задание (его ожидал другой поток).
        if (elemIter == TasksInProgress.end())
            continue;

        // Ожидаемое задание было выполнено, возвращаемся из функции Wait().
        if (elemIter->second->IsDone)
            break;
    }
}

void ThreadPool::WaitAll()
{
    std::unique_lock<std::mutex> lock(ThreadPoolBaseAccess);

    // Если все задачи были выполнены, то ожидание не требуется.
    if (TasksCount == DoneTasksCount)
        return;

    IsWaitingAllDone = true;

    while (!NotifyAllTasksDoneFlag)
        NotifyAllDone.wait(lock);
    NotifyAllTasksDoneFlag = false;
}

///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///
///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///

template <>
void ThreadPool::GetTaskResult<void>(TaskId id)
{
    THREAD_POOL_PRINTF("ThreadPool: find task #%zd\n", id);

    auto elemIter = TasksInProgress.find(id);
    // Задание не найдено.
    THREAD_POOL_ASSERT("Attempt to get result of not existing task",
                       elemIter != TasksInProgress.end());
    
    Task<void>* task = static_cast<Task<void>*>(elemIter->second);
    
    // Задание ещё не выполнено.
    THREAD_POOL_ASSERT("Task have not done yet",
                       task->IsDone);

    // Освобождаем ресурсы, занимаемые заданием.
    delete task;
    TasksInProgress.erase(elemIter);
}

///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///
///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///

Task<void>::Task(std::packaged_task<void()>&& packagedTask) :
        TaskBase(),
        PackagedTask(std::move(packagedTask))
{
}

void Task<void>::Execute()
{
    PackagedTask();
}

///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///
///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///