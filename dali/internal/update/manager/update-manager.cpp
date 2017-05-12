/*
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
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
#include <dali/internal/update/manager/update-manager.h>

// INTERNAL INCLUDES
#include <dali/public-api/common/stage.h>
#include <dali/devel-api/common/set-wrapper.h>
#include <dali/devel-api/common/owner-container.h>
#include <dali/devel-api/threading/mutex.h>

#include <dali/integration-api/core.h>
#include <dali/integration-api/render-controller.h>
#include <dali/internal/common/shader-data.h>
#include <dali/integration-api/debug.h>

#include <dali/internal/common/core-impl.h>
#include <dali/internal/common/message.h>

#include <dali/internal/event/common/notification-manager.h>
#include <dali/internal/event/common/property-notification-impl.h>
#include <dali/internal/event/common/property-notifier.h>
#include <dali/internal/event/effects/shader-factory.h>

#include <dali/internal/update/animation/scene-graph-animator.h>
#include <dali/internal/update/animation/scene-graph-animation.h>
#include <dali/internal/update/common/discard-queue.h>
#include <dali/internal/update/common/scene-graph-buffers.h>
#include <dali/internal/update/controllers/render-message-dispatcher.h>
#include <dali/internal/update/controllers/scene-controller-impl.h>
#include <dali/internal/update/gestures/scene-graph-pan-gesture.h>
#include <dali/internal/update/manager/render-task-processor.h>
#include <dali/internal/update/manager/sorted-layers.h>
#include <dali/internal/update/manager/update-algorithms.h>
#include <dali/internal/update/manager/update-manager-debug.h>
#include <dali/internal/update/manager/transform-manager.h>
#include <dali/internal/update/nodes/node.h>
#include <dali/internal/update/nodes/scene-graph-layer.h>
#include <dali/internal/update/queue/update-message-queue.h>
#include <dali/internal/update/render-tasks/scene-graph-render-task.h>
#include <dali/internal/update/render-tasks/scene-graph-render-task-list.h>
#include <dali/internal/update/render-tasks/scene-graph-camera.h>

#include <dali/internal/render/common/render-instruction-container.h>
#include <dali/internal/render/common/render-manager.h>
#include <dali/internal/render/queue/render-queue.h>
#include <dali/internal/render/shaders/scene-graph-shader.h>

// Un-comment to enable node tree debug logging
//#define NODE_TREE_LOGGING 1

#if ( defined( DEBUG_ENABLED ) && defined( NODE_TREE_LOGGING ) )
#define SNAPSHOT_NODE_LOGGING \
const int FRAME_COUNT_TRIGGER = 16;\
if( mImpl->frameCounter >= FRAME_COUNT_TRIGGER )\
  {\
    if ( NULL != mImpl->root )\
    {\
      mImpl->frameCounter = 0;\
      PrintNodeTree( *mImpl->root, mSceneGraphBuffers.GetUpdateBufferIndex(), "" );\
    }\
  }\
mImpl->frameCounter++;
#else
#define SNAPSHOT_NODE_LOGGING
#endif

#if defined(DEBUG_ENABLED)
extern Debug::Filter* gRenderTaskLogFilter;
#endif


using namespace Dali::Integration;
using Dali::Internal::Update::MessageQueue;

namespace Dali
{

namespace Internal
{

namespace SceneGraph
{

namespace
{
/**
 * Helper to reset animate-able objects to base values
 * @param container to iterate over
 * @param updateBufferIndex to use
 */
template< class T >
inline void ResetToBaseValues( OwnerContainer<T*>& container, BufferIndex updateBufferIndex )
{
  // Reset animatable properties to base values
  typename OwnerContainer<T*>::Iterator iter = container.Begin();
  const typename OwnerContainer<T*>::ConstIterator endIter = container.End();
  for ( ; iter != endIter; ++iter )
  {
    (*iter)->ResetToBaseValues( updateBufferIndex );
  }
}

/**
 * Helper to Erase an object from OwnerContainer using discard queue
 * @param container to remove from
 * @param object to remove
 * @param discardQueue to put the object to
 * @param updateBufferIndex to use
 */
template < class T >
inline void EraseUsingDiscardQueue( OwnerContainer<T*>& container, T* object, DiscardQueue& discardQueue, BufferIndex updateBufferIndex )
{
  typename OwnerContainer<T*>::Iterator iter = container.Begin();
  const typename OwnerContainer<T*>::ConstIterator endIter = container.End();
  for ( ; iter != endIter; ++iter )
  {
    if ( *iter == object )
    {
      // Transfer ownership to the discard queue, this keeps the object alive, until the render-thread has finished with it
      discardQueue.Add( updateBufferIndex, container.Release( iter ) );
      return;
    }
  }
}

}

typedef OwnerContainer< Shader* >              ShaderOwner;
typedef ShaderOwner::Iterator                  ShaderIter;
typedef std::vector<Internal::ShaderDataPtr>   ShaderDataBinaryQueue;

typedef OwnerContainer< TextureSet* >          TextureSetOwner;
typedef TextureSetOwner::Iterator              TextureSetIter;

typedef OwnerContainer<Renderer*>              RendererOwner;
typedef RendererOwner::Iterator                RendererIter;

typedef OwnerContainer< Camera* >              CameraOwner;
typedef OwnerContainer< PropertyOwner* >       CustomObjectOwner;

/**
 * Structure to contain UpdateManager internal data
 */
struct UpdateManager::Impl
{
  Impl( NotificationManager& notificationManager,
        CompleteNotificationInterface& animationFinishedNotifier,
        PropertyNotifier& propertyNotifier,
        DiscardQueue& discardQueue,
        RenderController& renderController,
        RenderManager& renderManager,
        RenderQueue& renderQueue,
        SceneGraphBuffers& sceneGraphBuffers,
        RenderTaskProcessor& renderTaskProcessor )
  : renderMessageDispatcher( renderManager, renderQueue, sceneGraphBuffers ),
    notificationManager( notificationManager ),
    transformManager(),
    animationFinishedNotifier( animationFinishedNotifier ),
    propertyNotifier( propertyNotifier ),
    shaderSaver( NULL ),
    discardQueue( discardQueue ),
    renderController( renderController ),
    sceneController( NULL ),
    renderManager( renderManager ),
    renderQueue( renderQueue ),
    renderInstructions( renderManager.GetRenderInstructionContainer() ),
    renderTaskProcessor( renderTaskProcessor ),
    backgroundColor( Dali::Stage::DEFAULT_BACKGROUND_COLOR ),
    taskList( renderMessageDispatcher ),
    systemLevelTaskList( renderMessageDispatcher ),
    root( NULL ),
    systemLevelRoot( NULL ),
    renderers(),
    textureSets(),
    shaders(),
    panGestureProcessor( NULL ),
    messageQueue( renderController, sceneGraphBuffers ),
    keepRenderingSeconds( 0.0f ),
    nodeDirtyFlags( TransformFlag ), // set to TransformFlag to ensure full update the first time through Update()
    frameCounter( 0 ),
    animationFinishedDuringUpdate( false ),
    previousUpdateScene( false ),
    renderTaskWaiting( false )
  {
    sceneController = new SceneControllerImpl( renderMessageDispatcher, renderQueue, discardQueue );

    // create first 'dummy' node
    nodes.PushBack(0u);
  }

