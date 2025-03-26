#include "SampleGame.hpp"
#include <box2d/box2d.h>
#include <regex>

#include "ContentManager.hpp"
#include "ImGui.hpp"
#include "RenderContext.hpp"


struct CharacterController
{
	vec2 position;
	b2ShapeId collider;

	bool hasLeftSensorHitFloor{ true };
	bool hasMidSensorHitFloor{ true };
	bool hasRightSensorHitFloor{ true };

	vec2 floorNormal{};
};

CharacterController controller;

struct PhysicsWorld
{
	static constexpr float pixelPerMeter = 100.0;
	static constexpr float debugLinesThickness = 4.0;
	static constexpr float debugLayerTransparency = 0.5f;

	b2WorldId worldId;
	b2DebugDraw debugDraw;

	std::vector<b2ShapeId> shapes;

	PhysicsWorld()
	{
		debugDraw = b2DefaultDebugDraw();

		debugDraw.DrawSegment = [](b2Vec2 p1, b2Vec2 p2, b2HexColor color, void* context)
		{
			auto& drawList = *ImGui::GetBackgroundDrawList();
			drawList.AddLine(vec2(p1.x, p1.y), vec2(p2.x, p2.y),
							 ImColor{ 0.0f, 0.0f, 0.0f, debugLayerTransparency } + color, debugLinesThickness);
		};
		debugDraw.DrawPolygon = [](const b2Vec2* vertices, int vertexCount, b2HexColor color, void* context)
		{
			auto& drawList = *ImGui::GetBackgroundDrawList();
			drawList.AddPolyline((ImVec2*)vertices, vertexCount,
								 ImColor{ 0.0f, 0.0f, 0.0f, debugLayerTransparency } + color, ImDrawFlags_Closed,
								 debugLinesThickness);
		};
		debugDraw.DrawSolidPolygon = [](b2Transform transform, const b2Vec2* vertices, int vertexCount, float radius,
										b2HexColor color, void* context)
		{
			auto& drawList = *ImGui::GetBackgroundDrawList();
			auto points = std::vector<vec2>{};
			points.resize(vertexCount);
			for (auto i = 0; i < vertexCount; i++)
			{
				const auto pos = b2TransformPoint(transform, vertices[i]);
				points[i] = vec2(pos.x, pos.y);
			}

			drawList.AddConcavePolyFilled((ImVec2*)points.data(), points.size(),
										  ImColor{ 0.0f, 0.0f, 0.0f, debugLayerTransparency } + color);
		};
		debugDraw.DrawCircle = [](b2Vec2 center, float radius, b2HexColor color, void* context)
		{
			auto& drawList = *ImGui::GetBackgroundDrawList();
			drawList.AddCircle(CastTo<vec2>(center), radius,
							   ImColor{ 0.0f, 0.0f, 0.0f, debugLayerTransparency } + color, debugLinesThickness);
		};
		debugDraw.DrawSolidCircle = [](b2Transform transform, float radius, b2HexColor color, void* context)
		{
			auto& drawList = *ImGui::GetBackgroundDrawList();
			const auto center = b2TransformPoint(transform, b2Vec2{ 0.0f, 0.0f });
			drawList.AddCircleFilled(CastTo<vec2>(center), radius,
									 ImColor{ 0.0f, 0.0f, 0.0f, debugLayerTransparency } + color);
		};
		debugDraw.DrawSolidCapsule = [](b2Vec2 p1, b2Vec2 p2, float radius, b2HexColor color, void* context)
		{
			auto& drawList = *ImGui::GetBackgroundDrawList();
			drawList.AddCircleFilled(CastTo<vec2>(p1), radius,
									 ImColor{ 0.0f, 0.0f, 0.0f, debugLayerTransparency } + color);
			drawList.AddCircleFilled(CastTo<vec2>(p2), radius,
									 ImColor{ 0.0f, 0.0f, 0.0f, debugLayerTransparency } + color);
		};

		debugDraw.DrawString = [](b2Vec2 p, const char* s, b2HexColor color, void* context)
		{
			auto& drawList = *ImGui::GetBackgroundDrawList();
			drawList.AddText(vec2(p.x, p.y), ImColor{ 0.0f, 0.0f, 0.0f, 1.0 } + color, s);
		};
		debugDraw.DrawTransform = [](b2Transform transform, void* context)
		{
			auto& drawList = *ImGui::GetBackgroundDrawList();
			const auto origin = CastTo<vec2>(transform.p);
			const auto upDirection = CastTo<vec2>(b2RotateVector(transform.q, b2Vec2{ 0.0, 1.0 }));
			const auto rightDirection = CastTo<vec2>(b2RotateVector(transform.q, b2Vec2{ 1.0, 0.0 }));

			drawList.AddLine(origin, origin + upDirection * 100.0f, ImColor{ 1.0f, 0.0f, 0.0f, debugLayerTransparency },
							 debugLinesThickness);
			drawList.AddLine(origin, origin + rightDirection * 100.0f,
							 ImColor{ 0.0f, 0.0f, 1.0f, debugLayerTransparency }, debugLinesThickness);
		};

		debugDraw.DrawPoint = [](b2Vec2 p, float size, b2HexColor color, void* context)
		{
			auto& drawList = *ImGui::GetBackgroundDrawList();
			drawList.AddCircle(CastTo<vec2>(p), size, ImColor{ 0.0f, 0.0f, 0.0f, debugLayerTransparency } + color);
		};

		debugDraw.drawShapes = true;

		auto worldDefinition = b2DefaultWorldDef();
		worldDefinition.gravity = CastTo<b2Vec2>(vec2{ 0.0f, 10.0f } * pixelPerMeter);
		worldId = b2CreateWorld(&worldDefinition);
	}

