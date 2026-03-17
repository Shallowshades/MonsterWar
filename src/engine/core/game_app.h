#pragma once

#ifndef GAME_APP_H
#define GAME_APP_H

#include <memory>
#include <functional>

// 前向声明, 减少头文件依赖, 增加编译速度
struct SDL_Window;
struct SDL_Renderer;

namespace engine::resource {
class ResourceManager;
}

namespace engine::render {
class Renderer;
class Camera;
class TextRenderer;
}

namespace engine::input {
class InputManager;
}

namespace engine::physics {
class PhysicsEngine;
}

namespace engine::scene {
class SceneManager;
}

namespace engine::audio {
class AudioPlayer;
}

namespace engine::core {
class Time;
class Config;
class Context;
class GameState;
/**
 * @brief 主游戏应用程序类, 初始化SDL, 管理游戏循环
 */
class GameApp final {	// final 表示不能被继承
public:
	GameApp();
	~GameApp();

	/**
	 * @brief 运行游戏应用程序,其中会调用init(),然后进入主循环,离开循环后自动调用close().
	 */
	void run();

	/**
	 * @brief 注册用于设置初始场景的函数. SceneManager初始化后被调用
	 * 
	 * @param func 用于接收SceneManager引用的函数对象
	 */
	void registerSceneSetup(std::function<void(engine::scene::SceneManager&)> func);

	// 禁止拷贝和移动构造
	GameApp(const GameApp&) = delete;
	GameApp& operator=(const GameApp&) = delete;
	GameApp(GameApp&&) = delete;
	GameApp& operator=(GameApp&&) = delete;

private:
	[[nodiscard]] bool init();	// nodiscard 表示函数返回值不应该被忽略
	void handleEvents();
	void update(float delta);
	void render();
	void close();

	// 各模块的初始化/创建函数, 在init()中调用
	[[nodiscard]] bool initConfig();
	[[nodiscard]] bool initSDL();
	[[nodiscard]] bool initTime();
	[[nodiscard]] bool initResourceManager();
	[[nodiscard]] bool initAudioPlayer();
	[[nodiscard]] bool initRenderer();
	[[nodiscard]] bool initTextRenderer();
	[[nodiscard]] bool initCamera();
	[[nodiscard]] bool initInputManager();
	[[nodiscard]] bool initPhysicsEngine();
	[[nodiscard]] bool initGameState();
	[[nodiscard]] bool initContext();
	[[nodiscard]] bool initSceneManager();

	// 测试函数
	void testResourceManager();
	void testRenderer();
	void testCamera();
	void testInputManager();
	void testGameObject();

private:
	static constexpr std::string_view mLogTag = "GameApp";

	SDL_Window* mWindow = nullptr;
	SDL_Renderer* mSDLRenderer = nullptr;
	bool mIsRunning = false;

	// 游戏场景设置函数, 用于在运行游戏前设置初始化场景(GameApp不再决定初始场景)
	// engine 模块作为底层不应该引用game中的文件, 应遵循上层引用下层的原则
	std::function<void(engine::scene::SceneManager&)> mSceneSetupFunc;

	// 引擎组件
	std::unique_ptr<engine::core::Time> mTime;
	std::unique_ptr<engine::core::Config> mConfig;
	std::unique_ptr<engine::resource::ResourceManager> mResourceManager;
	std::unique_ptr<engine::render::Renderer> mRenderer;
	std::unique_ptr<engine::render::Camera> mCamera;
	std::unique_ptr<engine::render::TextRenderer> mTextRenderer;
	std::unique_ptr<engine::input::InputManager> mInputManager;
	std::unique_ptr<engine::core::Context> mContext;
	std::unique_ptr<engine::scene::SceneManager> mSceneManager;
	std::unique_ptr<engine::physics::PhysicsEngine> mPhysicsEngine;
	std::unique_ptr<engine::audio::AudioPlayer> mAudioPlayer;
	std::unique_ptr<engine::core::GameState> mGameState;
};

}

#endif // !GAME_APP_H
