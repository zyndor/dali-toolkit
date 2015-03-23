/*
 * Copyright (c) 2015 Samsung Electronics Co., Ltd.
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

// CLASS HEADER
#include <dali-toolkit/internal/text/rendering/atlas/text-atlas-renderer.h>

// EXTERNAL INCLUDES
#include <dali/dali.h>
#include <dali/integration-api/debug.h>

// INTERNAL INCLUDES
#include <dali-toolkit/internal/atlas-manager/atlas-manager.h>
#include <dali-toolkit/internal/text/rendering/atlas/atlas-glyph-manager.h>
#include <dali-toolkit/internal/text/rendering/shaders/text-basic-shader.h>
#include <dali-toolkit/internal/text/rendering/shaders/text-bgra-shader.h>

#if defined(DEBUG_ENABLED)
Debug::Filter* gLogFilter = Debug::Filter::New(Debug::Concise, true, "LOG_TEXT_ATLAS_RENDERER");
#endif

using namespace Dali;
using namespace Dali::Toolkit;
using namespace Dali::Toolkit::Text;

namespace
{
  const Vector2 DEFAULT_ATLAS_SIZE( 512.0f, 512.0f );
  const Vector2 DEFAULT_BLOCK_SIZE( 16.0f, 16.0f );
  const Vector2 PADDING( 4.0f, 4.0f ); // Allow for variation in font glyphs
}

struct AtlasRenderer::Impl
{

  struct MeshRecord
  {
    uint32_t mAtlasId;
    MeshData mMeshData;
  };

  struct AtlasRecord
  {
    uint32_t mImageId;
    Text::GlyphIndex mIndex;
  };

  struct MaxBlockSize
  {
    FontId mFontId;
    Vector2 mNeededBlockSize;
  };

  Impl()
  : mSlotDelegate( this )
  {
    mGlyphManager = AtlasGlyphManager::Get();
    mFontClient = TextAbstraction::FontClient::Get();
    mGlyphManager.SetNewAtlasSize( DEFAULT_ATLAS_SIZE, DEFAULT_BLOCK_SIZE );
    mBasicShader = BasicShader::New();
    mBGRAShader = BgraShader::New();
  }

  void AddGlyphs( const std::vector<Vector2>& positions, const Vector<GlyphInfo>& glyphs )
  {
    AtlasManager::AtlasSlot slot;
    std::vector< MeshRecord > meshContainer;
    FontId lastFontId = 0;

    if (mImageIds.Size() )
    {
      // Unreference any currently used glyphs
      RemoveText();
    }

    CalculateBlocksSize( glyphs );

    for ( uint32_t i = 0; i < glyphs.Size(); ++i )
    {
      GlyphInfo glyph = glyphs[ i ];

      // No operation for white space
      if ( glyph.width && glyph.height )
      {
        Vector2 position = positions[ i ];
        MeshData newMeshData;
        mGlyphManager.Cached( glyph.fontId, glyph.index, slot );

        if ( slot.mImageId )
        {
          // This glyph already exists so generate mesh data plugging in our supplied position
          mGlyphManager.GenerateMeshData( slot.mImageId, position, newMeshData );
          mImageIds.PushBack( slot.mImageId );
        }
        else
        {

          // Select correct size for new atlas if needed....?
          if ( lastFontId != glyph.fontId )
          {
            for ( uint32_t j = 0; j < mBlockSizes.size(); ++j )
            {
              if ( mBlockSizes[ j ].mFontId == glyph.fontId )
              {
                mGlyphManager.SetNewAtlasSize( DEFAULT_ATLAS_SIZE, mBlockSizes[ j ].mNeededBlockSize );
              }
            }
            lastFontId = glyph.fontId;
          }

          // Glyph doesn't currently exist in atlas so upload
          BufferImage bitmap = mFontClient.CreateBitmap( glyph.fontId, glyph.index );

          // Locate a new slot for our glyph
          mGlyphManager.Add( glyph, bitmap, slot );

          // Generate mesh data for this quad, plugging in our supplied position
          if ( slot.mImageId )
          {
            mGlyphManager.GenerateMeshData( slot.mImageId, position, newMeshData );
            mImageIds.PushBack( slot.mImageId );
          }
        }
        // Find an existing mesh data object to attach to ( or create a new one, if we can't find one using the same atlas)
        StitchTextMesh( meshContainer, newMeshData, slot );
      }
    }

    // For each MeshData object, create a mesh actor and add to the renderable actor
    if ( meshContainer.size() )
    {
      for ( uint32_t i = 0; i < meshContainer.size(); ++i )
      {
        Mesh mesh = Mesh::New( meshContainer[ i ].mMeshData );
        MeshActor actor = MeshActor::New( mesh );
        actor.SetParentOrigin( ParentOrigin::TOP_LEFT );
        actor.SetColorMode( USE_OWN_MULTIPLY_PARENT_COLOR );;

        // Check to see what pixel format the shader should be
        if ( mGlyphManager.GetPixelFormat( meshContainer[ i ].mAtlasId ) == Pixel::L8 )
        {
          actor.SetShaderEffect( mBasicShader );
        }
        else
        {
          actor.SetShaderEffect( mBGRAShader );
        }

        if ( i )
        {
          mActor.Add( actor );
        }
        else
        {
          mActor = actor;
        }
      }
      mActor.OffStageSignal().Connect( mSlotDelegate, &AtlasRenderer::Impl::OffStageDisconnect );
    }
#if defined(DEBUG_ENABLED)
    Toolkit::AtlasGlyphManager::Metrics metrics = mGlyphManager.GetMetrics();
    DALI_LOG_INFO( gLogFilter, Debug::Concise, "TextAtlasRenderer::GlyphManager::GlyphCount: %i, AtlasCount: %i, TextureMemoryUse: %iK\n",
                                                metrics.mGlyphCount,
                                                metrics.mAtlasMetrics.mAtlasCount,
                                                metrics.mAtlasMetrics.mTextureMemoryUsed / 1024 );
    for ( uint32_t i = 0; i < metrics.mAtlasMetrics.mAtlasCount; ++i )
    {
      DALI_LOG_INFO( gLogFilter, Debug::Verbose, "Atlas [%i] %sPixels: %s Size: %ix%i, BlockSize: %ix%i, BlocksUsed: %i/%i\n",
                                                 i + 1, i > 8 ? "" : " ",
                                                 metrics.mAtlasMetrics.mAtlasMetrics[ i ].mPixelFormat == Pixel::L8 ? "L8  " : "BGRA",
                                                 metrics.mAtlasMetrics.mAtlasMetrics[ i ].mWidth,
                                                 metrics.mAtlasMetrics.mAtlasMetrics[ i ].mHeight,
                                                 metrics.mAtlasMetrics.mAtlasMetrics[ i ].mBlockWidth,
                                                 metrics.mAtlasMetrics.mAtlasMetrics[ i ].mBlockHeight,
                                                 metrics.mAtlasMetrics.mAtlasMetrics[ i ].mBlocksUsed,
                                                 metrics.mAtlasMetrics.mAtlasMetrics[ i ].mTotalBlocks );
    }
#endif
  }

  void StitchTextMesh( std::vector< MeshRecord >& meshContainer,
                       MeshData& newMeshData,
                       AtlasManager::AtlasSlot& slot )
  {
    if ( slot.mImageId )
    {
      // Check to see if there's a mesh data object that references the same atlas ?
      for ( uint32_t i = 0; i < meshContainer.size(); ++i )
      {
        if ( slot.mAtlasId == meshContainer[ i ].mAtlasId )
        {
          // Stitch the mesh to the existing mesh
          mGlyphManager.StitchMesh( meshContainer[ i ].mMeshData, newMeshData );
          return;
        }
      }

      // No mesh data object currently exists that references this atlas, so create a new one
      MeshRecord meshRecord;
      meshRecord.mAtlasId = slot.mAtlasId;
      meshRecord.mMeshData = newMeshData;
      meshContainer.push_back( meshRecord );
    }
  }

  // Unreference any glyphs that were used with this actor
  void OffStageDisconnect( Dali::Actor actor )
  {
    RemoveText();
  }

  void RemoveText()
  {
    for ( uint32_t i = 0; i < mImageIds.Size(); ++i )
    {
      mGlyphManager.Remove( mImageIds[ i ] );
    }
    mImageIds.Resize( 0 );
  }

  void CalculateBlocksSize( const Vector<GlyphInfo>& glyphs )
  {
    MaxBlockSize maxBlockSize;
    for ( uint32_t i = 0; i < glyphs.Size(); ++i )
    {
      // Get the fontId of this glyph and check to see if a max size exists?
      FontId fontId = glyphs[ i ].fontId;
      float paddedWidth = glyphs[ i ].width + PADDING.x;
      float paddedHeight = glyphs[ i ].height + PADDING.y;
      bool foundFont = false;

      for ( uint32_t j = 0; j < mBlockSizes.size(); ++j )
      {
        if ( mBlockSizes[ j ].mFontId == fontId )
        {
          foundFont = true;
          if ( mBlockSizes[ j ].mNeededBlockSize.x < paddedWidth )
          {
            mBlockSizes[ j ].mNeededBlockSize.x = paddedWidth;
          }
          if ( mBlockSizes[ j ].mNeededBlockSize.y < paddedHeight )
          {
            mBlockSizes[ j ].mNeededBlockSize.y = paddedHeight;
          }
        }
      }

      if ( !foundFont )
      {
        maxBlockSize.mNeededBlockSize = Vector2( paddedWidth, paddedHeight );
        maxBlockSize.mFontId = fontId;
        mBlockSizes.push_back( maxBlockSize );
      }
    }
  }

  RenderableActor mActor;                             ///< The actor parent which renders the text
  AtlasGlyphManager mGlyphManager;                    ///< Glyph Manager to handle upload and caching
  Vector< uint32_t > mImageIds;                       ///< A list of imageIDs used by the renderer
  TextAbstraction::FontClient mFontClient;            ///> The font client used to supply glyph information
  SlotDelegate< AtlasRenderer::Impl > mSlotDelegate;  ///> Signal generated to unreference glyphs when renderable actor is removed
  ShaderEffect mBasicShader;                          ///> Shader to render L8 glyphs
  ShaderEffect mBGRAShader;                           ///> Shader to render BGRA glyphs
  std::vector< MaxBlockSize > mBlockSizes;            ///> Maximum size needed to contain a glyph in a block within a new atlas
};

Text::RendererPtr AtlasRenderer::New()
{
  return Text::RendererPtr( new AtlasRenderer() );
}

RenderableActor AtlasRenderer::Render( Text::ViewInterface& view )
{

  UnparentAndReset( mImpl->mActor );

  Text::Length numberOfGlyphs = view.GetNumberOfGlyphs();

  if( numberOfGlyphs > 0 )
  {
    Vector<GlyphInfo> glyphs;
    glyphs.Resize( numberOfGlyphs );

    view.GetGlyphs( &glyphs[0], 0, numberOfGlyphs );

    std::vector<Vector2> positions;
    positions.resize( numberOfGlyphs );
    view.GetGlyphPositions( &positions[0], 0, numberOfGlyphs );
    mImpl->AddGlyphs( positions, glyphs );
  }
  return mImpl->mActor;
}

AtlasRenderer::AtlasRenderer()
{
  mImpl = new Impl();

}

AtlasRenderer::~AtlasRenderer()
{
  delete mImpl;
}