  ~Impl()
  {
    // Disconnect render tasks from nodes, before destroying the nodes
    RenderTaskList::RenderTaskContainer& tasks = taskList.GetTasks();
    for (RenderTaskList::RenderTaskContainer::Iterator iter = tasks.Begin(); iter != tasks.End(); ++iter)
    {
      (*iter)->SetSourceNode( NULL );
    }
    // ..repeat for system level RenderTasks
    RenderTaskList::RenderTaskContainer& systemLevelTasks = systemLevelTaskList.GetTasks();
    for (RenderTaskList::RenderTaskContainer::Iterator iter = systemLevelTasks.Begin(); iter != systemLevelTasks.End(); ++iter)
    {
      (*iter)->SetSourceNode( NULL );
    }

    // UpdateManager owns the Nodes
    Vector<Node*>::Iterator iter = nodes.Begin()+1;
    Vector<Node*>::Iterator endIter = nodes.End();
    for(;iter!=endIter;++iter)
    {
      (*iter)->OnDestroy();
      Node::Delete(*iter);
    }

    // If there is root, reset it, otherwise do nothing as rendering was never started
    if( root )
    {
      root->OnDestroy();

      Node::Delete( root );
      root = NULL;
    }

    if( systemLevelRoot )
    {
      systemLevelRoot->OnDestroy();

      Node::Delete( systemLevelRoot );
      systemLevelRoot = NULL;
    }

    delete sceneController;
  }

  SceneGraphBuffers                   sceneGraphBuffers;             ///< Used to keep track of which buffers are being written or read
  RenderMessageDispatcher             renderMessageDispatcher;       ///< Used for passing messages to the render-thread
  NotificationManager&                notificationManager;           ///< Queues notification messages for the event-thread.
  TransformManager                    transformManager;              ///< Used to update the transformation matrices of the nodes
  CompleteNotificationInterface&      animationFinishedNotifier;     ///< Provides notification to applications when animations are finished.
  PropertyNotifier&                   propertyNotifier;              ///< Provides notification to applications when properties are modified.
  ShaderSaver*                        shaderSaver;                   ///< Saves shader binaries.
  DiscardQueue&                       discardQueue;                  ///< Nodes are added here when disconnected from the scene-graph.
  RenderController&                   renderController;              ///< render controller
  SceneControllerImpl*                sceneController;               ///< scene controller
  RenderManager&                      renderManager;                 ///< This is responsible for rendering the results of each "update"
  RenderQueue&                        renderQueue;                   ///< Used to queue messages for the next render
  RenderInstructionContainer&         renderInstructions;            ///< Used to prepare the render instructions
  RenderTaskProcessor&                renderTaskProcessor;           ///< Handles RenderTasks and RenderInstrucitons

  Vector4                             backgroundColor;               ///< The glClear color used at the beginning of each frame.

  RenderTaskList                      taskList;                      ///< The list of scene graph render-tasks
  RenderTaskList                      systemLevelTaskList;           ///< Separate render-tasks for system-level content

  Layer*                              root;                          ///< The root node (root is a layer)
  Layer*                              systemLevelRoot;               ///< A separate root-node for system-level content

  Vector<Node*>                       nodes;                         ///< A container of all instantiated nodes

  SortedLayerPointers                 sortedLayers;                  ///< A container of Layer pointers sorted by depth
  SortedLayerPointers                 systemLevelSortedLayers;       ///< A separate container of system-level Layers

  CameraOwner                         cameras;                       ///< A container of cameras
  CustomObjectOwner                   customObjects;                 ///< A container of owned objects (with custom properties)

  AnimationContainer                  animations;                    ///< A container of owned animations
  PropertyNotificationContainer       propertyNotifications;         ///< A container of owner property notifications.

  RendererOwner                       renderers;                     ///< A container of owned renderers
  TextureSetOwner                     textureSets;                   ///< A container of owned texture sets
  ShaderOwner                         shaders;                       ///< A container of owned shaders
  OwnerPointer<PanGesture>            panGestureProcessor;           ///< Owned pan gesture processor; it lives for the lifecycle of UpdateManager

  MessageQueue                        messageQueue;                  ///< The messages queued from the event-thread
  ShaderDataBinaryQueue               renderCompiledShaders;         ///< Shaders compiled on Render thread are inserted here for update thread to pass on to event thread.
  ShaderDataBinaryQueue               updateCompiledShaders;         ///< Shaders to be sent from Update to Event
  Mutex                               compiledShaderMutex;           ///< lock to ensure no corruption on the renderCompiledShaders

  float                               keepRenderingSeconds;          ///< Set via Dali::Stage::KeepRendering
  int                                 nodeDirtyFlags;                ///< cumulative node dirty flags from previous frame
  int                                 frameCounter;                  ///< Frame counter used in debugging to choose which frame to debug and which to ignore.

  bool                                animationFinishedDuringUpdate; ///< Flag whether any animations finished during the Update()
  bool                                previousUpdateScene;           ///< True if the scene was updated in the previous frame (otherwise it was optimized out)
  bool                                renderTaskWaiting;             ///< A REFRESH_ONCE render task is waiting to be rendered

private:

  Impl( const Impl& ); ///< Undefined
  Impl& operator=( const Impl& ); ///< Undefined
};

UpdateManager::UpdateManager( NotificationManager& notificationManager,
                              CompleteNotificationInterface& animationFinishedNotifier,
                              PropertyNotifier& propertyNotifier,
                              DiscardQueue& discardQueue,
                              RenderController& controller,
                              RenderManager& renderManager,
                              RenderQueue& renderQueue,
                              RenderTaskProcessor& renderTaskProcessor )
  : mImpl(NULL)
{
  mImpl = new Impl( notificationManager,
                    animationFinishedNotifier,
                    propertyNotifier,
                    discardQueue,
                    controller,
                    renderManager,
                    renderQueue,
                    mSceneGraphBuffers,
                    renderTaskProcessor );

}

UpdateManager::~UpdateManager()
{
  delete mImpl;
}

