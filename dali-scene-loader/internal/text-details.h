#ifndef DALI_SCENE_LOADER_TEXT_DETAILS_H_
#define DALI_SCENE_LOADER_TEXT_DETAILS_H_
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

// INTERNAL
#include "dali-scene-loader/public-api/text-cache-item.h"
#include "dali-scene-loader/public-api/matrix-stack.h"

// EXTERNAL
#include "dali-toolkit/devel-api/text/text-utils-devel.h"
#include "dali-toolkit/public-api/image-loader/sync-image-loader.h"
#include "dali/public-api/rendering/renderer.h"
#include "dali/public-api/rendering/shader.h"
#include "dali/public-api/math/matrix.h"
#include <limits>
#include <string>

namespace Dali
{
namespace SceneLoader
{

class ViewProjection;

/**
* @brief The units of the font's size.
*/
enum class FontSizeUnits
{
  POINTS,
  PIXELS
};

struct TextParameters
{
  Renderer renderer;       ///< The renderer.
  Vector3 bottomRight;     ///< The bottom-right corner of the quad.
  Vector3 bottomLeft;      ///< The bottom-left corner of the quad.
  Vector3 topRight;        ///< The top-right corner of the quad.
  Vector3 topLeft;         ///< The top-left corner of the quad.
  Vector3 quadScaleFactor; ///< The quad scale factor (for 'quad' geometry).
};

/**
 * @return The given number of @a pixels in points, based on the DPI value that
 * Evas and Dali report.
 */
float PixelsToPoints(int pixels);

/**
 * @brief Calculates the geometry for the quad used to render the text when the mesh is given in model/world coordinates.
 *
 * Used to know the size of the text buffer where to render the text.
 *
 * @param[in] viewProjection The main camera's view and projection settings.
 * @param[in] modelStack A modelspace transform stack.
 * @param[in,out] textRenderer Contains the geometry and all the text parameters needed to build the text renderer.
 * @param[out] size The size of the bounding box in screen coordinates.
 */
void CalculateGeometry(const ViewProjection& viewProjection, const Matrix& model,
  TextParameters& textParameters, Size& size);


void CreateTextGeometryAndTexture(TextParameters& parameters, TextCacheItem& textCacheItem,
  bool generateBarycentrics, bool& isRgbaColorText);

}
}

#endif //DALI_SCENE_LOADER_TEXT_DETAILS_H_
