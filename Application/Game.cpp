#include "Game.hpp"

#include <SDL3/SDL.h>

#define TINYDDS_IMPLEMENTATION
#include <gli/gli.hpp>
#include <tinydds.h>

#include <array>
#include <assert.h>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <print>
#include <regex>
#include <stack>
#include <variant>
#include <vector>

#include "Animation.hpp"
#include "Common.hpp"
#include "ImGui.hpp"
#include "RenderContext.hpp"
#include "SpriteBatch.hpp"

namespace
{
	void __stdcall MessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
								   const GLchar* message, const void* userParam)
	{
		std::println(stderr, "GL debug message: {} type = 0x{}, severity = 0x{}, message = {}\n",
					 (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""), type, severity, message);
	}
} // namespace

struct Game::GameImpl
{
private:
public:
	void Run(Game& game, int argc, char* argv[])
	{
		// Setup SDL
		if (!SDL_Init(SDL_INIT_VIDEO))
		{
			std::println("Error: SDL_Init(): {}\n", SDL_GetError());
			return;
		}


		// GL 3.0 + GLSL 130
		const char* glsl_version = "#version 130";
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);


		// Create window with graphics context
		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
		SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);


		Uint32 window_flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN;
		SDL_Window* window = SDL_CreateWindow("Mini Sprite Batch", 1280, 720, window_flags);
		if (window == nullptr)
		{
			printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
			return;
		}
		SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

		SDL_GLContext gl_context = SDL_GL_CreateContext(window);


		if (gl_context == nullptr)
		{
			printf("Error: SDL_GL_CreateContext(): %s\n", SDL_GetError());
			return;
		}

		int version = gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress);
		// std::println("GL {}.{}\n", version, version);

		SDL_GL_MakeCurrent(window, gl_context);
		SDL_GL_SetSwapInterval(1); // Enable vsync
		SDL_ShowWindow(window);

		SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
		glEnable(GL_DEBUG_OUTPUT);
		glDebugMessageCallback((GLDEBUGPROC)&MessageCallback, NULL);
		uint32_t unusedIds = 0;
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, &unusedIds, GL_TRUE);
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, NULL, GL_FALSE);

		// Setup Dear ImGui context
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		(void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;


		ImGui::StyleColorsDark();
		ImGui_ImplSDL3_InitForOpenGL(window, gl_context);
		ImGui_ImplOpenGL3_Init(glsl_version);
		auto windowWidth = 0;
		auto windowHeight = 0;
		SDL_GetWindowSize(window, &windowWidth, &windowHeight);
		game.renderContext =
			std::make_unique<RenderContext>(static_cast<u32>(windowWidth), static_cast<u32>(windowHeight));

		// Our state
		bool showDemoWindow = true;
		bool show_another_window = false;
		ImVec4 clear_color = ImVec4(100.0f / 255.f, 149.f / 255.f, 237.f / 255.f, 1.00f);

		// OPENGL DSA example:
		// https://github.com/g-truc/ogl-samples/blob/38cada7a9458864265e25415ae61586d500ff5fc/samples/gl-450-direct-state-access.cpp
		//---------------------------------------------------
		//		Create Compressed MipMapped Texture
		//---------------------------------------------------
		const auto assetName = "Assets/great_husk_sentry.DDS";
		const auto texture_data = gli::texture2d(gli::load(assetName));
		gli::gl GL(gli::gl::PROFILE_GL33);
		const auto format = GL.translate(texture_data.format(), texture_data.swizzles());
		const auto mips = texture_data.levels();
		const auto width = texture_data.extent().x;
		const auto height = texture_data.extent().y;

		GLuint texture_handle;
		glCreateTextures(GL_TEXTURE_2D, 1, &texture_handle);
		glObjectLabel(GL_TEXTURE, texture_handle, glLabel(assetName));
		glTextureParameteri(texture_handle, GL_TEXTURE_BASE_LEVEL, 0);
		glTextureParameteri(texture_handle, GL_TEXTURE_MAX_LEVEL, static_cast<GLint>(mips - 1));
		glTextureParameteri(texture_handle, GL_TEXTURE_MIN_FILTER, mips == 1 ? GL_LINEAR : GL_LINEAR_MIPMAP_LINEAR);
		glTextureParameteri(texture_handle, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTextureStorage2D(texture_handle, static_cast<GLint>(mips), format.Internal, width, height);
		for (auto i = 0; i < mips; i++)
		{
			glCompressedTextureSubImage2D(texture_handle, static_cast<GLint>(i), 0, 0,
										  static_cast<GLsizei>(texture_data[i].extent().x),
										  static_cast<GLsizei>(texture_data[i].extent().y), format.Internal,
										  static_cast<GLsizei>(texture_data[i].size()), texture_data[i].data());
		}

		//---------------------------------------------------

		auto spriteBatchFramebuffer = game.renderContext->CreateFramebuffer(FramebufferDescriptor{
			.colorAttachment = { Texture2DDescriptor{ .extent = DynamicExtent{},
													  .format = TextureFormat::rgba8,
													  .debugName = "sprites_color_render_target" } },
			.depthAttachment = Texture2DDescriptor{ .extent = DynamicExtent{},
													.format = TextureFormat::d32f,
													.debugName = "sprites_depth_render_target" },
			.debugName = "sprites_fb" });
		//---------------------------------------------------------------

		const auto vertexShaderCode = R"(
	#version 460

	layout(location = 0) in vec2 Position;
	layout(location = 1) in vec2 Texcoord;
	layout(location = 2) in vec3 Color;

	out gl_PerVertex
	{
		vec4 gl_Position;
	};

	out block
	{
		vec2 Texcoord;
	}
	Out;

	void main()
	{
		Out.Texcoord = Texcoord;
		gl_Position = vec4(Position, 0.0, 1.0);
	}
	)";

		const auto fragmentShaderCode = R"(
	#version 460

	in block
	{
		vec2 Texcoord;
	} In;

	uniform sampler2D texture;
	layout(location = 0) out vec4 Color;

	void main()
	{
		Color = vec4(In.Texcoord,0.0f,1.0f);
	}
)";

		const auto sourcesVert = std::array{ vertexShaderCode };
		const auto sourcesFrag = std::array{ fragmentShaderCode };

		GLuint fooVertProgram = glCreateShaderProgramv(GL_VERTEX_SHADER, 1, sourcesVert.data());
		glObjectLabel(GL_PROGRAM, fooVertProgram, glLabel("vs_01"));
		GLuint fooFragProgram = glCreateShaderProgramv(GL_FRAGMENT_SHADER, 1, sourcesFrag.data());
		glObjectLabel(GL_PROGRAM, fooVertProgram, glLabel("fs_01"));


		GLuint fooPipeline;
		glCreateProgramPipelines(1, &fooPipeline);
		glObjectLabel(GL_PROGRAM_PIPELINE, fooVertProgram, glLabel("pipeline_01"));
		glUseProgramStages(fooPipeline, GL_VERTEX_SHADER_BIT, fooVertProgram);
		glUseProgramStages(fooPipeline, GL_FRAGMENT_SHADER_BIT, fooFragProgram);

		//-------------------------------------
		struct QuadVertex
		{
			glm::vec2 position;
			glm::vec2 uv;
		};
		auto quadData = std::array{
			QuadVertex{ { 0.0, 0.0 }, { 0.0, 0.0 } }, QuadVertex{ { 1.0, 0.0 }, { 1.0, 0.0 } },
			QuadVertex{ { 1.0, 1.0 }, { 1.0, 1.0 } }, QuadVertex{ { 0.0, 0.0 }, { 0.0, 0.0 } },
			QuadVertex{ { 1.0, 1.0 }, { 1.0, 1.0 } }, QuadVertex{ { 0.0, 1.0 }, { 0.0, 1.0 } },
		};

		//------------------------------------
		// TODO: per frame vertex generation
		GLuint vertexBuffer;
		glCreateBuffers(1, &vertexBuffer);
		glObjectLabel(GL_BUFFER, vertexBuffer, glLabel("vertex_buffer_01"));
		glNamedBufferStorage(vertexBuffer, sizeof(QuadVertex) * quadData.size(), quadData.data(), 0);
		//------------------------------------


		auto animationGraph = AnimationGraph{};
		for (auto i = 0; i < animations.size(); i++)
		{
			animationGraph.AddNode(animations[i].name, i);
		}

		animationGraph.AddTransition("idle-right", "turn-left", animation::sync::immediate);
		animationGraph.AddTransition("idle-right", "walk-right", animation::sync::immediate);
		animationGraph.AddTransition("idle-left", "turn-right", animation::sync::immediate);
		animationGraph.AddTransition("idle-left", "walk-left", animation::sync::immediate);
		animationGraph.AddTransition("walk-right", "idle-right", animation::sync::immediate);
		animationGraph.AddTransition("walk-left", "idle-left", animation::sync::immediate);
		animationGraph.AddTransition("walk-right", "turn-left", animation::sync::immediate);
		animationGraph.AddTransition("walk-left", "turn-right", animation::sync::immediate);

		animationGraph.AddTransition("turn-right", "idle-right", animation::sync::lastFrame);
		animationGraph.AddTransition("turn-right", "walk-right", animation::sync::lastFrame);
		animationGraph.AddTransition("turn-left", "idle-left", animation::sync::lastFrame);
		animationGraph.AddTransition("turn-left", "walk-left", animation::sync::lastFrame);

		auto characterAnimationInstance =
			AnimationInstance{ .currentNodeIndex = animationGraph.GetNodeIndex("idle-right"), .key = 0 };

		auto characterAnimationSequence = AnimationSequence{};

		auto player = AnimationPlayer{};

		SpriteBatch spriteBatch(game.renderContext.get());
		SpriteTexture texture1;
		SpriteTexture texture2;

		// Main loop
		bool done = false;

		/*====================================================================================*/
		/*====================================================================================*/

		//GLuint fullscreenQuadPipeline;
		//{
			const auto vertexShaderCode1 = R"(
	#version 460

	out gl_PerVertex
	{
		vec4 gl_Position;
	};

	void main()
	{	
		gl_Position = gl_Position = vec4(4.f * (gl_VertexID % 2) - 1.f, 4.f * (gl_VertexID / 2) - 1.f, 0.0, 1.0);
	}
	)";

			const auto fragmentShaderCode1 = R"(
	#version 460

	layout(binding = 0) uniform sampler2D basicTexture;

	layout(location = 0, index = 0) out vec4 color;

	void main()
	{
		vec2 textureSize = vec2(textureSize(basicTexture, 0));

		color = texture(basicTexture, gl_FragCoord.xy / textureSize);
	}
)";
//
//
//			auto validateProgram = [](GLuint program)
//			{
//				int success;
//				glGetProgramiv(program, GL_LINK_STATUS, &success);
//				if (!success)
//				{
//					char infoLog[512];
//					glGetProgramInfoLog(program, 512, NULL, infoLog);
//					std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
//				}
//			};
//
//			const auto sourcesVert1 = std::array{ vertexShaderCode1 };
//			const auto sourcesFrag1 = std::array{ fragmentShaderCode1 };
//
//			const auto fooVertProgram1 = glCreateShaderProgramv(GL_VERTEX_SHADER, 1, sourcesVert1.data());
//			glObjectLabel(GL_PROGRAM, fooVertProgram1, glLabel("vs_01"));
//			const auto fooFragProgram1 = glCreateShaderProgramv(GL_FRAGMENT_SHADER, 1, sourcesFrag1.data());
//			glObjectLabel(GL_PROGRAM, fooVertProgram1, glLabel("fs_01"));
//
//
//			glCreateProgramPipelines(1, &fullscreenQuadPipeline);
//			// glObjectLabel(GL_PROGRAM_PIPELINE, fooVertProgram1, glLabel("fullscreenQuadPipeline"));
//			glUseProgramStages(fullscreenQuadPipeline, GL_VERTEX_SHADER_BIT, fooVertProgram1);
//			glUseProgramStages(fullscreenQuadPipeline, GL_FRAGMENT_SHADER_BIT, fooFragProgram1);

			const auto fullscreenQuadPipeline = game.renderContext->CreateGraphicsPipeline(
				GraphicsPipelineDescriptor{ .vertexShaderCode = { std::string{ vertexShaderCode1 }, "vs_01" },
											.fragmentShaderCode = { std::string{ fragmentShaderCode1 }, "fs_01" },
											.debugName = "fullscreenQuadPipeline" });

			/*validateProgram(fooVertProgram1);
			validateProgram(fooFragProgram1);
		}*/


		/*====================================================================================*/
		/*====================================================================================*/
		game.OnLoad();

		while (!done)
		{
			auto event = SDL_Event{};
			while (SDL_PollEvent(&event))
			{
				ImGui_ImplSDL3_ProcessEvent(&event);
				if (event.type == SDL_EVENT_QUIT)
					done = true;
				if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(window))
					done = true;

				if (event.type == SDL_EVENT_WINDOW_RESIZED)
				{
					auto newWidth = u32{};
					auto newHeight = u32{};
					SDL_GetWindowSize(window, (int*)&newWidth, (int*)&newHeight);
					windowWidth = newWidth;
					windowHeight = newHeight;
					game.renderContext->UpdateWindowSize(newWidth, newHeight);
				}
			}
			if (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED)
			{
				SDL_Delay(10);
				continue;
			}

			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplSDL3_NewFrame();
			ImGui::NewFrame();

			game.OnDraw(io.DeltaTime);

			if (showDemoWindow)
			{
				ImGui::ShowDemoWindow(&showDemoWindow);
			}

			ImGui::SliderFloat("frame duration", &player.frameDuration, 0.01f, 1.0f);

			if (ImGui::Button("switch to walk right"))
			{
				characterAnimationSequence =
					animationGraph.FindAnimationSequence(characterAnimationInstance, "walk-right");
			}
			if (ImGui::Button("switch to walk left"))
			{
				characterAnimationSequence =
					animationGraph.FindAnimationSequence(characterAnimationInstance, "walk-left");
			}
			if (ImGui::Button("switch to idle"))
			{
				if (std::regex_match(animations[characterAnimationInstance.currentNodeIndex].name,
									 std::regex{ "[a-z]+\\-right" }))
				{
					characterAnimationSequence =
						animationGraph.FindAnimationSequence(characterAnimationInstance, "idle-right");
				}
				if (std::regex_match(animations[characterAnimationInstance.currentNodeIndex].name,
									 std::regex{ "[a-z]+\\-left" }))
				{
					characterAnimationSequence =
						animationGraph.FindAnimationSequence(characterAnimationInstance, "idle-left");
				}
			}

			player.ForwardTime(io.DeltaTime);
			characterAnimationInstance =
				player.ForwardAnimation(characterAnimationInstance, characterAnimationSequence);


			const auto& animation = animations[characterAnimationInstance.currentNodeIndex];
			const auto& animationKey = animationSequences[animation.animationIndex + characterAnimationInstance.key];
			const auto& frame = animationFrames[animationKey.frameIndex];

			const auto srcRect = frame.sourceSprite;
			const auto uv0 = (srcRect.position) / vec2{ width, height };
			const auto uv1 = (srcRect.position + srcRect.extent) / vec2{ width, height };


			spriteBatch.Begin();
			spriteBatch.Draw(texture1, vec2{ 0.0f, 0.0f });
			spriteBatch.End();

			if (animationKey.flip == FrameFlip::horizontal)
			{
				ImGui::Image((ImTextureID)texture_handle, vec2{ srcRect.extent.x, srcRect.extent.y },
							 vec2{ uv1.x, uv0.y }, vec2{ uv0.x, uv1.y });
			}
			else
			{
				ImGui::Image((ImTextureID)texture_handle, ImVec2{ srcRect.extent.x, srcRect.extent.y },
							 vec2{ uv0.x, uv0.y }, vec2{ uv1.x, uv1.y });
			}


			ImGui::Image((ImTextureID)texture_handle, vec2{ width, height } / 8.0f);
			ImGui::SameLine();

			const auto& spriteFramebuffer = game.renderContext->Get(spriteBatchFramebuffer);
			const auto& spriteColorTexture = game.renderContext->Get(spriteFramebuffer.colorAttachment[0]);
			const auto& spriteDepthTexture = game.renderContext->Get(spriteFramebuffer.depthAttachment.value());

			ImGui::Image((ImTextureID)spriteColorTexture.nativeHandle, vec2{ windowWidth, windowHeight } / 4.0f);

			ImGui::Render();

			//---------PASS--------------------------------
			glViewport(0, 0, windowWidth, windowHeight);
			glClipControl(GL_LOWER_LEFT, GL_NEGATIVE_ONE_TO_ONE);
			glFrontFace(GL_CCW);
			glCullFace(GL_BACK);

			glEnable(GL_CULL_FACE);

			const auto colorClearValue = std::array{ 1.0f, 1.0f, 1.0f, 0.0f };
			glClearNamedFramebufferfv(spriteFramebuffer.nativeHandle, GL_COLOR, 0, colorClearValue.data());
			glClearNamedFramebufferfi(spriteFramebuffer.nativeHandle, GL_DEPTH_STENCIL, 0, 0.0f, 0);

			glBindFramebuffer(GL_FRAMEBUFFER, spriteFramebuffer.nativeHandle);

			// auto result = glCheckNamedFramebufferStatus(spriteFramebuffer.nativeHandle, GL_FRAMEBUFFER);

			glBindProgramPipeline(fooPipeline);
			glBindVertexArray(spriteBatch.vertexArrayObject);
			// TODO:glBindTextureUnit, glBindSamplers, glBindBufferRange for uniforms
			glDrawArraysInstancedBaseInstance(GL_TRIANGLES, 0, 6 * 3, 1, 0);
			//---------------------------------------------

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
			glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w,
						 clear_color.w);
			glClear(GL_COLOR_BUFFER_BIT);
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glBindProgramPipeline(game.renderContext->Get(fullscreenQuadPipeline).nativeHandle);
			glBindTextureUnit(0, spriteColorTexture.nativeHandle);

			glDrawArraysInstancedBaseInstance(GL_TRIANGLES, 0, 3, 1, 0);
			glDisable(GL_BLEND);
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
			SDL_GL_SwapWindow(window);
		}

		// Cleanup
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplSDL3_Shutdown();
		ImGui::DestroyContext();

		glDeleteProgramPipelines(1, &fooPipeline);
		glDeleteBuffers(1, &vertexBuffer);
		glDeleteProgram(fooVertProgram);
		glDeleteProgram(fooFragProgram);
		glDeleteTextures(1, &texture_handle);
		game.renderContext->DestroyFramebuffer(spriteBatchFramebuffer);

		SDL_GL_DestroyContext(gl_context);
		SDL_DestroyWindow(window);
		SDL_Quit();
	}
};


Game::Game()
{
	impl = std::make_unique<Game::GameImpl>();
}

Game::~Game()
{
}

void Game::Run(int argc, char* argv[])
{
	impl->Run(*this, argc, argv);
}
