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
#include "dali-scene-loader/public-api/shader-utils.h"
#include "dali/public-api/rendering/shader.h"
#include "dali/public-api/object/property-map.h"
#include <sstream>
#include <cstring>

namespace Dali
{
namespace SceneLoader
{

ShaderCreator::ShaderCreator(const char * vsh, const char * fsh, const std::set<std::string>& defines)
{
  std::ostringstream buffer;
  for (auto& d : defines)
  {
    buffer << "#define " << d << '\n';
  }

  const std::string definesProcessed = buffer.str();

  std::string* processed[] = { &mVertexSource, &mFragmentSource };
  auto iProcessed = processed;
  for (auto src : { vsh, fsh })
  {
    auto iFind = strstr(src, SHADER_DEFINES);
    if (iFind)
    {
      buffer.str("");

      buffer << std::string(src, iFind - src);
      buffer << definesProcessed;
      buffer << std::string(iFind + strlen(SHADER_DEFINES));

      **iProcessed = buffer.str();
    }
    else
    {
      **iProcessed = src;
    }

    ++iProcessed;
  }
}

Shader ShaderCreator::Create(Shader::Hint::Value hints) const
{
  return Shader::New(mVertexSource, mFragmentSource, hints);
}

Shader::Hint::Value GetShaderHints(const Shader& shader)
{
  uint32_t hints = Shader::Hint::NONE;
  auto program = shader.GetProperty(Shader::Property::PROGRAM);
  auto map = program.GetMap();
  if (auto hintsValue = map->Find("hints"))
  {
    std::string hintsString;
    if (hintsValue->Get(hintsString))
    {
      if (hintsString.find("MODIFIES_GEOMETRY") != std::string::npos)
      {
        hints |= Shader::Hint::MODIFIES_GEOMETRY;
      }

      if (hintsString.find("OUTPUT_IS_TRANSPARENT") != std::string::npos)
      {
        hints |= Shader::Hint::OUTPUT_IS_TRANSPARENT;
      }
    }
  }

  return static_cast<Shader::Hint::Value>(hints);
}

}
}
