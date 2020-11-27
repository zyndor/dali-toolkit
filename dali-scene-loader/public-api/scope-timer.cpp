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
#include "dali-scene-loader/internal/hash.h"
#include "dali-scene-loader/public-api/scope-timer.h"
#include <fstream>

namespace Dali
{
namespace SceneLoader
{

std::mutex ScopeTimer::sMutex;
ScopeTimer::ThreadDataMap ScopeTimer::sThreadData;
std::list<const ScopeTimer::ThreadData*> ScopeTimer::sSortedThreadData;

void ScopeTimer::Dump(const char* fileName, const char* tag)
{
  std::ofstream proFile(fileName, std::ios::app);
  proFile << "===";
  if (tag)
  {
    proFile << "[ " << tag << " ]";
  }
  proFile << "============================" << std::endl;

  std::unique_lock<std::mutex> lock(sMutex);
  for (auto& d : sSortedThreadData)
  {
    proFile << "---[ " << d->mId << " ]---------------------------- sum (ms) / # calls / min (ms) / max (ms)" << std::endl;
    for (auto& f : d->mSequence)
    {
      proFile << f->mName << f->mSum << '\t' << f->mSamples << '\t' <<
        f->mMin << '\t' << f->mMax << std::endl;
    }
  }

  sSortedThreadData.clear();
  sThreadData.clear();

  proFile << std::endl;
}

ScopeTimer::ScopeTimer(const char * frameName)
:  mData(ProduceData(frameName))
{
  mStart = std::chrono::high_resolution_clock::now();
}

ScopeTimer::~ScopeTimer()
{
  namespace sc = std::chrono;
  auto delta = sc::duration<double, std::milli>(sc::high_resolution_clock::now() - mStart).count();

  mData.mSum += delta;
  ++mData.mSamples;
  mData.mMin = std::min(mData.mMin, delta);
  mData.mMax = std::max(mData.mMax, delta);

  mData.mStack->pop_back();
}

ScopeTimer::Data & ScopeTimer::ProduceData(const char* frameName)
{
  const auto len = strlen(frameName);
  const uint64_t nameHash = Hash().Add(frameName, len);

  const auto threadId = std::this_thread::get_id();

  std::unique_lock<std::mutex> lock(sMutex);
  auto& threadData = sThreadData[threadId];
  if (threadData.mData.empty())
  {
    threadData.mId = threadId;
    sSortedThreadData.push_back(&threadData);
  }

  Hash hash((threadData.mStack.empty() ? 0ul : threadData.mStack.back()));
  hash.Add(nameHash);

  auto& data = threadData.mData[hash];
  if (!data.mName[0])
  {
    snprintf(data.mName, sizeof(data.mName), "%*s%*s%*s",
      static_cast<int>(threadData.mStack.size()), "",
      static_cast<int>(len), frameName,
      static_cast<int>(sizeof(data.mName) - (threadData.mStack.size() + len + 1)), "");

    data.mMin = std::numeric_limits<double>::max();
    data.mMax = 0.;
    data.mSum = 0.;
    data.mSamples = 0;

    data.mStack = &threadData.mStack;

    threadData.mSequence.push_back(&data);
  }

  threadData.mStack.push_back(hash);

  return data;
}

}
}
