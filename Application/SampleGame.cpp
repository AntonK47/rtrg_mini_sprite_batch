#include "SampleGame.hpp"
#include <box2d/box2d.h>
#include <regex>

#include "ContentManager.hpp"
#include "ImGui.hpp"
#include "RenderContext.hpp"

#include "MapImporter.hpp"

#include <tracy/Tracy.hpp>

using namespace tiled;

struct CharacterController
{
	vec2 position{};
	b2ShapeId collider{};

	bool hasLeftSensorHitFloor{ true };
	bool hasMidSensorHitFloor{ true };
	bool hasRightSensorHitFloor{ true };

	vec2 floorNormal{};
};

CharacterController controller{};
tiled::TiledMap map{};

struct TileSet
{
	u32 firstGlobalId;
	Texture2DHandle image;
};

std::vector<TileSet> tileSets{};
constexpr i32 characterHeight = 64;

struct Camera2D
{
	vec2 position;
	b2BodyId cameraBody;
	b2BodyId lookAtBody;
};

Camera2D camera;

struct PhysicsWorld
{
	static constexpr float pixelPerMeter = 100.0;
	static constexpr float debugLinesThickness = 4.0;
	static constexpr float debugLayerTransparency = 0.5f;

	b2WorldId worldId;
	b2DebugDraw debugDraw;

	std::vector<b2ShapeId> shapes;

