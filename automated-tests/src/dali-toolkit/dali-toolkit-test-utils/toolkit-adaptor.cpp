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

#include <algorithm>

#include <toolkit-window-impl.h>

// Don't want to include the actual window.h which otherwise will be indirectly included by adaptor.h.
#define DALI_WINDOW_H
#include <dali/integration-api/adaptor-framework/adaptor.h>
#include <dali/integration-api/adaptor-framework/scene-holder.h>

#include <toolkit-scene-holder-impl.h>
#include <toolkit-adaptor-impl.h>
#include <dali/integration-api/debug.h>
#include <dali/integration-api/scene.h>
#include <test-application.h>
#include <toolkit-test-application.h>

namespace Dali
{

namespace Internal
{
namespace Adaptor
{

///////////////////////////////////////////////////////////////////////////////
//
// Dali::Internal::Adaptor::Adaptor Stub
//
///////////////////////////////////////////////////////////////////////////////

Dali::Adaptor* gAdaptor = nullptr;

Dali::Adaptor& Adaptor::New()
{
  DALI_ASSERT_ALWAYS( ! gAdaptor );
  gAdaptor = new Dali::Adaptor;
  return *gAdaptor;
}

Dali::Adaptor& Adaptor::Get()
{
  DALI_ASSERT_ALWAYS( gAdaptor );
  return *gAdaptor;
}

Adaptor::Adaptor()
{
}

Adaptor::~Adaptor()
{
  gAdaptor = nullptr;
}

void Adaptor::Start( Dali::Window window )
{
  AddWindow( &GetImplementation( window ) );
}

Integration::Scene Adaptor::GetScene( Dali::Window window )
{
  return window.GetScene();
}

bool Adaptor::AddIdle( CallbackBase* callback, bool hasReturnValue )
{
  mCallbacks.PushBack( callback );
  return true;
}

void Adaptor::RemoveIdle( CallbackBase* callback )
{
  mCallbacks.Erase( std::find_if( mCallbacks.Begin(), mCallbacks.End(),
                                  [ &callback ] ( CallbackBase* current ) { return callback == current; } ) );
}

void Adaptor::RunIdles()
{
  for( auto& callback : mCallbacks )
  {
    CallbackBase::Execute( *callback );
  }

  mCallbacks.Clear();
}

Dali::RenderSurfaceInterface& Adaptor::GetSurface()
{
  DALI_ASSERT_ALWAYS( ! mWindows.empty() );

  return reinterpret_cast < Dali::RenderSurfaceInterface& >( mWindows.front()->GetRenderSurface() );
}

Dali::WindowContainer Adaptor::GetWindows()
{
  Dali::WindowContainer windows;

  for ( auto iter = mWindows.begin(); iter != mWindows.end(); ++iter )
  {
    // Downcast to Dali::Window
    Dali::Window window( dynamic_cast<Dali::Internal::Adaptor::Window*>( *iter ) );
    if ( window )
    {
      windows.push_back( window );
    }
  }

  return windows;
}

Dali::SceneHolderList Adaptor::GetSceneHolders()
{
  Dali::SceneHolderList sceneHolderList;

  for( auto iter = mWindows.begin(); iter != mWindows.end(); ++iter )
  {
    sceneHolderList.push_back( Dali::Integration::SceneHolder( *iter ) );
  }

  return sceneHolderList;
}

Dali::Internal::Adaptor::SceneHolder* Adaptor::GetWindow( Dali::Actor& actor )
{
  Dali::Integration::Scene scene = Dali::Integration::Scene::Get( actor );

  for( auto window : mWindows )
  {
    if ( scene == window->GetScene() )
    {
      return window;
    }
  }

  return nullptr;
}

void Adaptor::AddWindow( Internal::Adaptor::SceneHolder* window )
{
  if ( window )
  {
    mWindows.push_back( window );

    Dali::Integration::SceneHolder newWindow( window );
    mWindowCreatedSignal.Emit( newWindow );
  }
}

void Adaptor::RemoveWindow( Internal::Adaptor::SceneHolder* window )
{
  auto iter = std::find( mWindows.begin(), mWindows.end(), window );
  if( iter != mWindows.end() )
  {
    mWindows.erase( iter );
  }
}

void Adaptor::RegisterProcessor( Integration::Processor& processor )
{
  Integration::Core& core = mTestApplication->GetCore();
  core.RegisterProcessor( processor );
}

void Adaptor::UnregisterProcessor( Integration::Processor& processor )
{
  Integration::Core& core = mTestApplication->GetCore();
  core.UnregisterProcessor( processor );
}

void Adaptor::SetApplication( Dali::TestApplication& testApplication )
{
  mTestApplication = &testApplication;
}

Dali::Adaptor::AdaptorSignalType& Adaptor::ResizedSignal()
{
  return mResizedSignal;
}

Dali::Adaptor::AdaptorSignalType& Adaptor::LanguageChangedSignal()
{
  return mLanguageChangedSignal;
}

Dali::Adaptor::WindowCreatedSignalType& Adaptor::WindowCreatedSignal()
{
  return mWindowCreatedSignal;
}

} // namespace Adaptor
} // namespace Internal

///////////////////////////////////////////////////////////////////////////////
//
// Dali::Adaptor Stub
//
///////////////////////////////////////////////////////////////////////////////

Adaptor::Adaptor()
: mImpl( new Internal::Adaptor::Adaptor )
{
}

Adaptor::~Adaptor()
{
  Internal::Adaptor::gAdaptor = nullptr;
  delete mImpl;
}

void Adaptor::Start()
{
}

void Adaptor::Pause()
{
}

void Adaptor::Resume()
{
}

void Adaptor::Stop()
{
}

bool Adaptor::AddIdle( CallbackBase* callback, bool hasReturnValue )
{
  return mImpl->AddIdle( callback, hasReturnValue );
}

void Adaptor::RemoveIdle( CallbackBase* callback )
{
  mImpl->RemoveIdle( callback );
}

void Adaptor::ReplaceSurface( Window window, Dali::RenderSurfaceInterface& surface )
{
}

void Adaptor::ReplaceSurface( Dali::Integration::SceneHolder window, Dali::RenderSurfaceInterface& surface )
{
}

Adaptor::AdaptorSignalType& Adaptor::ResizedSignal()
{
  return mImpl->ResizedSignal();
}

Adaptor::AdaptorSignalType& Adaptor::LanguageChangedSignal()
{
  return mImpl->LanguageChangedSignal();
}

Adaptor::WindowCreatedSignalType& Adaptor::WindowCreatedSignal()
{
  return mImpl->WindowCreatedSignal();
}

Dali::RenderSurfaceInterface& Adaptor::GetSurface()
{
  return mImpl->GetSurface();
}

Dali::WindowContainer Adaptor::GetWindows() const
{
  return mImpl->GetWindows();
}

Dali::SceneHolderList Adaptor::GetSceneHolders() const
{
  return mImpl->GetSceneHolders();
}

Any Adaptor::GetNativeWindowHandle()
{
  Any window;
  return window;
}

Any Adaptor::GetNativeWindowHandle( Actor actor )
{
  return GetNativeWindowHandle();
}

void Adaptor::ReleaseSurfaceLock()
{
}

void Adaptor::SetRenderRefreshRate( unsigned int numberOfVSyncsPerRender )
{
}

Adaptor& Adaptor::Get()
{
  return Internal::Adaptor::Adaptor::Get();
}

bool Adaptor::IsAvailable()
{
  return Internal::Adaptor::gAdaptor;
}

void Adaptor::NotifySceneCreated()
{
}

void Adaptor::NotifyLanguageChanged()
{
}

void Adaptor::FeedTouchPoint( TouchPoint& point, int timeStamp )
{
}

void Adaptor::FeedWheelEvent( WheelEvent& wheelEvent )
{
}

void Adaptor::FeedKeyEvent( KeyEvent& keyEvent )
{
}

void Adaptor::SceneCreated()
{
}

class LogFactory : public LogFactoryInterface
{
public:
  virtual void InstallLogFunction() const
  {
    Dali::Integration::Log::LogFunction logFunction(&ToolkitTestApplication::LogMessage);
    Dali::Integration::Log::InstallLogFunction(logFunction);
  }

  LogFactory()
  {
  }
  virtual ~LogFactory()
  {
  }
};

LogFactory* gLogFactory = NULL;
const LogFactoryInterface& Adaptor::GetLogFactory()
{
  if( gLogFactory == NULL )
  {
    gLogFactory = new LogFactory;
  }
  return *gLogFactory;
}

void Adaptor::RegisterProcessor( Integration::Processor& processor )
{
  mImpl->RegisterProcessor( processor );
}

void Adaptor::UnregisterProcessor( Integration::Processor& processor )
{
  mImpl->UnregisterProcessor( processor );
}

} // namespace Dali
