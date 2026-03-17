/*****************************************************************//**
 * @file   parallax_component.h
 * @brief  视差滚动效果组件
 * @version 1.0
 * 
 * @author Shallowshades
 * @date   2025.11.05
 *********************************************************************/

#pragma once
#ifndef PARALLAX_COMPONENT_H
#define PARALLAX_COMPONENT_H

#include <string>
#include <string_view>
#include <glm/glm.hpp>
#include "component.h"
#include "../render/sprite.h"

namespace engine::component {
class TransformComponent;
/**
 * @brief 在背景中渲染可滚动纹理的组件, 以创建视差效果.
 * 
 * 该组件根据相机的位置和滚动因子来移动纹理.
 */
class ParallaxComponent final : public engine::component::Component {
	friend class engine::object::GameObject;
public:
	/**
	 * @brief 构造函数.
	 * 
	 * @param textureId 资源id
	 * @param scrollFactor 控制背景相同对于相机移动速度的因子
	 *	- (0, 0)		表示完全静止
	 *	- (1, 1)		表示与相机完全同步移动
	 *	- (0.5, 0.5)	表示以相机一般的速度移动
	 */
	ParallaxComponent(std::string_view textureId, const glm::vec2& scrollFactor, const glm::bvec2& repeat);

	void setSprite(const engine::render::Sprite& sprite);					///< @brief 设置精灵对象
	void setScrollFactor(const glm::vec2& scrollFactor);					///< @brief 设置滚动速度因子
	void setRepeat(const glm::bvec2& repeat);								///< @brief 设置是否重复
	void setHidden(bool hidden);											///< @brief 设置是否隐藏

	const engine::render::Sprite& getSprite() const;						///< @brief 获取精灵对象
	const glm::vec2& getScrollFactor() const;								///< @brief 获取滚动速度因子
	const glm::bvec2& getRepeat() const;									///< @brief 获取是否重复
	bool getIsHidden() const;												///< @brief 获取是否隐藏

protected:
	// 核心循环函数重写
	void init() override;
	void update(float, engine::core::Context&) override;
	void render(engine::core::Context& context) override;
private:
	static constexpr std::string_view mLogTag = "ParallaxComponent";		///< @brief 日志标识

	TransformComponent* mTransform = nullptr;								///< @brief 缓存变化组件
	engine::render::Sprite mSprite;											///< @brief 精灵对象
	glm::vec2 mScrollFactor;												///< @brief 滚动速度因子
	glm::bvec2 mRepeat;														///< @brief 是否沿着X和Y轴周期性重复
	bool mIsHidden = false;													///< @brief 是否隐藏
};

} // engine::component

#endif // PARALLAX_COMPONENT_H
