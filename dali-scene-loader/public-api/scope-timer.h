#ifndef DALI_SCENE_LOADER_SCOPE_TIMER_H_
#define DALI_SCENE_LOADER_SCOPE_TIMER_H_
/*
 * Copyright (c) 2020 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

// INTERNAL INCLUDES
#include "dali-scene-loader/public-api/api.h"

// EXTERNAL INCLUDES
#include "dali/devel-api/common/map-wrapper.h"
#include <algorithm>
#include <chrono>
#include <list>
#include <thread>
#include <mutex>
#include <cstdint>

namespace Dali
{
namespace SceneLoader
{

/*
 * @brief Measures the time lapsed between the execution of its constructor and destructor,
 *  not including the housekeeping it has to do (there is of course no way avoiding said
 *  housekeeping factoring into the duration of the parent frame). It tracks the number of
 *  invocations from a certain location, as well as the the minimum, maximum and total
 *  durations.
 * @note The parent frame, if any, forms part of the "location", e.g. trying to measure
 *  the length of 3 functions, A(), B() and C(), where C() is called from both A() and B()
 *  will create 4 entries: A(), C() from A(), B(), C() from B().
 */
class DALI_SCENE_LOADER_API ScopeTimer
{
public:
  /*
   * @brief Finishes processing and writes the results to the file called @a filename
   *  (appending to the end), with an optional @a tag in the separator.
   */
  static void Dump(const char* fileName, const char* tag = nullptr);

  explicit ScopeTimer(const char* frameName);
  ~ScopeTimer();

private:
  struct Data
  {
    char mName[128];
    double mSum;
    uint32_t mSamples;
    double mMin;
    double mMax;

    std::list<uint64_t>* mStack;
  };

  struct ThreadData
  {
    std::list<uint64_t> mStack;
    std::map<uint64_t, Data> mData;
    std::list<const Data*> mSequence;

    std::thread::id mId;
  };

  using ThreadDataMap = std::map<std::thread::id, ThreadData>;

  static std::mutex sMutex;
  static ThreadDataMap sThreadData;
  static std::list<const ThreadData*> sSortedThreadData;

  static Data& ProduceData(const char* frameName);

  std::chrono::high_resolution_clock::time_point mStart;
  Data& mData;
};

}
}

#define SCOPE_TIMER_STRINGIFY(x) #x
#define SCOPE_TIMER_QUOTE(x) SCOPE_TIMER_STRINGIFY(x)
#define SCOPE_TIMER_CONCAT(a, b) a ## b
#define SCOPE_TIMER_UNIQUE_NAME(a) SCOPE_TIMER_CONCAT(a, __LINE__)
#define SCOPE_TIMER_FRAME SceneLoader::ScopeTimer SCOPE_TIMER_UNIQUE_NAME(privateScopeTimer)(__FUNCTION__ ":" SCOPE_TIMER_QUOTE(__LINE__))

#endif //DALI_SCENE_LOADER_SCOPE_TIMER_H_
