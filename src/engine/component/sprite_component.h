/*****************************************************************//**
 * @file   sprite_component.h
 * @brief  精灵组件
 * @version 1.0
 *
 * @author Shallowshades
 * @date   2026.07.14
 *********************************************************************/

#pragma once
#ifndef SPRITE_COMPONENT_H
#define SPRITE_COMPONENT_H

#include "../utils/math.h"
#include <SDL3/SDL_rect.h>
#include <entt/core/hashed_string.hpp>
#include <entt/entity/entity.hpp>
#include <glm/vec2.hpp>
#include <glm/common.hpp>
#include <utility>
#include <string>

namespace engine::component {
    /**
     * @brief 精灵数据结构
     *
     * 包含纹理名称、源矩形和是否翻转。
     */
    struct Sprite {
        entt::id_type mTextureId{ entt::null };                         ///< @brief 纹理ID
        std::string mTexturePath;                                       ///< @brief 纹理路径
        engine::utils::Rect mSourceRect{};                              ///< @brief 源矩形(为了保证效率，不再使用std::optional，构造时必须提供)
        bool mIsFlipped{ false };                                      ///< @brief 是否翻转

        Sprite() = default;                                             ///< @brief 空的构造函数

        /**
         * @brief 构造函数 (通过纹理路径构造)
         * @param texture_path 纹理路径
         * @param source_rect 源矩形
         * @param is_flipped 是否翻转，默认false
         */
        Sprite(std::string texture_path, engine::utils::Rect source_rect, bool is_flipped = false)
            : mTexturePath(std::move(texture_path)), mSourceRect(std::move(source_rect)), mIsFlipped(is_flipped) {
            mTextureId = entt::hashed_string(mTexturePath.c_str());
        }

        /**
         * @brief 构造函数 (通过纹理ID构造)
         * @param texture_id 纹理ID
         * @param source_rect 源矩形
         * @param is_flipped 是否翻转，默认false
         * @note 用此方法，需确保对应ID的纹理已经加载到ResourceManager中，因此不需要再提供纹理路径。
         */
        Sprite(entt::id_type texture_id, engine::utils::Rect source_rect, bool is_flipped = false)
            : mTextureId(texture_id), mSourceRect(std::move(source_rect)), mIsFlipped(is_flipped) {
        }
    };

    /**
     * @brief 精灵组件
     *
     * 包含精灵、大小、偏移和是否可见。
     */
    struct SpriteComponent {
        Sprite mSprite;                                             ///< @brief 精灵
        glm::vec2 mSize{ 0.0f };                                    ///< @brief 大小
        glm::vec2 mOffset{ 0.0f };                                  ///< @brief 偏移
        bool mIsVisible{ true };                                    ///< @brief 是否可见

        /**
         * @brief 构造函数
         * @param sprite 精灵
         * @param size 大小
         * @param offset 偏移
         * @param is_visible 是否可见，默认true
         */
        SpriteComponent(Sprite sprite, glm::vec2 size = glm::vec2(0.0f, 0.0f), glm::vec2 offset = glm::vec2(0.0f, 0.0f), bool is_visible = true)
            : mSprite(std::move(sprite)), mSize(std::move(size)), mOffset(std::move(offset)), mIsVisible(is_visible) {
            // 如果size为0（未提供），则使用精灵的源矩形大小
            if (glm::all(glm::equal(size, glm::vec2(0.0f)))) {
                mSize = glm::vec2(mSprite.mSourceRect.mSize.x, mSprite.mSourceRect.mSize.y);
            }
        }
    };
}
#endif
