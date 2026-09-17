#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <stdexcept>
#include <chrono>
#include <future>
#include <functional>
#include <cstddef>

namespace mtt
{
  class Clicker
  {
  public:
    Clicker();
    double getMillisec() const;

  private:
    std::chrono::time_point< std::chrono::high_resolution_clock > start_;
  };

  using DataT = std::vector< unsigned long long >;
  using ValueT = DataT::value_type;

  namespace detail
  {
    ValueT calculatePartialSum(const DataT& values, const std::size_t startIdx, const std::size_t endIdx);
  }

  ValueT calculateParallelSum(const DataT& values, const std::size_t threadCount);
}

mtt::Clicker::Clicker():
  start_(std::chrono::high_resolution_clock::now())
{
}

double mtt::Clicker::getMillisec() const
{
  using std::chrono::high_resolution_clock;
  using std::chrono::duration_cast;
  using std::chrono::milliseconds;

  const auto t = high_resolution_clock::now();
  return static_cast< double >(duration_cast< milliseconds >(t - start_).count());
}

mtt::ValueT mtt::detail::calculatePartialSum(const DataT& values, const std::size_t startIdx, const std::size_t endIdx)
{
  mtt::ValueT result = 0;
  for (std::size_t i = startIdx; i < endIdx; ++i)
  {
    result += values[i];
  }
  return result;
}

mtt::ValueT mtt::calculateParallelSum(const DataT& values, const std::size_t threadCount)
{
  constexpr std::size_t defaultThreadCount = 1;
  
  std::size_t actualThreadCount = threadCount;
  if (actualThreadCount == 0)
  {
    actualThreadCount = defaultThreadCount;
  }

  const std::size_t size = values.size();
  const std::size_t chunkSize = size / actualThreadCount;
  const std::size_t remainder = size % actualThreadCount;

  std::vector< std::future< mtt::ValueT > > futures;
  std::size_t currentStart = 0;
  constexpr std::size_t chunkIncrement = 1;

  for (std::size_t i = 0; i < actualThreadCount; ++i)
  {
    std::size_t currentChunk = chunkSize;
    if (i < remainder)
    {
      currentChunk += chunkIncrement;
    }

    const std::size_t currentEnd = currentStart + currentChunk;
    
    futures.push_back(
      std::async(std::launch::async, mtt::detail::calculatePartialSum, std::cref(values), currentStart, currentEnd)
    );
    
    currentStart = currentEnd;
  }

  mtt::ValueT totalSum = 0;
  for (std::size_t i = 0; i < actualThreadCount; ++i)
  {
    totalSum += futures[i].get();
  }

  return totalSum;
}

int main(int argc, char* argv[])
{
  constexpr int expectedArgc = 2;
  constexpr int errorExitCode = 1;
  constexpr int successExitCode = 0;

  if (argc != expectedArgc)
  {
    std::cerr << "Invalid arguments. Usage: " << argv[0] << " <thread_count>\n";
    return errorExitCode;
  }

  std::size_t threadCount = 0;
  try
  {
    threadCount = std::stoull(argv[1]);
  }
  catch (const std::exception& e)
  {
    std::cerr << "Thread count must be a valid positive integer.\n";
    return errorExitCode;
  }

  constexpr std::size_t arraySize = 1'000'000'000;
  constexpr std::size_t measurementsCount = 5;
  constexpr mtt::ValueT defaultValue = 1;

  mtt::DataT values(arraySize, defaultValue);
  std::vector< double > times;
  times.reserve(measurementsCount);

  mtt::ValueT finalSum = 0;

  for (std::size_t i = 0; i < measurementsCount; ++i)
  {
    mtt::Clicker cl;
    finalSum = mtt::calculateParallelSum(values, threadCount);
    const double elapsed = cl.getMillisec();
    
    times.push_back(elapsed);
  }

  std::sort(times.begin(), times.end());
  
  constexpr std::size_t medianDivisor = 2;
  const double medianTime = times[measurementsCount / medianDivisor];

  std::cout << "Threads: " << threadCount << '\n'
            << "Sum: " << finalSum << '\n'
            << "Median Time (ms): " << medianTime << '\n';

  return successExitCode;
}
