#include "animation_event_system.h"
#include "../component/enemy_component.h"
#include "../component/player_component.h"
#include "../component/blocked_by_component.h"
#include "../component/target_component.h"
#include "../component/stats_component.h"
#include "../component/projectile_component.h"
#include "../defs/tags.h"
#include "../defs/events.h"
#include "../../engine/component/transform_component.h"
#include <entt/entity/registry.hpp>
#include <entt/signal/dispatcher.hpp>
#include <entt/core/hashed_string.hpp>
#include <spdlog/spdlog.h>

using namespace entt::literals;

namespace game::system {

    AnimationEventSystem::AnimationEventSystem(entt::registry& registry, entt::dispatcher& dispatcher)
        : mRegistry(registry), mDispatcher(dispatcher) {
        mDispatcher.sink<engine::utils::AnimationEvent>().connect<&AnimationEventSystem::onAnimationEvent>(this);
    }

    AnimationEventSystem::~AnimationEventSystem() {
        mDispatcher.disconnect(this);
    }

    void AnimationEventSystem::onAnimationEvent(const engine::utils::AnimationEvent& event) {
        if (!mRegistry.valid(event.mEntity)) return;
        // 根据不同的事件id，调用不同的处理函数
        if (event.mEventNameId == "hit"_hs) {
            handleHitEvent(event);
        }
        else if (event.mEventNameId == "emit"_hs) {
            handleEmitEvent(event);
        }
        // TODO: 其他事件类型
    }

    void AnimationEventSystem::handleHitEvent(const engine::utils::AnimationEvent& event) {
        // 玩家命中事件：治疗或攻击当前目标
        if (mRegistry.all_of<game::component::PlayerComponent>(event.mEntity)) {
            // 命中时有可能目标已经解锁，因此需要检查
            if (auto target_component = mRegistry.try_get<game::component::TargetComponent>(event.mEntity); target_component) {
                const auto& stats_component = mRegistry.get<game::component::StatsComponent>(event.mEntity);
                // 根据玩家职业，执行治疗或攻击（事件）
                if (mRegistry.all_of<game::defs::HealerTag>(event.mEntity)) {
                    mDispatcher.enqueue(game::defs::HealEvent{ event.mEntity, target_component->mEntity, stats_component.mAtk });
                }
                else {
                    mDispatcher.enqueue(game::defs::AttackEvent{ event.mEntity, target_component->mEntity, stats_component.mAtk });
                }
                // 播放"hit"音效
                mDispatcher.enqueue(engine::utils::PlaySoundEvent{ event.mEntity, "hit"_hs });
            }
            return;
        }

        // 敌人命中事件：对阻挡者造成伤害
        if (mRegistry.all_of<game::component::EnemyComponent>(event.mEntity)) {
            // 命中时有可能目标（阻挡者）已经解锁，因此需要检查
            if (auto blocked_by_component = mRegistry.try_get<game::component::BlockedByComponent>(event.mEntity); blocked_by_component) {
                const auto& stats_component = mRegistry.get<game::component::StatsComponent>(event.mEntity);
                // 执行攻击事件
                mDispatcher.enqueue(game::defs::AttackEvent{ event.mEntity, blocked_by_component->mEntity, stats_component.mAtk });
            }
            // NOTE: 只有远程敌人才有Target组件，但远程攻击动画事件id为"emit"，不在这里处理
            // NOTE: 敌人命中事件不播放音效，未来如果需要可以补充
        }
    }

    void AnimationEventSystem::handleEmitEvent(const engine::utils::AnimationEvent& event) {
        // 发射事件：从角色身上找到投射物id，并执行发射投射物事件
        if (!mRegistry.valid(event.mEntity)) return;

        // 一次获取所有必要（且肯定存在的）组件
        const auto [transform, stats, projectile_id] = mRegistry.get<engine::component::TransformComponent,
            game::component::StatsComponent,
            game::component::ProjectileIDComponent>(event.mEntity);

        // 确认"目标组件"依然存在，且其中的实体也有效
        auto target = mRegistry.try_get<game::component::TargetComponent>(event.mEntity);
        if (!target || !mRegistry.valid(target->mEntity)) return;

        // 发射投射物事件
        mDispatcher.enqueue(game::defs::EmitProjectileEvent{ projectile_id.mId,
            target->mEntity,
            transform.mPosition,
            mRegistry.get<engine::component::TransformComponent>(target->mEntity).mPosition,
            stats.mAtk });

        // 播放"emit"音效
        mDispatcher.enqueue(engine::utils::PlaySoundEvent{ event.mEntity, "emit"_hs });
    }

} // namespace game::system
