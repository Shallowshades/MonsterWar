#include "effect_system.h"
#include "../defs/tags.h"
#include "../factory/entity_factory.h"
#include <entt/entity/registry.hpp>
#include <entt/signal/dispatcher.hpp>

namespace game::system {

    EffectSystem::EffectSystem(entt::registry& registry, entt::dispatcher& dispatcher, game::factory::EntityFactory& entity_factory)
        : mRegistry(registry), mDispatcher(dispatcher), mEntityFactory(entity_factory) {
        mDispatcher.sink<game::defs::EnemyDeadEffectEvent>().connect<&EffectSystem::onEnemyDeadEffectEvent>(this);
    }

    EffectSystem::~EffectSystem() {
        mDispatcher.disconnect(this);
    }

    void EffectSystem::onEnemyDeadEffectEvent(const game::defs::EnemyDeadEffectEvent& event) {
        mEntityFactory.createEnemyDeadEffect(event.mClassId, event.mPosition, event.mIsFlipped);
    }

} // namespace game::system
