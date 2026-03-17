#include "sprite.h"

namespace engine::render {
Sprite::Sprite(std::string_view textureId, const std::optional<SDL_FRect>& sourceRect, bool isFlipped)
	: mTextureId(textureId), mSourceRect(sourceRect), mIsFlipped(isFlipped) {}

std::string_view Sprite::getTextureId() const {
	return mTextureId;
}
const std::optional<SDL_FRect>& Sprite::getSourceRect() const {
	return mSourceRect;
}
bool Sprite::isFlipped() const {
	return mIsFlipped;
}
void Sprite::setTextureId(std::string_view textureId) {
	mTextureId = std::string(textureId);
}
void Sprite::setSourceRect(const std::optional<SDL_FRect>& sourceRect) {
	mSourceRect = sourceRect;
}
void Sprite::setFlipped(bool flipped) {
	mIsFlipped = flipped;
}
} // namespace engine::render
