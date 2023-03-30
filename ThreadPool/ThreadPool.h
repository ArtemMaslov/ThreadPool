#pragma once

#include <cstddef>
#include <thread>
#include <future>
#include <condition_variable>
#include <unordered_map>
#include <vector>
#include <deque>

#define THREAD_POOL_ENABLE_DEBUG

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
        /// Если true, то ThreadPool ожидает завершения выполнения этого задания.
        bool          IsWaiting = false;
        /// Если true, то задание было выполнено и можно получить его результат.
        bool          IsDone    = false;
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

    class ThreadPoolBase;

    class ThreadHandler
    {
    public:
        ThreadHandler(ThreadPoolBase& holder);

        ThreadHandler(ThreadHandler&& that) = default;

        ~ThreadHandler();

        void WaitThreadToExit() noexcept;

        void OnRunningThread();

    private:
        static ThreadId UniqueId;
        const  ThreadId Id;

        ThreadPoolBase& Holder;
        std::thread     Thread;
    };

    ///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///
    ///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///

    class ThreadPoolBase
    {
        friend class ThreadHandler;
    protected:
        TaskBase* const GetTaskForHandler();

    protected:
        /// Если true, то ThreadPool завершает работу и необходимо завершить выполнение
        /// всех потоков.
        bool IsTerminating    = false;
        /// В пустую очередь задач добавили задачу. Используется, чтобы избежать ложных пробуждений.
        bool QueueUpdatedFlag = false;
        /// Все задачи выполнены. Используется, чтобы избежать ложных пробуждений.
        bool AllTasksDone     = false;
        /// Ожидаемое задание выполнено. Используется, чтобы избежать ложных пробуждений.
        bool OneTaskDone      = false;
        /// Уведомить ThreadHandler, что очередь задач пополнилась.
        std::condition_variable NotifyThread;
        /// Уведомить ThreadPool, что все задачи были выполнены.
        std::condition_variable NotifyAllDone;
        /// Уведомить ThreadPool, что задание, которое он ожидал выполнено.
        std::condition_variable NotifyOneTaskDone;
        /// Контроль над доступом к общим для всех потоков данным.
        std::mutex              ThreadPoolBaseAccess;

        /// Количество добавленных в ThreadPool заданий.
        size_t TasksCount = 0;
        /// Количество выполненных заданий.
        size_t DoneTasks  = 0;

        /// Выполненные или находящиеся в процессе выполнения задания.
        std::unordered_map<TaskId, TaskBase*> TasksInProgress;
        /// Очередь заданий для выполнения.
        std::deque<TaskBase*> TasksQueue;
    };

    template <size_t ThreadsCount>
    class ThreadPool : public ThreadPoolBase
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
        std::vector<ThreadHandler> Handlers;
    };
};

///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///
///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///

#include "ThreadPool_impl.h"

///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///
///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///