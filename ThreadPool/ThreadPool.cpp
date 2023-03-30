#include <cassert>
#include <iostream>

#include "ThreadPool.h"

using namespace ThreadPool;

#ifdef THREAD_POOL_ENABLE_DEBUG
    #define THREAD_POOL_PRINTF(...) \
        printf(__VA_ARGS__)
#else
    #define THREAD_POOL_PRINTF(...) 
#endif

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

void ThreadHandler::WaitThreadToExit() noexcept
{
    Thread.join();
}

void ThreadHandler::OnRunningThread()
{
    THREAD_POOL_PRINTF("Thread #%zd is running\n", Id);

    while (!Holder.IsTerminating)
    {
        TaskBase* taskToDo = nullptr;
        {
            std::unique_lock<std::mutex> lock(Holder.ThreadPoolBaseAccess);
            
            while (Holder.TasksQueue.empty() && !Holder.QueueUpdatedFlag && !Holder.IsTerminating)
                Holder.NotifyThread.wait(lock);
            
            if (Holder.IsTerminating)
                break;

            taskToDo = Holder.GetTaskForHandler();
            Holder.QueueUpdatedFlag = false;
        }

        THREAD_POOL_PRINTF("Thread #%zd is starting task %zd\n", Id, taskToDo->Id);
        taskToDo->Execute();
        THREAD_POOL_PRINTF("Thread #%zd have done task %zd\n", Id, taskToDo->Id);
        
        {
            std::unique_lock<std::mutex> lock(Holder.ThreadPoolBaseAccess);

            taskToDo->IsDone = true;
            Holder.DoneTasks++;
            if (Holder.DoneTasks == Holder.TasksCount)
            {
                Holder.AllTasksDone = true;
                Holder.NotifyAllDone.notify_all();
            }
            
            if (taskToDo->IsWaiting)
            {
                Holder.OneTaskDone = true;
                Holder.NotifyOneTaskDone.notify_all();
            }
        }
    }

    THREAD_POOL_PRINTF("Thread #%zd is stopped\n", Id);
}

///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///
///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///