	void DrawSettingsUI()
	{
		ImGui::SeparatorText("Debug Settings");
		ImGui::Checkbox("Draw Shapes", &debugDraw.drawShapes);
		ImGui::Checkbox("Draw Joints", &debugDraw.drawJoints);
		ImGui::Checkbox("Draw Joints Extras", &debugDraw.drawJointExtras);
		ImGui::Checkbox("Draw AABBs", &debugDraw.drawAABBs);
		ImGui::Checkbox("Draw Mass", &debugDraw.drawMass);
		ImGui::Checkbox("Draw Body Names", &debugDraw.drawBodyNames);
		ImGui::Checkbox("Draw Contacts", &debugDraw.drawContacts);
		ImGui::Checkbox("Draw Graph Color", &debugDraw.drawGraphColors);
		ImGui::Checkbox("Draw Contact Normals", &debugDraw.drawContactNormals);
		ImGui::Checkbox("Draw Contact Impulses", &debugDraw.drawContactImpulses);
		ImGui::Checkbox("Draw Friction Impulses", &debugDraw.drawFrictionImpulses);
	}

	void DebugDraw()
	{
		b2World_Draw(worldId, &debugDraw);
	}
};

PhysicsWorld physicsWorld;

SampleGame::SampleGame()
{
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


	{
		auto bodyDefinition = b2DefaultBodyDef();
		bodyDefinition.position = CastTo<b2Vec2>(vec2{ 400.0f, 500.0f });
		bodyDefinition.name = "floor_0";
		auto body = b2CreateBody(physicsWorld.worldId, &bodyDefinition);

		auto box = b2MakeBox(300.0f, 10.0f);
		const auto shapeDefinition = b2DefaultShapeDef();
		const auto shape = b2CreatePolygonShape(body, &shapeDefinition, &box);
		physicsWorld.shapes.push_back(shape);
	}
	{
		auto bodyDefinition = b2DefaultBodyDef();
		bodyDefinition.position = CastTo<b2Vec2>(vec2{ 900.0f, 900.0f });
		bodyDefinition.name = "floor_1";
		auto body = b2CreateBody(physicsWorld.worldId, &bodyDefinition);

		auto box = b2MakeBox(40.0f, 10.0f);
		const auto shapeDefinition = b2DefaultShapeDef();
		const auto shape = b2CreatePolygonShape(body, &shapeDefinition, &box);
		physicsWorld.shapes.push_back(shape);
	}
	{
		auto bodyDefinition = b2DefaultBodyDef();
		bodyDefinition.position = CastTo<b2Vec2>(vec2{ 1900.0f, 900.0f });
		bodyDefinition.name = "floor_3";
		bodyDefinition.rotation = b2MakeRot(0.15f * B2_PI);
		auto bodyId = b2CreateBody(physicsWorld.worldId, &bodyDefinition);

		auto box = b2MakeBox(400.0f, 10.0f);
		const auto shapeDefinition = b2DefaultShapeDef();
		const auto shape = b2CreatePolygonShape(bodyId, &shapeDefinition, &box);
		physicsWorld.shapes.push_back(shape);
	}
	{
		auto bodyDefinition = b2DefaultBodyDef();
		bodyDefinition.type = b2_dynamicBody;
		bodyDefinition.position = CastTo<b2Vec2>(vec2{ 400.0f, 100.0f });
		// bodyDefinition.rotation = b2MakeRot(0.25f * B2_PI);
		bodyDefinition.name = "dynamic_element";
		bodyDefinition.fixedRotation = true;

		const auto body = b2CreateBody(physicsWorld.worldId, &bodyDefinition);
		const auto box = b2MakeBox(100.0f, 100.0f);


		auto shapeDefinition = b2DefaultShapeDef();
		// TODO: set more intuitive values
		shapeDefinition.density = 0.0001f;
		shapeDefinition.friction = 1.0f;
		shapeDefinition.restitution = 0.0f;

		auto capsulDefinition = b2Capsule{ .center1 = CastTo<b2Vec2>(vec2{ 0.0f, -40.0f }),
										   .center2 = CastTo<b2Vec2>(vec2{ 0.0f, 80.0f }),
										   .radius = 40 };
		const auto shape = b2CreateCapsuleShape(body, &shapeDefinition, &capsulDefinition);
		controller.collider = shape;
		physicsWorld.shapes.push_back(shape);
	}
}