void UpdateManager::InstallRoot( SceneGraph::Layer* layer, bool systemLevel )
{
  DALI_ASSERT_DEBUG( layer->IsLayer() );
  DALI_ASSERT_DEBUG( layer->GetParent() == NULL);

  if ( !systemLevel )
  {
    DALI_ASSERT_DEBUG( mImpl->root == NULL && "Root Node already installed" );
    mImpl->root = layer;
    mImpl->root->CreateTransform( &mImpl->transformManager );
  }
  else
  {
    DALI_ASSERT_DEBUG( mImpl->systemLevelRoot == NULL && "System-level Root Node already installed" );
    mImpl->systemLevelRoot = layer;
    mImpl->systemLevelRoot->CreateTransform( &mImpl->transformManager );
  }

  layer->SetRoot(true);
}

void UpdateManager::AddNode( Node* node )
{
  DALI_ASSERT_ALWAYS( NULL != node );
  DALI_ASSERT_ALWAYS( NULL == node->GetParent() ); // Should not have a parent yet

  // Nodes must be sorted by pointer
  Vector<Node*>::Iterator begin = mImpl->nodes.Begin();
  for(Vector<Node*>::Iterator iter = mImpl->nodes.End()-1; iter >= begin; --iter)
  {
    if(node > (*iter))
    {
      mImpl->nodes.Insert((iter+1), node);
      node->CreateTransform( &mImpl->transformManager );
      break;
    }
  }
}

void UpdateManager::ConnectNode( Node* parent, Node* node )
{
  DALI_ASSERT_ALWAYS( NULL != parent );
  DALI_ASSERT_ALWAYS( NULL != node );
  DALI_ASSERT_ALWAYS( NULL == node->GetParent() ); // Should not have a parent yet

  parent->ConnectChild( node );
}

void UpdateManager::DisconnectNode( Node* node )
{
  Node* parent = node->GetParent();
  DALI_ASSERT_ALWAYS( NULL != parent );
  parent->SetDirtyFlag( ChildDeletedFlag ); // make parent dirty so that render items dont get reused

  parent->DisconnectChild( mSceneGraphBuffers.GetUpdateBufferIndex(), *node );
}

void UpdateManager::DestroyNode( Node* node )
{
  DALI_ASSERT_ALWAYS( NULL != node );
  DALI_ASSERT_ALWAYS( NULL == node->GetParent() ); // Should have been disconnected

  Vector<Node*>::Iterator iter = mImpl->nodes.Begin()+1;
  Vector<Node*>::Iterator endIter = mImpl->nodes.End();
  for(;iter!=endIter;++iter)
  {
    if((*iter) == node)
    {
      mImpl->nodes.Erase(iter);
      break;
    }
  }

  mImpl->discardQueue.Add( mSceneGraphBuffers.GetUpdateBufferIndex(), node );

  // Notify the Node about impending destruction
  node->OnDestroy();
}

void UpdateManager::AddCamera( Camera* camera )
{
  DALI_ASSERT_DEBUG( camera != NULL );

  mImpl->cameras.PushBack( camera ); // takes ownership
}

void UpdateManager::RemoveCamera( const Camera* camera )
{
  // Find the camera and destroy it
  EraseUsingDiscardQueue( mImpl->cameras, const_cast<Camera*>( camera ), mImpl->discardQueue, mSceneGraphBuffers.GetUpdateBufferIndex() );
}

void UpdateManager::AddObject( PropertyOwner* object )
{
  DALI_ASSERT_DEBUG( NULL != object );

  mImpl->customObjects.PushBack( object );
}

void UpdateManager::RemoveObject( PropertyOwner* object )
{
  DALI_ASSERT_DEBUG( NULL != object );

  OwnerContainer< PropertyOwner* >& customObjects = mImpl->customObjects;

  // Find the object and destroy it
  for ( OwnerContainer< PropertyOwner* >::Iterator iter = customObjects.Begin(); iter != customObjects.End(); ++iter )
  {
    PropertyOwner* current = *iter;
    if ( current == object )
    {
      customObjects.Erase( iter );
      return;
    }
  }

  // Should not reach here
  DALI_ASSERT_DEBUG(false);
}

void UpdateManager::AddAnimation( Animation* animation )
{
  mImpl->animations.PushBack( animation );
}

void UpdateManager::StopAnimation( Animation* animation )
{
  DALI_ASSERT_DEBUG( animation && "NULL animation called to stop" );

  bool animationFinished = animation->Stop( mSceneGraphBuffers.GetUpdateBufferIndex() );

  mImpl->animationFinishedDuringUpdate = mImpl->animationFinishedDuringUpdate || animationFinished;
}

void UpdateManager::RemoveAnimation( Animation* animation )
{
  DALI_ASSERT_DEBUG( animation && "NULL animation called to remove" );

  animation->OnDestroy( mSceneGraphBuffers.GetUpdateBufferIndex() );

  DALI_ASSERT_DEBUG( animation->GetState() == Animation::Destroyed );
}

bool UpdateManager::IsAnimationRunning() const
{
  bool isRunning(false);
  AnimationContainer& animations = mImpl->animations;

  // Find any animation that isn't stopped or paused

  const AnimationIter endIter = animations.End();
  for ( AnimationIter iter = animations.Begin(); !isRunning && iter != endIter; ++iter )
  {
    const Animation::State state = (*iter)->GetState();

    if (state != Animation::Stopped &&
        state != Animation::Paused)
    {
      isRunning = true;
    }
  }

  return isRunning;
}

void UpdateManager::AddPropertyNotification( PropertyNotification* propertyNotification )
{
  mImpl->propertyNotifications.PushBack( propertyNotification );
}

void UpdateManager::RemovePropertyNotification( PropertyNotification* propertyNotification )
{
  PropertyNotificationContainer &propertyNotifications = mImpl->propertyNotifications;
  PropertyNotificationIter iter = propertyNotifications.Begin();

  while ( iter != propertyNotifications.End() )
  {
    if( *iter == propertyNotification )
    {
      propertyNotifications.Erase(iter);
      break;
    }
    ++iter;
  }
}

void UpdateManager::PropertyNotificationSetNotify( PropertyNotification* propertyNotification, PropertyNotification::NotifyMode notifyMode )
{
  DALI_ASSERT_DEBUG( propertyNotification && "propertyNotification scene graph object missing" );
  propertyNotification->SetNotifyMode( notifyMode );
}

void UpdateManager::AddShader( Shader* shader )
{
  DALI_ASSERT_DEBUG( NULL != shader );

  mImpl->shaders.PushBack( shader );
}

void UpdateManager::RemoveShader( Shader* shader )
{
  DALI_ASSERT_DEBUG(shader != NULL);

  // Find the shader and destroy it
  EraseUsingDiscardQueue( mImpl->shaders, shader, mImpl->discardQueue, mSceneGraphBuffers.GetUpdateBufferIndex() );
}

