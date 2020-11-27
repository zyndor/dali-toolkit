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

// FILE HEADER
#include "dali-scene-loader/internal/text-details.h"

// INTERNAL INCLUDES
#include "dali-scene-loader/public-api/utils.h"
#include "dali-scene-loader/public-api/view-projection.h"

// EXTERNAL INCLUDES
#include "dali/public-api/actors/sampling.h"
#include "dali/public-api/rendering/sampler.h"
#include "dali/devel-api/common/stage.h"
#include <cstring>

namespace Dali
{
using namespace Toolkit;

namespace SceneLoader
{
namespace
{
const float MAX_FLOAT = std::numeric_limits<float>::max();

float GetEvasFontDpi()
{
  auto strValue = getenv("EVAS_FONT_DPI");
  return strValue ? atoi(strValue) : 72.f;
}

float GetDaliFontDpi()
{
  const Vector2& dpi = Stage::GetCurrent().GetDpi();
  return 0.5f * (dpi.height + dpi.width)
#ifdef WIN32
    / 0.75f
#endif
  ;
}

/**
 * @brief Transform from Device Normalized Coordinates to Screen Coordinates.
 *
 * It asumes the viewport origin is (0,0).
 *
 * @param[in] position The point in device normalized coordinates.
 * @param[in] viewportWidth The viewport's width.
 * @param[in] viewportHeight The viewport's height.
 * @param[out] windowPos The point in screen coordinates.
 */
bool ClipToWindow(const Vector4& position, float viewportWidth, float viewportHeight, Vector4& windowPos)
{
  Vector2 depthRange(0.f, 1.f);

  bool ok = !EqualsZero(position.w);
  if (ok)
  {
    float div = 1.f / position.w;

    windowPos = Vector4((1.f + position.x * div) * viewportWidth * 0.5f,
      (1.f - position.y * div) * viewportHeight * 0.5f,
      (position.z * div) * (depthRange.y - depthRange.x) + depthRange.x,
      div);
  }

  return ok;
}

/**
 * @brief Transform from Screen Coordinates to Device Normalized Coordinates.
 *
 * It asumes the viewport origin is (0,0).
 *
 * @param[in] windowPos The point in screen coordinates.
 * @param[in] viewportWidth The viewport's width.
 * @param[in] viewportHeight The viewport's height.
 * @param[out] position The point in device normalized coordinates.
 */
bool WindowToClip(const Vector4& windowPos, float viewportWidth, float viewportHeight, Vector4& position )
{
  if (EqualsZero(windowPos.w))
  {
    return false;
  }

  const float div = 1.f / windowPos.w;

  position.x = ( -1.f + 2.f * windowPos.x / viewportWidth ) / div;
  position.y = -( -1.f + 2.f * windowPos.y / viewportHeight ) / div;
  position.z = windowPos.z / div;
  position.w = div;

  return true;
}

/**
 * @brief Calculates the size of the text buffer in pixel coordinates.
 *
 * It calculates as well the min and max points of the box in normalized device coordinates.
 *
 * @param[in] viewMatrix The view matrix.
 * @param[in] projectionMatrix The projection matrix.
 * @param[in] modelMatrix The model matrix.
 * @param[in] positionsBuffer Buffer with the vertices.
 * @param[in] numberOfVertices The number of vertices.
 * @param[out] size The size of the text buffer.
 * @param[out] pMinClip The min point of the bounding box (in normalized device coordinates).
 * @param[out] pMaxClip The max point of the bounding box (in normalized device coordinates).
 */
void CalculateProjectedSize(const Matrix& viewMatrix, const Matrix& projectionMatrix, const Matrix& modelMatrix,
  const Vector3 * const positionsBuffer, unsigned int numberOfVertices,
  Size & size, Vector4 & pMinClip, Vector4 & pMaxClip)
{
  // Project to normalized device coordinates to calculate the bounding box.
  Matrix vp{ false };
  Matrix::Multiply(vp, viewMatrix, projectionMatrix);

  Matrix mvp{ false };
  Matrix::Multiply(mvp, modelMatrix, vp);

  //  Step 3) Multiply by the model-view-projection matrix to get the clip coords.
  pMinClip = Vector4(MAX_FLOAT, MAX_FLOAT, MAX_FLOAT, MAX_FLOAT);
  pMaxClip = Vector4(-MAX_FLOAT, -MAX_FLOAT, -MAX_FLOAT, -MAX_FLOAT);
  for (unsigned int index = 0u; index < numberOfVertices; ++index)
  {
    Vector4 positionV4(positionsBuffer[index]);
    positionV4.w = 1.f;

    // Transform to clip coordinates.
    Vector4 clipPos = mvp * positionV4;

    pMinClip.x = std::min(pMinClip.x, clipPos.x);
    pMinClip.y = std::min(pMinClip.y, clipPos.y);
    pMinClip.z = std::min(pMinClip.z, clipPos.z);
    pMinClip.w = std::min(pMinClip.w, clipPos.w);
    pMaxClip.x = std::max(pMaxClip.x, clipPos.x);
    pMaxClip.y = std::max(pMaxClip.y, clipPos.y);
    pMaxClip.z = std::max(pMaxClip.z, clipPos.z);
    pMaxClip.w = std::max(pMaxClip.w, clipPos.w);
  }

  //  Step 4) Project to window coordinates to calculate the size in pixels.
  const Size viewportSize = Stage::GetCurrent().GetSize();

  Vector4 pMinWindow;
  Vector4 pMaxWindow;
  ClipToWindow(pMinClip, viewportSize.width, viewportSize.height, pMinWindow);
  ClipToWindow(pMaxClip, viewportSize.width, viewportSize.height, pMaxWindow);

  // Correct the geometry to avoid pixel alignment issues.
  pMinWindow.x = std::ceil(pMinWindow.x);
  pMinWindow.y = std::floor(pMinWindow.y); // The 'y' axis is flipped.
  pMaxWindow.x = std::floor(pMaxWindow.x);
  pMaxWindow.y = std::ceil(pMaxWindow.y);  // The 'y' axis is flipped.

  size.width = std::abs(pMaxWindow.x - pMinWindow.x);
  size.height = std::abs(pMaxWindow.y - pMinWindow.y);

  pMinWindow.w = 0.f;
  pMaxWindow.w = 0.f;

  //  Step 5) Transform the corrected geometry to clip coords. Clip coords are used to build the geometry for the text.
  WindowToClip(pMinWindow, viewportSize.width, viewportSize.height, pMinClip);
  WindowToClip(pMaxWindow, viewportSize.width, viewportSize.height, pMaxClip);
}

void BuildGeometry(const Matrix& viewMatrix, const Matrix& projectionMatrix, const Matrix& modelMatrix, TextParameters& textParameters, const Vector4 & pMinClip, const Vector4& pMaxClip)
{
  // Unproject to camera coordinates to build the mesh.
  Matrix invProjectionMatrix = projectionMatrix;
  if (!invProjectionMatrix.Invert())
  {
    ExceptionFlinger(ASSERT_LOCATION) << "Failed to build geometry for text. Unable to find inverse of projection matrix " << projectionMatrix << ".";
  }

  Vector4 pCameraMin = invProjectionMatrix * pMinClip;
  Vector4 pCameraMax = invProjectionMatrix * pMaxClip;

  //  Step 6) Unproject from clip coords to camera coords.

  pCameraMax.z = pCameraMin.z;

  textParameters.bottomRight.x = std::max(pCameraMin.x, pCameraMax.x);
  textParameters.bottomRight.y = std::min(pCameraMin.y, pCameraMax.y);
  textParameters.bottomRight.z = pCameraMin.z;
  textParameters.bottomLeft.x = std::min(pCameraMin.x, pCameraMax.x);
  textParameters.bottomLeft.y = std::min(pCameraMin.y, pCameraMax.y);
  textParameters.bottomLeft.z = pCameraMin.z;
  textParameters.topRight.x = std::max(pCameraMin.x, pCameraMax.x);
  textParameters.topRight.y = std::max(pCameraMin.y, pCameraMax.y);
  textParameters.topRight.z = pCameraMin.z;
  textParameters.topLeft.x = std::min(pCameraMin.x, pCameraMax.x);
  textParameters.topLeft.y = std::max(pCameraMin.y, pCameraMax.y);
  textParameters.topLeft.z = pCameraMin.z;

  //  Step 7) Unproject to world coordinates.

  Matrix modelViewMatrix{ false };
  Matrix::Multiply(modelViewMatrix, modelMatrix, viewMatrix );

  if (!modelViewMatrix.Invert())
  {
    ExceptionFlinger(ASSERT_LOCATION) << "Failed to build geometry for text. Unable to find inverse of model view matrix " << modelViewMatrix << ".";
  }

  Vector4 bottomRight(textParameters.bottomRight);
  Vector4 bottomLeft(textParameters.bottomLeft);
  Vector4 topRight(textParameters.topRight);
  Vector4 topLeft(textParameters.topLeft);
  bottomRight.w = 1.f;
  bottomLeft.w = 1.f;
  topRight.w = 1.f;
  topLeft.w = 1.f;
  textParameters.bottomRight = modelViewMatrix * bottomRight;
  textParameters.bottomLeft = modelViewMatrix * bottomLeft;
  textParameters.topRight = modelViewMatrix * topRight;
  textParameters.topLeft = modelViewMatrix * topLeft;
}

}  // nonamespace

float PixelsToPoints(int pixels)
{
  const static float dpi = GetEvasFontDpi() / GetDaliFontDpi();
  return pixels * dpi;
}

void CalculateGeometry(const ViewProjection& viewProjection, const Matrix& model, TextParameters& textParameters, Size& size)
{
  //  Step 1) Build a model and a view matrix that keeps the same distance between the model and the camera as the real ones but the camera faces the text.
  Vector3 modelPosition;
  Quaternion modelRotation;
  Vector3 modelScale;
  Vector3 cameraPosition;
  Quaternion cameraRotation;
  Vector3 cameraScale;

  model.GetTransformComponents(modelPosition, modelRotation, modelScale);
  viewProjection.GetView().GetTransformComponents(cameraPosition, cameraRotation, cameraScale);

  float distance = (modelPosition - cameraPosition).Length();

  Matrix modelMatrix{ false };
  modelRotation = Quaternion::IDENTITY;
  modelPosition = Vector3::ZERO;
  modelMatrix.SetTransformComponents(modelScale, modelRotation, modelPosition);

  Matrix viewMatrix{ false };
  cameraRotation = Quaternion(Radian(Math::PI),Vector3::YAXIS);
  cameraPosition = Vector3(0.f, 0.f, distance);
  viewMatrix.SetTransformComponents(cameraScale, cameraRotation, cameraPosition);

  // The geometry used for the 1unit side quad.
  Vector3 vertices[] = {
    Vector3(-0.5f, -0.5f, 0.0f),
    Vector3(0.5f, -0.5f, 0.0f),
    Vector3(-0.5f,  0.5f, 0.0f),
    Vector3(0.5f,  0.5f, 0.0f)
  };
  Vector3* positionsBuffer = &vertices[0u];
  const unsigned int numberOfVertices = 4u;

  //  Step 2) Scale vertices with the given quad size.
  for (unsigned int index = 0u; index < numberOfVertices; ++index)
  {
    Vector3& v = *(positionsBuffer + index);
    v.x *= textParameters.quadScaleFactor.x;
    v.y *= textParameters.quadScaleFactor.y;
  }

  // Calculates the size of the text buffer in pixels.
  Vector4 pMinClip;
  Vector4 pMaxClip;
  CalculateProjectedSize(viewMatrix, viewProjection.GetProjection(), modelMatrix, positionsBuffer, numberOfVertices, size, pMinClip, pMaxClip);

  BuildGeometry(viewMatrix, viewProjection.GetProjection(), modelMatrix, textParameters, pMinClip, pMaxClip);

  textParameters.bottomRight.z = 0.f;
  textParameters.bottomLeft.z = 0.f;
  textParameters.topRight.z = 0.f;
  textParameters.topLeft.z = 0.f;
}

void CreateTextGeometryAndTexture(TextParameters& parameters, TextCacheItem& textCacheItem, bool generateBarycentrics, bool& isRgbaColorText)
{
  // TODO: in the long term, some of this could be rolled into MakeTexturedQuadGeometry(), however
  // 1, this is not a unit quad (maybe it could be; we're baking the size into vertex positions and set the Actor property Size to one);
  // 2, the UVs seem to be flipped (which MTQG() supports), but also 90 degrees clockwise rotated (which it doesn't).
  Property::Map property;
  property.Add("aPosition", Property::VECTOR3).Add("aTexCoord", Property::VECTOR2);

  std::vector<uint8_t> bytes;
  size_t stride = 0;
  if (generateBarycentrics)
  {
    property.Add("aBarycentric", Property::VECTOR3);

    struct Vertex
    {
      Vector3 position;
      Vector2 textureCoord;
      Vector3 barycentric;
    };

    Vertex vertices[] =
    {
      { parameters.bottomRight, Vector2(0.f, 1.f), Vector3::XAXIS },
      { parameters.bottomLeft, Vector2(1.f, 1.f), Vector3::ZAXIS },
      { parameters.topRight, Vector2(0.f, 0.f), Vector3::YAXIS },
      { parameters.topLeft, Vector2(1.f, 0.f), Vector3::XAXIS }
    };

    bytes.resize(sizeof(vertices));
    std::memcpy(bytes.data(), vertices, sizeof(vertices));
    stride = sizeof(Vertex);
  }
  else
  {
    struct Vertex
    {
      Vector3 position;
      Vector2 textureCoord;
    };

    Vertex vertices[] =
    {
      { parameters.bottomRight, Vector2(0.f, 1.f) },
      { parameters.bottomLeft, Vector2(1.f, 1.f) },
      { parameters.topRight, Vector2(0.f, 0.f) },
      { parameters.topLeft, Vector2(1.f, 0.f) }
    };

    bytes.resize(sizeof(vertices));
    std::memcpy(bytes.data(), vertices, sizeof(vertices));
    stride = sizeof(Vertex);
  }

  VertexBuffer vertexBuffer = VertexBuffer::New(property);
  vertexBuffer.SetData(bytes.data(), bytes.size() / stride);

  Geometry geometry = Geometry::New();
  geometry.AddVertexBuffer(vertexBuffer);

  geometry.SetType(Geometry::TRIANGLE_STRIP);
  parameters.renderer.SetGeometry(geometry);

  Vector<Toolkit::DevelText::EmbeddedItemInfo> embeddedItemLayout;
  Devel::PixelBuffer pixelBuffer = Toolkit::DevelText::Render(textCacheItem.textParameters, embeddedItemLayout);

  if (Vector2::ZERO != textCacheItem.shadowOffset)
  {
    Toolkit::DevelText::ShadowParameters shadowParameters;
    shadowParameters.textColor = textCacheItem.textParameters.textColor;
    shadowParameters.color = textCacheItem.shadowColor;
    shadowParameters.offset = textCacheItem.shadowOffset;
    shadowParameters.input = pixelBuffer;
    shadowParameters.blendShadow = false;

    pixelBuffer = Toolkit::DevelText::CreateShadow(shadowParameters);
  }

  PixelData pixelData = Devel::PixelBuffer::Convert(pixelBuffer);

  isRgbaColorText = Pixel::RGBA8888 == pixelData.GetPixelFormat();

  textCacheItem.texture = Texture::New(TextureType::TEXTURE_2D,
    pixelData.GetPixelFormat(),
    pixelData.GetWidth(),
    pixelData.GetHeight());

  textCacheItem.texture.Upload(pixelData);

  textCacheItem.textureSet = TextureSet::New();
  textCacheItem.textureSet.SetTexture(0u, textCacheItem.texture);

  Sampler sampler = Sampler::New();
  sampler.SetFilterMode( FilterMode::NEAREST, FilterMode::NEAREST );
  textCacheItem.textureSet.SetSampler(0u, sampler);

  parameters.renderer.SetTextures(textCacheItem.textureSet);
}

}
}