	PhysicsWorld(SampleGame* game)
	{
		debugDraw = b2DefaultDebugDraw();

		debugDraw.DrawSegment = [](b2Vec2 p1, b2Vec2 p2, b2HexColor color, void* context)
		{
			auto game = reinterpret_cast<SampleGame*>(context);
			auto& drawList = *ImGui::GetBackgroundDrawList();

			const auto pp1 = vec2(game->cameraMatrix * vec3(p1.x, p1.y, 1.0f));
			const auto pp2 = vec2(game->cameraMatrix * vec3(p2.x, p2.y, 1.0f));
			drawList.AddLine(pp1, pp2, ImColor{ 0.0f, 0.0f, 0.0f, debugLayerTransparency } + color,
							 debugLinesThickness);
		};
		debugDraw.DrawPolygon = [](const b2Vec2* vertices, int vertexCount, b2HexColor color, void* context)
		{
			auto game = reinterpret_cast<SampleGame*>(context);
			auto& drawList = *ImGui::GetBackgroundDrawList();
			auto points = std::vector<vec2>{};
			points.resize(vertexCount);
			for (auto i = 0; i < vertexCount; i++)
			{
				points[i] = vec2(game->cameraMatrix * vec3(vertices[i].x, vertices[i].y, 1.0f));
			}
			drawList.AddPolyline((ImVec2*)points.data(), vertexCount,
								 ImColor{ 0.0f, 0.0f, 0.0f, debugLayerTransparency } + color, ImDrawFlags_Closed,
								 debugLinesThickness);
		};
		debugDraw.DrawSolidPolygon = [](b2Transform transform, const b2Vec2* vertices, int vertexCount,
										[[maybe_unused]] float radius, b2HexColor color, void* context)
		{
			auto game = reinterpret_cast<SampleGame*>(context);
			auto& drawList = *ImGui::GetBackgroundDrawList();
			auto points = std::vector<vec2>{};
			points.resize(vertexCount);
			for (auto i = 0; i < vertexCount; i++)
			{
				const auto pos = b2TransformPoint(transform, vertices[i]);
				points[i] = vec2(game->cameraMatrix * vec3(pos.x, pos.y, 1.0f));
			}

			drawList.AddConcavePolyFilled((ImVec2*)points.data(), static_cast<int>(points.size()),
										  ImColor{ 0.0f, 0.0f, 0.0f, debugLayerTransparency } + color);
		};
		debugDraw.DrawCircle = [](b2Vec2 center, float radius, b2HexColor color, void* context)
		{
			auto game = reinterpret_cast<SampleGame*>(context);
			const auto scale = glm::length(game->cameraMatrix * vec3{ 1.0, 0.0f, 0.0f });
			auto& drawList = *ImGui::GetBackgroundDrawList();
			drawList.AddCircle(vec2(game->cameraMatrix * vec3(CastTo<vec2>(center), 1.0f)), scale * radius,
							   ImColor{ 0.0f, 0.0f, 0.0f, debugLayerTransparency } + color,
							   static_cast<int>(debugLinesThickness));
		};
		debugDraw.DrawSolidCircle = [](b2Transform transform, float radius, b2HexColor color, void* context)
		{
			auto game = reinterpret_cast<SampleGame*>(context);
			const auto scale = glm::length(game->cameraMatrix * vec3{ 1.0, 0.0f, 0.0f });
			auto& drawList = *ImGui::GetBackgroundDrawList();
			const auto center = b2TransformPoint(transform, b2Vec2{ 0.0f, 0.0f });
			drawList.AddCircleFilled(vec2(game->cameraMatrix * vec3(CastTo<vec2>(center), 1.0f)), scale * radius,
									 ImColor{ 0.0f, 0.0f, 0.0f, debugLayerTransparency } + color);
		};
		debugDraw.DrawSolidCapsule = [](b2Vec2 p1, b2Vec2 p2, float radius, b2HexColor color, void* context)
		{
			auto game = reinterpret_cast<SampleGame*>(context);
			const auto scale = glm::length(game->cameraMatrix * vec3{ 1.0, 0.0f, 0.0f });
			auto& drawList = *ImGui::GetBackgroundDrawList();
			drawList.AddCircleFilled(vec2(game->cameraMatrix * vec3(CastTo<vec2>(p1), 1.0f)), scale * radius,
									 ImColor{ 0.0f, 0.0f, 0.0f, debugLayerTransparency } + color);
			drawList.AddCircleFilled(vec2(game->cameraMatrix * vec3(CastTo<vec2>(p2), 1.0f)), scale * radius,
									 ImColor{ 0.0f, 0.0f, 0.0f, debugLayerTransparency } + color);
		};

		debugDraw.DrawString = [](b2Vec2 p, const char* s, b2HexColor color, void* context)
		{
			auto game = reinterpret_cast<SampleGame*>(context);
			auto& drawList = *ImGui::GetBackgroundDrawList();
			drawList.AddText(vec2(game->cameraMatrix * vec3(CastTo<vec2>(p), 1.0f)),
							 ImColor{ 0.0f, 0.0f, 0.0f, 1.0 } + color, s);
		};
		debugDraw.DrawTransform = [](b2Transform transform, void* context)
		{
			auto game = reinterpret_cast<SampleGame*>(context);
			auto& drawList = *ImGui::GetBackgroundDrawList();
			const auto origin = vec2(game->cameraMatrix * vec3(CastTo<vec2>(transform.p), 1.0f));
			const auto upDirection = CastTo<vec2>(b2RotateVector(transform.q, b2Vec2{ 0.0, 1.0 }));
			const auto rightDirection = CastTo<vec2>(b2RotateVector(transform.q, b2Vec2{ 1.0, 0.0 }));

			drawList.AddLine(origin,
							 vec2(game->cameraMatrix * vec3(CastTo<vec2>(transform.p) + upDirection * 100.0f, 1.0f)),
							 ImColor{ 1.0f, 0.0f, 0.0f, debugLayerTransparency }, debugLinesThickness);
			drawList.AddLine(origin,
							 vec2(game->cameraMatrix * vec3(CastTo<vec2>(transform.p) + rightDirection * 100.0f, 1.0f)),
							 ImColor{ 0.0f, 0.0f, 1.0f, debugLayerTransparency }, debugLinesThickness);
		};

		debugDraw.DrawPoint = [](b2Vec2 p, float size, b2HexColor color, void* context)
		{
			auto game = reinterpret_cast<SampleGame*>(context);
			auto& drawList = *ImGui::GetBackgroundDrawList();
			drawList.AddCircle(vec2(game->cameraMatrix * vec3(CastTo<vec2>(p), 1.0f)), size,
							   ImColor{ 0.0f, 0.0f, 0.0f, debugLayerTransparency } + color);
		};

		debugDraw.drawShapes = true;
		debugDraw.context = game;

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

std::unique_ptr<PhysicsWorld> physicsWorld;

SampleGame::SampleGame()
{
	cameraMatrix = glm::identity<mat3>();
	physicsWorld = std::make_unique<PhysicsWorld>(this);
}

void SampleGame::OnLoad()
{
	spriteBatch = std::make_unique<SpriteBatch>(renderContext.get());
	animationPlayer = std::make_unique<AnimationPlayer>();
	animationGraph = std::make_unique<AnimationGraph>();

	huskTexture = content->LoadTexture("Textures/great_husk_sentry.DDS");
	map = ImportMap("Assets/Maps/test_for_engine_export.json");

	for (const auto& layer : map.layers)
	{
		if (layer.type == TiledLayerType::objectgroup)
		{
			for (const auto& object : layer.objects)
			{
				if (not object.polygon.empty())
				{
					auto points = std::vector<b2Vec2>{};
					points.resize(object.polygon.size());

					for (auto i = 0; i < object.polygon.size(); i++)
					{
						points[i] = b2Vec2{ static_cast<float>(object.polygon[i].x + object.x),
											static_cast<float>(object.polygon[i].y + object.y) };
					}

					auto material = b2SurfaceMaterial{};
					material.friction = 0.2f;
					material.customColor = b2_colorSteelBlue;
					material.material = 42;

					auto chainDefinition = b2DefaultChainDef();
					chainDefinition.points = points.data();
					chainDefinition.count = static_cast<int>(points.size());
					chainDefinition.isLoop = true;
					chainDefinition.materialCount = 1;
					chainDefinition.materials = &material;

					auto bodyDefinition = b2DefaultBodyDef();
					auto bodyId = b2CreateBody(physicsWorld->worldId, &bodyDefinition);

					auto shape = b2CreateChain(bodyId, &chainDefinition);

					// physicsWorld->shapes.push_back(shape);
				}
			}
		}
	}


	for (const auto& tileSet : map.tilesets)
	{
		const auto firstGlobalId = static_cast<u32>(tileSet.firstgid);
		const auto image = content->LoadTexture(tileSet.image);
		tileSets.push_back(TileSet{ firstGlobalId, image });
	}

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
		bodyDefinition.type = b2_dynamicBody;
		bodyDefinition.position = CastTo<b2Vec2>(vec2{ 400.0f, 100.0f });

		bodyDefinition.name = "dynamic_element";
		bodyDefinition.fixedRotation = true;

		const auto body = b2CreateBody(physicsWorld->worldId, &bodyDefinition);


		auto shapeDefinition = b2DefaultShapeDef();
		// TODO: set more intuitive values
		shapeDefinition.density = 0.007f;
		shapeDefinition.friction = 0.2f;
		shapeDefinition.restitution = 0.0f;

		auto capsuleDefinition = b2Capsule{ .center1 = CastTo<b2Vec2>(vec2{ 0.0f, -characterHeight / 4.0f }),
											.center2 = CastTo<b2Vec2>(vec2{ 0.0f, +characterHeight / 4.0f }),
											.radius = characterHeight / 4.0f };
		const auto shape = b2CreateCapsuleShape(body, &shapeDefinition, &capsuleDefinition);
		controller.collider = shape;
		physicsWorld->shapes.push_back(shape);
	}

	/*=============CAMERA=============*/
	{
		auto bodyDefinition = b2DefaultBodyDef();
		bodyDefinition.type = b2BodyType::b2_staticBody;
		bodyDefinition.isEnabled = true;
		camera.lookAtBody = b2CreateBody(physicsWorld->worldId, &bodyDefinition);
		bodyDefinition.type = b2BodyType::b2_kinematicBody;
		camera.cameraBody = b2CreateBody(physicsWorld->worldId, &bodyDefinition);
		auto box = b2MakeBox(10.0f, 10.0f);
		auto shapeDefinition = b2DefaultShapeDef();
		shapeDefinition.density = 0.1f;
		shapeDefinition.friction = 0.2f;
		shapeDefinition.restitution = 0.0f;
		const auto shape = b2CreatePolygonShape(camera.cameraBody, &shapeDefinition, &box);
		b2DistanceJointDef jointDef = b2DefaultDistanceJointDef();
		jointDef.bodyIdA = camera.cameraBody;
		jointDef.bodyIdB = camera.lookAtBody;
		jointDef.enableSpring = true;
		jointDef.hertz = 2.0f;
		jointDef.dampingRatio = 1.0f;


		jointDef.minLength = 0;
		jointDef.maxLength = 200;

		b2JointId myJointId = b2CreateDistanceJoint(physicsWorld->worldId, &jointDef);
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
		const auto leftRayResult = b2World_CastRayClosest(physicsWorld->worldId, CastTo<b2Vec2>(leftRayOrigin),
														  CastTo<b2Vec2>(vec2{ 0.0f, 400.0f }), b2DefaultQueryFilter());
		controller.hasLeftSensorHitFloor =
			leftRayResult.hit and glm::distance(CastTo<vec2>(leftRayResult.point), leftRayOrigin) < 150.0f;
	}
	{

		const auto midRayOrigin = controller.position;
		const auto midRayResult = b2World_CastRayClosest(physicsWorld->worldId, CastTo<b2Vec2>(midRayOrigin),
														 CastTo<b2Vec2>(vec2{ 0.0f, 400.0f }), b2DefaultQueryFilter());
		controller.hasMidSensorHitFloor =
			midRayResult.hit and glm::distance(CastTo<vec2>(midRayResult.point), midRayOrigin) < 150.0f;
		controller.floorNormal = CastTo<vec2>(midRayResult.normal);
	}
	{

		const auto rightRayOrigin = controller.position + vec2{ 150.0f, 0.0f };
		const auto rightRayResult =
			b2World_CastRayClosest(physicsWorld->worldId, CastTo<b2Vec2>(rightRayOrigin),
								   CastTo<b2Vec2>(vec2{ 0.0f, 400.0f }), b2DefaultQueryFilter());
		controller.hasRightSensorHitFloor =
			rightRayResult.hit and glm::distance(CastTo<vec2>(rightRayResult.point), rightRayOrigin) < 150.0f;
	}

	const auto stickDirection = (vec2)ImGui::GetKeyMagnitude2d(ImGuiKey_GamepadLStickLeft, ImGuiKey_GamepadLStickRight,
															   ImGuiKey_GamepadLStickUp, ImGuiKey_GamepadLStickDown);
	const auto rightStickDirection = (vec2)ImGui::GetKeyMagnitude2d(
		ImGuiKey_GamepadRStickLeft, ImGuiKey_GamepadRStickRight, ImGuiKey_GamepadRStickUp, ImGuiKey_GamepadRStickDown);

	const auto rotation =
		ImGui::GetKeyData(ImGuiKey_GamepadL2)->AnalogValue - ImGui::GetKeyData(ImGuiKey_GamepadR2)->AnalogValue;

	auto scale = 1.0f;
	if (ImGui::IsKeyPressed(ImGuiKey_GamepadL1, false))
	{
		scale = 0.9f;
	}
	if (ImGui::IsKeyPressed(ImGuiKey_GamepadR1, false))
	{
		scale = 1.1f;
	}

	cameraMatrix = glm::translate(cameraMatrix, rightStickDirection * 1000.0f * deltaTime);
	static auto origin =
		vec2(renderContext->GetWindowsContext().width / 2, renderContext->GetWindowsContext().height / 2);
	origin -= rightStickDirection * 1000.0f * deltaTime;
	cameraMatrix = glm::translate(cameraMatrix, origin);
	cameraMatrix = glm::rotate(cameraMatrix, glm::radians(rotation * 180 * deltaTime));
	cameraMatrix = glm::scale(cameraMatrix, vec2{ 1.0f, 1.0f } * scale);
	cameraMatrix = glm::translate(cameraMatrix, -origin);

	ImGui::SliderFloat2("left_stick", (float*)(&stickDirection), -1.0f, 1.0f);


	if (ImGui::IsKeyPressed(ImGuiKey_GamepadFaceDown))
	{
		const auto body = b2Shape_GetBody(controller.collider);
		const auto impulse = vec2(0.0f, -6000.0f) + CastTo<vec2>(b2Body_GetLinearVelocity(body));
		b2Body_ApplyLinearImpulseToCenter(body, CastTo<b2Vec2>(impulse), true);
	}

	if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
	{
		const auto body = b2Shape_GetBody(controller.collider);
		const auto forceSourcePosition = vec2{ ImGui::GetMousePos() };
		const auto bodyPosition = CastTo<vec2>(b2Body_GetPosition(body));
		const auto distance = glm::distance(forceSourcePosition, bodyPosition) / physicsWorld->pixelPerMeter;
		const auto forceFactor = 1.0f / (std::clamp(distance, 1.0f, 10.0f));
		const auto forceAtSource =
			glm::normalize(forceSourcePosition - bodyPosition) * physicsWorld->pixelPerMeter * 4.0f;
		const auto force = forceAtSource * forceFactor;

		b2Body_ApplyForce(body, CastTo<b2Vec2>(force), CastTo<b2Vec2>(forceSourcePosition), true);
	}


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

	b2Body_SetTransform(camera.lookAtBody, CastTo<b2Vec2>(controller.position), b2Rot_identity);
	b2World_Step(physicsWorld->worldId, deltaTime, 8);

	// cameraMatrix = glm::translate(glm::identity<mat3>(), origin-CastTo<vec2>(b2Body_GetPosition(camera.cameraBody)));
}

void SampleGame::OnDraw([[maybe_unused]] const f32 deltaTime)
{
	static auto showDemoWindow = true;
	if (showDemoWindow)
	{
		ImGui::ShowDemoWindow(&showDemoWindow);
	}

	physicsWorld->DrawSettingsUI();

	const auto& animation = animations[characterAnimationInstance.currentNodeIndex];
	const auto sequenceIndex = animation.animationIndex + characterAnimationInstance.key;
	const auto& animationKey = animationSequences[sequenceIndex];
	const auto& frame = animationFrames[animationKey.frameIndex];

	const auto body = b2Shape_GetBody(controller.collider);
	const auto transform = b2Body_GetTransform(body);

	const auto frameAspectRation = frame.sourceSprite.extent.x / frame.sourceSprite.extent.y;


	renderContext->Clear(Colors::CornflowerBlue);


	spriteBatch->Begin(cameraMatrix);
	const auto origin = frame.sourceSprite.position + vec2{ frame.sourceSprite.extent.x / 2.0f, 0.0f };
	const auto extent = vec2{ characterHeight * frameAspectRation, characterHeight };
	spriteBatch->Draw(huskTexture, frame.sourceSprite, Rectangle{ CastTo<vec2>(transform.p) - extent / 2.0f, extent },
					  Colors::White,
					  animationKey.flip == FrameFlip::horizontal ? FlipSprite::horizontal : FlipSprite::none, origin);
	/*spriteBatch->End();

	spriteBatch->Begin(cameraMatrix);*/
	const auto& texture = renderContext->Get(tileSets[0].image);
	const auto xs = (texture.width / 32);
	const auto ys = (texture.height / 32);


	for (const auto& layer : map.layers)
	{
		if (layer.type == TiledLayerType::tilelayer)
		{
			for (const auto& chunk : layer.chunks)
			{

				const auto& data = std::get<TiledLayerData>(chunk.data);
				const auto width = chunk.width;
				const auto height = chunk.height;

				/*for (auto ox = 0; ox < 100; ox++)
				{
					for (auto oy = 0; oy < 100; oy++)
					{*/
				auto index = 0;
				for (auto x = 0; x < width; x++)
				{
					for (auto y = 0; y < height; y++)
					{
						const auto dirtyGlobalId = data[index];
						index++;
						const auto globalId = static_cast<u32>(dirtyGlobalId & 0xfffffff);

						for (const auto& tileSet : tileSets)
						{

							if (tileSet.firstGlobalId <= globalId and globalId < (tileSet.firstGlobalId + xs * ys))
							{
								const auto localTileId = globalId - tileSet.firstGlobalId;
								const auto row = localTileId % xs;
								const auto col = localTileId / xs;
								const auto position = vec2{ y * 32 + chunk.x * 32, x * 32 + chunk.y * 32 } +
									vec2{ layer.offsetx, layer.offsety } /* + vec2{ ox, oy } * 500.0f*/;

								spriteBatch->Draw(tileSet.image, Rectangle{ { row * 32, col * 32 }, { 32, 32 } },
												  Rectangle{ position, { 32, 32 } }, Colors::White);
							}
						}
						/*	}
						}*/
					}
				}
			}
		}
	}

	spriteBatch->End();


	physicsWorld->DebugDraw();
}