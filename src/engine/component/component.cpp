#include "component.h"

namespace engine::component {
void Component::setOwner(engine::object::GameObject* owner) {
	mOwner = owner;
}
engine::object::GameObject* Component::getOwner() const {
	return mOwner;
}
} // namespace engine::component
