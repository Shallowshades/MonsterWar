/*****************************************************************//**
 * @file   ui_image.h
 * @brief  显示纹理的ui元素
 * @version 1.0
 * 
 * @author Shallowshades
 * @date   2026.01.19
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
    /**
     * @brief 构造一个UIImage对象。
     *
     * @param texture_id 要显示的纹理ID。
     * @param position 图像的局部位置。
     * @param size 图像元素的大小。（如果为{0,0}，则使用纹理的原始尺寸）
     * @param source_rect 可选：要绘制的纹理部分。（如果为空，则使用纹理的整个区域）
     * @param is_flipped 可选：精灵是否应该水平翻转。
     */
    UIImage(std::string_view texture_id,
        const glm::vec2& position = { 0.0f, 0.0f },
        const glm::vec2& size = { 0.0f, 0.0f },
        const std::optional<SDL_FRect>& source_rect = std::nullopt,
        bool is_flipped = false);

    // --- 核心方法 ---
    void render(engine::core::Context& context) override;

    // --- Setters & Getters ---
    const engine::render::Sprite& getSprite() const { return mSprite; }
    void setSprite(const engine::render::Sprite& sprite) { mSprite = sprite; }

    std::string_view getTextureId() const { return mSprite.getTextureId(); }
    void setTextureId(std::string_view textureId) { mSprite.setTextureId(textureId); }

    const std::optional<SDL_FRect>& getSourceRect() const { return mSprite.getSourceRect(); }
    void setSourceRect(const std::optional<SDL_FRect>& sourceRect) { mSprite.setSourceRect(sourceRect); }

    bool isFlipped() const { return mSprite.isFlipped(); }
    void setFlipped(bool flipped) { mSprite.setFlipped(flipped); }

protected:
    engine::render::Sprite mSprite;
};
}

#endif // !UI_IMAGE_H
