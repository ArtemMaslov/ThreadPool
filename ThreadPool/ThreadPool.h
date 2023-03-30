///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///
// Модуль ThreadPool.
// 
// Версия: 1.0.0.1
// Дата последнего изменения: 30.03.2023
// 
// Автор: Маслов А.С. (https://github.com/ArtemMaslov).
///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///

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

namespace ThreadPoolModule
{
    typedef size_t TaskId;

    /**
     * @brief Абстрактный класс исполняемой в потоке задачи.
     * 
     * Является обёрткой для вызываемых объектов: функций, функторов, лямбда-выражений.
    */
    class TaskBase
    {
    public:
        TaskBase();

        virtual ~TaskBase() = default;

        /**
         * @brief Выполнить задачу.
        */
        virtual void Execute() = 0;

    public:
        static TaskId UniqueId;
        /// Уникальный идентификатор задачи.
        const  TaskId Id;
        /// Если true, то ThreadPool ожидает завершения выполнения этого задания.
        bool          IsWaiting  = false;
        /// Если true, то задание было выполнено и можно получить его результат.
        bool          IsDone     = false;
        /// Если true, то ThreadPool будет хранить результат выполнения задания, пока его не
        /// прочитает пользователь.
        bool          IsWaitable = true;
    };

    /**
     * @brief Класс исполняемой в потоке задачи.
     * 
     * @tparam RetType Тип возвращаемого функцией значения.
    */
    template <typename RetType>
    class Task : public TaskBase
    {
    public:
        Task(std::packaged_task<RetType()>&& packagedTask);

        virtual ~Task() = default;

        /**
         * @brief Выполнить задачу.
        */
        void Execute();

        /**
         * @brief Получить результат выполнения задачи.
         * После этого объект ресурсы Task будут освобождены.
         * 
         * @return Результат выполнения задачи.
         */
        RetType GetResult();

    private:
        /// Оборачиваемая задача: функция / функтор / лямбда-выражение.
        std::packaged_task<RetType()> PackagedTask;
        /// Результат выполнения задачи.
        std::future<RetType> Result;
    };

    template <>
    class Task<void> : public TaskBase
    {
    public:
        Task(std::packaged_task<void()>&& packagedTask);

        virtual ~Task() = default;

        /**
         * @brief Выполнить задачу.
        */
        void Execute();

    private:
        /// Оборачиваемая задача: функция / функтор / лямбда-выражение.
        std::packaged_task<void()> PackagedTask;
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
        /// Если true, то ThreadPool завершает работу и необходимо завершить выполнение всех потоков.
        bool IsTerminating = false;
        /// В пустую очередь задач добавили задачу. Используется, чтобы избежать ложных пробуждений.
        bool NotifyThreadFlag = false;
        /// Все задачи были выполнены. Используется, чтобы избежать ложных пробуждений.
        bool NotifyAllTasksDoneFlag = false;
        /// Ожидаемая задача была выполнена. Используется, чтобы избежать ложных пробуждений.
        bool NotifyOneTaskDoneFlag = false;
        /// Если true, то ThreadPool ожидает окончания выполнения всех задач.
        bool IsWaitingAllDone = false;
        /// Уведомить ThreadHandler, что очередь задач пополнилась.
        std::condition_variable NotifyThread;
        /// Уведомить ThreadPool, что все задачи были выполнены.
        std::condition_variable NotifyAllDone;
        /// Уведомить ThreadPool, что задание, которое он ожидал выполнено.
        std::condition_variable NotifyOneTaskDone;
        /// Контроль над доступом к общим для всех потоков данным.
        std::mutex ThreadPoolBaseAccess;

        /// Количество добавленных в ThreadPool заданий.
        size_t TasksCount = 0;
        /// Количество выполненных заданий.
        size_t DoneTasksCount = 0;

        /// Выполненные или находящиеся в процессе выполнения задания.
        std::unordered_map<TaskId, TaskBase*> TasksInProgress;
        /// Очередь заданий для выполнения.
        std::deque<TaskBase*> TasksQueue;
    };

    class ThreadPool : public ThreadPoolBase
    {
    public:
        ThreadPool(const size_t threadsCount);

        ~ThreadPool();

        template <typename Funct, typename... Args>
        TaskId AddTask(const bool isWaitable, Funct funct, Args... args);

        template <typename RetType>
        RetType GetTaskResult(TaskId id);

        template <>
        void GetTaskResult<void>(TaskId id);

        void Wait(TaskId id);

        void WaitAll();

    private:
        std::vector<ThreadHandler> Handlers;
    };
};

///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///
///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///

#include "ThreadPool_impl.h"

///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///
///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///