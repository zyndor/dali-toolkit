#ifndef DALI_SCENE_LOADER_OUTPUT_H
#define DALI_SCENE_LOADER_OUTPUT_H
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
#include "dali-scene-loader/public-api/animation-definition.h"
#include "dali-scene-loader/public-api/light-parameters.h"
#include "dali-scene-loader/public-api/camera-parameters.h"

namespace Dali
{
namespace SceneLoader
{

class ResourceBundle;
class SceneDefinition;

/**
 * @brief The outputs from loading and processing a scene.
 */
struct DALI_SCENE_LOADER_API LoadResult
{
  /**
   * @brief The bundle to store resources in.
   */
  ResourceBundle& mResources;

  /**
   * @brief The scene definition to populate.
   */
  SceneDefinition& mScene;

  /**
   * @brief The list of animation definitions, in lexicographical order of their names.
   */
  std::vector<AnimationDefinition>& mAnimationDefinitions;

  /**
   * @brief The list of animation group definitions, in lexicographical order of their names.
   */
  std::vector<AnimationGroupDefinition>& mAnimationGroupDefinitions;

  /**
   * @brief The camera parameters that were loaded from the scene.
   */
  std::vector<CameraParameters>& mCameraParameters;

  /**
   * @brief The light parameters that were loaded from the scene.
   */
  std::vector<LightParameters>& mLightParameters;
};

}
}

#endif //DALI_SCENE_LOADER_OUTPUT_H
