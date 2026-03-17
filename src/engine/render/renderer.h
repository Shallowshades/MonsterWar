/*****************************************************************//**
 * @file   renderer.h
 * @brief  渲染器
 * @version 1.0
 * 
 * @author 29230
 * @date   October 2025
 *********************************************************************/

#pragma once
#ifndef RENDERER_H
#define RENDERER_H

#include <string>
#include <optional>
#include <glm/glm.hpp>

#include "sprite.h"
#include "../utils/math.h"

struct SDL_Renderer;
struct SDL_FRect;
struct SDL_FColor;

namespace engine::resource {
	class ResourceManager;
}

namespace engine::render {
class Camera;
/**
 * @brief.
 */
class Renderer final {
public:
	/**
	 * @brief 构造函数.
	 * 
	 * @param renderer 指向有效的SDL_Renderer的指针, 不能为空
	 * @param resourceManager 指向有效的ResourceManager的指针, 不能为空
	 * @throws 如果任一指针为nullptr则抛出std::runtime_error
	 */
	Renderer(SDL_Renderer* renderer, engine::resource::ResourceManager* resourceManager);

	/**
	 * @brief 绘制精灵.
	 *
	 * @param sprite 包含纹理ID, 源矩阵和翻转状态的Sprite对象
	 * @param positioin 世界坐标中的坐上角位置
	 * @param scale 缩放因子
	 * @param angle 旋转角度(度)
	 */
	void drawSprite(const Camera& camera, const Sprite& sprite, const glm::vec2& positioin, const glm::vec2& scale = glm::vec2(1.f), double angle = 0.f);
	
	/**
	 * @brief 绘制视差滚动背景.
	 *
	 * @param sprite 包含纹理ID, 源矩阵和翻转状态的Sprite对象
	 * @param positioin 世界坐标中的坐上角位置
	 * @param scrollFactor 滚动因子
	 * @param scale 缩放因子
	 */
	void drawParallax(const Camera& camera, const Sprite& sprite, const glm::vec2& position, const glm::vec2& scrollFactor, const glm::bvec2& repeat = glm::bvec2(true), const glm::vec2& scale = glm::vec2(1.f));
	
	/**
	 * @brief 在屏幕坐标中直接渲染一个用于UI的Sprite对象.
	 *
	 * @param sprite 包含纹理ID, 源矩阵和翻转状态的Sprite对象
	 * @param positioin 世界坐标中的坐上角位置
	 * @param size 可选: 目标矩阵的大小. 如果为std::nullopt, 则使用Sprite的原始大小
	 */
	void drawUISprite(const Sprite& sprite, const glm::vec2& position, const std::optional<glm::vec2>& size = std::nullopt);
	
	/**
	 * @brief 绘制填充矩形.
	 * 
	 * @param rect 矩形区域
	 * @param color 填充颜色
	 */
	void drawUIFilledRect(const engine::utils::Rect& rect, const engine::utils::FColor& color);

	void present();															///< @brief 更新屏幕, 包装SDL_RenderPresent函数
	void clearScreen();														///< @brief 清屏, 包装SDL_RenderClear函数

	void setDrawColor(Uint8 r, Uint8 g, Uint8 b, Uint8 a = 255);			///< @brief 设置绘制颜色, 包装SDL_SetRenderDrawColor函数, 使用Uint8类型
	void setDrawColorFloat(float r, float g, float b, float a = 1.f);		///< @brief 设置绘制颜色, 包装SDL_SetRenderDrawColorFloat函数, 使用float类型

	SDL_Renderer* getSDLRenderer() const;									///< @brief 获取底层的SDL_Renderer指针

	// 禁用拷贝和移动语义
	Renderer(const Renderer&) = delete;										///< @brief 删除拷贝构造
	Renderer& operator=(const Renderer&) = delete;							///< @brief 删除拷贝赋值构造
	Renderer(Renderer&&) = delete;											///< @brief 删除移动构造
	Renderer& operator=(Renderer&&) = delete;								///< @brief 删除移动赋值构造

private:
	std::optional<SDL_FRect> getSpriteSourceRect(const Sprite& sprite);		///< @brief 获取精灵的源矩阵, 用于具体绘制. 出现错误则返回std::nullopt并跳过绘制
	bool isRectInViewPort(const Camera& camera, const SDL_FRect& rect);		///< @brief 判断矩形是否在视口中, 用于视口裁剪
private:
	static constexpr std::string_view mLogTag = "Renderer";
	SDL_Renderer* mRenderer = nullptr;										///< @brief 指向SDL_Renderer的非拥有指针
	engine::resource::ResourceManager* mResourceManager = nullptr;			///< @brief 指向ResourceManager的非拥有指针
};
}

#endif // RENDERER_H
