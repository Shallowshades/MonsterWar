#include <SDL3/SDL.h>
#include <spdlog/spdlog.h>
#include <entt/signal/dispatcher.hpp>
#include "game_app.h"
#include "time.h"
#include "config.h"
#include "context.h"
#include "game_state.h"
#include "../resource/resource_manager.h"
#include "../audio/audio_player.h"
#include "../render/renderer.h"
#include "../render/camera.h"
#include "../render/text_renderer.h"
#include "../input/input_manager.h"
#include "../scene/scene_manager.h"
#include "../utils/events.h"

engine::core::GameApp::GameApp() = default;

engine::core::GameApp::~GameApp() {
	if (mIsRunning) {
		spdlog::warn("{} 被销毁时没有显式关闭, 正在关闭...", mLogTag.data());
		close();
	}
}

void engine::core::GameApp::run() {
	if (!init()) {
		spdlog::error("{} 初始化失败, 无法运行游戏.", mLogTag.data());
		return;
	}


	while (mIsRunning) {
		mTime->update();
		float delta = mTime->getDeltaTime(); // 每帧的时间间隔
		handleEvents();
		update(delta);
		render();

		// spdlog::info("Delta Time: {}", delta);
	}

	close();
}

void engine::core::GameApp::registerSceneSetup(std::function<void(engine::core::Context&)> func) {
	mSceneSetupFunc = std::move(func);
	spdlog::trace("GameApp 已注册场景设置函数");
}

bool engine::core::GameApp::init() {
	spdlog::trace("{} 初始化...", mLogTag.data());

	if (!mSceneSetupFunc) {
		spdlog::error("GameApp 未注册场景设置函数, 无法初始化 GameApp");
		return false;
	}

	if (!initDispatcher()) return false;
	if (!initConfig()) return false;
	if (!initSDL()) return false;
	if (!initGameState()) return false;
	if (!initTime()) return false;
	if (!initResourceManager()) return false;
	if (!initAudioPlayer()) return false;
	if (!initRenderer()) return false;
	if (!initCamera()) return false;
	if (!initTextRenderer()) return false;
	if (!initInputManager()) return false;
	if (!initContext()) return false;
	if (!initSceneManager()) return false;

	// (调用场景设置函数) 创建第一个场景并压入栈
	mSceneSetupFunc(*mContext);

	// 注册退出事件 (回调函数可以无参数, 代表不使用事件结构体中的数据)
	mDispatcher->sink<utils::QuitEvent>().connect<&GameApp::onQuitEvent>(this);

	mIsRunning = true;
	spdlog::trace("{} 初始化成功", mLogTag.data());
	return true;
}

void engine::core::GameApp::handleEvents() {
	// 处理并分发输入事件
	mInputManager->update();

	mSceneManager->handleInput();
}

void engine::core::GameApp::update(float delta) {
	// 游戏逻辑更新
	mSceneManager->update(delta);

	// 分发事件
	mDispatcher->update();
}

void engine::core::GameApp::render() {
	//1. 清除屏幕
	mRenderer->clearScreen();
	//2. 具体渲染代码
	mSceneManager->render();
	//3. 更新屏幕显示
	mRenderer->present();
}

void engine::core::GameApp::close() {
	spdlog::trace("{} 关闭...", mLogTag.data());

	mDispatcher->sink<utils::QuitEvent>().disconnect<&GameApp::onQuitEvent>(this);

	// 先关闭场景管理器, 确保所有场景都被清理
	mSceneManager->clean();

	// 为了确保正确的销毁顺序, 有些智能指针对象需要手动管理
	mResourceManager.reset();

	if (mSDLRenderer != nullptr) {
		SDL_DestroyRenderer(mSDLRenderer);
		mSDLRenderer = nullptr;
	}
	if (mWindow != nullptr) {
		SDL_DestroyWindow(mWindow);
		mWindow = nullptr;
	}
	SDL_Quit();
	mIsRunning = false;
}

