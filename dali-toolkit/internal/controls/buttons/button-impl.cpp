/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
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
#include "button-impl.h"

// EXTERNAL INCLUDES
#include <cstring> // for strcmp
#include <dali/devel-api/scripting/enum-helper.h>
#include <dali/integration-api/debug.h>
#include <dali/public-api/events/touch-data.h>
#include <dali/public-api/images/resource-image.h>
#include <dali/public-api/object/type-registry.h>
#include <dali/public-api/object/type-registry-helper.h>
#include <dali/public-api/size-negotiation/relayout-container.h>
#include <dali/devel-api/scripting/scripting.h>

// INTERNAL INCLUDES
#include <dali-toolkit/public-api/controls/text-controls/text-label.h>
#include <dali-toolkit/public-api/controls/image-view/image-view.h>
#include <dali-toolkit/public-api/visuals/color-visual-properties.h>
#include <dali-toolkit/devel-api/visuals/visual-properties-devel.h>
#include <dali-toolkit/public-api/visuals/image-visual-properties.h>
#include <dali-toolkit/devel-api/align-enums.h>
#include <dali-toolkit/devel-api/controls/control-depth-index-ranges.h>
#include <dali-toolkit/devel-api/visual-factory/devel-visual-properties.h>
#include <dali-toolkit/devel-api/visual-factory/visual-factory.h>


#if defined(DEBUG_ENABLED)
    Debug::Filter* gLogButtonFilter = Debug::Filter::New(Debug::NoLogging, false, "LOG_BUTTON_CONTROL");
#endif

