#ifndef DALI_TOOLKIT_ADAPTOR_IMPL_H
#define DALI_TOOLKIT_ADAPTOR_IMPL_H

/*
 * Copyright (c) 2019 Samsung Electronics Co., Ltd.
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

#include <dali/integration-api/adaptors/render-surface-interface.h>

namespace Dali
{
class EglInterface;
class DisplayConnection;
class ThreadSynchronizationInterface;
class Window;

namespace Integration
{
class Scene;
}

using WindowContainer = std::vector<Window>;

namespace Internal
{

namespace Adaptor
{

class GraphicsInterface;

} // namespace Adaptor

} // namespace Internal

namespace Integration
{

class GlAbstraction;

} // namespace Integration

namespace Internal
{
namespace Adaptor
{

class Adaptor
{
public:
  static Dali::Adaptor& New();
  static Dali::Adaptor& Get();
  Adaptor();
  ~Adaptor();

  void Start( Dali::Window window );

  bool AddIdle( CallbackBase* callback, bool hasReturnValue );
  void RemoveIdle( CallbackBase* callback );
  void RunIdles();

  static Integration::Scene GetScene( Dali::Window window );

  Dali::RenderSurfaceInterface& GetSurface();
  Dali::WindowContainer GetWindows();
  Dali::SceneHolderList GetSceneHolders();

  Dali::Adaptor::AdaptorSignalType& ResizedSignal();
  Dali::Adaptor::AdaptorSignalType& LanguageChangedSignal();
  Dali::Adaptor::WindowCreatedSignalType& WindowCreatedSignal();

  static Adaptor& GetImpl( Dali::Adaptor& adaptor ) { return *adaptor.mImpl; }
  static const Adaptor& GetImpl( const Dali::Adaptor& adaptor ) { return *adaptor.mImpl; }

private:

  Vector<CallbackBase*> mCallbacks;
  Dali::WindowContainer mWindows;
  Dali::SceneHolderList mSceneHolders;
  Dali::Adaptor::AdaptorSignalType mResizedSignal;
  Dali::Adaptor::AdaptorSignalType mLanguageChangedSignal;
  Dali::Adaptor::WindowCreatedSignalType mWindowCreatedSignal;
};

} // namespace Adaptor
} // namespace Internal
} // namespace Dali

#endif // DALI_TOOLKIT_ADAPTOR_IMPL_H
