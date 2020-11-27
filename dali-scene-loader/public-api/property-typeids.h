#ifndef DALI_SCENE_LOADER_PROPERTY_TYPEIDS_H_
#define DALI_SCENE_LOADER_PROPERTY_TYPEIDS_H_
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
#include "dali/public-api/object/property.h"
#include <string>
#include <cstdint>

namespace Dali
{
namespace SceneLoader
{

/*
 * @return A fourCC relating to the given Property::Type.
 */
DALI_SCENE_LOADER_API uint32_t GetPropertyTypeId(Property::Type type);

/*
 * @return A fourCC string relating to the given Property::Type.
 */
DALI_SCENE_LOADER_API std::string GetPropertyTypeIdString(Property::Type type);

/*
 * @return Property type enum value based on fourCC typeId, NONE if invalid.
 */
DALI_SCENE_LOADER_API Property::Type DecodePropertyTypeId(uint32_t typeId);

}
}

#endif //#ifdef DALI_SCENE_LOADER_PROPERTY_TYPEIDS_H_