void UpdateManager::SetShaderProgram( Shader* shader,
                                      Internal::ShaderDataPtr shaderData, bool modifiesGeometry )
{
  if( shaderData )
  {

    typedef MessageValue3< Shader, Internal::ShaderDataPtr, ProgramCache*, bool> DerivedType;

    // Reserve some memory inside the render queue
    unsigned int* slot = mImpl->renderQueue.ReserveMessageSlot( mSceneGraphBuffers.GetUpdateBufferIndex(), sizeof( DerivedType ) );

    // Construct message in the render queue memory; note that delete should not be called on the return value
    new (slot) DerivedType( shader, &Shader::SetProgram, shaderData, mImpl->renderManager.GetProgramCache(), modifiesGeometry );
  }
}

void UpdateManager::SaveBinary( Internal::ShaderDataPtr shaderData )
{
  DALI_ASSERT_DEBUG( shaderData && "No NULL shader data pointers please." );
  DALI_ASSERT_DEBUG( shaderData->GetBufferSize() > 0 && "Shader binary empty so nothing to save." );
  {
    // lock as update might be sending previously compiled shaders to event thread
    Mutex::ScopedLock lock( mImpl->compiledShaderMutex );
    mImpl->renderCompiledShaders.push_back( shaderData );
  }
}

void UpdateManager::SetShaderSaver( ShaderSaver& upstream )
{
  mImpl->shaderSaver = &upstream;
}

void UpdateManager::AddRenderer( Renderer* renderer )
{
  DALI_ASSERT_DEBUG( renderer != NULL );

  mImpl->renderers.PushBack( renderer );

  renderer->ConnectToSceneGraph( *mImpl->sceneController, mSceneGraphBuffers.GetUpdateBufferIndex() );
}

void UpdateManager::RemoveRenderer( Renderer* renderer )
{
  DALI_ASSERT_DEBUG( renderer != NULL );

  renderer->DisconnectFromSceneGraph( *mImpl->sceneController, mSceneGraphBuffers.GetUpdateBufferIndex() );

  // Find the renderer and destroy it
  EraseUsingDiscardQueue( mImpl->renderers, renderer, mImpl->discardQueue, mSceneGraphBuffers.GetUpdateBufferIndex() );
}

void UpdateManager::SetPanGestureProcessor( PanGesture* panGestureProcessor )
{
  DALI_ASSERT_DEBUG( NULL != panGestureProcessor );

  mImpl->panGestureProcessor = panGestureProcessor;
}

void UpdateManager::AddTextureSet( TextureSet* textureSet )
{
  DALI_ASSERT_DEBUG( NULL != textureSet );
  mImpl->textureSets.PushBack( textureSet );
}

void UpdateManager::RemoveTextureSet( TextureSet* textureSet )
{
  DALI_ASSERT_DEBUG( textureSet != NULL );

  // Find the texture and destroy it
  TextureSetOwner& textures = mImpl->textureSets;
  for ( TextureSetIter iter = textures.Begin(), endIter = textures.End(); iter != endIter; ++iter )
  {
    if ( *iter == textureSet )
    {
      textures.Erase( iter );
      return;
    }
  }
}

RenderTaskList* UpdateManager::GetRenderTaskList( bool systemLevel )
{
  if ( !systemLevel )
  {
    // copy the list, this is only likely to happen once in application life cycle
    return &(mImpl->taskList);
  }
  else
  {
    // copy the list, this is only likely to happen once in application life cycle
    return &(mImpl->systemLevelTaskList);
  }
}

unsigned int* UpdateManager::ReserveMessageSlot( std::size_t size, bool updateScene )
{
  return mImpl->messageQueue.ReserveMessageSlot( size, updateScene );
}

void UpdateManager::EventProcessingStarted()
{
  mImpl->messageQueue.EventProcessingStarted();
}

bool UpdateManager::FlushQueue()
{
  return mImpl->messageQueue.FlushQueue();
}

void UpdateManager::ResetProperties( BufferIndex bufferIndex )
{
  // Clear the "animations finished" flag; This should be set if any (previously playing) animation is stopped
  mImpl->animationFinishedDuringUpdate = false;

  // Animated properties have to be reset to their original value each frame

  // Reset root properties
  if ( mImpl->root )
  {
    mImpl->root->ResetToBaseValues( bufferIndex );
  }
  if ( mImpl->systemLevelRoot )
  {
    mImpl->systemLevelRoot->ResetToBaseValues( bufferIndex );
  }

  // Reset all the nodes
  Vector<Node*>::Iterator iter = mImpl->nodes.Begin()+1;
  Vector<Node*>::Iterator endIter = mImpl->nodes.End();
  for( ;iter != endIter; ++iter )
  {
    (*iter)->ResetToBaseValues( bufferIndex );
  }

  // Reset system-level render-task list properties to base values
  ResetToBaseValues( mImpl->systemLevelTaskList.GetTasks(), bufferIndex );

  // Reset render-task list properties to base values.
  ResetToBaseValues( mImpl->taskList.GetTasks(), bufferIndex );

  // Reset custom object properties to base values
  ResetToBaseValues( mImpl->customObjects, bufferIndex );

  // Reset animatable renderer properties to base values
  ResetToBaseValues( mImpl->renderers, bufferIndex );

  // Reset animatable shader properties to base values
  ResetToBaseValues( mImpl->shaders, bufferIndex );
}

bool UpdateManager::ProcessGestures( BufferIndex bufferIndex, unsigned int lastVSyncTimeMilliseconds, unsigned int nextVSyncTimeMilliseconds )
{
  bool gestureUpdated( false );

  if( mImpl->panGestureProcessor )
  {
    // gesture processor only supports default properties
    mImpl->panGestureProcessor->ResetDefaultProperties( bufferIndex ); // Needs to be done every time as gesture data is written directly to an update-buffer rather than via a message
    gestureUpdated |= mImpl->panGestureProcessor->UpdateProperties( lastVSyncTimeMilliseconds, nextVSyncTimeMilliseconds );
  }

  return gestureUpdated;
}

void UpdateManager::Animate( BufferIndex bufferIndex, float elapsedSeconds )
{
  AnimationContainer &animations = mImpl->animations;
  AnimationIter iter = animations.Begin();
  bool animationLooped = false;
  while ( iter != animations.End() )
  {
    Animation* animation = *iter;
    bool finished = false;
    bool looped = false;
    animation->Update( bufferIndex, elapsedSeconds, looped, finished );

    mImpl->animationFinishedDuringUpdate = mImpl->animationFinishedDuringUpdate || finished;
    animationLooped = animationLooped || looped;

    // Remove animations that had been destroyed but were still waiting for an update
    if (animation->GetState() == Animation::Destroyed)
    {
      iter = animations.Erase(iter);
    }
    else
    {
      ++iter;
    }
  }

  // queue the notification on finished or looped (to update loop count)
  if ( mImpl->animationFinishedDuringUpdate || animationLooped )
  {
    // The application should be notified by NotificationManager, in another thread
    mImpl->notificationManager.QueueCompleteNotification( &mImpl->animationFinishedNotifier );
  }
}

