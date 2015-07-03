#ifndef __DALI_TOOLKIT_ATLAS_MANAGER_IMPL_H__
#define __DALI_TOOLKIT_ATLAS_MANAGER_IMPL_H__

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
 */


// EXTERNAL INCLUDES
#include <dali/public-api/common/vector-wrapper.h>
#include <dali/public-api/object/base-object.h>

// INTERNAL INCLUDES
#include <dali-toolkit/internal/atlas-manager/atlas-manager.h>

namespace Dali
{

namespace Toolkit
{

class AtlasManager;

} // namespace Toolkit

namespace Toolkit
{

namespace Internal
{

typedef Dali::Vector< Toolkit::AtlasManager::AtlasSlot > slotContainer;

class AtlasManager;
typedef IntrusivePtr<AtlasManager> AtlasManagerPtr;

class AtlasManager : public Dali::BaseObject, public ConnectionTracker
{
public:

  typedef uint32_t SizeType;
  typedef SizeType AtlasId;
  typedef SizeType ImageId;

  /**
   * @brief Internal storage of atlas attributes and image upload results
   */
  struct AtlasDescriptor
  {
    Dali::Atlas mAtlas;                                                 // atlas image
    Toolkit::AtlasManager::AtlasSize mSize;                             // size of atlas
    Pixel::Format mPixelFormat;                                         // pixel format used by atlas
    BufferImage mHorizontalStrip;                                       // Image used to pad upload
    BufferImage mVerticalStrip;                                         // Image used to pad upload
    BufferImage mFilledPixelImage;                                      // Image used by atlas for operations such as underline
    PixelBuffer* mStripBuffer;                                          // Blank image buffer used to pad upload
    Material mMaterial;                                                 // material used for atlas texture
    Sampler mSampler;                                                   // sampler used for atlas texture
    SizeType mTotalBlocks;                                              // total number of blocks in atlas
    SizeType mAvailableBlocks;                                          // number of blocks available in atlas
    Dali::Vector< SizeType > mFreeBlocksList;                           // unless there are any previously freed blocks
  };

  struct AtlasSlotDescriptor
  {
    SizeType mCount;                                                    // Reference count for this slot
    SizeType mImageWidth;                                               // Width of image stored
    SizeType mImageHeight;                                              // Height of image stored
    AtlasId mAtlasId;                                                   // Image is stored in this Atlas
    Dali::Vector< SizeType > mBlocksList;                               // List of blocks within atlas used for image
  };

  AtlasManager();

  /**
   * Create a new AtlasManager
   */
  static AtlasManagerPtr New();

  virtual ~AtlasManager();

  /**
   * @copydoc: Toolkit::AtlasManager::CreateAtlas
   */
  AtlasId CreateAtlas( const Toolkit::AtlasManager::AtlasSize& size, Pixel::Format pixelformat );

  /**
   * @copydoc Toolkit::AtlasManager::SetAddPolicy
   */
  void SetAddPolicy( Toolkit::AtlasManager::AddFailPolicy policy );

  /**
   * @copydoc Toolkit::AtlasManager::Add
   */
  void Add( const BufferImage& image,
            Toolkit::AtlasManager::AtlasSlot& slot,
            Toolkit::AtlasManager::AtlasId atlas );

  /**
   * @copydoc Toolkit::AtlasManager::GenerateMeshData
   */
  void GenerateMeshData( ImageId id,
                         const Vector2& position,
                         Toolkit::AtlasManager::Mesh2D& mesh,
                         bool addReference );

  /**
   * @copydoc Toolkit::AtlasManager::StitchMesh
   */
  void StitchMesh( Toolkit::AtlasManager::Mesh2D& first,
                   const Toolkit::AtlasManager::Mesh2D& second,
                   bool optimize );

  /**
   * @copydoc Toolkit::AtlasManager::StitchMesh
   */
  void StitchMesh(  const Toolkit::AtlasManager::Mesh2D& first,
                    const Toolkit::AtlasManager::Mesh2D& second,
                    Toolkit::AtlasManager::Mesh2D& out,
                    bool optimize );

  /**
   * @copydoc Toolkit::AtlasManager::Remove
   */
  bool Remove( ImageId id );

