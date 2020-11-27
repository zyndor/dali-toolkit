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
#include "dali-scene-loader/public-api/skinning-details.h"
#include "dali/public-api/rendering/shader.h"
#include "dali/public-api/object/property.h"
#include "dali/public-api/animation/constraints.h"

namespace Dali
{
namespace SceneLoader
{

const unsigned int Skinning::MAX_JOINTS = 64;

const std::string Skinning::BONE_UNIFORM_NAME = "uBone";

void Skinning::HelpConstrainBoneTransforms(Shader source, Shader target)
{
  char nameBuffer[16];
  char* const pWrite = nameBuffer + sprintf(nameBuffer, "%s[", Skinning::BONE_UNIFORM_NAME.c_str());
  for (uint32_t j = 0; j < Skinning::MAX_JOINTS; ++j)
  {
    sprintf(pWrite, "%d]", j);
    std::string name{ nameBuffer };
    Property::Index iSource = source.GetPropertyIndex(name);
    if (iSource == Property::INVALID_INDEX)
    {
      break;
    }

    Property::Index iTarget = target.RegisterProperty(name, target.GetProperty(iSource).Get<Matrix>());

    Constraint constraint = Constraint::New<Matrix>(target, iTarget, EqualToConstraint());
    constraint.AddSource(Source(source, iSource));
    constraint.Apply();
  }
}

}
}
