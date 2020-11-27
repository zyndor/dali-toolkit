#ifndef DALI_SCENE_LOADER_BVH_LOADER_H_
#define DALI_SCENE_LOADER_BVH_LOADER_H_
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
#include "dali-scene-loader/public-api/animation-definition.h"

namespace Dali
{
namespace SceneLoader
{

struct BvhHierarchy;

using BhvHierarchyPtr = std::unique_ptr <BvhHierarchy, void(*)(BvhHierarchy*)> ;

/**
 * @brief Attempts to load the first HIERARCHY section of a BioVision Hierarchy file from the given
 *  @a url, setting the given @a scale to it.
 * @param url The location of the file in the file system.
 * @param scale The scale that translations defined in motions will be applied to.
 * @return An opaque handle to the resulting BvhHierarchy; nullptr if the loading or parsing of the
 *  file has failed.
 * @note Throws DaliException on failure.
 */
BhvHierarchyPtr DALI_SCENE_LOADER_API LoadBvhHierarchy(const std::string& url, float scale = 1.f);

/**
 * @brief Attempts to load a motion file for the given @a hierarchy, from the given @a url, in the
 *  binary BioVision Hierarchy MOTION format, which is as follows:<br/>
 *  4 bytes (integer) number of frames in the animation<br/>
 *  4 bytes (float) seconds of frame delay<br/>
 *  4 bytes (integer) amount of channel data per frame. This must match @a hierarchy.<br/>
 *  4 x number of frames x amount of channel data bytes (float) of translation and rotation data
 *  for the joints, defined in the order they are encountered in the @a hierarchy. N.B. Rotations
 *  are in degrees.
 * @param url The location of the file in the file system.
 * @param hierarchy The joint hierarchy that the motion applies to.
 * @return AnimationDefinition for the translations and rotations of the joints of the given
 *  @a hierarchy.
 * @note Throws DaliException on failure.
 */
AnimationDefinition DALI_SCENE_LOADER_API LoadBvhMotion(const std::string& url, BvhHierarchy const& hierarchy);

}
}

#endif  //DALI_SCENE_LOADER_BVH_LOADER_H_
