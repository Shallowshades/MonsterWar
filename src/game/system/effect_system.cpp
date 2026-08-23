#include "effect_system.h"
#include "../factory/entity_factory.h"
#include <entt/entity/registry.hpp>
#include <entt/signal/dispatcher.hpp>

namespace game::system {

    EffectSystem::EffectSystem(entt::registry& registry, entt::dispatcher& dispatcher, game::factory::EntityFactory& entity_factory)
        : mRegistry(registry), mDispatcher(dispatcher), mEntityFactory(entity_factory) {
        mDispatcher.sink<game::defs::EnemyDeadEffectEvent>().connect<&EffectSystem::onEnemyDeadEffectEvent>(this);
        mDispatcher.sink<game::defs::EffectEvent>().connect<&EffectSystem::onEffectEvent>(this);
    }

    EffectSystem::~EffectSystem() {
        mDispatcher.disconnect(this);
    }

    void EffectSystem::onEnemyDeadEffectEvent(const game::defs::EnemyDeadEffectEvent& event) {
        mEntityFactory.createEnemyDeadEffect(event.mClassId, event.mPosition, event.mIsFlipped);
    }

    void EffectSystem::onEffectEvent(const game::defs::EffectEvent& event) {
        mEntityFactory.createEffect(event.mNameId, event.mPosition, event.mIsFlipped);
    }

} // namespace game::system
