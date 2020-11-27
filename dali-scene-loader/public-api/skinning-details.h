#ifndef DALI_SCENE_LOADER_SKINNING_DETAILS_H_
#define DALI_SCENE_LOADER_SKINNING_DETAILS_H_
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
#include "dali/public-api/rendering/shader.h"
#include <string>

namespace Dali
{
namespace SceneLoader
{
struct DALI_SCENE_LOADER_API Skinning
{
  /*
   * @brief Upper limit on the number of joints supported.
   */
  static const uint32_t MAX_JOINTS;

  /*
   * @brief Name of bone matrix uniform (array).
   */
  static const std::string BONE_UNIFORM_NAME;

  /*
   * @brief For each element in the bone transform uniform array
   *  in the @a source, in the [0, min(MAX_JOINTS, first that doesn't exist)) range,
   *  it creates an EqualsTo constraint for @a target.
   *  I.e. subsequent changes to @a source bone uniforms are applied to @a target.
   */
  static void HelpConstrainBoneTransforms(Shader source, Shader target);

  Skinning() = delete;
};

}
}

#endif // DALI_SCENE_LOADER_SKINNING_DETAILS_H_
