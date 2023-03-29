#include <cassert>
#include <iostream>

#include "ThreadPool.h"

using namespace ThreadPool;

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

ThreadHandler::ThreadHandler() :
    Id(UniqueId++),
    Thread(&ThreadHandler::OnRunningThread, this)
{
    printf("Thread #%zd is constructed\n", Id);
    Thread.detach();
}

///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///
///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///

bool ThreadHandler::CheckDoingTask() const noexcept
{
    return TaskToDo;
}

void ThreadHandler::SetTask(TaskBase* task) noexcept
{
    assert(task);
    assert(IsWaiting);

    printf("Thread #%zd is setting new task %zd\n", Id, task->Id);

    TaskToDo = task;
}

void ThreadHandler::StopThread() noexcept
{
    IsRunning = false;
}

void ThreadHandler::OnRunningThread()
{
    printf("Thread #%zd is running\n", Id);

    while (IsRunning)
    {
        if (TaskToDo)
        {
            printf("Thread #%zd is starting task %zd\n", Id, TaskToDo->Id);
            TaskToDo->Execute();
            printf("Thread #%zd have done task %zd\n", Id, TaskToDo->Id);
            TaskToDo = nullptr;
        }
    }
}

///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///
///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///