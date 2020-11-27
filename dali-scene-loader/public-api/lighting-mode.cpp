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

// INTERNALS
#include "dali-scene-loader/public-api/lighting-mode.h"
#include "dali-scene-loader/public-api/utils.h"

// EXTERNALS
#include <algorithm>

namespace Dali
{
namespace SceneLoader
{
namespace
{

const char* const LIGHTING_MODE_VALUES[] =
{
  "unlit",
  "lit",
  "unlit-solid",
  "lit-solid"
};

}//namespace

static_assert(std::extent<decltype(LIGHTING_MODE_VALUES)>::value == static_cast<int>(LightingMode::LIT_SOLID) + 1,
  "Mismatch in number of enum / string LightingMode values.");

const char* const LightingMode::PROPERTY_NAME = "lightingMode";

const char* LightingMode::GetValue(Type lm)
{
  return (lm >= 0 && lm < static_cast<int>(std::extent<decltype(LIGHTING_MODE_VALUES)>::value)) ? LIGHTING_MODE_VALUES[lm] : nullptr;
}

bool LightingMode::InterpretValue(const std::string& value, Type& lm)
{
  auto iterEnd = LIGHTING_MODE_VALUES + std::extent<decltype(LIGHTING_MODE_VALUES)>::value;
  auto iterFind = std::find_if(LIGHTING_MODE_VALUES, iterEnd, [value](const char* rhs) {
    return CaseInsensitiveStringCompare(value, rhs);
  });

  const bool success = iterFind != iterEnd;
  if (success)
  {
    lm = static_cast<Type>(std::distance(LIGHTING_MODE_VALUES, iterFind));
  }
  return success;
}

}
}
