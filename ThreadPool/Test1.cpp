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

static double Integrate(double min_x, double max_x);
static double CalcFunction(double x);

static const double IntegrateEpsilon = 1e-6;

///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///
///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///

int main()
{
    ThreadPool::ThreadPool<4> threadPool;

    const int    x_min = -50;
    const int    x_max = 50;
    const int    x_step  = 10;
    const size_t count = (x_max - x_min) / x_step;

    std::array<ThreadPool::TaskId, count> tasksIds = {};

    int x = x_min;
    for (auto& taskId: tasksIds)
    {
        taskId = threadPool.AddTask(Integrate, x, x + x_step);
        x += x_step;
    }

    threadPool.WaitAll();

    double sum = 0;
    for (auto& taskId : tasksIds)
    {
        sum += threadPool.GetTaskResult<double>(taskId);
    }

    double rightSum = 0;
    x = x_min;
    for (size_t st = 0; st < count; st++)
    {
        rightSum += Integrate(x, x + x_step);
        x += x_step;
    }

    const int doublePrecision = std::numeric_limits<double>::max_digits10;
    printf("sum      = %.*lf\n", doublePrecision, sum);
    printf("rightSum = %.*lf\n", doublePrecision, rightSum);
    printf("sqrt(pi) = %.*lf\n", doublePrecision, std::sqrt(M_PI));

    return 0;
}

///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///
///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///

static double Integrate(double min_x, double max_x)
{
    double sum = 0;
    double prev_f = CalcFunction(min_x);
    for (double x = min_x + IntegrateEpsilon; x < max_x; x += IntegrateEpsilon)
    {
        double curr_f = CalcFunction(x);
        sum += curr_f * IntegrateEpsilon;
        prev_f = curr_f;
    }
    return sum;
}

static double CalcFunction(double x)
{
    return std::exp(-(x * x));
}

///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///
///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///