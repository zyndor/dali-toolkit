#ifndef DALI_SCENE_LOADER_ANIMATED_IMAGE_STATE_H_
#define DALI_SCENE_LOADER_ANIMATED_IMAGE_STATE_H_
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

// INTERNAL INCLUDESs
#include "dali-scene-loader/public-api/index.h"

// EXTERNAL INCLUDES
#include "dali/public-api/actors/actor.h"
#include "dali/public-api/adaptor-framework/timer.h"
#include <functional>
#include <memory>

namespace Dali
{
namespace SceneLoader
{

/**
 * @brief Controller class for an animated image entity.
 */
class DALI_SCENE_LOADER_API AnimatedImage : public ConnectionTracker
{
public:
  using Ptr = std::unique_ptr<AnimatedImage>;
  using UpdateAnimatedMaterialFn = std::function<bool(Actor, Index, int)>;

  AnimatedImage(Actor actor, Index materialIndex, int frameRate, int frameStart, int frameEnd, int loopCount, UpdateAnimatedMaterialFn callback);

  /**
   * @return Const handle to the actor whom the animated image is applied to.
   */
  const Actor GetActor() const;

  /**
   * @brief Starts the animation.
   */
  void Play();

  /**
   * @brief Pauses the animation.
   */
  void Pause();

  /**
   * @brief Resumes the animation.
   */
  void Resume();

  /**
   * @brief Stops the animation.
   */
  void Stop();

private:
  bool OnTimer();
  bool IncreaseFrame();
  bool IncreaseLoopCount();
  void Reset();

  Timer mTimer;

  Actor mActor;
  Index mMaterialIndex;
  int mFrameRate;
  int mFrameStart;
  int mFrameEnd;
  int mFrameCurrent;
  int mLoopCount;
  int mLoopCountCurrent;
  UpdateAnimatedMaterialFn mCallback;
};

}
}

#endif //DALI_SCENE_LOADER_ANIMATED_IMAGE_STATE_H_

