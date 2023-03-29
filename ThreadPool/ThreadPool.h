#pragma once

#include <cstddef>
#include <thread>
#include <future>
#include <unordered_map>
#include <queue>

///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///
///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///

namespace ThreadPool
{
    typedef size_t TaskId;

    class TaskBase
    {
    public:
        TaskBase();

        virtual ~TaskBase() = default;

        virtual void Execute() = 0;

    public:
        static TaskId UniqueId;
        const  TaskId Id;
    };

    template <typename RetType>
    class Task : public TaskBase
    {
    public:
        Task(std::packaged_task<RetType()>&& packagedTask);

        virtual ~Task() = default;

        void Execute();

        RetType GetResult();

    private:
        std::packaged_task<RetType()> PackagedTask;
        std::future<RetType> Result;
    };

    ///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///
    ///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///

    typedef size_t ThreadId;

    class ThreadHandler
    {
    public:
        ThreadHandler();

        bool CheckDoingTask() const noexcept;

        void SetTask(TaskBase* task) noexcept;

        void StopThread() noexcept;

        void OnRunningThread();

    private:
        static ThreadId UniqueId;
        const  ThreadId Id;

        /// @brief  У потока нет задач для выполнения.
        bool        IsWaiting = true;
        bool        IsRunning = true;
        std::thread Thread;
        TaskBase*   TaskToDo  = nullptr;
    };

    ///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///
    ///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///

    template <size_t ThreadsCount>
    class ThreadPool
    {
    public:
        ThreadPool();

        ~ThreadPool();

        template <typename Funct, typename... Args>
        TaskId AddTask(Funct funct, Args... args);

        template <typename RetType>
        RetType GetTaskResult(TaskId id);

        void Wait(TaskId id);

        void WaitAll();

        void PlanTasks();

    private:
        ThreadHandler Handlers[ThreadsCount];
        std::unordered_map<TaskId, TaskBase*> TasksInProgress;
        std::queue<TaskBase*> TasksQueue;
    };
};

///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///
///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///

#include "ThreadPool_impl.h"

///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///
///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///