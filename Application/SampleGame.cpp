#include "SampleGame.hpp"
#include <box2d/box2d.h>
#include <regex>

#include "ContentManager.hpp"
#include "ImGui.hpp"
#include "RenderContext.hpp"

b2DebugDraw debugDraw;
b2WorldId worldId;

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

	b2World_Draw(worldId, &debugDraw);

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


	b2World_Step(worldId, deltaTime, 4);
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


	debugDraw = b2DefaultDebugDraw();

	debugDraw.DrawSegment = [](b2Vec2 p1, b2Vec2 p2, b2HexColor color, void* context)
	{
		auto& spriteBatch = *((SpriteBatch*)context);
		spriteBatch.Draw(SpriteTexture{}, vec2(p1.x, p1.y));
	};
	debugDraw.DrawPolygon = [](const b2Vec2* vertices, int vertexCount, b2HexColor color, void* context)
	{
		auto& drawList = *ImGui::GetBackgroundDrawList();
		// auto& spriteBatch = *((SpriteBatch*)context);
		for (auto i = 0; i < vertexCount; i++)
		{
			auto& v0 = vertices[i];
			auto& v1 = vertices[(i + 1)%vertexCount];

			drawList.AddLine(vec2(v0.x, v0.y), vec2(v1.x, v1.y), ImColor{0.0f,0.0f,0.0f,1.0f} + color, 3.0f);

			/*spriteBatch.Draw(SpriteTexture{}, vec2(v0.x, v0.y));
			spriteBatch.Draw(SpriteTexture{}, vec2(v1.x, v1.y));*/
		}
	};
	debugDraw.DrawString = [](b2Vec2 p, const char* s, b2HexColor color, void* context)
	{
		auto& drawList = *ImGui::GetBackgroundDrawList();
		ImGui::GetIO().FontGlobalScale = 2.0f;
		drawList.AddText(vec2(p.x, p.y), ImColor{0.0f,0.0f,0.0f,1.0f} + color, s);
	};
	debugDraw.drawAABBs = true;
	//debugDraw.drawBodyNames = true;
	auto worldDefinition = b2DefaultWorldDef();
	worldDefinition.gravity = ToBox2Vector(vec2{ 0.0f, 10.0f });
	worldId = b2CreateWorld(&worldDefinition);

	{
		auto bodyDefinition = b2DefaultBodyDef();
		bodyDefinition.position = ToBox2Vector(vec2{ 400.0f, 500.0f });
		bodyDefinition.name = "floor";
		auto bodyId = b2CreateBody(worldId, &bodyDefinition);

		auto box = b2MakeBox(150.0f, 10.0f);
		const auto shapeDefinition = b2DefaultShapeDef();
		b2CreatePolygonShape(bodyId, &shapeDefinition, &box);
	}
	{
		b2BodyDef bodyDef = b2DefaultBodyDef();
		bodyDef.type = b2_dynamicBody;
		bodyDef.position = ToBox2Vector(vec2{ 400.0f, 100.0f });
		bodyDef.rotation = b2MakeRot(0.25f * B2_PI);
		bodyDef.name = "dynamic_element";
		bodyDef.linearDamping = 0.0f;
		bodyDef.angularDamping = 0.1f;

		b2BodyId bodyId = b2CreateBody(worldId, &bodyDef);
		b2Polygon dynamicBox = b2MakeBox(100.0f, 100.0f);
		b2ShapeDef shapeDef = b2DefaultShapeDef();
		shapeDef.density = 0.4f;
		shapeDef.friction = 0.3f;
		
		b2CreatePolygonShape(bodyId, &shapeDef, &dynamicBox);
	}
}

void SampleGame::OnUnload()
{
	renderContext->DestroyTexture2D(huskTexture);
}
