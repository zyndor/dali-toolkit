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

 // CLASS HEADER
#include "dali-scene-loader/public-api/animated-image.h"
#include "dali/integration-api/debug.h"

namespace Dali
{
namespace SceneLoader
{

AnimatedImage::AnimatedImage(Actor actor, Index materialIndex, int frameRate, int frameStart, int frameEnd, int loopCount, UpdateAnimatedMaterialFn callback)
:  mActor(actor),
  mMaterialIndex(materialIndex),
  mFrameRate(frameRate),
  mFrameStart(frameStart),
  mFrameEnd(frameEnd),
  mFrameCurrent(frameStart),
  mLoopCount(loopCount),
  mLoopCountCurrent(0),
  mCallback(callback)
{
  mTimer = Timer::New(1000 / mFrameRate);
  mTimer.TickSignal().Connect(this, &AnimatedImage::OnTimer);
}

const Actor AnimatedImage::GetActor() const
{
  return mActor;
}

void AnimatedImage::Play()
{
  // Do same behaviour at this moment.
  Resume();
}

void AnimatedImage::Pause()
{
  if (mTimer.IsRunning())
  {
    mTimer.Stop();
  }
}

void AnimatedImage::Resume()
{
  if (!mTimer.IsRunning())
  {
    mTimer.Start();
  }
}

void AnimatedImage::Stop()
{
  Reset();
  Pause();
}

bool AnimatedImage::OnTimer()
{
  bool ret = IncreaseFrame();
  if (!mCallback(mActor, mMaterialIndex, mFrameCurrent))
  {
    DALI_LOG_ERROR("UpdateAnimatedMaterialFn failed!");
  }
  return ret;
}

bool AnimatedImage::IncreaseFrame()
{
  mFrameCurrent++;
  if (mFrameCurrent >= mFrameEnd + 1)
  {
    mFrameCurrent = mFrameStart;
    return IncreaseLoopCount();
  }
  return true;
}

bool AnimatedImage::IncreaseLoopCount()
{
  if (mLoopCount > 0)
  {
    mLoopCountCurrent++;
    if (mLoopCountCurrent == mLoopCount)
    {
      //Timer will be stoped because loop done.
      return false;
    }
  }
  return true;
}

void AnimatedImage::Reset()
{
  mFrameCurrent = mFrameStart;
  mLoopCountCurrent = 0;
}

}
}
