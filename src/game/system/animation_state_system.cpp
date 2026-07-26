#include "animation_state_system.h"
#include "../component/enemy_component.h"
#include "../component/player_component.h"
#include "../component/blocked_by_component.h"
#include "../defs/tags.h"
#include <entt/entity/registry.hpp>
#include <entt/signal/dispatcher.hpp>
#include <entt/core/hashed_string.hpp>
#include <spdlog/spdlog.h>

using namespace entt::literals;

namespace game::system {

    AnimationStateSystem::AnimationStateSystem(entt::registry& registry, entt::dispatcher& dispatcher)
        : mRegistry(registry), mDispatcher(dispatcher) {
        mDispatcher.sink<engine::utils::AnimationFinishedEvent>().connect<&AnimationStateSystem::onAnimationFinishedEvent>(this);
    }

    AnimationStateSystem::~AnimationStateSystem() {
        mDispatcher.disconnect(this);
    }

    void AnimationStateSystem::onAnimationFinishedEvent(const engine::utils::AnimationFinishedEvent& event) {

        if (!mRegistry.valid(event.mEntity)) return;
        // 敌人动画结束逻辑
        if (mRegistry.all_of<game::component::EnemyComponent>(event.mEntity)) {
            // 如果敌人被阻挡，则返回idle动画
            if (auto blocked_by = mRegistry.try_get<game::component::BlockedByComponent>(event.mEntity); blocked_by) {
                mDispatcher.enqueue(engine::utils::PlayAnimationEvent{ event.mEntity, "idle"_hs, true });
                spdlog::info("敌人行动动画结束, 返回idle动画, ID: {}", entt::to_integral(event.mEntity));
                // 如果没有被阻挡，则返回walk动画
            }
            else {
                mDispatcher.enqueue(engine::utils::PlayAnimationEvent{ event.mEntity, "walk"_hs, true });
                spdlog::info("敌人行动动画结束, 没有BlockedBy组件, 返回walk动画, ID: {}", entt::to_integral(event.mEntity));
            }
            // 移除动作锁定（硬直）标签
            mRegistry.remove<game::defs::ActionLockTag>(event.mEntity);
            return;
        }

        // 玩家动画结束，直接返回idle动画
        if (mRegistry.all_of<game::component::PlayerComponent>(event.mEntity)) {
            mDispatcher.enqueue(engine::utils::PlayAnimationEvent{ event.mEntity, "idle"_hs, true });
            spdlog::info("玩家动画结束, 返回idle动画, ID: {}", entt::to_integral(event.mEntity));
            return;
        }
    }

} // namespace game::system