#include "context.h"
#include "../input/input_manager.h"
#include "../render/renderer.h"
#include "../render/camera.h"
#include "../render/text_renderer.h"
#include "../resource/resource_manager.h"
#include "../physics/physics_engine.h"
#include "../audio/audio_player.h"
#include <spdlog/spdlog.h>

namespace engine::core {
engine::core::Context::Context(engine::input::InputManager& inputManager, engine::render::Renderer& renderer, engine::render::Camera& camera, engine::render::TextRenderer& textRenderer, engine::resource::ResourceManager& resourceManager, engine::physics::PhysicsEngine& physicsEngine, engine::audio::AudioPlayer& audioPlayer, engine::core::GameState& gameState)
	: mInputManager(inputManager)
	, mRenderer(renderer)
	, mCamera(camera)
	, mTextRenderer(textRenderer)
	, mResourceManager(resourceManager)
	, mPhysicsEngine(physicsEngine)
	, mAudioPlayer(audioPlayer)
	, mGameState(gameState)
{
	spdlog::trace("上下文创建并初始化, 包含输入管理器,渲染器,相机和资源管理器.");
}

engine::input::InputManager& Context::getInputManager() const {
	return mInputManager;
}

engine::render::Renderer& Context::getRenderer() const {
	return mRenderer;
}

engine::render::Camera& Context::getCamera() const {
	return mCamera;
}

engine::render::TextRenderer& Context::getTextRenderer() const {
	return mTextRenderer;
}

engine::resource::ResourceManager& Context::getResourceManager() const {
	return mResourceManager;
}

engine::physics::PhysicsEngine& Context::getPhysicsEngine() const {
	return mPhysicsEngine;
}

engine::audio::AudioPlayer& Context::getAudioPlayer() const {
	return mAudioPlayer;
}

engine::core::GameState& Context::getGameState() const {
	return mGameState;
}
}
