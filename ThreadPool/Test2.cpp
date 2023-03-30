#include <iostream>
#define _USE_MATH_DEFINES
#include <cmath>
#include <limits>

#include <array>
#include <functional>
#include <future>

#include "ThreadPool.h"

using ThreadPoolModule::ThreadPool;

///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///
///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///

static int LongTask();
static int ShortTask();
static void VoidTask();

///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///
///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///

int main()
{
    const int doublePrecision = std::numeric_limits<double>::max_digits10;

    ThreadPool threadPool(4);

    for (size_t st = 0; st < 3; st++)
    {
        ThreadPoolModule::TaskId taskId = threadPool.AddTask(true, VoidTask);
        threadPool.Wait(taskId);
        //threadPool.GetTaskResult<void>(taskId);
    }

    ThreadPoolModule::TaskId longTaskId = threadPool.AddTask(true, LongTask);

    for (size_t st = 0; st < 10; st++)
        threadPool.AddTask(false, ShortTask);

    threadPool.Wait(longTaskId);
    //threadPool.GetTaskResult<int>(longTaskId);

    printf("!!!!!!!!!!!LongTask have been waited!!!!!!!!!!!\n");

    threadPool.WaitAll();

    return 0;
}

///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///
///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///

static int LongTask()
{
    static int LongTaskNumber = 0;
    int TaskNumber = LongTaskNumber++;
    std::this_thread::sleep_for(std::chrono::milliseconds(2'000));
    fprintf(stderr, "LongTask #%d is done\n", TaskNumber);
    return TaskNumber;
}

static int ShortTask()
{
    static int ShortTaskNumber = 0;
    int TaskNumber = ShortTaskNumber++;
    std::this_thread::sleep_for(std::chrono::milliseconds(1'000));
    fprintf(stderr, "ShortTask #%d is done\n", TaskNumber);
    return TaskNumber;
}

static void VoidTask()
{
    static int VoidTaskNumber = 0;
    int TaskNumber = VoidTaskNumber++;
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    fprintf(stderr, "VoidTask #%d is done\n", TaskNumber);
}

///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///
///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///