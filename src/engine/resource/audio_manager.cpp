#include "audio_manager.h"
#include <SDL3_mixer/SDL_mixer.h>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <entt/core/hashed_string.hpp>

namespace engine::resource {
// 构造函数：初始化SDL_mixer并创建混音器
AudioManager::AudioManager() {
	if (!MIX_Init()) {
		throw std::runtime_error("AudioManager 错误: MIX_Init 失败: " + std::string(SDL_GetError()));
	}

	// SDL3_mixer 3.2：MIX_CreateMixerDevice 打开默认播放设备。
	// 音频格式传 nullptr，交给 SDL_mixer 自动选择，内部会处理所有数据转换。
	if (!(mMixer = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr))) {
		MIX_Quit(); // 如果创建混音器失败，先清理MIX_Init，再抛出异常
		throw std::runtime_error("AudioManager 错误: MIX_CreateMixerDevice 失败: " + std::string(SDL_GetError()));
	}
	spdlog::trace("AudioManager 构造成功。");
}

AudioManager::~AudioManager() {
	// 立即停止所有音轨播放
	MIX_StopAllTracks(mMixer, 0);

	// 清理资源映射（unique_ptrs会调用删除器）
	clearSounds();
	clearMusic();

	// 销毁混音器（同时关闭音频设备）
	MIX_DestroyMixer(mMixer);
	mMixer = nullptr;

	// 退出SDL_mixer子系统
	MIX_Quit();
	spdlog::trace("AudioManager 析构成功。");
}

// --- 音效管理 ---
MIX_Audio* AudioManager::loadSound(entt::id_type id, std::string_view filePath) {
	// 首先检查缓存
	auto it = mSounds.find(id);
	if (it != mSounds.end()) {
		return it->second.get();
	}

	// 加载音效（预解码，反复播放时无需每次解码）
	spdlog::debug("加载音效: {}", id);
	MIX_Audio* rawAudio = MIX_LoadAudio(mMixer, filePath.data(), true);
	if (!rawAudio) {
		spdlog::error("加载音效失败: '{}': {}", id, SDL_GetError());
		return nullptr;
	}

	// 使用unique_ptr存储在缓存中
	mSounds.emplace(id, std::unique_ptr<MIX_Audio, SDLMixAudioDeleter>(rawAudio));
	spdlog::debug("成功加载并缓存音效: {}", id);
	return rawAudio;
}

MIX_Audio* AudioManager::loadSound(entt::hashed_string strHash) {
	return loadSound(strHash.value(), strHash.data());
}

MIX_Audio* AudioManager::getSound(entt::id_type id, std::string_view filePath) {
	auto it = mSounds.find(id);
	if (it != mSounds.end()) {
		return it->second.get();
	}
	// 如果未找到，判断是否提供了file_path
	if (filePath.empty()) {
		spdlog::error("音效 '{}' 未找到缓存，且未提供文件路径，返回nullptr。", id);
		return nullptr;
	}

	spdlog::warn("音效 '{}' 未找到缓存，尝试加载。", id);
	return loadSound(id, filePath);
}

MIX_Audio* AudioManager::getSound(entt::hashed_string strHash) {
	return getSound(strHash.value(), strHash.data());
}

void AudioManager::unloadSound(entt::id_type id) {
	auto it = mSounds.find(id);
	if (it != mSounds.end()) {
		spdlog::debug("卸载音效: {}", id);
		mSounds.erase(it);      // unique_ptr处理MIX_DestroyAudio
	}
	else {
		spdlog::warn("尝试卸载不存在的音效: id = {}", id);
	}
}

void AudioManager::clearSounds() {
	if (!mSounds.empty()) {
		spdlog::debug("正在清除所有 {} 个缓存的音效。", mSounds.size());
		mSounds.clear(); // unique_ptr处理删除
	}
}

// --- 音乐管理 ---
MIX_Audio* AudioManager::loadMusic(entt::id_type id, std::string_view filePath) {
	// 首先检查缓存
	auto it = mMusic.find(id);
	if (it != mMusic.end()) {
		return it->second.get();
	}

	// 加载音乐（不预解码，边播边解码，节省内存）
	spdlog::debug("加载音乐: {}", id);
	MIX_Audio* rawAudio = MIX_LoadAudio(mMixer, filePath.data(), false);
	if (!rawAudio) {
		spdlog::error("加载音乐失败: '{}': {}", id, SDL_GetError());
		return nullptr;
	}

	// 使用unique_ptr存储在缓存中
	mMusic.emplace(id, std::unique_ptr<MIX_Audio, SDLMixAudioDeleter>(rawAudio));
	spdlog::debug("成功加载并缓存音乐: {}", id);
	return rawAudio;
}

MIX_Audio* AudioManager::loadMusic(entt::hashed_string strHash) {
	return loadMusic(strHash.value(), strHash.data());
}

MIX_Audio* AudioManager::getMusic(entt::id_type id, std::string_view filePath) {
	auto it = mMusic.find(id);
	if (it != mMusic.end()) {
		return it->second.get();
	}
	// 如果未找到，判断是否提供了file_path
	if (filePath.empty()) {
		spdlog::error("音乐 '{}' 未找到缓存，且未提供文件路径，返回nullptr。", id);
		return nullptr;
	}

	spdlog::warn("音乐 '{}' 未找到缓存，尝试加载。", id);
	return loadMusic(id, filePath);
}

MIX_Audio* AudioManager::getMusic(entt::hashed_string strHash) {
	return getMusic(strHash.value(), strHash.data());
}

void AudioManager::unloadMusic(entt::id_type id) {
	auto it = mMusic.find(id);
	if (it != mMusic.end()) {
		spdlog::debug("卸载音乐: {}", id);
		mMusic.erase(it); // unique_ptr处理MIX_DestroyAudio
	}
	else {
		spdlog::warn("尝试卸载不存在的音乐: id = {}", id);
	}
}

void AudioManager::clearMusic() {
	if (!mMusic.empty()) {
		spdlog::debug("正在清除所有 {} 个缓存的音乐曲目。", mMusic.size());
		mMusic.clear(); // unique_ptr处理删除
	}
}

void AudioManager::clearAudio() {
	clearSounds();
	clearMusic();
}
} // namespace engine::resource
