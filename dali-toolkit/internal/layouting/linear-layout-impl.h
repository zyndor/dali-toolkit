#ifndef DALI_TOOLKIT_INTERNAL_LAYOUTING_LINEAR_LAYOUT_H
#define DALI_TOOLKIT_INTERNAL_LAYOUTING_LINEAR_LAYOUT_H

/*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd.
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

#include <dali/public-api/common/intrusive-ptr.h>
#include <dali/public-api/object/base-object.h>
#include <dali-toolkit/devel-api/layouting/layout-group-impl.h>
#include <dali-toolkit/devel-api/layouting/linear-layout.h>

namespace Dali
{
namespace Toolkit
{
namespace Internal
{

class LinearLayout;
using LinearLayoutPtr = IntrusivePtr<LinearLayout>;

class LinearLayout final : public LayoutGroup
{
public:
  static LinearLayoutPtr New();

public:
  void SetCellPadding( LayoutSize size );
  LayoutSize GetCellPadding();
  void SetOrientation( Dali::Toolkit::LinearLayout::Orientation orientation );
  Dali::Toolkit::LinearLayout::Orientation GetOrientation();

protected:
  LinearLayout();
  virtual ~LinearLayout();

  /**
   * @copydoc LayoutItem::OnMeasure
   */
  virtual void OnMeasure( MeasureSpec widthMeasureSpec, MeasureSpec heightMeasureSpec ) override;

  /**
   * @copydoc LayoutItem::OnLayout
   */
  virtual void OnLayout( bool changed, LayoutLength l, LayoutLength t, LayoutLength r, LayoutLength b ) override;

private:
  LinearLayout( const LinearLayout& other ) = delete;
  LinearLayout& operator=( const LinearLayout& other ) = delete;

 /**
  * Measure children for horizontal orientation
  */
  void MeasureHorizontal( MeasureSpec widthMeasureSpec, MeasureSpec heightMeasureSpec );

 /**
  * Apply a uniform height to the children
  */
  void ForceUniformHeight( int count, MeasureSpec widthMeasureSpec );

 /**
  * Layout the children horizontally
  */
  void LayoutHorizontal( LayoutLength l, LayoutLength t, LayoutLength r, LayoutLength b );

 /**
  * Measure children for horizontal orientation
  */
  void MeasureVertical( MeasureSpec widthMeasureSpec, MeasureSpec heightMeasureSpec );

 /**
  * Apply a uniform width to the children
  */
  void ForceUniformWidth( int count, MeasureSpec heightMeasureSpec );

 /**
  * Layout the children vertically
  */
  void LayoutVertical( LayoutLength l, LayoutLength t, LayoutLength r, LayoutLength b );

private:
  LayoutSize mCellPadding;
  Dali::Toolkit::LinearLayout::Orientation mOrientation;
  LayoutLength mTotalLength;
};

} // namespace Internal

inline Internal::LinearLayout& GetImplementation( Dali::Toolkit::LinearLayout& handle )
{
  DALI_ASSERT_ALWAYS( handle && "LinearLayout handle is empty" );
  BaseObject& object = handle.GetBaseObject();
  return static_cast<Internal::LinearLayout&>( object );
}

inline const Internal::LinearLayout& GetImplementation( const Dali::Toolkit::LinearLayout& handle )
{
  DALI_ASSERT_ALWAYS( handle && "LinearLayout handle is empty" );
  const BaseObject& object = handle.GetBaseObject();
  return static_cast<const Internal::LinearLayout&>( object );
}

} // namespace Toolkit
} // namespace Dali

#endif // DALI_TOOLKIT_INTERNAL_LAYOUTING_LINEAR_LAYOUT_H
