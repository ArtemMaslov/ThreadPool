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

template <size_t ParallelTasksCount, typename Funct>
static double ParallelIntegrate(Funct funct, ThreadPool& threadPool,
                                const double x_min, const double x_max);

static double Integrate(const double min_x, const double max_x);
static double CalcFunction(const double x);

static const double IntegrateEpsilon = 1e-6;

///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///
///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///

int main()
{
    const int doublePrecision = std::numeric_limits<double>::max_digits10;

    ThreadPool threadPool(4);

    double parallelSum = 0;
    parallelSum += ParallelIntegrate<8>(Integrate, threadPool, -120, -40);

    auto lambda = [](double min_x, double max_x) -> double
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
    };
    parallelSum += ParallelIntegrate<8>(lambda, threadPool, -40, 40);

    struct Functor
    {
        double operator() (double min_x, double max_x)
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
    } functor;
    parallelSum += ParallelIntegrate<8>(functor, threadPool, 40, 120);

    printf("parallelSum   = %.*lf\n", doublePrecision, parallelSum);

    double sequentialSum = 0;
    const double x_min  = -120;
    const double x_max  = 120;
    const size_t count   = 24;
    const double x_step = (x_max - x_min) / count;
    double x = x_min;
    for (size_t st = 0; st < count; st++)
    {
        sequentialSum += Integrate(x, x + x_step);
        x += x_step;
    }

    printf("sequentialSum = %.*lf\n", doublePrecision, sequentialSum);
    printf("sqrt(pi)      = %.*lf\n", doublePrecision, std::sqrt(M_PI));
    return 0;
}

///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///
///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///

template <size_t ParallelTasksCount, typename Funct>
static double ParallelIntegrate(Funct funct, ThreadPool& threadPool,
                                const double x_min, const double x_max)
{
    const double x_step = (x_max - x_min) / ParallelTasksCount;

    std::array<ThreadPoolModule::TaskId, ParallelTasksCount> tasksIds = {};

    double x = x_min;
    for (auto& taskId: tasksIds)
    {
        taskId = threadPool.AddTask(true, Integrate, x, x + x_step);
        x += x_step;
    }

    threadPool.WaitAll();

    double sum = 0;
    for (auto& taskId : tasksIds)
        sum += threadPool.GetTaskResult<double>(taskId);

    return sum;
}

///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///
///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///

static double Integrate(const double min_x, const double max_x)
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

static double CalcFunction(const double x)
{
    return std::exp(-(x * x));
}

///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///
///***///***///---\\\***\\\***\\\___///***___***\\\___///***///***///---\\\***\\\***///