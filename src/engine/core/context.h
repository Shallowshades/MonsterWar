/*****************************************************************//**
 * @file   context.h
 * @brief  上下文类
 * @version 1.0
 * 
 * @author 29230
 * @date   October 2025
 *********************************************************************/

#pragma once
#ifndef CONTEXT_H
#define CONTEXT_H

namespace engine::input {
	class InputManager;
}

namespace engine::render {
	class Renderer;
	class Camera;
	class TextRenderer;
}

namespace engine::resource {
	class ResourceManager;
}

namespace engine::physics {
	class PhysicsEngine;
}

namespace engine::audio {
	class AudioPlayer;
}

namespace engine::core {
	class GameState;
/**
 * brief 持有对核心引擎模块引用的上下文对象.
 * 
 * 用于简化依赖注入, 传递Context即可获取引擎的各个模块
 */
class Context final {
public:
	/**
	 * @brief 构造函数.
	 * 
	 * @param inputManager 对 InputManager 实例的引用
	 * @param renderer 对 Renderer 实例的引用
	 * @param camera 对 Camera 实例的引用
	 * @param resourceManager 对 ResourceManager 实例的引用
	 */
	Context(engine::input::InputManager& inputManager,
		engine::render::Renderer& renderer,
		engine::render::Camera& camera,
		engine::render::TextRenderer& textRenderer,
		engine::resource::ResourceManager& resourceManager,
		engine::physics::PhysicsEngine& physicsEngine,
		engine::audio::AudioPlayer& audioPlayer,
		engine::core::GameState& gameState);

	// 禁用拷贝和移动语义
	Context(const Context&) = delete;										///< @brief 删除拷贝构造
	Context& operator=(const Context&) = delete;							///< @brief 删除拷贝赋值构造
	Context(Context&&) = delete;											///< @brief 删除移动构造
	Context& operator=(Context&&) = delete;									///< @brief 删除移动赋值构造

	engine::input::InputManager& getInputManager() const;					///< @brief 获取输入管理器	
	engine::render::Renderer& getRenderer() const;							///< @brief 获取渲染器
	engine::render::Camera& getCamera() const;								///< @brief 获取相机
	engine::render::TextRenderer& getTextRenderer() const;					///< @brief 获取文字渲染器
	engine::resource::ResourceManager& getResourceManager() const;			///< @brief 获取资源管理器
	engine::physics::PhysicsEngine& getPhysicsEngine() const;				///< @brief 获取物理引擎
	engine::audio::AudioPlayer& getAudioPlayer() const;						///< @brief 获取音频播放器
	engine::core::GameState& getGameState() const;							///< @brief 获取游戏状态
private:
	// 引用, 确保每个模块都有效, 使用时不需要检查指针是否为空
	engine::input::InputManager& mInputManager;								///< @brief 输入管理器			
	engine::render::Renderer& mRenderer;									///< @brief 渲染器
	engine::render::Camera& mCamera;										///< @brief 相机
	engine::render::TextRenderer& mTextRenderer;							///< @brief 文字渲染器
	engine::resource::ResourceManager& mResourceManager;					///< @brief 资源管理器
	engine::physics::PhysicsEngine& mPhysicsEngine;							///< @brief 物理引擎
	engine::audio::AudioPlayer& mAudioPlayer;								///< @brief 音频播放器
	engine::core::GameState& mGameState;										///< @brief 游戏状态
};
}

#endif // CONTEXT_H