bool engine::core::GameApp::initDispatcher() {
	try {
		mDispatcher = std::make_unique<entt::dispatcher>();
	}
	catch (const std::exception& e) {
		spdlog::error("{} : 初始化事件分发器失败, e : {}.", mLogTag.data(), e.what());
		return false;
	}
	spdlog::trace("{} : 事件分发器初始化成功.", mLogTag.data());
	return true;
}

bool engine::core::GameApp::initConfig() {
	try {
		mConfig = std::make_unique<engine::core::Config>("assets/config.json");
	}
	catch (const std::exception& e) {
		spdlog::error("{} 初始化配置失败: {}", mLogTag.data(), e.what());
		return false;
	}
	spdlog::trace("{} 配置初始化成功", mLogTag.data());
	return true;
}

bool engine::core::GameApp::initSDL() {
	if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
		spdlog::error("{} 无法创建窗口! SDL错误: {}", mLogTag.data(), SDL_GetError());
		return false;
	}

	int window_width = static_cast<int>(static_cast<float>(mConfig->mWindowWidth) * mConfig->mWindowScale);
	int window_height = static_cast<int>(static_cast<float>(mConfig->mWindowHeight) * mConfig->mWindowScale);
	mWindow = SDL_CreateWindow(mConfig->mWindowTitle.c_str(), window_width, window_height, SDL_WINDOW_RESIZABLE);
	if (mWindow == nullptr) {
		spdlog::error("{} 无法创建窗口! SDL错误: {}", mLogTag.data(), SDL_GetError());
		return false;
	}

	mSDLRenderer = SDL_CreateRenderer(mWindow, nullptr);
	if (mSDLRenderer == nullptr) {
		spdlog::error("{} 无法创建渲染器! SDL错误: {}", mLogTag.data(), SDL_GetError());
		return false;
	}

	// 设置渲染器支持透明色
	SDL_SetRenderDrawBlendMode(mSDLRenderer, SDL_BLENDMODE_BLEND);

	// 设置VSync(注意:VSync开启时, 驱动程序会尝试将帧率限制到显示器刷新率, 有可能会覆盖手动设置的mTargetFps)
	int vsyncMode = mConfig->mVsyncEnabled ? SDL_RENDERER_VSYNC_ADAPTIVE : SDL_RENDERER_VSYNC_DISABLED;
	SDL_SetRenderVSync(mSDLRenderer, vsyncMode);
	spdlog::trace("{} Vsync设置为: {}", mLogTag.data(), mConfig->mVsyncEnabled ? "Enable" : "Disable");

	// 设置逻辑分辨率 (窗口大小 * 逻辑缩放比例)
	int logical_width = static_cast<int>(static_cast<float>(mConfig->mWindowWidth) * mConfig->mWindowLogicalScale);
	int logical_height = static_cast<int>(static_cast<float>(mConfig->mWindowHeight) * mConfig->mWindowLogicalScale);
	SDL_SetRenderLogicalPresentation(mSDLRenderer, logical_width, logical_height, SDL_LOGICAL_PRESENTATION_LETTERBOX);
	spdlog::trace("{} 初始化SDL成功", mLogTag.data());
	return true;
}

bool engine::core::GameApp::initTime() {
	try {
		mTime = std::make_unique<Time>();
	}
	catch (const std::exception& e) {
		spdlog::error("{} 初始化时间管理器失败: {}", mLogTag.data(), e.what());
		return false;
	}
	mTime->setTargetFps(mConfig->mTargetFps);
	spdlog::trace("{} 时间管理初始化成功", mLogTag.data());
	return true;
}