namespace Dali
{

namespace Toolkit
{

namespace Internal
{

namespace
{

BaseHandle Create()
{
  // empty handle as we cannot create button (but type registered for clicked signal)
  return BaseHandle();
}

// Setup properties, signals and actions using the type-registry.
DALI_TYPE_REGISTRATION_BEGIN( Toolkit::Button, Toolkit::Control, Create )

DALI_PROPERTY_REGISTRATION( Toolkit, Button, "disabled",                           BOOLEAN, DISABLED                              )
DALI_PROPERTY_REGISTRATION( Toolkit, Button, "autoRepeating",                      BOOLEAN, AUTO_REPEATING                        )
DALI_PROPERTY_REGISTRATION( Toolkit, Button, "initialAutoRepeatingDelay",          FLOAT,   INITIAL_AUTO_REPEATING_DELAY          )
DALI_PROPERTY_REGISTRATION( Toolkit, Button, "nextAutoRepeatingDelay",             FLOAT,   NEXT_AUTO_REPEATING_DELAY             )
DALI_PROPERTY_REGISTRATION( Toolkit, Button, "togglable",                          BOOLEAN, TOGGLABLE                             )
DALI_PROPERTY_REGISTRATION( Toolkit, Button, "selected",                           BOOLEAN, SELECTED                              )
DALI_PROPERTY_REGISTRATION( Toolkit, Button, "unselectedStateImage",               MAP,     UNSELECTED_STATE_IMAGE                ) // Deprecated property
DALI_PROPERTY_REGISTRATION( Toolkit, Button, "selectedStateImage",                 MAP,     SELECTED_STATE_IMAGE                  ) // Deprecated property
DALI_PROPERTY_REGISTRATION( Toolkit, Button, "disabledStateImage",                 MAP,     DISABLED_STATE_IMAGE                  ) // Deprecated property
DALI_PROPERTY_REGISTRATION( Toolkit, Button, "unselectedColor",                    VECTOR4, UNSELECTED_COLOR                      ) // Deprecated property
DALI_PROPERTY_REGISTRATION( Toolkit, Button, "selectedColor",                      VECTOR4, SELECTED_COLOR                        ) // Deprecated property
DALI_PROPERTY_REGISTRATION( Toolkit, Button, "label",                              MAP,     LABEL                                 )
DALI_PROPERTY_REGISTRATION( Toolkit, Button, "labelText",                          STRING,  LABEL_TEXT                            ) // Deprecated property
DALI_PROPERTY_REGISTRATION( Toolkit, Button, "unselectedVisual",                   MAP,     UNSELECTED_VISUAL                     )
DALI_PROPERTY_REGISTRATION( Toolkit, Button, "selectedVisual",                     MAP,     SELECTED_VISUAL                       )
DALI_PROPERTY_REGISTRATION( Toolkit, Button, "disabledSelectedVisual",             MAP,     DISABLED_SELECTED_VISUAL              )
DALI_PROPERTY_REGISTRATION( Toolkit, Button, "disabledUnselectedVisual",           MAP,     DISABLED_UNSELECTED_VISUAL            )
DALI_PROPERTY_REGISTRATION( Toolkit, Button, "unselectedBackgroundVisual",         MAP,     UNSELECTED_BACKGROUND_VISUAL          )
DALI_PROPERTY_REGISTRATION( Toolkit, Button, "selectedBackgroundVisual",           MAP,     SELECTED_BACKGROUND_VISUAL            )
DALI_PROPERTY_REGISTRATION( Toolkit, Button, "disabledUnselectedBackgroundVisual", MAP,     DISABLED_UNSELECTED_BACKGROUND_VISUAL )
DALI_PROPERTY_REGISTRATION( Toolkit, Button, "disabledSelectedBackgroundVisual",   MAP,     DISABLED_SELECTED_BACKGROUND_VISUAL   )
DALI_PROPERTY_REGISTRATION( Toolkit, Button, "labelStrutLength",                   INTEGER, LABEL_STRUT_LENGTH                    )
DALI_PROPERTY_REGISTRATION( Toolkit, Button, "labelRelativeAlignment",             STRING,  LABEL_RELATIVE_ALIGNMENT              )

// Signals:
DALI_SIGNAL_REGISTRATION(   Toolkit, Button, "pressed",                               SIGNAL_PRESSED               )
DALI_SIGNAL_REGISTRATION(   Toolkit, Button, "released",                              SIGNAL_RELEASED              )
DALI_SIGNAL_REGISTRATION(   Toolkit, Button, "clicked",                               SIGNAL_CLICKED               )
DALI_SIGNAL_REGISTRATION(   Toolkit, Button, "stateChanged",                          SIGNAL_STATE_CHANGED         )

// Actions:
DALI_ACTION_REGISTRATION(   Toolkit, Button, "buttonClick",                           ACTION_BUTTON_CLICK          )

DALI_TYPE_REGISTRATION_END()

DALI_ENUM_TO_STRING_TABLE_BEGIN( ALIGNMENT )
DALI_ENUM_TO_STRING_WITH_SCOPE( Toolkit::Internal::Button, BEGIN )
DALI_ENUM_TO_STRING_WITH_SCOPE( Toolkit::Internal::Button, END )
DALI_ENUM_TO_STRING_WITH_SCOPE( Toolkit::Internal::Button, TOP )
DALI_ENUM_TO_STRING_WITH_SCOPE( Toolkit::Internal::Button, BOTTOM )
DALI_ENUM_TO_STRING_TABLE_END( ALIGNMENT )

const Scripting::StringEnum ALIGNMENT_STRING_TABLE[] =
{
  { "BEGIN",  Button::BEGIN   },
  { "END",    Button::END     },
  { "TOP",    Button::TOP     },
  { "BOTTOM", Button::BOTTOM  },
};

const unsigned int ALIGNMENT_STRING_TABLE_COUNT = sizeof( ALIGNMENT_STRING_TABLE ) / sizeof( ALIGNMENT_STRING_TABLE[0] );

const Property::Index GET_VISUAL_INDEX_FOR_STATE[][Button::STATE_COUNT] =
{
  { Toolkit::Button::Property::UNSELECTED_BACKGROUND_VISUAL, Toolkit::Button::Property::UNSELECTED_VISUAL },
  { Toolkit::Button::Property::SELECTED_BACKGROUND_VISUAL, Toolkit::Button::Property::SELECTED_VISUAL  },
  { Toolkit::Button::Property::DISABLED_UNSELECTED_BACKGROUND_VISUAL, Toolkit::Button::Property::DISABLED_UNSELECTED_VISUAL },
  { Toolkit::Button::Property::DISABLED_SELECTED_BACKGROUND_VISUAL, Toolkit::Button::Property::DISABLED_SELECTED_VISUAL }
};

} // unnamed namespace

Button::Button()
: Control( ControlBehaviour( CONTROL_BEHAVIOUR_DEFAULT ) ),
  mAutoRepeatingTimer(),
  mForeGroundToLabelStrutLength( 0.0f ),
  mTextLabelAlignment( END ),
  mAutoRepeating( false ),
  mTogglableButton( false ),
  mInitialAutoRepeatingDelay( 0.0f ),
  mNextAutoRepeatingDelay( 0.0f ),
  mAnimationTime( 0.0f ),
  mButtonPressedState( UNPRESSED ),
  mButtonState( UNSELECTED_STATE ),
  mPreviousButtonState( mButtonState ),
  mClickActionPerforming( false )
{
}

Button::~Button()
{
}

void Button::SetAutoRepeating( bool autoRepeating )
{
  mAutoRepeating = autoRepeating;

  // An autorepeating button can't be a togglable button.
  if( autoRepeating )
  {
    mTogglableButton = false;

    if( IsSelected() )
    {
      SetSelected( false );
    }
  }
}

bool Button::IsAutoRepeating() const
{
  return mAutoRepeating;
}

void Button::SetInitialAutoRepeatingDelay( float initialAutoRepeatingDelay )
{
  DALI_ASSERT_DEBUG( initialAutoRepeatingDelay > 0.f );
  mInitialAutoRepeatingDelay = initialAutoRepeatingDelay;
}

float Button::GetInitialAutoRepeatingDelay() const
{
  return mInitialAutoRepeatingDelay;
}

void Button::SetNextAutoRepeatingDelay( float nextAutoRepeatingDelay )
{
  DALI_ASSERT_DEBUG( nextAutoRepeatingDelay > 0.f );
  mNextAutoRepeatingDelay = nextAutoRepeatingDelay;
}

float Button::GetNextAutoRepeatingDelay() const
{
  return mNextAutoRepeatingDelay;
}

void Button::SetTogglableButton( bool togglable )
{
  mTogglableButton = togglable;

  // A toggle button can't be an autorepeating button.
  if( togglable )
  {
    mAutoRepeating = false;
  }
}

bool Button::IsTogglableButton() const
{
  return mTogglableButton;
}

void Button::SetSelected( bool selected )
{
  if( mTogglableButton )
  {
    DALI_LOG_INFO( gLogButtonFilter, Debug::Verbose, "Button::SetSelected (%s)\n", (selected?"true":"false") );

    if ( selected && ( mButtonState != SELECTED_STATE ) )
    {
      ChangeState( SELECTED_STATE );
    }
    else if ( !selected && ( mButtonState != UNSELECTED_STATE ) )
    {
      ChangeState( UNSELECTED_STATE );
    }
  }
}

void Button::SetDisabled( bool disabled )
{
  DALI_LOG_INFO( gLogButtonFilter, Debug::Verbose, "Button::SetDisabled(%s) state(%d)\n", (disabled)?"disabled":"active", mButtonState );

  if ( disabled )
  {
    if ( mButtonState == SELECTED_STATE )
    {
      ChangeState( DISABLED_SELECTED_STATE );
    }
    else if ( mButtonState == UNSELECTED_STATE )
    {
      ChangeState( DISABLED_UNSELECTED_STATE );
    }
  }
  else
  {
    if ( mButtonState == DISABLED_SELECTED_STATE )
    {
      ChangeState( SELECTED_STATE );
    }
    else if ( mButtonState == DISABLED_UNSELECTED_STATE )
    {
      ChangeState( UNSELECTED_STATE );
    }
  }
}

bool Button::IsDisabled() const
{
  return ( mButtonState == DISABLED_SELECTED_STATE || mButtonState == DISABLED_UNSELECTED_STATE ) ;
}

bool Button::ValidateState( State requestedState )
{
  /*  Below tables shows allowed state transitions
   *  Match rows in first column to following columns, if true then transition allowed.
   *  eg UNSELECTED_STATE to DISABLED_UNSELECTED_STATE is true so state transition allowed.
   *
                                                             to| UNSELECTED_STATE | SELECTED_STATE | DISABLED_UNSELECTED_STATE | DISABLED_SELECTED_STATE |*/
                                 /* from*/
  bool transitionTable[4][4] = { /* UNSELECTED_STATE*/         {      false,            true,               true,                   false         },
                                 /* SELECTED_STATE*/           {      true,             false,              false,                  true          },
                                 /* DISABLED_UNSELECTED_STATE*/{      true,             true,               false,                  false         },
                                 /* DISABLED_SELECTED_STATE*/  {      false,            true,               false,                  false         }
                               };

  DALI_LOG_INFO( gLogButtonFilter, Debug::Verbose, "Button::ValidateState ReuestedState:%d, CurrentState:%d, result:%s\n",
                 requestedState, mButtonState, (transitionTable[mButtonState][requestedState])?"change-accepted":"change-denied");

  return transitionTable[mButtonState][requestedState];
}

void Button::PerformFunctionOnVisualsInState( void(Button::*functionPtr)( Property::Index visualIndex), State state )
{
  DALI_LOG_INFO( gLogButtonFilter, Debug::Verbose, "Button::PerformFunctionOnVisualsInState BACKROUND visual(%d) for state (%d)\n", GET_VISUAL_INDEX_FOR_STATE[state][BACKGROUND], state );
  DALI_LOG_INFO( gLogButtonFilter, Debug::Verbose, "Button::PerformFunctionOnVisualsInState FOREGROUND visuals(%d)  for state (%d)\n", GET_VISUAL_INDEX_FOR_STATE[state][FOREGROUND], state );

  (this->*functionPtr)( GET_VISUAL_INDEX_FOR_STATE[state][BACKGROUND] );
  (this->*functionPtr)( GET_VISUAL_INDEX_FOR_STATE[state][FOREGROUND] );

  RelayoutRequest();
}

void Button::ChangeState( State requestedState )
{
  DALI_LOG_INFO( gLogButtonFilter, Debug::Verbose, "Button::ChangeState ReuestedState(%d)\n", requestedState );

  // Validate State before changing
  if ( !ValidateState( requestedState ))
  {
    DALI_LOG_INFO( gLogButtonFilter, Debug::Verbose, "Button::ChangeState ReuestedState(%d) not validated\n", requestedState );
    return;
  }

  // If not on stage the button could have still been set to selected so update state/
  mPreviousButtonState = mButtonState; // Store previous state for visual removal (used when animations ended)
  mButtonState = requestedState; // Update current state

  if ( Self().OnStage() )
  {
    // Clear existing animation and remove visual being transitioned out before starting a new transition of visuals. ClearTransitionAnimation();
    OnStateChange( mButtonState ); // Notify derived buttons
    //// When animations enabled PerformFunctionOnVisualsInState( &Button::TransitionButtonVisualOut, mPreviousButtonState );
    PerformFunctionOnVisualsInState( &Button::SelectRequiredVisual, mButtonState );
    // When animations enabled then call PerformFunctionOnVisualsInState( &Button::TransitionButtonVisualIn, mButtonState );
    // then StartTransitionAnimation();
    // and ClearTransitionAnimation();
    PerformFunctionOnVisualsInState( &Button::OnButtonVisualRemoval, mPreviousButtonState ); // Derived button can override OnButtonVisualRemoval
  }

  Toolkit::Button handle( GetOwner() );
  // Emit signal.
  mStateChangedSignal.Emit( handle );
}

bool Button::IsSelected() const
{
  bool selected = ( mButtonState == SELECTED_STATE ) || ( mButtonState == DISABLED_SELECTED_STATE );
  return mTogglableButton && selected;
}

void Button::SetAnimationTime( float animationTime )
{
  mAnimationTime = animationTime;
}

float Button::GetAnimationTime() const
{
  return mAnimationTime;
}

void Button::SetLabelText( const std::string& label )
{
  Property::Map labelProperty;
  labelProperty.Insert( "text", label );
  SetupLabel( labelProperty );
}

std::string Button::GetLabelText() const
{
  Toolkit::TextLabel label = Dali::Toolkit::TextLabel::DownCast( mLabel );
  if( label )
  {
    return label.GetProperty<std::string>( Dali::Toolkit::TextLabel::Property::TEXT );
  }
  return std::string();
}

void Button::SetupLabel( const Property::Map& properties )
{
  DALI_LOG_INFO( gLogButtonFilter, Debug::Verbose, "SetupLabel\n");

  // If we don't have a label yet, create one.
  if( !mLabel )
  {
    // If we don't have a label, create one and set it up.
    // Note: The label text is set from the passed in property map after creation.
    mLabel = Toolkit::TextLabel::New();
    mLabel.SetProperty( Toolkit::TextLabel::Property::HORIZONTAL_ALIGNMENT, "CENTER" );
    mLabel.SetProperty( Toolkit::TextLabel::Property::VERTICAL_ALIGNMENT, "CENTER" );
    mLabel.SetParentOrigin( ParentOrigin::TOP_LEFT );
    mLabel.SetAnchorPoint( AnchorPoint::TOP_LEFT );

    // todo DEBUG
    mLabel.SetProperty( Toolkit::Control::Property::BACKGROUND, Dali::Property::Map()
                        .Add( Toolkit::Visual::Property::TYPE, Dali::Toolkit::Visual::COLOR )
                        .Add( Toolkit::ColorVisual::Property::MIX_COLOR, Color::RED )
                      );

    ResizePolicy::Type policy = Self().GetResizePolicy(  Dimension::ALL_DIMENSIONS );
    if ( policy == ResizePolicy::USE_NATURAL_SIZE || policy == ResizePolicy::FIT_TO_CHILDREN  )
    {
      mLabel.SetResizePolicy(ResizePolicy::USE_NATURAL_SIZE, Dimension::ALL_DIMENSIONS );
    }
    else
    {
      // todo Can't set Text Label to USE_ASSIGNED_SIZE as causes a relayout in it whilst doing a relayout = error
     //mLabel.SetResizePolicy(ResizePolicy::USE_ASSIGNED_SIZE, Dimension::ALL_DIMENSIONS );
    }
    Self().Add( mLabel );
  }

  // Set any properties specified for the label by iterating through all property key-value pairs.
  for( unsigned int i = 0, mapCount = properties.Count(); i < mapCount; ++i )
  {
    const StringValuePair& propertyPair( properties.GetPair( i ) );

    // Convert the property string to a property index.
    Property::Index setPropertyIndex = mLabel.GetPropertyIndex( propertyPair.first );
    if( setPropertyIndex != Property::INVALID_INDEX )
    {
      // If the conversion worked, we have a valid property index,
      // Set the property to the new value.
      mLabel.SetProperty( setPropertyIndex, propertyPair.second );
    }
  }

  // Notify derived button classes of the change.
  OnLabelSet( false );

  RelayoutRequest();
}

void Button::SetLabelStrutLength( unsigned int length )
{
  mForeGroundToLabelStrutLength = length;
}

void Button::SetLabelAlignment( Button::Align labelAlignment)
{
  mTextLabelAlignment = labelAlignment;
  RelayoutRequest();
}

float Button::GetLabelStrutLength()
{
  return mForeGroundToLabelStrutLength;
}

Button::Align Button::GetLabelAlignment()
{
  return mTextLabelAlignment;
}

void Button::CreateVisualsForComponent( Property::Index index, const Property::Value& value, const float visualDepth )
{
  DALI_LOG_INFO( gLogButtonFilter, Debug::Verbose, "CreateVisualsForComponent index(%d)\n", index );
  Toolkit::VisualFactory visualFactory = Toolkit::VisualFactory::Get();
  Toolkit::Visual::Base buttonVisual;

  std::string imageUrl;
  if( value.Get( imageUrl ) )
  {
    DALI_LOG_INFO( gLogButtonFilter, Debug::Verbose, "CreateVisualsForComponent Using image URL(%d)\n", index );
    if ( !imageUrl.empty() )
    {
      buttonVisual = visualFactory.CreateVisual(  imageUrl, ImageDimensions()  );
    }
  }
  else
  {
    // if its not a string then get a Property::Map from the property if possible.
    Property::Map *map = value.GetMap();
    if( map && !map->Empty()  ) // Empty map results in current visual removal.
    {
      DALI_LOG_INFO( gLogButtonFilter, Debug::Verbose, "CreateVisualsForComponent Using Map(%d)\n", index );
      buttonVisual = visualFactory.CreateVisual( *map );
    }
  }

  if ( buttonVisual )
  {
    DALI_LOG_INFO( gLogButtonFilter, Debug::Verbose, "RegisterVisual index(%d)\n", index );
    buttonVisual.SetDepthIndex( visualDepth );
    // Background Visuals take full size of control
    RegisterVisual( index, buttonVisual, false );
  }
  else
  {
    UnregisterVisual( index );
    DALI_LOG_INFO( gLogButtonFilter, Debug::General, "Button visual not created or empty map provided (clearing visual).(%d)\n", index);
  }
}

const Vector4 Button::GetUnselectedColor() const
{
  return mUnselectedColor;
}

const Vector4 Button::GetSelectedColor() const
{
  return mSelectedColor;
}

// Legacy code whilst Color can be set by direct Property setting ( deprecated ) instead of setting a Visual
void Button::SetColor( const Vector4& color, Property::Index visualIndex )
{
  if ( visualIndex == Toolkit::Button::Property::SELECTED_BACKGROUND_VISUAL )
  {
    mSelectedColor = color;
  }
  else
  {
    mUnselectedColor = color;
  }

  Property::Map map;
  map[ Toolkit::Visual::Property::TYPE ] = Toolkit::Visual::COLOR;
  map[ Toolkit::ColorVisual::Property::MIX_COLOR ] = color;

  CreateVisualsForComponent( visualIndex, map, DepthIndex::BACKGROUND );
}

bool Button::DoAction( BaseObject* object, const std::string& actionName, const Property::Map& attributes )
{
  bool ret = false;

  Dali::BaseHandle handle( object );

  Toolkit::Button button = Toolkit::Button::DownCast( handle );

  DALI_ASSERT_DEBUG( button );

  if( 0 == strcmp( actionName.c_str(), ACTION_BUTTON_CLICK ) )
  {
    ret = GetImplementation( button ).DoClickAction( attributes );
  }

  return ret;
}

bool Button::DoClickAction( const Property::Map& attributes )
{
  // Prevents the button signals from doing a recursive loop by sending an action
  // and re-emitting the signals.
  if( !mClickActionPerforming )
  {
    mClickActionPerforming = true;
    OnButtonDown();
    if ( !mTogglableButton )
    {
      mButtonPressedState = DEPRESSED;
    }
    OnButtonUp();
    mClickActionPerforming = false;

    return true;
  }

  return false;
}

void Button::OnButtonDown()
{
  if( mTogglableButton )
  {
    if ( mButtonState != SELECTED_STATE )
    {
      SetSelected( true );
      mButtonPressedState = TOGGLE_DEPRESSED;
    }
    else
    {
      mButtonPressedState = DEPRESSED;
    }
  }
  else
  {
    Pressed();
    mButtonPressedState = DEPRESSED;
    if( mAutoRepeating )
    {
       SetUpTimer( mInitialAutoRepeatingDelay );
    }
  }

  // The pressed signal should be emitted regardless of toggle mode.
  Toolkit::Button handle( GetOwner() );
  mPressedSignal.Emit( handle );
}

void Button::OnButtonUp()
{
  if( DEPRESSED == mButtonPressedState )
  {
    if( mTogglableButton )
    {
      if ( TOGGLE_DEPRESSED != mButtonPressedState )
      {
        SetSelected( !IsSelected() );
        mButtonPressedState = UNPRESSED;
      }
    }
    else
    {
      Released();
      if( mAutoRepeating )
      {
        mAutoRepeatingTimer.Reset();
      }
    }

    // The clicked and released signals should be emitted regardless of toggle mode.
    Toolkit::Button handle( GetOwner() );
    mReleasedSignal.Emit( handle );
    mClickedSignal.Emit( handle );
  }
}

void Button::OnTouchPointLeave()
{
  if( DEPRESSED == mButtonPressedState )
  {
    if( !mTogglableButton )
    {
      Released();

      if( mAutoRepeating )
      {
        mAutoRepeatingTimer.Reset();
      }
    }

    mButtonPressedState = UNPRESSED;

    // The released signal should be emitted regardless of toggle mode.
    Toolkit::Button handle( GetOwner() );
    mReleasedSignal.Emit( handle );
  }
}

void Button::OnTouchPointInterrupted()
{
  OnTouchPointLeave();
}

Toolkit::Button::ButtonSignalType& Button::PressedSignal()
{
  return mPressedSignal;
}

Toolkit::Button::ButtonSignalType& Button::ReleasedSignal()
{
  return mReleasedSignal;
}

Toolkit::Button::ButtonSignalType& Button::ClickedSignal()
{
  return mClickedSignal;
}

Toolkit::Button::ButtonSignalType& Button::StateChangedSignal()
{
  return mStateChangedSignal;
}

bool Button::DoConnectSignal( BaseObject* object, ConnectionTrackerInterface* tracker, const std::string& signalName, FunctorDelegate* functor )
{
  Dali::BaseHandle handle( object );

  bool connected( true );
  Toolkit::Button button = Toolkit::Button::DownCast( handle );

  if( 0 == strcmp( signalName.c_str(), SIGNAL_PRESSED ) )
  {
    button.PressedSignal().Connect( tracker, functor );
  }
  else if( 0 == strcmp( signalName.c_str(), SIGNAL_RELEASED ) )
  {
    button.ReleasedSignal().Connect( tracker, functor );
  }
  else if( 0 == strcmp( signalName.c_str(), SIGNAL_CLICKED ) )
  {
    button.ClickedSignal().Connect( tracker, functor );
  }
  else if( 0 == strcmp( signalName.c_str(), SIGNAL_STATE_CHANGED ) )
  {
    button.StateChangedSignal().Connect( tracker, functor );
  }
  else
  {
    // signalName does not match any signal
    connected = false;
  }

  return connected;
}

void Button::OnInitialize()
{
  DALI_LOG_INFO( gLogButtonFilter, Debug::Verbose, "Button::OnInitialize\n" );

  Actor self = Self();

  mTapDetector = TapGestureDetector::New();
  mTapDetector.Attach( self );
  mTapDetector.DetectedSignal().Connect(this, &Button::OnTap);

  self.SetKeyboardFocusable( true );

  self.TouchSignal().Connect( this, &Button::OnTouch );
}

bool Button::OnAccessibilityActivated()
{
  return OnKeyboardEnter();
}

bool Button::OnTouch( Actor actor, const TouchData& touch )
{

  // Only events are processed when the button is not disabled and the touch event has only
  // one touch point.

  if( !IsDisabled() && ( 1 == touch.GetPointCount() ) )
  {
    switch( touch.GetState( 0 ) )
    {
      case PointState::DOWN:
      {
        OnButtonDown();
        break;
      }
      case PointState::UP:
      {
        OnButtonUp();
        break;
      }
      case PointState::INTERRUPTED:
      {
        OnTouchPointInterrupted();
        break;
      }
      case PointState::LEAVE:
      {
        OnTouchPointLeave();
        break;
      }
      case PointState::MOTION:
      case PointState::STATIONARY: // FALLTHROUGH
      {
        // Nothing to do
        break;
      }
    }
  }
  else if( 1 < touch.GetPointCount() )
  {
    OnTouchPointLeave(); // Notification for derived classes.

    // Sets the button state to the default
    mButtonPressedState = UNPRESSED;
  }

  return false;
}

bool Button::OnKeyboardEnter()
{
  // When the enter key is pressed, or button is activated, the click action is performed.
  Property::Map attributes;
  bool ret = DoClickAction( attributes );

  return ret;
}

void Button::OnStageDisconnection()
{
  if( DEPRESSED == mButtonPressedState )
  {
    if( !mTogglableButton )
    {
      Released();

      if( mAutoRepeating )
      {
        mAutoRepeatingTimer.Reset();
      }
    }
  }

  mButtonPressedState = UNPRESSED;

  Control::OnStageDisconnection(); // Visuals will be set off stage
}

void Button::OnStageConnection( int depth )
{
  DALI_LOG_INFO( gLogButtonFilter, Debug::Verbose, "Button::OnStageConnection ptr(%p) \n", this );
  PerformFunctionOnVisualsInState( &Button::SelectRequiredVisual, mButtonState );
  Control::OnStageConnection( depth ); // Enabled visuals will be put on stage

}

Vector3 Button::GetNaturalSize()
{
  Vector3 size = Vector3::ZERO;

  bool horizontalAlignment = mTextLabelAlignment == BEGIN || mTextLabelAlignment == END; // label and visual side by side

  // Get natural size of foreground ( largest of the possible visuals )
  Size largestForegroundVisual;
  Size labelSize;

  for ( int state = Button::UNSELECTED_STATE; state < Button::STATE_COUNT; state++)
  {
    Toolkit::Visual::Base visual = GetVisual( GET_VISUAL_INDEX_FOR_STATE[state][FOREGROUND] );
    Size visualSize;
    if ( visual )
    {
      visual.GetNaturalSize( visualSize );
      largestForegroundVisual.width = std::max(largestForegroundVisual.width, visualSize.width );
      largestForegroundVisual.height = std::max(largestForegroundVisual.height, visualSize.height );
    }
  }

  // Get horizontal padding total
  if ( largestForegroundVisual.width > 0 )  // if visual exists
  {
    size.width += largestForegroundVisual.width + mForegroundPadding.left + mForegroundPadding.right;
  }
  // Get vertical padding total
  if ( largestForegroundVisual.height > 0 )
  {
    size.height += largestForegroundVisual.height + mForegroundPadding.top + mForegroundPadding.bottom;
  }

  DALI_LOG_INFO( gLogButtonFilter, Debug::General, "GetNaturalSize visual Size(%f,%f)\n",
                 largestForegroundVisual.width, largestForegroundVisual.height );

  // Get natural size of label
  if ( mLabel )
  {
    labelSize = Vector2( mLabel.GetNaturalSize());

    DALI_LOG_INFO( gLogButtonFilter, Debug::General, "GetNaturalSize labelSize(%f,%f) padding(%f,%f)\n",
                   labelSize.width, labelSize.height, mLabelPadding.left + mLabelPadding.right, mLabelPadding.top + mLabelPadding.bottom);

    labelSize.width += mLabelPadding.left + mLabelPadding.right;
    labelSize.height += mLabelPadding.top + mLabelPadding.bottom;

    // Add label size to height or width depending on alignment position
    if ( horizontalAlignment )
    {
      size.width += labelSize.width;
      size.height = std::max(size.height, labelSize.height );
    }
    else
    {
      size.height += labelSize.height;
      size.width = std::max(size.width, labelSize.width );
    }
  }

  if( size.width < 1 && size.height < 1 )
  {
    // if no image or label then use Control's natural size
    DALI_LOG_INFO( gLogButtonFilter, Debug::General, "GetNaturalSize Using control natural size\n");
    size = Control::GetNaturalSize();
  }

  DALI_LOG_INFO( gLogButtonFilter, Debug::General, "Button GetNaturalSize (%f,%f)\n", size.width, size.height );

  return size;
}

void Button::OnSetResizePolicy( ResizePolicy::Type policy, Dimension::Type dimension )
{
  DALI_LOG_INFO( gLogButtonFilter, Debug::General, "OnSetResizePolicy\n");

  if ( policy != ResizePolicy::USE_NATURAL_SIZE || policy != ResizePolicy::FIT_TO_CHILDREN  )
  {
    if ( mLabel )
    {
      // todo Can't set Text Label to USE_ASSIGNED_SIZE as causes a relayout in it whilst doing a relayout = error
      //mLabel.SetResizePolicy(ResizePolicy::USE_ASSIGNED_SIZE, Dimension::ALL_DIMENSIONS );
    }
  }

  RelayoutRequest();
}

void Button::OnRelayout( const Vector2& size, RelayoutContainer& container )
{
  DALI_LOG_INFO( gLogButtonFilter, Debug::General, "OnRelayout targetSize(%f,%f) ptr(%p) state[%d]\n", size.width, size.height, this, mButtonState );

  PerformFunctionOnVisualsInState( &Button::SelectRequiredVisual, mButtonState );

  ResizePolicy::Type widthResizePolicy = Self().GetResizePolicy(  Dimension::WIDTH );
  ResizePolicy::Type heightResizePolicy = Self().GetResizePolicy(  Dimension::HEIGHT );

  DALI_LOG_INFO( gLogButtonFilter, Debug::General, "OnRelayout resize policy: width:%d height:%d\n", heightResizePolicy, widthResizePolicy);

  Toolkit::Visual::Base currentVisual = GetVisual( GET_VISUAL_INDEX_FOR_STATE[mButtonState][FOREGROUND] );

  Toolkit::Visual::Base currentBackGroundVisual = GetVisual( GET_VISUAL_INDEX_FOR_STATE[mButtonState][BACKGROUND] );

  // Sizes and padding set to zero, if not present then values will no effect calculations.
  Vector2 visualPosition = Vector2::ZERO;
  Vector2 labelPosition = Vector2::ZERO;
  Size visualSize = Size::ZERO;
  Padding foregroundVisualPadding = Padding(0.0f, 0.0f, 0.0f, 0.0f );
  Padding labelVisualPadding = Padding(0.0f, 0.0f, 0.0f, 0.0f );

  if ( mLabel )
  {
    DALI_LOG_INFO( gLogButtonFilter, Debug::General, "OnRelayout Label padding setting padding:%f,%f,%f,%f\n", mLabelPadding.y, mLabelPadding.x, mLabelPadding.width,mLabelPadding.height );
    labelVisualPadding = mLabelPadding;
  }

  if ( currentVisual )
  {
    DALI_LOG_INFO( gLogButtonFilter, Debug::General, "OnRelayout Foreground Visual setting padding:%f,%f,%f,%f\n", mForegroundPadding.y, mForegroundPadding.x, mForegroundPadding.width,mForegroundPadding.height );
    currentVisual.GetNaturalSize( visualSize );
    foregroundVisualPadding = mForegroundPadding;
  }

  Toolkit::Align::Type visualAnchorPoint = Toolkit::Align::TOP_BEGIN;

  Vector2 visualAndPaddingSize = Vector2( ( foregroundVisualPadding.x + visualSize.width + foregroundVisualPadding.y ),
                                          ( foregroundVisualPadding.width + visualSize.height + foregroundVisualPadding.height ));

  DALI_LOG_INFO( gLogButtonFilter, Debug::General, "OnRelayout visualAndPaddingSize(%f,%f)\n", visualAndPaddingSize.width, visualAndPaddingSize.height);

  switch ( mTextLabelAlignment )
  {
    case BEGIN :
    {
      visualAnchorPoint = Toolkit::Align::TOP_END;
      visualPosition.x = foregroundVisualPadding.right;
      visualPosition.y = foregroundVisualPadding.top;

      labelPosition.x = labelVisualPadding.x;
      labelPosition.y = labelVisualPadding.top;
      break;
    }
    case END :
    {
      visualAnchorPoint = Toolkit::Align::TOP_BEGIN;
      visualPosition.x = foregroundVisualPadding.left;
      visualPosition.y = foregroundVisualPadding.top;

      labelPosition.x = visualAndPaddingSize.width + labelVisualPadding.x;
      labelPosition.y = labelVisualPadding.top;
      break;
    }
    case TOP :
    {
      visualAnchorPoint = Toolkit::Align::BOTTOM_END;
      visualPosition.x = foregroundVisualPadding.left;
      visualPosition.y = foregroundVisualPadding.bottom;

      labelPosition.x = labelVisualPadding.left;
      labelPosition.y = labelVisualPadding.top;
      break;
    }
    case BOTTOM :
    {
      visualAnchorPoint = Toolkit::Align::TOP_END;
      visualPosition.x = foregroundVisualPadding.left;
      visualPosition.y = foregroundVisualPadding.top;

      labelPosition.x = labelVisualPadding.left;
      labelPosition.y = visualAndPaddingSize.height + labelVisualPadding.top;
      break;
    }
  }

  if ( currentBackGroundVisual )
  {
    DALI_LOG_INFO( gLogButtonFilter, Debug::General, "OnRelayout Setting visual bakcground size to(%f,%f)\n", size.width, size.height);

    Property::Map visualTransform;

    visualTransform.Add( Toolkit::DevelVisual::Transform::Property::SIZE, size )
                   .Add( Toolkit::DevelVisual::Transform::Property::OFFSET_SIZE_MODE, Vector4( 0.0f, 0.0f, 1.0f, 1.0f) );  // Use relative size

    currentBackGroundVisual.SetTransformAndSize( visualTransform, size );
  }

  if ( currentVisual )
  {
      DALI_LOG_INFO( gLogButtonFilter, Debug::General, "OnRelayout Setting visual size to(%f,%f)\n", visualSize.width, visualSize.height);

      currentVisual.SetProperty( Toolkit::Visual::DevelProperty::TRANSFORM,
                                 Dali::Property::Map()
                                 .Add( Toolkit::Visual::DevelProperty::Transform::Property::SIZE, visualSize )
                                 .Add( Toolkit::Visual::DevelProperty::Transform::Property::OFFSET, visualPosition )
                                 .Add( Toolkit::Visual::DevelProperty::Transform::Property::OFFSET_SIZE_MODE, Vector4(1.0f, 1.0f, 1.0f,1.0f) )
                                 .Add( Toolkit::Visual::DevelProperty::Transform::Property::ORIGIN, Toolkit::Align::TOP_BEGIN )
                                 .Add( Toolkit::Visual::DevelProperty::Transform::Property::ANCHOR_POINT, visualAnchorPoint )
                                );
  }

  if ( mLabel )
  {
    // When Text visual size can be set, determine the size here.
    // Text Visual should take all space available after foreground visual size and all padding is considered.
    // Remaining Space priority, Foreground padding, foreground visual, Text padding then Text visual.

    Size remainingSpaceForText = Size::ZERO;
    remainingSpaceForText.width = size.width - visualAndPaddingSize.width - labelVisualPadding.x - labelVisualPadding.y;
    remainingSpaceForText.height = size.height - visualAndPaddingSize.height - labelVisualPadding.width - labelVisualPadding.height;

    if ( !currentVisual )
    {
      DALI_LOG_INFO( gLogButtonFilter, Debug::General, "OnRelayout Only Text\n");

      // Center Text if no foreground visual
      Size labelNaturalSize = Vector2( mLabel.GetNaturalSize() );

      // A Text visual will take up all the remainingSpaceForText, for now TextLabel natural size needed for positioning.
      labelPosition.x = labelVisualPadding.left + remainingSpaceForText.width*0.5 - labelNaturalSize.width *0.5;
      labelPosition.y = labelVisualPadding.height + remainingSpaceForText.height*0.5 - labelNaturalSize.height *0.5;
    }

    DALI_LOG_INFO( gLogButtonFilter, Debug::General, "OnRelayout text Size(%f,%f) text Position(%f,%f) \n", remainingSpaceForText.width, remainingSpaceForText.height, labelPosition.x, labelPosition.y);

    mLabel.SetPosition( labelPosition.x, labelPosition.y );
    container.Add( mLabel, remainingSpaceForText ); // Currently a TextLabel is used and size can not be set here.
  }

  DALI_LOG_INFO( gLogButtonFilter, Debug::General, "OnRelayout << \n");
}

void Button::OnTap(Actor actor, const TapGesture& tap)
{
  DALI_LOG_INFO( gLogButtonFilter, Debug::General, "OnTap\n" );
}

void Button::SetUpTimer( float delay )
{
  mAutoRepeatingTimer = Dali::Timer::New( static_cast<unsigned int>( 1000.f * delay ) );
  mAutoRepeatingTimer.TickSignal().Connect( this, &Button::AutoRepeatingSlot );
  mAutoRepeatingTimer.Start();
}

bool Button::AutoRepeatingSlot()
{
  bool consumed = false;
  if( !IsDisabled() )
  {
    // Restart the autorepeat timer.
    SetUpTimer( mNextAutoRepeatingDelay );

    Pressed();

    Toolkit::Button handle( GetOwner() );

    //Emit signal.
    consumed = mReleasedSignal.Emit( handle );
    consumed = mClickedSignal.Emit( handle );
    consumed |= mPressedSignal.Emit( handle );
 }

  return consumed;
}

void Button::Pressed()
{
  DALI_LOG_INFO( gLogButtonFilter, Debug::Verbose, "Button::Pressed\n" );

  if( mButtonState == UNSELECTED_STATE )
  {
    ClearTransitionAnimation();
    ChangeState( SELECTED_STATE );
    OnPressed();  // Notifies the derived class the button has been pressed.
  }
}

void Button::Released()
{
  DALI_LOG_INFO( gLogButtonFilter, Debug::Verbose, "Button::Released\n" );

  if( mButtonState == SELECTED_STATE && !mTogglableButton )
  {
    ClearTransitionAnimation();
    ChangeState( UNSELECTED_STATE );
    OnReleased(); //    // Notifies the derived class the button has been released.
  }
  mButtonPressedState = UNPRESSED;
}

Button::PressState Button::GetPressedState()
{
  return mButtonPressedState;
}

Button::State Button::GetButtonState()
{
  return mButtonState;
}

void Button::SelectRequiredVisual( Property::Index visualIndex )
{
  DALI_LOG_INFO( gLogButtonFilter, Debug::Verbose, "Button::SelectRequiredVisual index(%d) state(%d)\n", visualIndex, mButtonState );

  EnableVisual( visualIndex, true );
}

void Button::TransitionButtonVisualOut( Property::Index visualIndex )
{
  DALI_LOG_INFO( gLogButtonFilter, Debug::Verbose, "Button::TransitionButtonVisualOut index(%d)\n", visualIndex);

  // PrepareForTranstionOut and OnTransitionOut needs to be called on visual instead of Actor once animating is possible
}

void Button::TransitionButtonVisualIn( Property::Index visualIndex )
{
  DALI_LOG_INFO( gLogButtonFilter, Debug::Verbose, "Button::TransitionButtonVisualIn index(%d)\n", visualIndex );
}

void Button::OnTransitionIn( Actor actor )
{
  PerformFunctionOnVisualsInState( &Button::OnButtonVisualRemoval, mPreviousButtonState ); // Derived button can override OnButtonVisualRemoval
}

void Button::RemoveVisual( Property::Index visualIndex )
{
  // Use OnButtonVisualRemoval if want button developer to have the option to override removal.
  DALI_LOG_INFO( gLogButtonFilter, Debug::Verbose, "Button::RemoveVisual index(%d) state(%d)\n", visualIndex, mButtonState );

  Toolkit::Visual::Base visual = GetVisual( visualIndex );

  if( visual )
  {
    EnableVisual( visualIndex, false );
  }
}

void Button::OnButtonVisualRemoval( Property::Index visualIndex )
{
  // Derived Buttons can over ride this to prevent default removal.
  DALI_LOG_INFO( gLogButtonFilter, Debug::Verbose, "Button::OnButtonVisualRemoval index(%d)\n", visualIndex );
  RemoveVisual( visualIndex );
}

void Button::StartTransitionAnimation()
{
  if( mTransitionAnimation )
  {
    DALI_LOG_INFO( gLogButtonFilter, Debug::Verbose, "Button::StartTransitionAnimation progress(%f) duration(%f) state(%d) \n",
                   mTransitionAnimation.GetCurrentProgress(), mTransitionAnimation.GetDuration(),
                   mTransitionAnimation.GetState());
    mTransitionAnimation.Play();
  }
}

void Button::ClearTransitionAnimation()
{
  if( mTransitionAnimation )
  {
    DALI_LOG_INFO( gLogButtonFilter, Debug::Verbose, "Button::ClearTransitionAnimation progress(%f) duration(%f) state(%d) \n",
                   mTransitionAnimation.GetCurrentProgress(), mTransitionAnimation.GetDuration(),
                   mTransitionAnimation.GetState());
    mTransitionAnimation.Clear();
    mTransitionAnimation.Reset();
  }
}

Dali::Animation Button::GetTransitionAnimation()
{
  if( !mTransitionAnimation )
  {
    mTransitionAnimation = Dali::Animation::New( GetAnimationTime() );
    mTransitionAnimation.FinishedSignal().Connect( this, &Button::TransitionAnimationFinished );
  }

  return mTransitionAnimation;
}

void Button::TransitionAnimationFinished( Dali::Animation& source )
{
  DALI_LOG_INFO( gLogButtonFilter, Debug::Verbose, "Button::TransitionAnimationFinished\n" );
  ClearTransitionAnimation();
  PerformFunctionOnVisualsInState( &Button::OnButtonVisualRemoval, mPreviousButtonState ); // Derived button can override OnButtonVisualRemoval
}

void Button::SetProperty( BaseObject* object, Property::Index index, const Property::Value& value )
{
  Toolkit::Button button = Toolkit::Button::DownCast( Dali::BaseHandle( object ) );

  DALI_LOG_INFO( gLogButtonFilter, Debug::Verbose, "Button::SetProperty index[%d]\n", index );

  if ( button )
  {
    switch ( index )
    {
      case Toolkit::Button::Property::DISABLED:
      {
        GetImplementation( button ).SetDisabled( value.Get< bool >() );
        break;
      }

      case Toolkit::Button::Property::AUTO_REPEATING:
      {
        GetImplementation( button ).SetAutoRepeating( value.Get< bool >() );
        break;
      }

      case Toolkit::Button::Property::INITIAL_AUTO_REPEATING_DELAY:
      {
        GetImplementation( button ).SetInitialAutoRepeatingDelay( value.Get< float >() );
        break;
      }

      case Toolkit::Button::Property::NEXT_AUTO_REPEATING_DELAY:
      {
        GetImplementation( button ).SetNextAutoRepeatingDelay( value.Get< float >() );
        break;
      }

      case Toolkit::Button::Property::TOGGLABLE:
      {
        GetImplementation( button ).SetTogglableButton( value.Get< bool >() );
        break;
      }

      case Toolkit::Button::Property::SELECTED:
      {
        GetImplementation( button ).SetSelected( value.Get< bool >() );
        break;
      }

      case Toolkit::Button::Property::UNSELECTED_STATE_IMAGE: // Legacy Tizen 3.0
      {
        GetImplementation( button ).CreateVisualsForComponent( Toolkit::Button::Property::UNSELECTED_VISUAL, value, DepthIndex::CONTENT );
        break;
      }
      case Toolkit::Button::Property::DISABLED_STATE_IMAGE:  // Legacy Tizen 3.0
      {
        GetImplementation( button ).CreateVisualsForComponent( Toolkit::Button::Property::DISABLED_UNSELECTED_VISUAL, value, DepthIndex::CONTENT );
        break;
      }
      case Toolkit::Button::Property::SELECTED_STATE_IMAGE:  // Legacy Tizen 3.0
      {
        GetImplementation( button ).CreateVisualsForComponent( Toolkit::Button::Property::SELECTED_VISUAL, value, DepthIndex::CONTENT );
        break;
      }
      case Toolkit::Button::Property::UNSELECTED_VISUAL:
      case Toolkit::Button::Property::SELECTED_VISUAL:
      case Toolkit::Button::Property::DISABLED_SELECTED_VISUAL:
      case Toolkit::Button::Property::DISABLED_UNSELECTED_VISUAL:
      {
        GetImplementation( button ).CreateVisualsForComponent( index, value, DepthIndex::CONTENT );
        break;
      }

      case Toolkit::Button::Property::UNSELECTED_BACKGROUND_VISUAL:
      case Toolkit::Button::Property::SELECTED_BACKGROUND_VISUAL:
      case Toolkit::Button::Property::DISABLED_SELECTED_BACKGROUND_VISUAL:
      case Toolkit::Button::Property::DISABLED_UNSELECTED_BACKGROUND_VISUAL:
      {
        GetImplementation( button ).CreateVisualsForComponent( index , value, DepthIndex::BACKGROUND);
        break;
      }

      case Toolkit::Button::Property::UNSELECTED_COLOR:
      {
        DALI_LOG_WARNING("[%s] Using deprecated Property Button::Property::UNSELECTED_COLOR instead use Button::Property::UNSELECTED_BACKGROUND_VISUAL\n", __FUNCTION__);
        GetImplementation( button ).SetColor( value.Get< Vector4 >(), Toolkit::Button::Property::UNSELECTED_BACKGROUND_VISUAL );
        break;
      }

      case Toolkit::Button::Property::SELECTED_COLOR:
      {
        DALI_LOG_WARNING("[%s] Using deprecated Property Button::Property::SELECTED_COLOR instead use Button::Property::SELECTED_BACKGROUND_VISUAL\n", __FUNCTION__);
        GetImplementation( button ).SetColor( value.Get< Vector4 >(), Toolkit::Button::Property::SELECTED_BACKGROUND_VISUAL );
        break;
      }

      case Toolkit::Button::Property::LABEL_TEXT:
      {
        DALI_LOG_WARNING("[%s] Using deprecated Property Button::Property::LABEL_TEXT instead use Button::Property::LABEL\n", __FUNCTION__);
        Property::Map labelTextProperty;
        labelTextProperty.Insert( "text", value.Get< std::string >() );
        GetImplementation( button ).SetupLabel( labelTextProperty );
        break;
      }

      case Toolkit::Button::Property::LABEL:
      {
        // Get a Property::Map from the property if possible.
        Property::Map setPropertyMap;
        if( value.Get( setPropertyMap ) )
        {
          GetImplementation( button ).SetupLabel( setPropertyMap );
        }
        break;
      }

      case Toolkit::Button::Property::LABEL_STRUT_LENGTH:
      {
        GetImplementation( button ).SetLabelStrutLength( value.Get< int >() );
        break;
      }

      case Toolkit::Button::Property::LABEL_RELATIVE_ALIGNMENT:
      {
        Button::Align labelAlignment(END);
        Scripting::GetEnumeration< Button::Align> ( value.Get< std::string >().c_str(),
                                                    ALIGNMENT_TABLE, ALIGNMENT_TABLE_COUNT,
                                                    labelAlignment );

        GetImplementation( button ).SetLabelAlignment( labelAlignment );
        break;
      }
    }
  }
}

Property::Value Button::GetProperty( BaseObject* object, Property::Index propertyIndex )
{
  Property::Value value;

  Toolkit::Button button = Toolkit::Button::DownCast( Dali::BaseHandle( object ) );

  if ( button )
  {
    switch ( propertyIndex )
    {
      case Toolkit::Button::Property::DISABLED:
      {
        value = GetImplementation( button ).IsDisabled();
        break;
      }

      case Toolkit::Button::Property::AUTO_REPEATING:
      {
        value = GetImplementation( button ).mAutoRepeating;
        break;
      }

      case Toolkit::Button::Property::INITIAL_AUTO_REPEATING_DELAY:
      {
        value = GetImplementation( button ).mInitialAutoRepeatingDelay;
        break;
      }

      case Toolkit::Button::Property::NEXT_AUTO_REPEATING_DELAY:
      {
        value = GetImplementation( button ).mNextAutoRepeatingDelay;
        break;
      }

      case Toolkit::Button::Property::TOGGLABLE:
      {
        value = GetImplementation( button ).mTogglableButton;
        break;
      }

      case Toolkit::Button::Property::SELECTED:
      {
        value = GetImplementation( button ).IsSelected();
        break;
      }

      case Toolkit::Button::Property::UNSELECTED_STATE_IMAGE:
      {
        value = GetImplementation( button ).GetUrlForImageVisual( Toolkit::Button::Property::UNSELECTED_VISUAL );
        break;
      }

      case Toolkit::Button::Property::SELECTED_STATE_IMAGE:
      {
        value = GetImplementation( button ).GetUrlForImageVisual( Toolkit::Button::Property::SELECTED_VISUAL );
        break;
      }

      case Toolkit::Button::Property::DISABLED_STATE_IMAGE:
      {
        value = GetImplementation( button ).GetUrlForImageVisual( Toolkit::Button::Property::DISABLED_UNSELECTED_VISUAL );
        break;
      }

      case Toolkit::Button::Property::UNSELECTED_COLOR:
      {
        value = GetImplementation( button ).GetUnselectedColor();
        break;
      }

      case Toolkit::Button::Property::SELECTED_COLOR:
      {
        value = GetImplementation( button ).GetSelectedColor();
        break;
      }

      case Toolkit::Button::Property::LABEL_TEXT:
      {
        value = GetImplementation( button ).GetLabelText();
        break;
      }

      case Toolkit::Button::Property::LABEL:
      {
        Property::Map emptyMap;
        value = emptyMap;
        break;
      }

      case Toolkit::Button::Property::LABEL_STRUT_LENGTH:
      {
        value = GetImplementation( button ).GetLabelStrutLength();
        break;
      }

      case Toolkit::Button::Property::LABEL_RELATIVE_ALIGNMENT:
      {
        const char* alignment = Scripting::GetEnumerationName< Button::Align >( GetImplementation( button ).GetLabelAlignment(),
                                                                                ALIGNMENT_STRING_TABLE,
                                                                                ALIGNMENT_STRING_TABLE_COUNT );
        if( alignment )
        {
          value = std::string( alignment );
        }

        break;
      }
    }
  }

  return value;
}

void Button::SetLabelPadding( const Padding& padding)
{
  DALI_LOG_INFO( gLogButtonFilter, Debug::Verbose, "Button::SetLabelPadding padding(%f,%f,%f,%f)\n", padding.left, padding.right, padding.bottom, padding.top );
  mLabelPadding = Padding( padding.left, padding.right, padding.bottom, padding.top );
  RelayoutRequest();
}

Padding Button::GetLabelPadding()
{
  return mLabelPadding;
}

void Button::SetForegroundPadding( const Padding& padding)
{
  DALI_LOG_INFO( gLogButtonFilter, Debug::Verbose, "Button::SetForegroundPadding padding(%f,%f,%f,%f)\n", padding.left, padding.right, padding.bottom, padding.top );
  mForegroundPadding = Padding( padding.left, padding.right, padding.bottom, padding.top );
  RelayoutRequest();
}

Padding Button::GetForegroundPadding()
{
  return mForegroundPadding;
}

////////////////////////////////////////////////////////////////////////
// Legacy functions from Tizen 2.4 and 3.0

void Button::SetLabel( Actor label )
{
  if ( label )
  {
    Property::Value value ="";
    value = label.GetProperty(Toolkit::TextLabel::Property::TEXT);

    SetLabelText( value.Get<std::string>() );
  }
}

void Button::SetUnselectedImage( const std::string& filename )
{
  if( !filename.empty() )
  {
    CreateVisualsForComponent( Toolkit::Button::Property::UNSELECTED_VISUAL, filename, DepthIndex::CONTENT );
  }
}

void Button::SetBackgroundImage( const std::string& filename )
{
  if( !filename.empty() )
  {
    CreateVisualsForComponent( Toolkit::Button::Property::UNSELECTED_BACKGROUND_VISUAL, filename, DepthIndex::BACKGROUND );
  }
}

void Button::SetSelectedImage( const std::string& filename )
{
  if( !filename.empty() )
  {
    CreateVisualsForComponent( Toolkit::Button::Property::SELECTED_VISUAL, filename, DepthIndex::CONTENT );
  }
}

void Button::SetSelectedBackgroundImage( const std::string& filename )
{
  if( !filename.empty() )
  {
    CreateVisualsForComponent( Toolkit::Button::Property::SELECTED_BACKGROUND_VISUAL, filename, DepthIndex::BACKGROUND );
  }
}

void Button::SetDisabledBackgroundImage( const std::string& filename )
{
  if( !filename.empty() )
  {
    CreateVisualsForComponent( Toolkit::Button::Property::DISABLED_UNSELECTED_BACKGROUND_VISUAL, filename, DepthIndex::BACKGROUND );
  }
}

void Button::SetDisabledImage( const std::string& filename )
{
  if( !filename.empty() )
  {
    CreateVisualsForComponent( Toolkit::Button::Property::DISABLED_UNSELECTED_VISUAL, filename, DepthIndex::CONTENT );
  }
}

void Button::SetDisabledSelectedImage( const std::string& filename )
{
  if( !filename.empty() )
  {
    CreateVisualsForComponent( Toolkit::Button::Property::DISABLED_SELECTED_VISUAL, filename, DepthIndex::CONTENT );
  }
}

std::string Button::GetUrlForImageVisual( Property::Index index )
{
  Toolkit::Visual::Base visual = GetVisual( index );
  std::string result;

  if ( visual )
  {
    Dali::Property::Map retreivedMap;
    visual.CreatePropertyMap( retreivedMap );
    Property::Value* value = retreivedMap.Find(  Toolkit::ImageVisual::Property::URL,  Property::STRING );
    if ( value )
    {
      result = value->Get<std::string>();
    }
  }

  return result;
}

// Below functions DEPRECATED_1_0.50 - Return empty Actors

Actor Button::GetButtonImage() const
{
  DALI_LOG_WARNING("Button::GetButtonImage @DEPRECATED_1_0.50 Returning empty Actor \n");
  return Actor();;
}

Actor Button::GetSelectedImage() const
{
  DALI_LOG_WARNING("Button::GetSelectedImage @DEPRECATED_1_0.50 Returning empty Actor \n");
  return Actor();
}

} // namespace Internal

} // namespace Toolkit

} // namespace Dali