  /**
   * @copydoc Toolkit::AtlasManager::GetAtlasContainer
   */
  Dali::Atlas GetAtlasContainer( AtlasId atlas ) const;

  /**
   * @copydoc Toolkit::AtlasManager::GetAtlas
   */
  AtlasId GetAtlas( ImageId id ) const;

  /**
   * @copydoc Toolkit::AtlasManager::SetNewAtlasSize
   */
  void SetNewAtlasSize( const Toolkit::AtlasManager::AtlasSize& size );

  /**
   * @copydoc Toolkit::AtlasManager::GetAtlasSize
   */
  const Toolkit::AtlasManager::AtlasSize& GetAtlasSize( AtlasId atlas );

  /**
   * @copydoc Toolkit::AtlasManager::GetBlockSize
   */
  Vector2 GetBlockSize( AtlasId atlas );

  /**
   * @copydoc Toolkit::AtlasManager::GetFreeBlocks
   */
  SizeType GetFreeBlocks( AtlasId atlas ) const;

  /*
   * @copydoc Toolkit::AtlasManager::GetAtlasCount
   */
  SizeType GetAtlasCount() const;

  /**
   * @copydoc Toolkit::AtlasManager::GetPixelFormat
   */
  Pixel::Format GetPixelFormat( AtlasId atlas );

  /**
   * @copydoc Toolkit::AtlasManager::GetMetrics
   */
  void GetMetrics( Toolkit::AtlasManager::Metrics& metrics );

  /**
   * @copydoc Toolkit::AtlasManager::GetMaterial
   */
  Material GetMaterial( AtlasId atlas ) const;

/**
   * @copydoc Toolkit::AtlasManager::GetSampler
   */
  Sampler GetSampler( AtlasId atlas ) const;

private:

  std::vector< AtlasDescriptor > mAtlasList;            // List of atlases created
  std::vector< AtlasSlotDescriptor > mImageList;        // List of bitmaps store in atlases
  Vector< PixelBuffer* > mUploadedImages;               // List of PixelBuffers passed to UploadedSignal
  Toolkit::AtlasManager::AtlasSize mNewAtlasSize;       // Atlas size to use in next creation
  Toolkit::AtlasManager::AddFailPolicy mAddFailPolicy;  // Policy for faling to add an Image
  SizeType mFilledPixel;                                // 32Bit pixel image for underlining

  SizeType CheckAtlas( SizeType atlas,
                       SizeType width,
                       SizeType height,
                       Pixel::Format pixelFormat,
                       SizeType& blockArea );

  void CreateMesh( SizeType atlas,
                   SizeType imageWidth,
                   SizeType imageHeight,
                   const Vector2& position,
                   SizeType widthInBlocks,
                   SizeType heightInBlocks,
                   Toolkit::AtlasManager::Mesh2D& mesh,
                   AtlasSlotDescriptor& desc );

  void OptimizeMesh( const Toolkit::AtlasManager::Mesh2D& in,
                     Toolkit::AtlasManager::Mesh2D& out );

  void UploadImage( const BufferImage& image,
                    const AtlasSlotDescriptor& desc );

  void PrintMeshData( const Toolkit::AtlasManager::Mesh2D& mesh );

  void OnUpload( Image image );

  Shader mShaderL8;
  Shader mShaderRgba;

};

} // namespace Internal

inline const Internal::AtlasManager& GetImplementation(const Toolkit::AtlasManager& manager)
{
  DALI_ASSERT_ALWAYS( manager && "AtlasManager handle is empty" );

  const BaseObject& handle = manager.GetBaseObject();

  return static_cast<const Internal::AtlasManager&>(handle);
}

inline Internal::AtlasManager& GetImplementation(Toolkit::AtlasManager& manager)
{
  DALI_ASSERT_ALWAYS( manager && "AtlasManager handle is empty" );

  BaseObject& handle = manager.GetBaseObject();

  return static_cast<Internal::AtlasManager&>(handle);
}

} // namespace Toolkit

} // namespace Dali


 #endif // __DALI_TOOLKIT_ATLAS_MANAGER_IMPL_H__