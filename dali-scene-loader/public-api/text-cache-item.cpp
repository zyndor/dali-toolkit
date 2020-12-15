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
#include "dali-scene-loader/public-api/text-cache-item.h"

// EXTERNAL INCLUDES
#include "dali/devel-api/adaptor-framework/image-loading.h"
#include "dali/devel-api/adaptor-framework/pixel-buffer.h"
#include "dali/devel-api/text-abstraction/text-abstraction-definitions.h"
#include "dali/public-api/actors/sampling.h"
#include "dali/public-api/rendering/sampler.h"

namespace Dali
{
namespace SceneLoader
{

const std::string TEXT_ACTOR_POSTFIX_NAME( "TEXT_ACTOR" );

const EmbeddedItemCache* const FindEmbeddedItemCache( const std::vector<EmbeddedItemCache>& embeddedItemCache, const std::string& url, uint16_t x, uint16_t y, uint16_t width, uint16_t height, Degree angle )
{
  for( const auto& item : embeddedItemCache )
  {
    if( ( x == item.x ) &&
        ( y == item.y ) &&
        ( width == item.width ) &&
        ( height == item.height ) &&
        ( angle == item.angle ) &&
        ( item.url == url ) )
    {
      return &item;
    }
  }
  return nullptr;
}

void UpdateTextRenderer(TextCacheItem& parameters, std::vector<EmbeddedItemCache>& embeddedItemCache,
  TextCacheItem::Localizer getLocalisedText)
{
  parameters.textParameters.text = getLocalisedText ?
    getLocalisedText(parameters.internationalizationTextCode) : parameters.internationalizationTextCode;

  Vector<Toolkit::DevelText::EmbeddedItemInfo> embeddedItemsInfo;
  Devel::PixelBuffer pixelBuffer = Toolkit::DevelText::Render(parameters.textParameters, embeddedItemsInfo);

  // Process the drop shadow.
  if( Vector2::ZERO != parameters.shadowOffset)
  {
    Toolkit::DevelText::ShadowParameters shadowParameters;
    shadowParameters.textColor = parameters.textParameters.textColor;
    shadowParameters.color = parameters.shadowColor;
    shadowParameters.offset = parameters.shadowOffset;
    shadowParameters.input = pixelBuffer;
    shadowParameters.blendShadow = false;

    pixelBuffer = Toolkit::DevelText::CreateShadow(shadowParameters);
  }

  const int dstWidth = static_cast<int>(pixelBuffer.GetWidth());
  const int dstHeight = static_cast<int>(pixelBuffer.GetHeight());

  // Add the embedded items
  for( unsigned int index = 0u, endLayout = embeddedItemsInfo.Count(), endItems = parameters.embeddedItems.size(); ( index < endLayout ) && ( index < endItems ); ++index )
  {
    const Toolkit::DevelText::EmbeddedItemInfo& embeddedItemInfo = embeddedItemsInfo[index];

    // Check if the item is out of the buffer.

    if ((embeddedItemInfo.position.x + embeddedItemInfo.rotatedSize.width < 0.f) ||
      (embeddedItemInfo.position.x > dstWidth) ||
      (embeddedItemInfo.position.y < 0.f) ||
      (embeddedItemInfo.position.y - embeddedItemInfo.rotatedSize.height > dstHeight))
    {
      // The embedded item is completely out of the buffer.
      continue;
    }

    // Crop if it exceeds the boundaries of the destination buffer.
    const int width = static_cast<int>(embeddedItemInfo.rotatedSize.width);
    const int height = static_cast<int>(embeddedItemInfo.rotatedSize.height);
    int x = static_cast<int>(embeddedItemInfo.position.x);
    int y = static_cast<int>(embeddedItemInfo.position.y);

    int layoutX = 0;
    int layoutY = 0;
    int cropX = 0;
    int cropY = 0;
    int newWidth = width;
    int newHeight = height;

    bool crop = false;

    if( 0 > x )
    {
      newWidth += x;
      cropX = std::abs( x );
      crop = true;
    }
    else
    {
      layoutX = x;
    }

    if( cropX + newWidth > dstWidth )
    {
      crop = true;
      newWidth -= ( ( cropX + newWidth ) - dstWidth );
    }

    layoutY = y;
    if( 0 > layoutY )
    {
      newHeight += layoutY;
      cropY = std::abs(layoutY);
      crop = true;
    }

    if( cropY + newHeight > dstHeight )
    {
      crop = true;
      newHeight -= ( ( cropY + newHeight ) - dstHeight );
    }

    uint16_t uiCropX = static_cast<uint16_t>(cropX);
    uint16_t uiCropY = static_cast<uint16_t>(cropY);
    uint16_t uiNewWidth = static_cast<uint16_t>(newWidth);
    uint16_t uiNewHeight = static_cast<uint16_t>(newHeight);

    // Find whether the pixel buffer is already in the cache.
    const EmbeddedItemCache* const embeddedItem = FindEmbeddedItemCache(  embeddedItemCache,
                                        parameters.embeddedItems[index],
                                        uiCropX,
                                        uiCropY,
                                        uiNewWidth,
                                        uiNewHeight,
                                        embeddedItemInfo.angle );

    Devel::PixelBuffer itemPixelBuffer;
    Pixel::Format itemPixelFormat = Pixel::A8;
    if( nullptr != embeddedItem )
    {
      itemPixelBuffer = embeddedItem->pixelBuffer;

      if (!itemPixelBuffer)
      {
        // There is nothing to render.
        continue;
      }

      itemPixelFormat = itemPixelBuffer.GetPixelFormat();
    }
    else
    {
      itemPixelBuffer = LoadImageFromFile(parameters.embeddedItems[index]);

      if( !itemPixelBuffer )
      {
        // There is nothing to render.
        continue;
      }

      itemPixelFormat = itemPixelBuffer.GetPixelFormat();

      if ((Pixel::A8 == pixelBuffer.GetPixelFormat()) && (Pixel::RGBA8888 == itemPixelFormat))
      {
        pixelBuffer = Toolkit::DevelText::ConvertToRgba8888( pixelBuffer, parameters.textParameters.textColor, true );
      }
      else if ((Pixel::RGBA8888 == pixelBuffer.GetPixelFormat()) && (Pixel::A8 == itemPixelFormat))
      {
        itemPixelBuffer = Toolkit::DevelText::ConvertToRgba8888(itemPixelBuffer, parameters.textParameters.textColor, true );
        itemPixelFormat = Pixel::RGBA8888;
      }

      itemPixelBuffer.Resize( static_cast<uint16_t>( embeddedItemInfo.size.width ), static_cast<uint16_t>( embeddedItemInfo.size.height ) );
      itemPixelBuffer.Rotate(embeddedItemInfo.angle);

      if (crop)
      {
        itemPixelBuffer.Crop( uiCropX, uiCropY, uiNewWidth, uiNewHeight );
      }

      EmbeddedItemCache item( itemPixelBuffer,
                  parameters.embeddedItems[index],
                  embeddedItemInfo.angle,
                  uiCropX,
                  uiCropY,
                  uiNewWidth,
                  uiNewHeight );

      embeddedItemCache.push_back( item );
    }

    // Blend the item pixel buffer with the text's color according its blending mode.
    if( TextAbstraction::ColorBlendingMode::MULTIPLY == embeddedItemInfo.colorBlendingMode )
    {
      Devel::PixelBuffer buffer = Devel::PixelBuffer::New(  uiNewWidth,
                                        uiNewHeight,
                                        itemPixelFormat );

      unsigned char* bufferPtr = buffer.GetBuffer();
      const unsigned char* itemBufferPtr = itemPixelBuffer.GetBuffer();
      const unsigned int bytesPerPixel = Pixel::GetBytesPerPixel(itemPixelFormat);
      const unsigned int size = uiNewWidth * uiNewHeight * bytesPerPixel;

      for (unsigned int i = 0u; i < size; i += bytesPerPixel)
      {
        *(bufferPtr + 0u) = static_cast<unsigned char>( static_cast<float>( *(itemBufferPtr + 0u) ) * parameters.textParameters.textColor.r );
        *(bufferPtr + 1u) = static_cast<unsigned char>( static_cast<float>( *(itemBufferPtr + 1u) ) * parameters.textParameters.textColor.g );
        *(bufferPtr + 2u) = static_cast<unsigned char>( static_cast<float>( *(itemBufferPtr + 2u) ) * parameters.textParameters.textColor.b );
        *(bufferPtr + 3u) = static_cast<unsigned char>( static_cast<float>( *(itemBufferPtr + 3u) ) * parameters.textParameters.textColor.a );

        itemBufferPtr += bytesPerPixel;
        bufferPtr += bytesPerPixel;
      }

      itemPixelBuffer = buffer;
    }

    Toolkit::DevelText::UpdateBuffer(itemPixelBuffer, pixelBuffer, layoutX, layoutY, true);
  }

  PixelData pixelData = Devel::PixelBuffer::Convert(pixelBuffer);

  if ((parameters.textParameters.textWidth != pixelData.GetWidth()) || (parameters.textParameters.textHeight != pixelData.GetHeight()))
  {
    // Can't update the texture because has different size. Create a new one.
    parameters.texture = Texture::New(TextureType::TEXTURE_2D,
      pixelData.GetPixelFormat(),
      pixelData.GetWidth(),
      pixelData.GetHeight());

    parameters.textureSet.SetTexture(0u, parameters.texture);

    Sampler sampler = Sampler::New();
    sampler.SetFilterMode( FilterMode::NEAREST, FilterMode::NEAREST );
    parameters.textureSet.SetSampler(0u, sampler);
  }
  parameters.texture.Upload(pixelData);
}

}
}
