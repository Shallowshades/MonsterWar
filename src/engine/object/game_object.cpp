#include "game_object.h"
#include "../render/renderer.h"
#include "../input/input_manager.h"
#include "../render/camera.h"
#include <spdlog/spdlog.h>

namespace engine::object {
GameObject::GameObject(std::string_view name, std::string_view tag)
	: mName(name), mTag(tag)
{
	spdlog::trace("{} created: {} {}", mLogTag.data(), mName, mTag);
}

void GameObject::setName(std::string_view name){
	mName = name;
}

std::string_view GameObject::getName() const{
	return mName;
}

void GameObject::setTag(std::string_view tag){
	mTag = tag;
}

std::string_view GameObject::getTag() const{
	return mTag;
}

void GameObject::setNeedRemove(bool needRemove) {
	mNeedRemove = needRemove;
}

bool GameObject::isNeedRemove() const{
	return mNeedRemove;
}

void GameObject::update(float deltaTime, engine::core::Context& context){
	// 遍历所有组件并调用他们的update方法
	for (auto& pair : mComponents) {
		pair.second->update(deltaTime, context);
	}
}

void GameObject::render(engine::core::Context& context){
	// 遍历所有组件并调用他们的render方法
	for (auto& pair : mComponents) {
		pair.second->render(context);
	}
}

void GameObject::clean(){
	spdlog::trace("{} cleaning GameObject '{}', '{}' ...", mLogTag.data(), mName, mTag);
	// 遍历所有组件并调用他们的clean方法
	for (auto& pair : mComponents) {
		pair.second->clean();
	}
	mComponents.clear();
}

void GameObject::handleInput(engine::core::Context& context){
	// 遍历所有组件并调用他们的handleInput方法
	for (auto& pair : mComponents) {
		pair.second->handleInput(context);
	}
}
} // namespace engine::object
