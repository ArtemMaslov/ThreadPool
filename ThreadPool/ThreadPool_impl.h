#include <cassert>
#include <functional>

#include "ThreadPool.h"

#ifdef THREAD_POOL_ENABLE_DEBUG
    #define THREAD_POOL_PRINTF(...) \
        printf(__VA_ARGS__)
#else
    #define THREAD_POOL_PRINTF(...) 
#endif

///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///
///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///

namespace ThreadPool
{
    template <size_t ThreadsCount>
    ThreadPool<ThreadsCount>::ThreadPool() :
        Handlers()
    {
        // В случае исключения созданные потоки будут корректно освобождены.
        Handlers.reserve(ThreadsCount);
        for (size_t st = 0; st < ThreadsCount; st++)
            Handlers.emplace_back(*this);
    }

    template <size_t ThreadsCount>
    ThreadPool<ThreadsCount>::~ThreadPool()
    {
        IsTerminating = true;
        NotifyThread.notify_all();
        // В ThreadHandler вызывается std::thread.join().
    }

///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///
///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///

    template <size_t ThreadsCount>
    template <typename Funct, typename... Args>
    TaskId ThreadPool<ThreadsCount>::AddTask(Funct funct, Args... args)
    {
        typedef decltype(funct(args...)) retType;

        std::packaged_task<retType()> packagedTask(std::bind(funct, args...));
        Task<retType>* task = new Task<retType>(std::move(packagedTask));

        {
            std::unique_lock<std::mutex> lock(ThreadPoolBaseAccess);
            THREAD_POOL_PRINTF("ThreadPool: adding task %zd to queue\n", task->Id);
            TasksQueue.push_back(task);

            TasksCount++;
            QueueUpdatedFlag = true;
        }
        NotifyThread.notify_one();

        return task->Id;
    }

    template <size_t ThreadsCount>
    template <typename RetType>
    RetType ThreadPool<ThreadsCount>::GetTaskResult(TaskId id)
    {
        THREAD_POOL_PRINTF("ThreadPool: find task #%zd\n", id);

        auto elemIter = TasksInProgress.find(id);
        // Задание не найдено.
        assert(elemIter != TasksInProgress.end());
        
        Task<RetType>* task = static_cast<Task<RetType>*>(elemIter->second);
        THREAD_POOL_PRINTF("ThreadPool: getting task %zd result\n", task->Id);
        RetType retValue = task->GetResult();

        delete task;
        TasksInProgress.erase(elemIter);

        return retValue;
    }

    ///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///
    ///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///

    template <size_t ThreadsCount>
    void ThreadPool<ThreadsCount>::Wait(TaskId id)
    {
        {
            std::unique_lock<std::mutex> lock(ThreadPoolBaseAccess);

            auto elemIter = TasksInProgress.find(id);

            if (elemIter != TasksInProgress.end())
            {
                // Задание найдено.
                TaskBase* const task = elemIter->second;
                // Задание было завершено, ожидание не нужно.
                if (task->IsDone)
                    return;
                task->IsWaiting = true;
                // Задание ещё выполняется, ожидаем его.
            }
            else
            {
                TaskBase* task = nullptr;
                // Задание не найдено среди выполняющихся, ищем его в очереди.
                for (TaskBase* elem: TasksQueue)
                {
                    if (elem->Id == id)
                    {
                        task = elem;
                        break;
                    }
                }
                // Попытка ожидания не существующего задания => ошибка.
                assert(task != nullptr);
                task->IsWaiting = true;
            }
        }

        while (true)
        {
            std::unique_lock<std::mutex> lock(ThreadPoolBaseAccess);

            while (!OneTaskDone)
                NotifyOneTaskDone.wait(lock);
            OneTaskDone = false;

            auto elemIter = TasksInProgress.find(id);
            // Задание не найдено.
            if (elemIter == TasksInProgress.end())
                continue;

            if (elemIter->second->IsDone)
                break;
        }
    }

    template <size_t ThreadsCount>
    void ThreadPool<ThreadsCount>::WaitAll()
    {
        std::unique_lock<std::mutex> lock(ThreadPoolBaseAccess);

        while (!AllTasksDone)
            NotifyAllDone.wait(lock);
        AllTasksDone = false;
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
        assert(IsDone == true);
        return Result.get();
    }
}


///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///
///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///

#undef THREAD_POOL_PRINTF