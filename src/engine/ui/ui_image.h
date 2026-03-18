/*****************************************************************//**
 * @file   ui_image.h
 * @brief  显示纹理的ui元素
 * @version 1.0
 * 
 * @author Shallowshades
 * @date   2026.03.18
 *********************************************************************/

#pragma once

#ifndef UI_IMAGE_H
#define UI_IMAGE_H

#include "ui_element.h"
#include "../render/sprite.h"
#include <string>
#include <string_view>
#include <optional>
#include <SDL3/SDL_rect.h>

namespace engine::ui {

/**
 * @brief 一个用于显示纹理或者部分纹理的UI元素.
 * 
 * 继承自UIElement并添加了渲染图像的功能
 */
class UIImage final : public UIElement {
public:
public:
    /**
     * @brief 构造一个UIImage对象。（通过纹理路径构造）
     *
     * @param texturePath 要显示的纹理路径。
     * @param position 图像的局部位置。
     * @param size 图像元素的大小。（如果为{0,0}，则使用纹理的原始尺寸）
     * @param sourceRect 可选：要绘制的纹理部分。（如果为空，则使用纹理的整个区域）
     * @param isFlipped 可选：精灵是否应该水平翻转。
     */
    UIImage(std::string_view texturePath,
        glm::vec2 position = { 0.0f, 0.0f },
        glm::vec2 size = { 0.0f, 0.0f },
        std::optional<engine::utils::Rect> sourceRect = std::nullopt,
        bool isFlipped = false);

    /**
     * @brief 构造一个UIImage对象。（通过纹理ID构造）
     *
     * @param textureId 要显示的纹理ID。
     * @param position 图像的局部位置。
     * @param size 图像元素的大小。（如果为{0,0}，则使用纹理的原始尺寸）
     * @param sourceRect 可选：要绘制的纹理部分。（如果为空，则使用纹理的整个区域）
     * @param isFlipped 可选：精灵是否应该水平翻转。
     * @note 用此方法，需确保对应ID的纹理已经加载到ResourceManager中，因此不需要再提供纹理路径。
     */
    UIImage(entt::id_type textureId,
        glm::vec2 position = { 0.0f, 0.0f },
        glm::vec2 size = { 0.0f, 0.0f },
        std::optional<engine::utils::Rect> sourceRect = std::nullopt,
        bool isFlipped = false);

    /**
     * @brief 构造一个UIImage对象。（通过Sprite对象构造）
     *
     * @param sprite 要显示的Sprite对象。
     * @param position 图像的局部位置。
     * @param size 图像元素的大小。（如果为{0,0}，则使用纹理的原始尺寸）
     */
    UIImage(engine::render::Sprite& sprite,
        glm::vec2 position = { 0.0f, 0.0f },
        glm::vec2 size = { 0.0f, 0.0f });

    // --- 核心方法 ---
    void render(engine::core::Context& context) override;

    // --- Setters & Getters ---
    const engine::render::Sprite& getSprite() const { return mSprite; }
    void setSprite(const engine::render::Sprite& sprite) { mSprite = sprite; }

    entt::id_type getTextureId() const { return mSprite.getTextureId(); }
    void setTextureId(entt::id_type textureId) { mSprite.setTextureId(textureId); }

	std::string_view getTexturePath() const { return mSprite.getTexturePath(); }
	void setTexture(std::string_view texturePath) { mSprite.setTexture(texturePath); }

    const std::optional<engine::utils::Rect>& getsourceRect() const { return mSprite.getSourceRect(); }
    void setsourceRect(const std::optional<engine::utils::Rect>& sourceRect) { mSprite.setSourceRect(sourceRect); }

    bool isFlipped() const { return mSprite.isFlipped(); }
    void setFlipped(bool flipped) { mSprite.setFlipped(flipped); }

protected:
    engine::render::Sprite mSprite;
};
}

#endif // !UI_IMAGE_H
