/*****************************************************************//**
 * @file   sprite_component.h
 * @brief  精灵组件
 * @version 1.0
 * 
 * @author Shallowshades
 * @date   2025.10.23
 *********************************************************************/

#pragma once
#ifndef SPRITE_COMPONENT_H
#define SPRITE_COMPONENT_H

#include <string>
#include <string_view>
#include <optional>

#include <SDL3/SDL_rect.h>
#include <glm/vec2.hpp>

#include "../render/sprite.h"
#include "component.h"
#include "../utils/alignment.h"

namespace engine::core {
	class Context;
}

namespace engine::resource {
	class ResourceManager;
}

namespace engine::component {
class TransformComponent;
/**
* @brief 管理GameObject的视觉表示, 通过持有一个Sprite对象.
*
* 协调Sprite数据和渲染逻辑, 并于TransformComponent
*/
class SpriteComponent final : public Component {
	friend class engine::object::GameObject;
public:
	/**
	* @brief 构造函数
	* 
	* @param textureId 纹理资源的标识符。
	* @param resourceManager 资源管理器指针。
	* @param alignment 初始对齐方式。
	* @param sourceRectOptional 可选的源矩形。
	* @param isFlipped 初始翻转状态。
	*/
	SpriteComponent(std::string_view textureId,
		engine::resource::ResourceManager& resourceManager,
		engine::utils::Alignment alignment = engine::utils::Alignment::NONE,
		std::optional<SDL_FRect> sourceRectOptional = std::nullopt,
		bool isFlipped = false);

	/**
	 * @brief 构造函数.
	 * 
	 * @param sprite 精灵对象
	 * @param resourceManager 资源管理器引用
	 * @param alignment 初始化对齐方式
	 */
	SpriteComponent(engine::render::Sprite&& sprite, engine::resource::ResourceManager& resourceManager, engine::utils::Alignment alignment = engine::utils::Alignment::NONE);
	~SpriteComponent() override = default;

	// 禁用拷贝和移动语义
	SpriteComponent(const SpriteComponent&) = delete;							///< @brief 删除拷贝构造
	SpriteComponent& operator=(const SpriteComponent&) = delete;				///< @brief 删除拷贝赋值构造
	SpriteComponent(SpriteComponent&&) = delete;								///< @brief 删除移动构造
	SpriteComponent& operator=(SpriteComponent&&) = delete;						///< @brief 删除移动赋值构造

	void updateOffset();														///< @brief 更新偏移量

	const engine::render::Sprite& getSprite() const;							///< @brief 获取精灵对象
	std::string_view getTextureId() const;										///< @brief 获取纹理ID
	bool isFlipped() const;														///< @brief 获取是否翻转
	bool isHidden() const;														///< @brief 获取是否隐藏
	const glm::vec2& getSpriteSize() const;										///< @brief 获取精灵尺寸
	const glm::vec2& getOffset() const;											///< @brief 获取偏移量
	engine::utils::Alignment getAlignment() const;								///< @brief 获取对齐方式

	/**
	 * @brief 通过纹理ID设置精灵对象.
	 */
	void setSpriteById(std::string_view textureId, const std::optional<SDL_FRect>& sourceRectOptional = std::nullopt);
	void setFlipped(bool flipped);												///< @brief 设置是否翻转
	void setHidden(bool hidden);												///< @brief 设置是否隐藏
	void setSourceRect(const std::optional<SDL_FRect>& sourceRectOptional);		///< @brief 设置源矩阵
	void setAlignment(engine::utils::Alignment anchor);							///< @brief 设置对齐方式

private:
	void updateSpriteSize();													///< @brief 辅助函数, 根据mSprite的sourceRect更新spriteSize

	// Component 虚函数覆盖
	void init() override;														///< @brief 初始化函数需要覆盖
	void update(float, engine::core::Context&) override {}						///< @brief 更新函数留空
	void render(engine::core::Context& context) override;						///< @brief 渲染函数需要覆盖
private:
	static constexpr std::string_view mLogTag = "SpriteComponent";

	engine::resource::ResourceManager* mResourceManager = nullptr;				///< @brief 资源管理器指针
	TransformComponent* mTransform = nullptr;									///< @brief 变换组件指针(非必须)

	engine::render::Sprite mSprite;												///< @brief 精灵对象
	engine::utils::Alignment mAlignment = engine::utils::Alignment::NONE;		///< @brief 对齐方式
	glm::vec2 mSpriteSize = { 0.f, 0.f };										///< @brief 精灵尺寸
	glm::vec2 mOffset = { 0.f, 0.f };											///< @brief 偏移量
	bool mIsHidden = false;														///< @brief 是否隐藏(不渲染)
};
} // engine::component
#endif
