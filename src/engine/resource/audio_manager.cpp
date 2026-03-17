#include "audio_manager.h"
#include <spdlog/spdlog.h>
#include <stdexcept>

namespace engine::resource {
	AudioManager::AudioManager() {
		// 初始化SDL_mixer(推荐OGG, MP3)
		MIX_InitFlags flags = MIX_INIT_OGG | MIX_INIT_MP3;
		if ((Mix_Init(flags) & flags) != flags) {
			throw std::runtime_error(mLogTag.data() + std::string(" 错误: Mix_Init失败: ") + std::string(SDL_GetError()));
		}

		// SDL3打开音频设备的方法; 默认值: 44100Hz, 默认格式: 2声道(立体声), 2048采样块大小
		if (!Mix_OpenAudio(0, nullptr)) {
			Mix_Quit();
			throw std::runtime_error(mLogTag.data() + std::string(" 错误: Mix_OpenAudio失败: ") + std::string(SDL_GetError()));
		}
		spdlog::trace("{} 构造成功", mLogTag);
	}

	AudioManager::~AudioManager() {
		// 立即停止所有音频播放
		Mix_HaltChannel(-1);	// 停止所有音效
		Mix_HaltMusic();		// 停止音乐

		// 清理资源映射
		clearSounds();
		clearMusic();

		// 关闭音频设备
		Mix_CloseAudio();

		// 退出SDL_Mixer子系统
		Mix_Quit();
		spdlog::trace("{} 析构成功", mLogTag);
	}

	Mix_Chunk* AudioManager::loadSound(std::string_view filePath) {
		// 首先检查缓存
		auto iter = mSounds.find(std::string(filePath));
		if (iter != mSounds.end()) {
			return iter->second.get();
		}

		// 加载音效块
		spdlog::debug("{} 加载音效: {}", mLogTag.data(), filePath);
		Mix_Chunk* rawChunk = Mix_LoadWAV(filePath.data());
		if (!rawChunk) {
			spdlog::error("{} 加载音效失败: '{}':{}", mLogTag.data(), filePath.data(), SDL_GetError());
			return nullptr;
		}

		// 使用unique_ptr存储在缓存中
		mSounds.emplace(filePath, std::unique_ptr<Mix_Chunk, SDLMixChunkDeleter>(rawChunk));
		spdlog::debug("{} 成功加载并缓存音效: {}", mLogTag.data(), filePath);
		return rawChunk;
	}

	Mix_Chunk* AudioManager::getSound(std::string_view filePath) {
		auto iter = mSounds.find(std::string(filePath));
		if (iter != mSounds.end()) {
			return iter->second.get();
		}
		spdlog::warn("{} 音效 '{}' 未找到缓存，尝试加载。", mLogTag.data(), filePath.data());
		return loadSound(filePath);
	}

	void AudioManager::unloadSound(std::string_view filePath) {
		auto iter = mSounds.find(std::string(filePath));
		if (iter != mSounds.end()) {
			spdlog::debug("{} 卸载音效: {}", mLogTag.data(), filePath.data());
			mSounds.erase(iter);
		}
		else {
			spdlog::warn("{} 尝试卸载不存在的音效: {}", mLogTag.data(), filePath.data());
		}
	}

	void AudioManager::clearSounds() {
		if (!mSounds.empty()) {
			spdlog::debug("{} 正在清除所有{}个缓存的音效", mLogTag.data(), mSounds.size());
			mSounds.clear();
		}
	}

	Mix_Music* AudioManager::loadMusic(std::string_view filePath) {
		// 检查缓存
		auto iter = mMusic.find(std::string(filePath));
		if (iter != mMusic.end()) {
			return iter->second.get();
		}

		// 加载音乐
		spdlog::debug("{} 加载音乐: {}", mLogTag.data(), filePath);
		Mix_Music* rawMusic = Mix_LoadMUS(filePath.data());
		if (!rawMusic) {
			spdlog::error("{} 加载音乐失败: '{}': {}", mLogTag.data(), filePath.data(), SDL_GetError());
			return nullptr;
		}

		// 存储
		mMusic.emplace(filePath, std::unique_ptr<Mix_Music, SDLMixMusicDeleter>(rawMusic));
		spdlog::debug("{} 成功加载并缓存音乐: {}", mLogTag.data(), filePath.data());
		return rawMusic;
	}

	Mix_Music* AudioManager::getMusic(std::string_view filePath) {
		auto iter = mMusic.find(std::string(filePath));
		if (iter != mMusic.end()) {
			return iter->second.get();
		}
		spdlog::warn("{} 音乐 '{}' 未找到缓存, 尝试加载", mLogTag.data(), filePath.data());
		return loadMusic(filePath);
	}

	void AudioManager::unloadMusic(std::string_view filePath) {
		auto iter = mMusic.find(std::string(filePath));
		if (iter != mMusic.end()) {
			spdlog::debug("{} 卸载音乐: {}", mLogTag.data(), filePath.data());
			mMusic.erase(iter);
		}
		else {
			spdlog::warn("{} 尝试卸载不存在的音乐: {}", mLogTag.data(), filePath.data());
		}
	}

	void AudioManager::clearMusic() {
		if (!mMusic.empty()) {
			spdlog::debug("{} 正在清除所有{}个缓存的音乐曲目", mLogTag.data(), mMusic.size());
			mMusic.clear();
		}
	}

	void AudioManager::clearAudio() {
		clearSounds();
		clearMusic();
	}

} // namespace engine::resource
