#ifndef DALI_SCENE_LOADER_SHADER_UTILS_H_
#define DALI_SCENE_LOADER_SHADER_UTILS_H_
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
#include "dali/devel-api/common/set-wrapper.h"
#include <string>

namespace Dali
{
namespace SceneLoader
{

// A token which ShaderCreator is looking for to replace with defines.
#define SHADER_DEFINES "//$DEFINES//"

// Header composed of version + defines placeholder
#define SHADER_HEADER(version) "#version " #version "\n" SHADER_DEFINES "\n"

// To work around the restriction of using the '#' character in DALI_COMPOSE_SHADER() blocks.
#define SHADER_PREPROCESS(x) "\n#" #x "\n"

// #ifdef block
#define SHADER_IFDEF(symbol, block) \
  SHADER_PREPROCESS(ifdef symbol) \
  #block

// #else block
#define SHADER_ELSE(block) \
  SHADER_PREPROCESS(else) \
  #block

// #elseif block
#define SHADER_ELSEIF(symbol, block) \
  SHADER_PREPROCESS(elseif symbol) \
  #block

// convenience #endif
#define SHADER_ENDIF \
  SHADER_PREPROCESS(endif)

/*
 * @brief Processes a shader, which is using SHADER_DEFINES, with the given defines,
 *  storing its vertex and fragment source and offering capability to instantiate
 *  a new Shader using them, and the given hints.
 */
struct DALI_SCENE_LOADER_API ShaderCreator
{
  std::string mVertexSource;
  std::string mFragmentSource;

  ShaderCreator(const char* vsh, const char* fsh, const std::set<std::string>& defines);

  /*
   * @brief Creates a new Shader instance from the vertex and fragment source at
   *  hand, and using the given @a hints.
   */
  Shader Create(Shader::Hint::Value hints = Shader::Hint::NONE) const;
};

/*
 * @return A bitmask of shader hint values that could be obtained from the given @a shader.
 */
DALI_SCENE_LOADER_API Shader::Hint::Value GetShaderHints(const Shader& shader);

}
}

#endif //DALI_SCENE_LOADER_SHADER_UTILS_H_
