#include "combat_resolve_system.h"
#include "../component/stats_component.h"
#include "../component/player_component.h"
#include "../component/enemy_component.h"
#include "../component/blocked_by_component.h"
#include "../component/blocker_component.h"
#include "../defs/tags.h"
#include "../defs/events.h"
#include <entt/entity/registry.hpp>
#include <entt/signal/dispatcher.hpp>
#include <glm/common.hpp>
#include <spdlog/spdlog.h>

using namespace entt::literals;

namespace game::system {

    CombatResolveSystem::CombatResolveSystem(entt::registry& registry, entt::dispatcher& dispatcher)
        : mRegistry(registry), mDispatcher(dispatcher) {
        mDispatcher.sink<game::defs::AttackEvent>().connect<&CombatResolveSystem::onAttackEvent>(this);
        mDispatcher.sink<game::defs::HealEvent>().connect<&CombatResolveSystem::onHealEvent>(this);
    }

    CombatResolveSystem::~CombatResolveSystem() {
        mDispatcher.disconnect(this);
    }

    void CombatResolveSystem::onAttackEvent(const game::defs::AttackEvent& event) {
        if (!mRegistry.valid(event.mTarget)) return;
        // 根据伤害公式，让目标扣血
        auto& target_stats = mRegistry.get<game::component::StatsComponent>(event.mTarget);
        float damage = calculateEffectiveDamage(event.mDamage, target_stats.mDef);
        target_stats.mHp -= damage;

        // 如果目标是玩家
        if (mRegistry.all_of<game::component::PlayerComponent>(event.mTarget)) {
            spdlog::info("玩家 ID: {} 受到 ID: {} 的伤害, 剩余生命值: {}",
                entt::to_integral(event.mTarget), entt::to_integral(event.mAttacker), target_stats.mHp);
            // 死亡情况
            if (target_stats.mHp <= 0) {
                target_stats.mHp = 0;
                mRegistry.emplace<game::defs::DeadTag>(event.mTarget);
                spdlog::info("玩家 ID: {} 死亡", entt::to_integral(event.mTarget));
                // NOTE: 可添加死亡特效, 统计信息等
            // 受伤情况
            }
            else if (target_stats.mHp < target_stats.mMaxHp) {
                mRegistry.emplace_or_replace<game::defs::InjuredTag>(event.mTarget);
            }
            return;
        }

        // 如果目标是敌人
        if (mRegistry.all_of<game::component::EnemyComponent>(event.mTarget)) {
            spdlog::info("敌人 ID: {} 受到 ID: {} 的伤害, 剩余生命值: {}",
                entt::to_integral(event.mTarget), entt::to_integral(event.mAttacker), target_stats.mHp);
            // 死亡情况
            if (target_stats.mHp <= 0) {
                target_stats.mHp = 0;
                mRegistry.emplace<game::defs::DeadTag>(event.mTarget);
                spdlog::info("敌人 ID: {} 死亡", entt::to_integral(event.mTarget));
                // TODO: 添加死亡特效
                // TODO: 更新统计信息
                // 如果敌人被阻挡，减少阻挡者的阻挡计数
                if (auto blocked_by = mRegistry.try_get<game::component::BlockedByComponent>(event.mTarget); blocked_by) {
                    auto blocker_entity = blocked_by->mEntity;
                    if (mRegistry.valid(blocker_entity)) {
                        auto& blocker = mRegistry.get<game::component::BlockerComponent>(blocker_entity);
                        blocker.mCurrentCount = glm::max(0, blocker.mCurrentCount - 1);
                    }
                }
            // 受伤情况
            }
            else if (target_stats.mHp < target_stats.mMaxHp) {
                mRegistry.emplace_or_replace<game::defs::InjuredTag>(event.mTarget);
            }
            return;
        }
    }

    void CombatResolveSystem::onHealEvent(const game::defs::HealEvent& event) {
        if (!mRegistry.valid(event.mTarget)) return;
        if (!mRegistry.all_of<game::component::PlayerComponent>(event.mTarget)) return;
        // 根据治疗量，让目标回血
        auto& target_stats = mRegistry.get<game::component::StatsComponent>(event.mTarget);
        target_stats.mHp += event.mAmount;
        spdlog::info("治疗者 ID: {}, 治疗目标 ID: {}, 治疗量: {}",
            entt::to_integral(event.mHealer), entt::to_integral(event.mTarget), event.mAmount);
        // 如果治疗后满血，移除受伤标签
        if (target_stats.mHp >= target_stats.mMaxHp) {
            target_stats.mHp = target_stats.mMaxHp;
            mRegistry.remove<game::defs::InjuredTag>(event.mTarget);
        }
        // TODO: 添加治疗特效
    }

    // --- 辅助函数 ---
    float CombatResolveSystem::calculateEffectiveDamage(float attacker_atk, float target_def) {
        // 最终伤害 = 攻击力 - 防御力
        float damage = attacker_atk - target_def;
        // 最小伤害为攻击力的10%
        damage = std::max(damage, 0.1f * attacker_atk);
        return damage;
    }

} // namespace game::system
