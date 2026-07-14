/*****************************************************************//**
 * @file   Image.h
 * @brief  精灵图类
 * @version 2.0
 * 
 * @author Shallowshades
 * @date   2026.03.19
 *********************************************************************/
#pragma once
#ifndef IMAGE_H
#define IMAGE_H

#pragma once
#include "../utils/math.h"
#include <optional>
#include <string>
#include <string_view>
#include <entt/core/hashed_string.hpp>
#include <entt/entity/entity.hpp>

namespace engine::render {

/**
    * @brief 表示要绘制的视觉UI图片的数据。
    *
    * 包含纹理标识符、要绘制的纹理部分（源矩形）以及翻转状态。
    * 位置、缩放和旋转由外部标识。
    * 渲染工作由 Renderer 类完成。（传入Image作为参数）
    */
class Image final {
public:
    /**
    * @brief 默认构造函数（创建一个空的/无效的精灵）
    */
    Image() = default;

    /**
    * @brief 构造一个精灵 （通过纹理路径构造）
    *
    * @param texturePath 纹理资源的文件路径。不应为空。
    * @param sourceRect 可选的源矩形（SDL_FRect），定义要使用的纹理部分。如果为 std::nullopt，则使用整个纹理。
    * @param isFlipped 是否水平翻转
    */
    Image(std::string_view texturePath, std::optional<engine::utils::Rect> sourceRect = std::nullopt, bool isFlipped = false)
        : mTexturePath(texturePath.data()),
        mTextureId(entt::hashed_string(texturePath.data())),
        mSourceRect(std::move(sourceRect)),
        mIsFlipped(isFlipped)
    {}

    /**
    * @brief 构造一个精灵 （通过纹理ID构造）
    *
    * @param textureId 纹理资源的标识符。不应为空。
    * @param sourceRect 可选的源矩形（SDL_FRect），定义要使用的纹理部分。如果为 std::nullopt，则使用整个纹理。
    * @param isFlipped 是否水平翻转
    * @note 用此方法，需确保对应ID的纹理已经加载到ResourceManager中，因此不需要再提供纹理路径。
    */
    Image(entt::id_type textureId, std::optional<engine::utils::Rect> sourceRect = std::nullopt, bool isFlipped = false)
        : mTextureId(textureId),
        mSourceRect(std::move(sourceRect)),
        mIsFlipped(isFlipped)
    {}

    // --- getters and setters ---
    std::string_view getTexturePath() const { return mTexturePath; }                           ///< @brief 获取纹理路径
    entt::id_type getTextureId() const { return mTextureId; }                                  ///< @brief 获取纹理 ID
    const std::optional<engine::utils::Rect>& getSourceRect() const { return mSourceRect; }    ///< @brief 获取源矩形 (如果使用整个纹理则为 std::nullopt)
    bool isFlipped() const { return mIsFlipped; }                                              ///< @brief 获取是否水平翻转

    /**
    * @brief 设置纹理路径同时更新纹理ID
    * @param texturePath 纹理资源的文件路径。不应为空。
    */
    void setTexture(std::string_view texturePath) {
        mTexturePath = texturePath.data();
        mTextureId = entt::hashed_string(texturePath.data());
    }

    void setTextureId(entt::id_type textureId) { mTextureId = textureId; }   ///< @brief 设置纹理ID (需确保已载入)

    /**
	* @brief 设置源矩形 (如果使用整个纹理则为 std::nullopt)
	* @param sourceRect 源矩形。如果使用整个纹理则为 std::nullopt
	*/
    void setSourceRect(std::optional<engine::utils::Rect> sourceRect) { mSourceRect = std::move(sourceRect); }

    /**
	* @brief 设置是否水平翻转
	* @param flipped 是否水平翻转
	*/
    void setFlipped(bool flipped) { mIsFlipped = flipped; }

private:
	std::string mTexturePath;                                       ///< @brief 纹理资源的文件路径
	entt::id_type mTextureId{ entt::null };                      ///< @brief 纹理资源的标识符 (entt::null是推荐的初始化方式，表示无效的ID)
	std::optional<engine::utils::Rect> mSourceRect;                 ///< @brief 可选：要绘制的纹理部分
	bool mIsFlipped = false;                                        ///< @brief 是否水平翻转
};

} // namespace engine::render

#endif // IMAGE_H