bool engine::core::GameApp::initResourceManager() {
	try {
		mResourceManager = std::make_unique<engine::resource::ResourceManager>(mSDLRenderer);
	}
	catch (const std::exception& e) {
		spdlog::error("{} 初始化资源管理器失败: {}", mLogTag.data(), e.what());
		return false;
	}
	spdlog::trace("{} 资源管理器成功", mLogTag.data());
	mResourceManager->loadResources("assets/data/resource_mapping.json");
	return true;
}

bool engine::core::GameApp::initAudioPlayer() {
	try {
		mAudioPlayer = std::make_unique<engine::audio::AudioPlayer>(mResourceManager.get());
		mAudioPlayer->setMusicVolume(mConfig->mMusicVolume);
		mAudioPlayer->setSoundVolume(mConfig->mSoundVolume);
	}
	catch (const std::exception& e) {
		spdlog::error("{} : 音频播放器初始化失败: {}", mLogTag.data(), e.what());
		return false;
	}
	spdlog::trace("{} : 音频播放器初始化成功.", mLogTag.data());
	return true;
}

bool engine::core::GameApp::initRenderer() {
	try {
		mRenderer = std::make_unique<engine::render::Renderer>(mSDLRenderer, mResourceManager.get());
	}
	catch (const std::exception& e) {
		spdlog::error("{} 初始化渲染器失败: {}", mLogTag.data(), e.what());
		return false;
	}
	spdlog::trace("{} 渲染器初始化成功", mLogTag.data());
	return true;
}

bool engine::core::GameApp::initTextRenderer() {
	try {
		mTextRenderer = std::make_unique<engine::render::TextRenderer>(mSDLRenderer, mResourceManager.get());
	}
	catch (const std::exception& e) {
		spdlog::error("初始化文字渲染引擎失败: {}", e.what());
		return false;
	}
	spdlog::trace("文字渲染引擎初始化成功。");
	return true;
}

bool engine::core::GameApp::initCamera() {
	try {
		mCamera = std::make_unique<engine::render::Camera>(mGameState->getLogicalSize());	
	}
	catch (const std::exception& e) {
		spdlog::error("{} 初始化相机失败: {}", mLogTag.data(), e.what());
		return false;
	}
	spdlog::trace("{} 相机初始化成功", mLogTag.data());
	return true;
}

bool engine::core::GameApp::initInputManager() {
	try {
		mInputManager = std::make_unique<engine::input::InputManager>(mSDLRenderer, mConfig.get(), mDispatcher.get());
	}
	catch (const std::exception& e) {
		spdlog::error("{} 初始化输入管理器失败: {}", mLogTag.data(), e.what());
		return false;
	}
	spdlog::trace("{} 输入管理器初始化成功", mLogTag.data());
	return true;
}

bool engine::core::GameApp::initGameState() {
	try {
		mGameState = std::make_unique<engine::core::GameState>(mWindow, mSDLRenderer);
	}
	catch (const std::exception& e) {
		spdlog::error("初始化游戏状态失败: {}", e.what());
		return false;
	}
	return true;
}

bool engine::core::GameApp::initContext() {
	try {
		mContext = std::make_unique<engine::core::Context>(*mDispatcher, *mInputManager, *mRenderer, *mCamera, *mTextRenderer, *mResourceManager, *mAudioPlayer, *mGameState);
	}
	catch (const std::exception& e) {
		spdlog::error("{} 初始化上下文失败: {}", mLogTag.data(), e.what());
		return false;
	}
	return true;
}

bool engine::core::GameApp::initSceneManager() {
	try {
		mSceneManager = std::make_unique<engine::scene::SceneManager>(*mContext);
	}
	catch (const std::exception& e) {
		spdlog::error("{} 初始化场景管理器失败: {}", mLogTag.data(), e.what());
		return false;
	}
	spdlog::trace("{} 场景管理器初始化成功.", mLogTag.data());
	return true;
}

void engine::core::GameApp::onQuitEvent() {
	spdlog::trace("{} : GameApp 收到来自事件分发器的退出请求.");
	mIsRunning = false;
}
