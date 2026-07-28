#include "projectile_system.h"
#include "../component/projectile_component.h"
#include "../defs/tags.h"
#include "../defs/events.h"
#include "../factory/entity_factory.h"
#include "../../engine/component/transform_component.h"
#include "../../engine/utils/events.h"
#include <entt/entity/registry.hpp>
#include <entt/signal/dispatcher.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/common.hpp>
#include <glm/trigonometric.hpp>
#include <spdlog/spdlog.h>

using namespace entt::literals;

namespace game::system {

    ProjectileSystem::ProjectileSystem(entt::registry& registry, entt::dispatcher& dispatcher, game::factory::EntityFactory& entity_factory)
        : mRegistry(registry), mDispatcher(dispatcher), mEntityFactory(entity_factory) {
        mDispatcher.sink<game::defs::EmitProjectileEvent>().connect<&ProjectileSystem::onEmitProjectileEvent>(this);
    }

    ProjectileSystem::~ProjectileSystem() {
        mDispatcher.disconnect(this);
    }

    void ProjectileSystem::update(float delta_time) {
        // 获取所有投射物
        auto view = mRegistry.view<game::component::ProjectileComponent, engine::component::TransformComponent>();
        for (auto entity : view) {
            auto& projectile = mRegistry.get<game::component::ProjectileComponent>(entity);
            auto& transform = mRegistry.get<engine::component::TransformComponent>(entity);
            // 更新飞行时间
            projectile.mCurrentFlightTime += delta_time;
            // 如果飞行时间超过总飞行时间，则命中目标（发送攻击事件以及播放音效）并销毁
            if (projectile.mCurrentFlightTime >= projectile.mTotalFlightTime) {
                mDispatcher.enqueue(game::defs::AttackEvent{ entity, projectile.mTarget, projectile.mDamage });
                mDispatcher.enqueue(engine::utils::PlaySoundEvent{ entity, "hit"_hs });
                mRegistry.emplace<game::defs::DeadTag>(entity);
                continue;
            }
            // 计算飞行进度 (t 从 0 到 1)
            float t = projectile.mCurrentFlightTime / projectile.mTotalFlightTime;
            t = glm::clamp(t, 0.0f, 1.0f); // 确保 t 在 [0, 1] 区间

            // 1. 计算水平位置 (线性插值)
            glm::vec2 horizontal_pos = glm::mix(projectile.mStartPosition, projectile.mTargetPosition, t);

            // 2. 计算垂直方向的弧线偏移
            // 使用 sin 函数可以轻松创建弧线: sin(0)=0, sin(PI/2)=1, sin(PI)=0
            float arc_offset = glm::sin(t * glm::pi<float>()) * projectile.mArcHeight;

            // 3. 合成最终位置
            transform.mPosition = horizontal_pos;
            transform.mPosition.y -= arc_offset; // Y轴向下为正，所以减去偏移使其向上拱起

            // 4. 根据上一帧的位置计算朝向，并更新TransformComponent的旋转参数
            auto direction = transform.mPosition - projectile.mPreviousPosition;
            transform.mRotation = glm::atan(direction.y, direction.x) * 180.0f / glm::pi<float>();

            // 5. 更新上一帧的位置
            projectile.mPreviousPosition = transform.mPosition;
        }
    }

    void ProjectileSystem::onEmitProjectileEvent(const game::defs::EmitProjectileEvent& event) {
        spdlog::info("发射投射物: {}", event.mId);
        mEntityFactory.createProjectile(event.mId,
            event.mStartPosition,
            event.mTargetPosition,
            event.mTarget,
            event.mDamage);
    }

}   // namespace game::system
