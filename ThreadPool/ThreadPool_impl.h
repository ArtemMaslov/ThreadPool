///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///
// Модуль ThreadPool, шаблонные методы.
// 
// Версия: 1.0.0.1
// Дата последнего изменения: 30.03.2023
// 
// Автор: Маслов А.С. (https://github.com/ArtemMaslov).
///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///

#include <cassert>
#include <functional>

#include "ThreadPool.h"

///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///
///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///

#ifdef THREAD_POOL_ENABLE_DEBUG
    #define THREAD_POOL_PRINTF(...) \
        printf(__VA_ARGS__)
#else
    #define THREAD_POOL_PRINTF(...) 
#endif

#define THREAD_POOL_ASSERT(msgerr, expr) \
    do { \
        if (static_cast<bool>(expr) == false) \
        { \
            fprintf(stderr, "!!!!!!!!!!!!!ERROR!!!!!!!!!!!!!\n%s\n", msgerr); \
            std::terminate(); \
        } \
    } while(0)

///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///
///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///

namespace ThreadPoolModule
{
    template <typename Funct, typename... Args>
    TaskId ThreadPool::AddTask(const bool isWaitable, Funct funct, Args... args)
    {
        typedef decltype(funct(args...)) retType;

        std::packaged_task<retType()> packagedTask(std::bind(funct, args...));
        Task<retType>* task = new Task<retType>(std::move(packagedTask));
        task->IsWaitable = isWaitable;

        {
            std::unique_lock<std::mutex> lock(ThreadPoolBaseAccess);
            THREAD_POOL_PRINTF("ThreadPool: adding task %zd to queue\n", task->Id);
            TasksQueue.push_back(task);

            TasksCount++;
            QueueUpdatedFlag = true;
            NotifyThread.notify_one();
        }

        return task->Id;
    }

    template <typename RetType>
    RetType ThreadPool::GetTaskResult(TaskId id)
    {
        THREAD_POOL_PRINTF("ThreadPool: find task #%zd\n", id);

        auto elemIter = TasksInProgress.find(id);
        // Задание не найдено.
        THREAD_POOL_ASSERT("Attempt to get result of not existing task",
                           elemIter != TasksInProgress.end());
        
        Task<RetType>* task = static_cast<Task<RetType>*>(elemIter->second);
        
        // Задание не планировалось к ожиданию.
        THREAD_POOL_ASSERT("Task cannot be waited",
                           task->IsWaitable);

        // Задание ещё не выполнено.
        THREAD_POOL_ASSERT("Task have not done yet",
                           task->IsDone);

        THREAD_POOL_PRINTF("ThreadPool: getting task %zd result\n", task->Id);
        RetType retValue = task->GetResult();

        // Освобождаем ресурсы, занимаемые заданием.
        delete task;
        TasksInProgress.erase(elemIter);

        return retValue;
    }

    ///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///
    ///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///
    
    template <typename RetType>
    Task<RetType>::Task(std::packaged_task<RetType()>&& packagedTask) :
        TaskBase(),
        PackagedTask(std::move(packagedTask)),
        Result(PackagedTask.get_future())
    {
    }

    template <typename RetType>
    void Task<RetType>::Execute()
    {
        PackagedTask();
    }

    template <typename RetType>
    RetType Task<RetType>::GetResult()
    {
        return Result.get();
    }
}

///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///
///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///