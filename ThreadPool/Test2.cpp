#include <iostream>
#define _USE_MATH_DEFINES
#include <cmath>
#include <limits>

#include <array>
#include <functional>
#include <future>

#include "ThreadPool.h"

///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///
///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///

static int LongTask();
static int ShortTask();

///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///
///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///

int main()
{
    const int doublePrecision = std::numeric_limits<double>::max_digits10;

    ThreadPool::ThreadPool<4> threadPool;

    ThreadPool::TaskId longTaskId = threadPool.AddTask(LongTask);

    for (size_t st = 0; st < 10; st++)
        threadPool.AddTask(ShortTask);

    threadPool.Wait(longTaskId);

    printf("!!!!!!!!!!!LongTask have been waited!!!!!!!!!!!\n");

    return 0;
}

///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///
///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///

static int LongTask()
{
    static int LongTaskNumber = 1;
    int TaskNumber = LongTaskNumber++;
    std::this_thread::sleep_for(std::chrono::milliseconds(2'000));
    printf("LongTask #%d is done\n", TaskNumber);
    return TaskNumber;
}

static int ShortTask()
{
    static int ShortTaskNumber = 1;
    int TaskNumber = ShortTaskNumber++;
    std::this_thread::sleep_for(std::chrono::milliseconds(1'000));
    printf("ShortTask #%d is done\n", TaskNumber);
    return TaskNumber;
}

///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///
///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///