void UpdateManager::ConstrainCustomObjects( BufferIndex bufferIndex )
{
  //Constrain custom objects (in construction order)
  OwnerContainer< PropertyOwner* >& customObjects = mImpl->customObjects;
  const OwnerContainer< PropertyOwner* >::Iterator endIter = customObjects.End();
  for ( OwnerContainer< PropertyOwner* >::Iterator iter = customObjects.Begin(); endIter != iter; ++iter )
  {
    PropertyOwner& object = **iter;
    ConstrainPropertyOwner( object, bufferIndex );
  }
}

void UpdateManager::ConstrainRenderTasks( BufferIndex bufferIndex )
{
  // Constrain system-level render-tasks
  const RenderTaskList::RenderTaskContainer& systemLevelTasks = mImpl->systemLevelTaskList.GetTasks();
  for ( RenderTaskList::RenderTaskContainer::ConstIterator iter = systemLevelTasks.Begin(); iter != systemLevelTasks.End(); ++iter )
  {
    RenderTask& task = **iter;
    ConstrainPropertyOwner( task, bufferIndex );
  }

  // Constrain render-tasks
  const RenderTaskList::RenderTaskContainer& tasks = mImpl->taskList.GetTasks();
  for ( RenderTaskList::RenderTaskContainer::ConstIterator iter = tasks.Begin(); iter != tasks.End(); ++iter )
  {
    RenderTask& task = **iter;
    ConstrainPropertyOwner( task, bufferIndex );
  }
}

void UpdateManager::ConstrainShaders( BufferIndex bufferIndex )
{
  // constrain shaders... (in construction order)
  ShaderOwner& shaders = mImpl->shaders;
  for ( ShaderIter iter = shaders.Begin(); iter != shaders.End(); ++iter )
  {
    Shader& shader = **iter;
    ConstrainPropertyOwner( shader, bufferIndex );
  }
}

void UpdateManager::ProcessPropertyNotifications( BufferIndex bufferIndex )
{
  PropertyNotificationContainer &notifications = mImpl->propertyNotifications;
  PropertyNotificationIter iter = notifications.Begin();

  while ( iter != notifications.End() )
  {
    PropertyNotification* notification = *iter;
    bool valid = notification->Check( bufferIndex );
    if(valid)
    {
      mImpl->notificationManager.QueueMessage( PropertyChangedMessage( mImpl->propertyNotifier, notification, notification->GetValidity() ) );
    }
    ++iter;
  }
}

void UpdateManager::ForwardCompiledShadersToEventThread()
{
  DALI_ASSERT_DEBUG( (mImpl->shaderSaver != 0) && "shaderSaver should be wired-up during startup." );
  if( mImpl->shaderSaver )
  {
    // lock and swap the queues
    {
      // render might be attempting to send us more binaries at the same time
      Mutex::ScopedLock lock( mImpl->compiledShaderMutex );
      mImpl->renderCompiledShaders.swap( mImpl->updateCompiledShaders );
    }

    if( mImpl->updateCompiledShaders.size() > 0 )
    {
      ShaderSaver& factory = *mImpl->shaderSaver;
      ShaderDataBinaryQueue::iterator i   = mImpl->updateCompiledShaders.begin();
      ShaderDataBinaryQueue::iterator end = mImpl->updateCompiledShaders.end();
      for( ; i != end; ++i )
      {
        mImpl->notificationManager.QueueMessage( ShaderCompiledMessage( factory, *i ) );
      }
      // we don't need them in update anymore
      mImpl->updateCompiledShaders.clear();
    }
  }
}

void UpdateManager::UpdateRenderers( BufferIndex bufferIndex )
{
  const unsigned int rendererCount = mImpl->renderers.Count();
  for( unsigned int i = 0; i < rendererCount; ++i )
  {
    //Apply constraints
    ConstrainPropertyOwner( *mImpl->renderers[i], bufferIndex );

    mImpl->renderers[i]->PrepareRender( bufferIndex );
  }
}

void UpdateManager::UpdateNodes( BufferIndex bufferIndex )
{
  mImpl->nodeDirtyFlags = NothingFlag;

  if ( !mImpl->root )
  {
    return;
  }

  // Prepare resources, update shaders, for each node
  // And add the renderers to the sorted layers. Start from root, which is also a layer
  mImpl->nodeDirtyFlags = UpdateNodeTree( *( mImpl->root ),
                                          bufferIndex,
                                          mImpl->renderQueue );

  if ( mImpl->systemLevelRoot )
  {
    mImpl->nodeDirtyFlags |= UpdateNodeTree( *( mImpl->systemLevelRoot ),
                                             bufferIndex,
                                             mImpl->renderQueue );
  }
}

