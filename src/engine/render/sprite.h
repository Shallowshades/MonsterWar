/*****************************************************************//**
 * @file   sprite.h
 * @brief  精灵图类
 * @version 1.0
 * 
 * @author Shallowshades
 * @date   2025.11.05
 *********************************************************************/
#pragma once
#ifndef SPRITE_H
#define SPRITE_H

#include <optional>
#include <string>
#include <string_view>
#include <SDL3/SDL_rect.h>

namespace engine::render {
/**
 * @brief 精灵类.
 */
class Sprite final {
public:
	Sprite() = default;												///< @brief 默认构造函数

	/**
	 * @brief 构造一个精灵.
	 * 
	 * @param textureId	纹理资源的标识符. 不应为空
	 * @param sourceRect 可选的源矩阵, 定义要使用的纹理部分. 如果为std::nullopt, 则使用整个纹理
	 * @param isFlipped 是否水平翻转
	 */
	Sprite(std::string_view textureId, const std::optional<SDL_FRect>& sourceRect = std::nullopt, bool isFlipped = false);

	std::string_view getTextureId() const;							///< @brief 获取纹理Id
	const std::optional<SDL_FRect>& getSourceRect() const;			///< @brief 获取源矩形
	bool isFlipped() const;											///< @brief 获取是否水平翻转

	void setTextureId(std::string_view textureId);					///< @brief 设置纹理Id
	void setSourceRect(const std::optional<SDL_FRect>& sourceRect);	///< @brief 设置源矩阵
	void setFlipped(bool flipped);									///< @brief 设置是否水平翻转

private:
	std::string mTextureId;											///< @brief 纹理资源的标识符
	std::optional<SDL_FRect> mSourceRect;							///< @brief 可选: 要绘制的纹理部分
	bool mIsFlipped;												///< @brief 是否水平翻转
};
} // namespace engine::render

#endif // SPRITE_H
