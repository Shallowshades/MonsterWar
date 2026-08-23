#include "combat_resolve_system.h"
#include "../component/stats_component.h"
#include "../component/player_component.h"
#include "../component/enemy_component.h"
#include "../component/blocked_by_component.h"
#include "../component/blocker_component.h"
#include "../component/class_name_component.h"
#include "../data/game_stats.h"
#include "../../engine/component/transform_component.h"
#include "../../engine/component/sprite_component.h"
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
        // 如果目标无效或标记死亡，直接返回
        if (!mRegistry.valid(event.mTarget) || mRegistry.all_of<game::defs::DeadTag>(event.mTarget)) return;
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
                // 用emplace重复添加会报错，用emplace_or_replace更加健壮，可重复添加
                mRegistry.emplace_or_replace<game::defs::DeadTag>(event.mTarget);
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
                mRegistry.emplace_or_replace<game::defs::DeadTag>(event.mTarget);
                spdlog::info("敌人 ID: {} 死亡", entt::to_integral(event.mTarget));

                // 发送死亡特效事件（需要先获取class_id、位置和是否翻转）
                const auto [class_name, transform, sprite] = mRegistry.get<game::component::ClassNameComponent,
                    engine::component::TransformComponent,
                    engine::component::SpriteComponent>(event.mTarget);
                mDispatcher.enqueue(game::defs::EnemyDeadEffectEvent{ class_name.mClassId, transform.mPosition, sprite.mSprite.mIsFlipped });

                // 更新统计信息：击杀数量+1
                auto& game_stats = mRegistry.ctx().get<game::data::GameStats&>();
                game_stats.mEnemyKilledCount++;
                // 全歼敌人时，通关成功（与敌人到达时共用同一套判定逻辑，延迟切换场景）
                if ((game_stats.mEnemyKilledCount + game_stats.mEnemyArrivedCount) >= game_stats.mEnemyCount) {
                    spdlog::warn("敌人全部死亡，通关成功");
                    mDispatcher.enqueue(game::defs::LevelClearDelayedEvent{ 2.0f });
                }
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
        // 添加治疗特效
        const auto& transform = mRegistry.get<engine::component::TransformComponent>(event.mTarget);
        mDispatcher.enqueue(game::defs::EffectEvent{ "heal"_hs, transform.mPosition, false });
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