unsigned int UpdateManager::Update( float elapsedSeconds,
                                    unsigned int lastVSyncTimeMilliseconds,
                                    unsigned int nextVSyncTimeMilliseconds )
{
  const BufferIndex bufferIndex = mSceneGraphBuffers.GetUpdateBufferIndex();

  //Clear nodes/resources which were previously discarded
  mImpl->discardQueue.Clear( bufferIndex );

  //Process Touches & Gestures
  const bool gestureUpdated = ProcessGestures( bufferIndex, lastVSyncTimeMilliseconds, nextVSyncTimeMilliseconds );

  bool updateScene = // The scene-graph requires an update if..
      (mImpl->nodeDirtyFlags & RenderableUpdateFlags) ||    // ..nodes were dirty in previous frame OR
      IsAnimationRunning()                            ||    // ..at least one animation is running OR
      mImpl->messageQueue.IsSceneUpdateRequired()     ||    // ..a message that modifies the scene graph node tree is queued OR
      gestureUpdated;                                       // ..a gesture property was updated


  // Although the scene-graph may not require an update, we still need to synchronize double-buffered
  // values if the scene was updated in the previous frame.
  if( updateScene || mImpl->previousUpdateScene )
  {
    //Reset properties from the previous update
    ResetProperties( bufferIndex );
    mImpl->transformManager.ResetToBaseValue();
  }

  // Process the queued scene messages. Note, MessageQueue::FlushQueue may be called
  // between calling IsSceneUpdateRequired() above and here, so updateScene should
  // be set again
  updateScene |= mImpl->messageQueue.ProcessMessages( bufferIndex );

  //Forward compiled shader programs to event thread for saving
  ForwardCompiledShadersToEventThread();

  // Although the scene-graph may not require an update, we still need to synchronize double-buffered
  // renderer lists if the scene was updated in the previous frame.
  // We should not start skipping update steps or reusing lists until there has been two frames where nothing changes
  if( updateScene || mImpl->previousUpdateScene )
  {
    //Animate
    Animate( bufferIndex, elapsedSeconds );

    //Constraint custom objects
    ConstrainCustomObjects( bufferIndex );

    //Clear the lists of renderers from the previous update
    for( size_t i(0); i<mImpl->sortedLayers.size(); ++i )
    {
      mImpl->sortedLayers[i]->ClearRenderables();
    }

    for( size_t i(0); i<mImpl->systemLevelSortedLayers.size(); ++i )
    {
      mImpl->systemLevelSortedLayers[i]->ClearRenderables();
    }

    //Update node hierarchy, apply constraints and perform sorting / culling.
    //This will populate each Layer with a list of renderers which are ready.
    UpdateNodes( bufferIndex );

    //Apply constraints to RenderTasks, shaders
    ConstrainRenderTasks( bufferIndex );
    ConstrainShaders( bufferIndex );

    //Update renderers and apply constraints
    UpdateRenderers( bufferIndex );

    //Update the trnasformations of all the nodes
    mImpl->transformManager.Update();

    //Process Property Notifications
    ProcessPropertyNotifications( bufferIndex );

    //Process the RenderTasks; this creates the instructions for rendering the next frame.
    //reset the update buffer index and make sure there is enough room in the instruction container
    mImpl->renderInstructions.ResetAndReserve( bufferIndex,
                                               mImpl->taskList.GetTasks().Count() + mImpl->systemLevelTaskList.GetTasks().Count() );

    if ( NULL != mImpl->root )
    {
      mImpl->renderTaskProcessor.Process( bufferIndex,
                                        mImpl->taskList,
                                        *mImpl->root,
                                        mImpl->sortedLayers,
                                        mImpl->renderInstructions );

      // Process the system-level RenderTasks last
      if ( NULL != mImpl->systemLevelRoot )
      {
        mImpl->renderTaskProcessor.Process( bufferIndex,
                                          mImpl->systemLevelTaskList,
                                          *mImpl->systemLevelRoot,
                                          mImpl->systemLevelSortedLayers,
                                          mImpl->renderInstructions );
      }
    }
  }

  // check the countdown and notify (note, at the moment this is only done for normal tasks, not for systemlevel tasks)
  bool doRenderOnceNotify = false;
  mImpl->renderTaskWaiting = false;
  const RenderTaskList::RenderTaskContainer& tasks = mImpl->taskList.GetTasks();
  for ( RenderTaskList::RenderTaskContainer::ConstIterator iter = tasks.Begin(), endIter = tasks.End();
        endIter != iter; ++iter )
  {
    RenderTask& renderTask(*(*iter));

    renderTask.UpdateState();

    if( renderTask.IsWaitingToRender() &&
        renderTask.ReadyToRender( bufferIndex ) /*avoid updating forever when source actor is off-stage*/ )
    {
      mImpl->renderTaskWaiting = true; // keep update/render threads alive
    }

    if( renderTask.HasRendered() )
    {
      doRenderOnceNotify = true;
    }
  }

  if( doRenderOnceNotify )
  {
    DALI_LOG_INFO(gRenderTaskLogFilter, Debug::General, "Notify a render task has finished\n");
    mImpl->notificationManager.QueueCompleteNotification( mImpl->taskList.GetCompleteNotificationInterface() );
  }

  // Macro is undefined in release build.
  SNAPSHOT_NODE_LOGGING;

  // A ResetProperties() may be required in the next frame
  mImpl->previousUpdateScene = updateScene;

  // Check whether further updates are required
  unsigned int keepUpdating = KeepUpdatingCheck( elapsedSeconds );

  // tell the update manager that we're done so the queue can be given to event thread
  mImpl->notificationManager.UpdateCompleted();

  // The update has finished; swap the double-buffering indices
  mSceneGraphBuffers.Swap();

  return keepUpdating;
}

unsigned int UpdateManager::KeepUpdatingCheck( float elapsedSeconds ) const
{
  // Update the duration set via Stage::KeepRendering()
  if ( mImpl->keepRenderingSeconds > 0.0f )
  {
    mImpl->keepRenderingSeconds -= elapsedSeconds;
  }

  unsigned int keepUpdatingRequest = KeepUpdating::NOT_REQUESTED;

  // If Stage::KeepRendering() has been called, then continue until the duration has elapsed.
  // Keep updating until no messages are received and no animations are running.
  // If an animation has just finished, update at least once more for Discard end-actions.
  // No need to check for renderQueue as there is always a render after update and if that
  // render needs another update it will tell the adaptor to call update again

  if ( mImpl->keepRenderingSeconds > 0.0f )
  {
    keepUpdatingRequest |= KeepUpdating::STAGE_KEEP_RENDERING;
  }

  if ( IsAnimationRunning() ||
       mImpl->animationFinishedDuringUpdate )
  {
    keepUpdatingRequest |= KeepUpdating::ANIMATIONS_RUNNING;
  }

  if ( mImpl->renderTaskWaiting )
  {
    keepUpdatingRequest |= KeepUpdating::RENDER_TASK_SYNC;
  }

  return keepUpdatingRequest;
}

void UpdateManager::SetBackgroundColor( const Vector4& color )
{
  typedef MessageValue1< RenderManager, Vector4 > DerivedType;

  // Reserve some memory inside the render queue
  unsigned int* slot = mImpl->renderQueue.ReserveMessageSlot( mSceneGraphBuffers.GetUpdateBufferIndex(), sizeof( DerivedType ) );

  // Construct message in the render queue memory; note that delete should not be called on the return value
  new (slot) DerivedType( &mImpl->renderManager, &RenderManager::SetBackgroundColor, color );
}

void UpdateManager::SetDefaultSurfaceRect( const Rect<int>& rect )
{
  typedef MessageValue1< RenderManager, Rect<int> > DerivedType;

  // Reserve some memory inside the render queue
  unsigned int* slot = mImpl->renderQueue.ReserveMessageSlot( mSceneGraphBuffers.GetUpdateBufferIndex(), sizeof( DerivedType ) );

  // Construct message in the render queue memory; note that delete should not be called on the return value
  new (slot) DerivedType( &mImpl->renderManager,  &RenderManager::SetDefaultSurfaceRect, rect );
}

void UpdateManager::KeepRendering( float durationSeconds )
{
  mImpl->keepRenderingSeconds = std::max( mImpl->keepRenderingSeconds, durationSeconds );
}

void UpdateManager::SetLayerDepths( const SortedLayerPointers& layers, bool systemLevel )
{
  if ( !systemLevel )
  {
    // just copy the vector of pointers
    mImpl->sortedLayers = layers;
  }
  else
  {
    mImpl->systemLevelSortedLayers = layers;
  }
}

void UpdateManager::SetDepthIndices( NodeDepths* nodeDepths )
{
  if( nodeDepths )
  {
    // note,this vector is already in depth order. It could be used as-is to
    // remove sorting in update algorithm. However, it lacks layer boundary markers.
    for( std::vector<NodeDepthPair>::iterator iter = nodeDepths->nodeDepths.begin(),
           end = nodeDepths->nodeDepths.end() ;
         iter != end ; ++iter )
    {
      iter->node->SetDepthIndex( iter->sortedDepth );
    }
  }
}

