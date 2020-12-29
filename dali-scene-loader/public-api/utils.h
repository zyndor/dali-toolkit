#ifndef DALI_SCENE_LOADER_UTILS_H_
#define DALI_SCENE_LOADER_UTILS_H_
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
#include "dali/public-api/actors/actor.h"
#include "dali/public-api/rendering/renderer.h"
#include "dali/public-api/common/dali-common.h"
#include <sstream>
#include <cctype>

namespace Dali
{
namespace SceneLoader
{

/*
 * @brief Fixed size backing buffer to use with std::ostreams where control over
 *  allocations (which this does not make), is required.
 * @note All stream insertions that would overflow the buffer that StreamBuffer
 *  was created with, will fail.
 */
class DALI_SCENE_LOADER_API StreamBuffer : public std::basic_streambuf<char>
{
public:
  StreamBuffer(char* buffer, size_t size) noexcept(true);
};

/*
 * @brief Wraps an ostream with a pre-allocated, fixed size backing buffer
 *  which a message can be formatted into. Upon destruction, it throws a
 *  DaliException with the message.
 */
class DALI_SCENE_LOADER_API ExceptionFlinger
{
public:
  enum { MESSAGE_BUFFER_SIZE = 512 };

  ExceptionFlinger(const char* location) noexcept(true);

  [[noreturn]]
  ~ExceptionFlinger() noexcept(false);

  template <typename T>
  ExceptionFlinger& operator<<(const T& rhs) noexcept(true)
  {
    mStream << rhs;
    return *this;
  }

private:
  struct Impl
  {
    const char* mLocation;

    [[noreturn]]
    ~Impl() noexcept(false);
  };

  static char* GetMessageBuffer() noexcept(true);

  Impl mImpl;
  StreamBuffer mStreamBuffer;
  std::ostream mStream;
};

/*
 * @brief Formats the given printf-style varargs into a std::string.
 */
DALI_SCENE_LOADER_API std::string FormatString(const char* format, ...);

/*
 * @return The @n th bit in a bitmask.
 */
DALI_SCENE_LOADER_API constexpr size_t NthBit(size_t n) { return 1 << n; }

/*
 * @return Whether all of @a mask 's bits are set on @a value.
 */
inline
DALI_SCENE_LOADER_API bool MaskMatch(uint32_t value, uint32_t mask)
{
  return (value & mask) == mask;
}

/*
 * @brief Convert a four-letter(, null-terminated) string literal into a uint32_t.
 */
inline
DALI_SCENE_LOADER_API constexpr uint32_t FourCC(const char(&fourCC)[5])
{
  return (fourCC[3] << 24) | (fourCC[2] << 16) | (fourCC[1] << 8) | fourCC[0];
}

/*
 * @brief Insensitive case compare function.
 * @param[in] a, compare string
 * @param[in] b, compare string
 * @return true if strings are equal
 */
inline
DALI_SCENE_LOADER_API bool CaseInsensitiveCharacterCompare( unsigned char a, unsigned char b )
{
  // Converts to lower case in the current locale.
  return std::tolower( a ) == std::tolower( b );
}

/*
 * @return true if the lower cased ASCII strings are equal.
 * @param[in] a, compare string
 * @param[in] b, compare string
 */
inline
DALI_SCENE_LOADER_API bool CaseInsensitiveStringCompare( const std::string& a, const std::string& b )
{
  bool result = false;
  if( a.length() == b.length() )
  {
    result = std::equal( a.begin(), a.end(), b.begin(), &CaseInsensitiveCharacterCompare );
  }
  return result;
}

/*
 * @brief Attempts to load the contents of a text file; returns empty string on
 *  failure. A pointer to a boolean may be passed in @a fail; this will be set
 *  to true in case of failure (should only be checked if the returned string
 *  was empty()).
 */
DALI_SCENE_LOADER_API std::string LoadTextFile(const char* path, bool* fail = nullptr);

/*
 * @brief Makes a number of calls to @a fn, passing to each one the given
 *  @a actor then each of its children, in depth-first traversal.
 * @note @a fn must not change the actor hierarchy during traversal.
 * @note Use of a @a fn that is itself recursing in @a is also discouraged
 *  for performance and stability reasons.
 */
template <typename Func>
inline
DALI_SCENE_LOADER_API void VisitActor(Actor a, Func fn)
{
  fn(a);

  unsigned int numChildren = a.GetChildCount();
  for(unsigned int i = 0; i < numChildren; ++i)
  {
    VisitActor(a.GetChildAt(i), fn);
  }
}

/*
 * @brief Convenience function to set the given actor @a 's anchor point
 *  and parent origin to center.
 */
inline
DALI_SCENE_LOADER_API void SetActorCentered(Actor a)
{
  a.SetProperty(Actor::Property::ANCHOR_POINT, AnchorPoint::CENTER);
  a.SetProperty(Actor::Property::PARENT_ORIGIN, ParentOrigin::CENTER);
}

/*
 * @brief Creates a copy of the given texture set, by creating a new instance and
 *  copying all Texture and Sampler references from it.
 */
DALI_SCENE_LOADER_API TextureSet CloneTextures(TextureSet ts);

namespace CloneOptions
{
using Type = uint32_t;

enum Values: Type
{
  NONE = 0x00,

