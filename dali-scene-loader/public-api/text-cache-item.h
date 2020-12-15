#ifndef DALI_SCENE_LOADER_TEXT_CACHE_ITEM_H
#define DALI_SCENE_LOADER_TEXT_CACHE_ITEM_H
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
#include "dali/public-api/common/vector-wrapper.h"
#include "dali/public-api/rendering/texture.h"
#include "dali/public-api/rendering/texture-set.h"
#include "dali-toolkit/devel-api/text/text-utils-devel.h"
#include <functional>

namespace Dali
{
namespace SceneLoader
{

/**
 * @brief Cache of text textures. Useful to access the texture quickly in order to update the text.
 */
struct DALI_SCENE_LOADER_API TextCacheItem
{
  using Registrator = std::function<void(TextCacheItem&&)>;    ///< Function that TextCacheItems can be posted to as they are created.
  using Localizer = std::string(*)(const std::string&);        ///< Function that attempts to translate a text code to a string.

  std::string actorName;                                       ///< The name of the actor where the texture with the text is set.
  Texture texture;                                       ///< The texture with the text.
  TextureSet textureSet;                                 ///< The texture set where the texture is stored.
  Toolkit::DevelText::RendererParameters textParameters; ///< The text parameters to create the texture's renderer.
  Vector4 shadowColor;                                   ///< The color of the text's shadow. The default is black.
  Vector2 shadowOffset;                                  ///< The offset of the text's shadow.
  std::vector<std::string> embeddedItems;                      ///< The url of the embedded items.
  std::string internationalizationTextCode;                    ///< The text code to be translated.
};

/**
 * @brief Caches embedded items.
 */
struct DALI_SCENE_LOADER_API EmbeddedItemCache
{
  EmbeddedItemCache()
  : pixelBuffer{},
    url{},
    angle{},
    x{ 0u },
    y{ 0u },
    width{ 0u },
    height{ 0u }
  {}

  EmbeddedItemCache(  Devel::PixelBuffer pixelBuffer,
            std::string url,
            Degree angle,
            uint16_t x,
            uint16_t y,
            uint16_t width,
            uint16_t height)
  :  pixelBuffer{ pixelBuffer },
    url{ url },
    angle{ angle },
    x{ x },
    y{ y },
    width{ width },
    height{ height }
  {}

  ~EmbeddedItemCache()
  {}

  Devel::PixelBuffer pixelBuffer;
  std::string url;
  Degree angle;
  uint16_t x;
  uint16_t y;
  uint16_t width;
  uint16_t height;
};

DALI_SCENE_LOADER_API extern const std::string TEXT_ACTOR_POSTFIX_NAME;

/**
* @brief Creates a text renderer with the given @p parameters.
*
* @param[in,out] parameters The text's parameters.
*/
DALI_SCENE_LOADER_API void UpdateTextRenderer( TextCacheItem& parameters, std::vector<EmbeddedItemCache>& embeddedItemCache,
  TextCacheItem::Localizer getLocalisedText );

}
}

#endif // DALI_SCENE_LOADER_TEXT_CACHE_ITEM_H