void UpdateManager::AddSampler( Render::Sampler* sampler )
{
  // Message has ownership of Sampler while in transit from update to render
  typedef MessageValue1< RenderManager, OwnerPointer< Render::Sampler > > DerivedType;

  // Reserve some memory inside the render queue
  unsigned int* slot = mImpl->renderQueue.ReserveMessageSlot( mSceneGraphBuffers.GetUpdateBufferIndex(), sizeof( DerivedType ) );

  // Construct message in the render queue memory; note that delete should not be called on the return value
  new (slot) DerivedType( &mImpl->renderManager,  &RenderManager::AddSampler, sampler );
}

void UpdateManager::RemoveSampler( Render::Sampler* sampler )
{
  typedef MessageValue1< RenderManager, Render::Sampler* > DerivedType;

  // Reserve some memory inside the render queue
  unsigned int* slot = mImpl->renderQueue.ReserveMessageSlot( mSceneGraphBuffers.GetUpdateBufferIndex(), sizeof( DerivedType ) );

  // Construct message in the render queue memory; note that delete should not be called on the return value
  new (slot) DerivedType( &mImpl->renderManager,  &RenderManager::RemoveSampler, sampler );
}

void UpdateManager::SetFilterMode( Render::Sampler* sampler, unsigned int minFilterMode, unsigned int magFilterMode )
{
  typedef MessageValue3< RenderManager, Render::Sampler*, unsigned int, unsigned int > DerivedType;

  // Reserve some memory inside the render queue
  unsigned int* slot = mImpl->renderQueue.ReserveMessageSlot( mSceneGraphBuffers.GetUpdateBufferIndex(), sizeof( DerivedType ) );

  // Construct message in the render queue memory; note that delete should not be called on the return value
  new (slot) DerivedType( &mImpl->renderManager,  &RenderManager::SetFilterMode, sampler, minFilterMode, magFilterMode );
}

void UpdateManager::SetWrapMode( Render::Sampler* sampler, unsigned int rWrapMode, unsigned int sWrapMode, unsigned int tWrapMode )
{
  typedef MessageValue4< RenderManager, Render::Sampler*, unsigned int, unsigned int, unsigned int > DerivedType;

  // Reserve some memory inside the render queue
  unsigned int* slot = mImpl->renderQueue.ReserveMessageSlot( mSceneGraphBuffers.GetUpdateBufferIndex(), sizeof( DerivedType ) );

  // Construct message in the render queue memory; note that delete should not be called on the return value
  new (slot) DerivedType( &mImpl->renderManager,  &RenderManager::SetWrapMode, sampler, rWrapMode, sWrapMode, tWrapMode );
}

void UpdateManager::AddPropertyBuffer( Render::PropertyBuffer* propertyBuffer )
{
  // Message has ownership of format while in transit from update -> render
  typedef MessageValue1< RenderManager, OwnerPointer< Render::PropertyBuffer > > DerivedType;

  // Reserve some memory inside the render queue
  unsigned int* slot = mImpl->renderQueue.ReserveMessageSlot( mSceneGraphBuffers.GetUpdateBufferIndex(), sizeof( DerivedType ) );

  // Construct message in the render queue memory; note that delete should not be called on the return value
  new (slot) DerivedType( &mImpl->renderManager,  &RenderManager::AddPropertyBuffer, propertyBuffer );
}

void UpdateManager::RemovePropertyBuffer( Render::PropertyBuffer* propertyBuffer )
{
  typedef MessageValue1< RenderManager, Render::PropertyBuffer* > DerivedType;

  // Reserve some memory inside the render queue
  unsigned int* slot = mImpl->renderQueue.ReserveMessageSlot( mSceneGraphBuffers.GetUpdateBufferIndex(), sizeof( DerivedType ) );

  // Construct message in the render queue memory; note that delete should not be called on the return value
  new (slot) DerivedType( &mImpl->renderManager,  &RenderManager::RemovePropertyBuffer, propertyBuffer );
}

void UpdateManager::SetPropertyBufferFormat(Render::PropertyBuffer* propertyBuffer, Render::PropertyBuffer::Format* format )
{
  // Message has ownership of format while in transit from update -> render
  typedef MessageValue2< RenderManager, Render::PropertyBuffer*, OwnerPointer< Render::PropertyBuffer::Format > > DerivedType;

  // Reserve some memory inside the render queue
  unsigned int* slot = mImpl->renderQueue.ReserveMessageSlot( mSceneGraphBuffers.GetUpdateBufferIndex(), sizeof( DerivedType ) );

  // Construct message in the render queue memory; note that delete should not be called on the return value
  new (slot) DerivedType( &mImpl->renderManager,  &RenderManager::SetPropertyBufferFormat, propertyBuffer, format );
}

void UpdateManager::SetPropertyBufferData( Render::PropertyBuffer* propertyBuffer, Dali::Vector<char>* data, size_t size )
{
  // Message has ownership of format while in transit from update -> render
  typedef MessageValue3< RenderManager, Render::PropertyBuffer*, OwnerPointer< Dali::Vector<char> >, size_t > DerivedType;

  // Reserve some memory inside the render queue
  unsigned int* slot = mImpl->renderQueue.ReserveMessageSlot( mSceneGraphBuffers.GetUpdateBufferIndex(), sizeof( DerivedType ) );

  // Construct message in the render queue memory; note that delete should not be called on the return value
  new (slot) DerivedType( &mImpl->renderManager, &RenderManager::SetPropertyBufferData, propertyBuffer, data, size );
}

void UpdateManager::AddGeometry( Render::Geometry* geometry )
{
  // Message has ownership of format while in transit from update -> render
  typedef MessageValue1< RenderManager, OwnerPointer< Render::Geometry > > DerivedType;

  // Reserve some memory inside the render queue
  unsigned int* slot = mImpl->renderQueue.ReserveMessageSlot( mSceneGraphBuffers.GetUpdateBufferIndex(), sizeof( DerivedType ) );

  // Construct message in the render queue memory; note that delete should not be called on the return value
  new (slot) DerivedType( &mImpl->renderManager,  &RenderManager::AddGeometry, geometry );
}

void UpdateManager::RemoveGeometry( Render::Geometry* geometry )
{
  typedef MessageValue1< RenderManager, Render::Geometry* > DerivedType;

  // Reserve some memory inside the render queue
  unsigned int* slot = mImpl->renderQueue.ReserveMessageSlot( mSceneGraphBuffers.GetUpdateBufferIndex(), sizeof( DerivedType ) );

  // Construct message in the render queue memory; note that delete should not be called on the return value
  new (slot) DerivedType( &mImpl->renderManager,  &RenderManager::RemoveGeometry, geometry );
}