void SampleGame::OnUnload()
{
	renderContext->DestroyTexture2D(huskTexture);
}

void SampleGame::OnUpdate(const f32 deltaTime)
{
	animationPlayer->ForwardTime(deltaTime);
	characterAnimationInstance =
		animationPlayer->ForwardAnimation(characterAnimationInstance, characterAnimationSequence);

	{

		const auto leftRayOrigin = controller.position + vec2{ -150.0f, 0.0f };
		const auto leftRayResult = b2World_CastRayClosest(physicsWorld.worldId, CastTo<b2Vec2>(leftRayOrigin),
														  CastTo<b2Vec2>(vec2{ 0.0f, 400.0f }), b2DefaultQueryFilter());
		controller.hasLeftSensorHitFloor =
			leftRayResult.hit and glm::distance(CastTo<vec2>(leftRayResult.point), leftRayOrigin) < 150.0f;
	}
	{

		const auto midRayOrigin = controller.position;
		const auto midRayResult = b2World_CastRayClosest(physicsWorld.worldId, CastTo<b2Vec2>(midRayOrigin),
														 CastTo<b2Vec2>(vec2{ 0.0f, 400.0f }), b2DefaultQueryFilter());
		controller.hasMidSensorHitFloor =
			midRayResult.hit and glm::distance(CastTo<vec2>(midRayResult.point), midRayOrigin) < 150.0f;
		controller.floorNormal = CastTo<vec2>(midRayResult.normal);
	}
	{

		const auto rightRayOrigin = controller.position + vec2{ 150.0f, 0.0f };
		const auto rightRayResult =
			b2World_CastRayClosest(physicsWorld.worldId, CastTo<b2Vec2>(rightRayOrigin),
								   CastTo<b2Vec2>(vec2{ 0.0f, 400.0f }), b2DefaultQueryFilter());
		controller.hasRightSensorHitFloor =
			rightRayResult.hit and glm::distance(CastTo<vec2>(rightRayResult.point), rightRayOrigin) < 150.0f;
	}

	const auto stickDirection = (vec2)ImGui::GetKeyMagnitude2d(ImGuiKey_GamepadLStickLeft, ImGuiKey_GamepadLStickRight,
															   ImGuiKey_GamepadLStickUp, ImGuiKey_GamepadLStickDown);


	ImGui::SliderFloat2("left_stick", (float*)(&stickDirection), -1.0f, 1.0f);


	if (ImGui::IsKeyPressed(ImGuiKey_GamepadFaceDown))
	{
		const auto body = b2Shape_GetBody(controller.collider);
		const auto impulse = vec2(0.0f, -600.0f) + CastTo<vec2>(b2Body_GetLinearVelocity(body));
		b2Body_ApplyLinearImpulseToCenter(body, CastTo<b2Vec2>(impulse), true);
	}

	if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
	{
		const auto body = b2Shape_GetBody(controller.collider);
		const auto forceSourcePosition = vec2{ ImGui::GetMousePos() };
		const auto bodyPosition = CastTo<vec2>(b2Body_GetPosition(body));
		const auto distance = glm::distance(forceSourcePosition, bodyPosition) / physicsWorld.pixelPerMeter;
		const auto forceFactor = 1.0f / (std::clamp(distance, 1.0f, 10.0f));
		const auto forceAtSource =
			glm::normalize(forceSourcePosition - bodyPosition) * physicsWorld.pixelPerMeter * 4.0f;
		const auto force = forceAtSource * forceFactor;

		b2Body_ApplyForce(body, CastTo<b2Vec2>(force), CastTo<b2Vec2>(forceSourcePosition), true);
	}

	b2World_Step(physicsWorld.worldId, deltaTime, 8);

	controller.position = CastTo<vec2>(b2Body_GetPosition(b2Shape_GetBody(controller.collider)));

	ImGui::Checkbox("left_edge_sensor_hit_floor", &controller.hasLeftSensorHitFloor);
	ImGui::Checkbox("mid_edge_sensor_hit_floor", &controller.hasMidSensorHitFloor);
	ImGui::Checkbox("right_edge_sensor_hit_floor", &controller.hasRightSensorHitFloor);
	ImGui::Separator();
	ImGui::SliderFloat3("mid_floor_normal", (float*)&controller.floorNormal, -1.0f, 1.0f);

	if (ImGui::Button("Reset Physics World"))
	{
		const auto body = b2Shape_GetBody(controller.collider);
		b2Body_SetTransform(body, CastTo<b2Vec2>(vec2{ 400.0f, 100.0f }), b2MakeRot(0.0f));
		b2Body_SetAngularVelocity(body, 0.0f);
		b2Body_SetLinearVelocity(body, CastTo<b2Vec2>(vec2{ 0.0f, 0.0f }));
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


	const auto body = b2Shape_GetBody(controller.collider);
	/*const auto forceFactor = 100.0f;
	auto force = stickDirection * forceFactor;*/
	auto f = vec2{ 0.0f, 0.0f };


	if (glm::abs(stickDirection.x) > 0.3)
	{
		if (stickDirection.x > 0)
		{
			if (std::regex_match(animations[characterAnimationInstance.currentNodeIndex].name,
								 std::regex{ "walk-right" }))
			{
				f.x = 100.0f;
			}
			else
			{
				characterAnimationSequence =
					animationGraph->FindAnimationSequence(characterAnimationInstance, "walk-right");
			}
		}
		if (stickDirection.x < 0)
		{
			if (std::regex_match(animations[characterAnimationInstance.currentNodeIndex].name,
								 std::regex{ "walk-left" }))
			{
				f.x = -100.0f;
			}
			else
			{
				characterAnimationSequence =
					animationGraph->FindAnimationSequence(characterAnimationInstance, "walk-left");
			}
		}
	}
	else
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
	b2Body_ApplyLinearImpulseToCenter(body, CastTo<b2Vec2>(f), true);
}

void SampleGame::OnDraw(const f32 deltaTime)
{
	static auto showDemoWindow = true;
	if (showDemoWindow)
	{
		ImGui::ShowDemoWindow(&showDemoWindow);
	}

	physicsWorld.DrawSettingsUI();

	const auto& animation = animations[characterAnimationInstance.currentNodeIndex];
	const auto& animationKey = animationSequences[animation.animationIndex + characterAnimationInstance.key];
	const auto& frame = animationFrames[animationKey.frameIndex];

	const auto body = b2Shape_GetBody(controller.collider);
	const auto transform = b2Body_GetTransform(body);

	const auto frameAspectRation = frame.sourceSprite.extent.x / frame.sourceSprite.extent.y;

	

	renderContext->Clear(Colors::CornflowerBlue);

	spriteBatch->Begin();
	const auto origin = frame.sourceSprite.position + vec2{ frame.sourceSprite.extent.x / 2.0f, 0.0f };
	const auto extent = vec2{ 240 * frameAspectRation, 240 };
	spriteBatch->Draw(huskTexture, frame.sourceSprite, Rectangle{ CastTo<vec2>(transform.p) - extent / 2.0f, extent },
					  Colors::White,
					  animationKey.flip == FrameFlip::horizontal ? FlipSprite::horizontal : FlipSprite::none, origin);
	spriteBatch->End();

	physicsWorld.DebugDraw();
}