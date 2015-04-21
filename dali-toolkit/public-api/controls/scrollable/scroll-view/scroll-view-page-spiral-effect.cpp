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
#include <dali-toolkit/public-api/controls/scrollable/scroll-view/scroll-view-page-spiral-effect.h>

// INTERNAL HEADERS
#include <dali-toolkit/internal/controls/scrollable/scroll-view/scroll-view-page-spiral-effect-impl.h>

using namespace Dali;

namespace Dali
{

namespace Toolkit
{

ScrollViewPageSpiralEffect ScrollViewPageSpiralEffect::New()
{
  return ScrollViewPageSpiralEffect(new Internal::ScrollViewPageSpiralEffect());
}

ScrollViewPageSpiralEffect::ScrollViewPageSpiralEffect()
{

}

ScrollViewPageSpiralEffect::ScrollViewPageSpiralEffect(Internal::ScrollViewPageSpiralEffect *impl)
: ScrollViewEffect(impl)
{
}

ScrollViewPageSpiralEffect ScrollViewPageSpiralEffect::DownCast( BaseHandle handle )
{
  return ScrollViewPageSpiralEffect( dynamic_cast<Internal::ScrollViewPageSpiralEffect*>(handle.GetObjectPtr()) );
}

void ScrollViewPageSpiralEffect::ApplyToPage( Actor page, const Vector2& spiralAngle )
{
  GetImpl(*this).ApplyToPage( page, spiralAngle );
}

} // namespace Toolkit

} // namespace Dali
