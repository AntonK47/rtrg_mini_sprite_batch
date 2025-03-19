#include "SampleGame.hpp"
#include <regex>
#include "ContentManager.hpp"
#include "ImGui.hpp"
#include "RenderContext.hpp"

SampleGame::SampleGame()
{
}

void SampleGame::OnDraw(const f32 deltaTime)
{
	static auto showDemoWindow = true;
	if (showDemoWindow)
	{
		ImGui::ShowDemoWindow(&showDemoWindow);
	}

	ImGui::SliderFloat("frame duration", &animationPlayer->frameDuration, 0.01f, 1.0f);

	if (ImGui::Button("switch to walk right"))
	{
		characterAnimationSequence = animationGraph->FindAnimationSequence(characterAnimationInstance, "walk-right");
	}
	if (ImGui::Button("switch to walk left"))
	{
		characterAnimationSequence = animationGraph->FindAnimationSequence(characterAnimationInstance, "walk-left");
	}
	if (ImGui::Button("switch to idle"))
	{
		if (std::regex_match(animations[characterAnimationInstance.currentNodeIndex].name,
							 std::regex{ "[a-z]+\\-right" }))
		{
			characterAnimationSequence =
				animationGraph->FindAnimationSequence(characterAnimationInstance, "idle-right");
		}
		if (std::regex_match(animations[characterAnimationInstance.currentNodeIndex].name,
							 std::regex{ "[a-z]+\\-left" }))
		{
			characterAnimationSequence = animationGraph->FindAnimationSequence(characterAnimationInstance, "idle-left");
		}
	}


	const auto& animation = animations[characterAnimationInstance.currentNodeIndex];
	const auto& animationKey = animationSequences[animation.animationIndex + characterAnimationInstance.key];
	const auto& frame = animationFrames[animationKey.frameIndex];

	const auto& huskSpriteTexture = renderContext->Get(huskTexture);

	const auto srcRect = frame.sourceSprite;
	const auto uv0 = (srcRect.position) / vec2{ huskSpriteTexture.width, huskSpriteTexture.height };
	const auto uv1 = (srcRect.position + srcRect.extent) / vec2{ huskSpriteTexture.width, huskSpriteTexture.height };


	if (animationKey.flip == FrameFlip::horizontal)
	{
		ImGui::Image((ImTextureID)huskSpriteTexture.nativeHandle, vec2{ srcRect.extent.x, srcRect.extent.y },
					 vec2{ uv1.x, uv0.y }, vec2{ uv0.x, uv1.y });
	}
	else
	{
		ImGui::Image((ImTextureID)huskSpriteTexture.nativeHandle, ImVec2{ srcRect.extent.x, srcRect.extent.y },
					 vec2{ uv0.x, uv0.y }, vec2{ uv1.x, uv1.y });
	}


	ImGui::Image((ImTextureID)huskSpriteTexture.nativeHandle,
				 vec2{ huskSpriteTexture.width, huskSpriteTexture.height } / 8.0f);
	ImGui::SameLine();


	/*spriteBatch.Begin();
	spriteBatch.Draw(texture1, vec2{ 0.0f, 0.0f });
	spriteBatch.End();

	

	const auto& spriteFramebuffer = game.renderContext->Get(spriteBatchFramebuffer);
	const auto& spriteColorTexture = game.renderContext->Get(spriteFramebuffer.colorAttachment[0]);
	const auto& spriteDepthTexture = game.renderContext->Get(spriteFramebuffer.depthAttachment.value());

	ImGui::Image((ImTextureID)spriteColorTexture.nativeHandle, vec2{ windowWidth, windowHeight } / 4.0f);*/
}

void SampleGame::OnUpdate(const f32 deltaTime)
{
	animationPlayer->ForwardTime(deltaTime);
	characterAnimationInstance =
		animationPlayer->ForwardAnimation(characterAnimationInstance, characterAnimationSequence);
}

void SampleGame::OnLoad()
{
	spriteBatch = std::make_unique<SpriteBatch>(renderContext.get());
	animationPlayer = std::make_unique<AnimationPlayer>();
	animationGraph = std::make_unique<AnimationGraph>();

	huskTexture = content->LoadTexture("Textures/great_husk_sentry.DDS");

	for (auto i = 0; i < animations.size(); i++)
	{
		animationGraph->AddNode(animations[i].name, i);
	}

	animationGraph->AddTransition("idle-right", "turn-left", animation::sync::immediate);
	animationGraph->AddTransition("idle-right", "walk-right", animation::sync::immediate);
	animationGraph->AddTransition("idle-left", "turn-right", animation::sync::immediate);
	animationGraph->AddTransition("idle-left", "walk-left", animation::sync::immediate);
	animationGraph->AddTransition("walk-right", "idle-right", animation::sync::immediate);
	animationGraph->AddTransition("walk-left", "idle-left", animation::sync::immediate);
	animationGraph->AddTransition("walk-right", "turn-left", animation::sync::immediate);
	animationGraph->AddTransition("walk-left", "turn-right", animation::sync::immediate);

	animationGraph->AddTransition("turn-right", "idle-right", animation::sync::lastFrame);
	animationGraph->AddTransition("turn-right", "walk-right", animation::sync::lastFrame);
	animationGraph->AddTransition("turn-left", "idle-left", animation::sync::lastFrame);
	animationGraph->AddTransition("turn-left", "walk-left", animation::sync::lastFrame);

	characterAnimationInstance =
		AnimationInstance{ .currentNodeIndex = animationGraph->GetNodeIndex("idle-right"), .key = 0 };
}

void SampleGame::OnUnload()
{
	renderContext->DestroyTexture2D(huskTexture);
}
