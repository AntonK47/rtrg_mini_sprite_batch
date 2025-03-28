#pragma once
#include "Game.hpp"
#include "SpriteBatch.hpp"
#include "Animation.hpp"
#include "RenderResources.hpp"

#include <memory>

struct SampleGame : Game
{
	SampleGame();

	void OnDraw(const f32 deltaTime) override;
	void OnUpdate(const f32 deltaTime) override;
	void OnLoad() override;
	void OnUnload() override;
	mat3 cameraMatrix{};

private:
	std::unique_ptr<SpriteBatch> spriteBatch;
	std::unique_ptr<AnimationPlayer> animationPlayer;
	std::unique_ptr<AnimationGraph> animationGraph;

	AnimationInstance characterAnimationInstance{};
	AnimationSequence characterAnimationSequence{};

	Texture2DHandle huskTexture{};

};