  OMIT_TEXTURES = 0x01,   // Do not create TextureSet and share Texture references (Renderer).

  OMIT_RENDERERS = 0x10,  // Do not clone Renderers (Actor).
  NO_RECURSE = 0x20,      // Do not clone children (Actor).
  CONSTRAIN_XFORM = 0x40, // Constrain the position and orientation and scale of the clone to the original.
  CONSTRAIN_VISIBILITY = 0x80, // Constrain the visibility of the clone to the original.
  CONSTRAIN_SIZE = 0x100, // Constrain the size of the clone to the original.

  // Local additions to CloneOptions should start at this value; consider all preceding ones reserved.
  FIRST_USER_OPTION = 0x200,
};

}

/*
 * @brief Creates a new Renderer with all conceivable properties of the given
 *  Renderer @a r. A new TextureSet will be created if the original had one and
 *  CloneOptions::OMIT_TEXTURES was not specified, but all Texture, Shader and
 *  Geometry references are shared between @a r and the new instance.
 * @param r The Renderer to clone.
 * @param cloneOptions A bitmask constructed from the combination of CloneOptions
 *  values (plus any user defined values).
 */
DALI_SCENE_LOADER_API Renderer CloneRenderer(Renderer r, CloneOptions::Type cloneOptions = CloneOptions::NONE);

/*
 * @brief Create a new actor with all conceivable properties and Renderers of
 *  the given actor @a a.
 *  Will clone all children unless CloneOptions::NO_RECURSE was set.
 *  Will clone Renderers unless CloneOptions::OMIT_RENDERERS was set.
 * @param a The Actor to clone.
 * @param cloneOptions A bitmask constructed from the combination of CloneOptions
 *  values (plus any user defined values).
 */
DALI_SCENE_LOADER_API Actor CloneActor(Actor a, CloneOptions::Type cloneOptions = CloneOptions::NONE);

namespace TexturedQuadOptions
{
using Type = uint32_t;

enum DALI_SCENE_LOADER_API Values : Type
{
  NONE = 0x00,
  FLIP_VERTICAL = 0x01,
  GENERATE_BARYCENTRICS = 0x02,
};
}

/*
 * @brief Makes... geometry for a textured quad.
 */
DALI_SCENE_LOADER_API Geometry MakeTexturedQuadGeometry(TexturedQuadOptions::Type options = TexturedQuadOptions::NONE);

/*
 * @brief Makes... a textured quad renderer. With back face culling enabled.
 * @param texture The texture to use as texture 0 of a set we're creating.
 * @param shader The shader to use with the renderer.
 */
DALI_SCENE_LOADER_API Renderer MakeTexturedQuadRenderer(Texture texture, Shader shader,
  TexturedQuadOptions::Type options = TexturedQuadOptions::NONE);

/*
 * @brief Makes... a textured quad actor. Centered. With back face culling enabled.
 * @param texture The texture to use as texture 0 of a set we're creating.
 * @param shader The shader to use with the renderer.
 */
DALI_SCENE_LOADER_API Actor MakeTexturedQuadActor(Texture texture, Shader shader,
  TexturedQuadOptions::Type options = TexturedQuadOptions::NONE);

/*
 * @brief Fixes the path of a file. Replaces the '\\' separator by the '/' one.
 * @param[in,out] path The path to be fixed.
 */
DALI_SCENE_LOADER_API void ToUnixFileSeparators( std::string& path );

}
}

#endif /* DALI_SCENE_LOADER_UTILS_H_ */
