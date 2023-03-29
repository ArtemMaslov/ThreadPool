#include <functional>

#include "ThreadPool.h"

///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///
///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///

namespace ThreadPool
{
    template <size_t ThreadsCount>
    ThreadPool<ThreadsCount>::ThreadPool()
    {
    }

    template <size_t ThreadsCount>
    ThreadPool<ThreadsCount>::~ThreadPool()
    {
        for (size_t st = 0; st < ThreadsCount; st++)
            Handlers[st].StopThread();
    }

    template <size_t ThreadsCount>
    template <typename Funct, typename... Args>
    TaskId ThreadPool<ThreadsCount>::AddTask(Funct funct, Args... args)
    {
        typedef decltype(funct(args...)) retType;

        std::packaged_task<retType()> packagedTask(std::bind(funct, args...));
        Task<retType>* task = new Task<retType>(std::move(packagedTask));

        printf("ThreadPool: adding task %zd to queue\n", task->Id);
        TasksQueue.push(task);

        return task->Id;
    }

    template <size_t ThreadsCount>
    template <typename RetType>
    RetType ThreadPool<ThreadsCount>::GetTaskResult(TaskId id)
    {
        printf("ThreadPool: find task #%zd\n", id);
        auto elemIter = TasksInProgress.find(id);

        Task<RetType>* task = static_cast<Task<RetType>*>((*elemIter).second);
        printf("ThreadPool: getting task %zd result\n", task->Id);
        RetType retValue = task->GetResult();

        delete task;
        TasksInProgress.erase(elemIter);

        return std::move(retValue);
    }

    template <size_t ThreadsCount>
    void ThreadPool<ThreadsCount>::PlanTasks()
    {
        for (size_t st = 0; st < ThreadsCount; st++)
        {
            if (Handlers[st].CheckDoingTask() == false)
            {
                TaskBase* task = TasksQueue.front();
                TasksInProgress.insert(std::pair<TaskId, TaskBase*>(task->Id, task));
                TasksQueue.pop();

                Handlers[st].SetTask(task);
            }
        }
    }

    template <size_t ThreadsCount>
    void ThreadPool<ThreadsCount>::Wait(TaskId id)
    {

    }

    template <size_t ThreadsCount>
    void ThreadPool<ThreadsCount>::WaitAll()
    {
        while (TasksQueue.empty() == false)
        {
            PlanTasks();
        }

        bool allDone = true;
        do
        {
            allDone = true;
            for (size_t st = 0; st < ThreadsCount; st++)
            {
                if (Handlers[st].CheckDoingTask())
                {
                    allDone = false;
                    break;
                }
            }
        } while (!allDone);
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