void UpdateManager::SetGeometryType( Render::Geometry* geometry, unsigned int geometryType )
{
  typedef MessageValue2< RenderManager, Render::Geometry*, unsigned int > DerivedType;

  // Reserve some memory inside the render queue
  unsigned int* slot = mImpl->renderQueue.ReserveMessageSlot( mSceneGraphBuffers.GetUpdateBufferIndex(), sizeof( DerivedType ) );

  // Construct message in the render queue memory; note that delete should not be called on the return value
  new (slot) DerivedType( &mImpl->renderManager,  &RenderManager::SetGeometryType, geometry, geometryType );
}

void UpdateManager::SetIndexBuffer( Render::Geometry* geometry, Dali::Vector<unsigned short>& indices )
{
  typedef IndexBufferMessage< RenderManager > DerivedType;

  // Reserve some memory inside the render queue
  unsigned int* slot = mImpl->renderQueue.ReserveMessageSlot( mSceneGraphBuffers.GetUpdateBufferIndex(), sizeof( DerivedType ) );

  // Construct message in the render queue memory; note that delete should not be called on the return value
  new (slot) DerivedType( &mImpl->renderManager, geometry, indices );
}

void UpdateManager::RemoveVertexBuffer( Render::Geometry* geometry, Render::PropertyBuffer* propertyBuffer )
{
  typedef MessageValue2< RenderManager, Render::Geometry*, Render::PropertyBuffer* > DerivedType;

  // Reserve some memory inside the render queue
  unsigned int* slot = mImpl->renderQueue.ReserveMessageSlot( mSceneGraphBuffers.GetUpdateBufferIndex(), sizeof( DerivedType ) );

  // Construct message in the render queue memory; note that delete should not be called on the return value
  new (slot) DerivedType( &mImpl->renderManager,  &RenderManager::RemoveVertexBuffer, geometry, propertyBuffer );
}

void UpdateManager::AddVertexBuffer( Render::Geometry* geometry, Render::PropertyBuffer* propertyBuffer )
{
  typedef MessageValue2< RenderManager, Render::Geometry*, Render::PropertyBuffer* > DerivedType;

  // Reserve some memory inside the render queue
  unsigned int* slot = mImpl->renderQueue.ReserveMessageSlot( mSceneGraphBuffers.GetUpdateBufferIndex(), sizeof( DerivedType ) );

  // Construct message in the render queue memory; note that delete should not be called on the return value
  new (slot) DerivedType( &mImpl->renderManager,  &RenderManager::AddVertexBuffer, geometry, propertyBuffer );
}

void UpdateManager::AddTexture( Render::Texture* texture )
{
  // Message has ownership of Texture while in transit from update -> render
  typedef MessageValue1< RenderManager, OwnerPointer< Render::Texture > > DerivedType;

  // Reserve some memory inside the render queue
  unsigned int* slot = mImpl->renderQueue.ReserveMessageSlot( mSceneGraphBuffers.GetUpdateBufferIndex(), sizeof( DerivedType ) );

  // Construct message in the render queue memory; note that delete should not be called on the return value
  new (slot) DerivedType( &mImpl->renderManager,  &RenderManager::AddTexture, texture );
}

void UpdateManager::RemoveTexture( Render::Texture* texture)
{
  typedef MessageValue1< RenderManager, Render::Texture* > DerivedType;

  // Reserve some memory inside the render queue
  unsigned int* slot = mImpl->renderQueue.ReserveMessageSlot( mSceneGraphBuffers.GetUpdateBufferIndex(), sizeof( DerivedType ) );

  // Construct message in the render queue memory; note that delete should not be called on the return value
  new (slot) DerivedType( &mImpl->renderManager,  &RenderManager::RemoveTexture, texture );
}

void UpdateManager::UploadTexture( Render::Texture* texture, PixelDataPtr pixelData, const Texture::UploadParams& params )
{
  typedef MessageValue3< RenderManager, Render::Texture*, PixelDataPtr, Texture::UploadParams > DerivedType;

  // Reserve some memory inside the message queue
  unsigned int* slot = mImpl->renderQueue.ReserveMessageSlot( mSceneGraphBuffers.GetUpdateBufferIndex(), sizeof( DerivedType ) );

  // Construct message in the message queue memory; note that delete should not be called on the return value
  new (slot) DerivedType( &mImpl->renderManager, &RenderManager::UploadTexture, texture, pixelData, params );
}

void UpdateManager::GenerateMipmaps( Render::Texture* texture )
{
  typedef MessageValue1< RenderManager, Render::Texture* > DerivedType;

  // Reserve some memory inside the render queue
  unsigned int* slot = mImpl->renderQueue.ReserveMessageSlot( mSceneGraphBuffers.GetUpdateBufferIndex(), sizeof( DerivedType ) );

  // Construct message in the render queue memory; note that delete should not be called on the return value
  new (slot) DerivedType( &mImpl->renderManager,  &RenderManager::GenerateMipmaps, texture );
}

void UpdateManager::AddFrameBuffer( Render::FrameBuffer* frameBuffer )
{
  typedef MessageValue1< RenderManager, Render::FrameBuffer* > DerivedType;

  // Reserve some memory inside the render queue
  unsigned int* slot = mImpl->renderQueue.ReserveMessageSlot( mSceneGraphBuffers.GetUpdateBufferIndex(), sizeof( DerivedType ) );

  // Construct message in the render queue memory; note that delete should not be called on the return value
  new (slot) DerivedType( &mImpl->renderManager,  &RenderManager::AddFrameBuffer, frameBuffer );
}

void UpdateManager::RemoveFrameBuffer( Render::FrameBuffer* frameBuffer)
{
  typedef MessageValue1< RenderManager, Render::FrameBuffer* > DerivedType;

  // Reserve some memory inside the render queue
  unsigned int* slot = mImpl->renderQueue.ReserveMessageSlot( mSceneGraphBuffers.GetUpdateBufferIndex(), sizeof( DerivedType ) );

  // Construct message in the render queue memory; note that delete should not be called on the return value
  new (slot) DerivedType( &mImpl->renderManager,  &RenderManager::RemoveFrameBuffer, frameBuffer );
}

void UpdateManager::AttachColorTextureToFrameBuffer( Render::FrameBuffer* frameBuffer, Render::Texture* texture, unsigned int mipmapLevel, unsigned int layer )
{
  typedef MessageValue4< RenderManager, Render::FrameBuffer*, Render::Texture*, unsigned int, unsigned int > DerivedType;

  // Reserve some memory inside the render queue
  unsigned int* slot = mImpl->renderQueue.ReserveMessageSlot( mSceneGraphBuffers.GetUpdateBufferIndex(), sizeof( DerivedType ) );

  // Construct message in the render queue memory; note that delete should not be called on the return value
  new (slot) DerivedType( &mImpl->renderManager,  &RenderManager::AttachColorTextureToFrameBuffer, frameBuffer, texture, mipmapLevel, layer );
}

} // namespace SceneGraph

} // namespace Internal

} // namespace Dali
