#include "Game.hpp"

#include <SDL3/SDL.h>
#include <gli/gli.hpp>

#include <assert.h>
#include <print>

#include "Animation.hpp"
#include "Common.hpp"
#include "ContentManager.hpp"
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
		if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
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
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NavEnableGamepad;
		io.BackendFlags = ImGuiBackendFlags_HasGamepad;


		ImGui::StyleColorsDark();
		ImGui_ImplSDL3_InitForOpenGL(window, gl_context);
		ImGui_ImplOpenGL3_Init(glsl_version);
		auto windowWidth = 0;
		auto windowHeight = 0;
		SDL_GetWindowSize(window, &windowWidth, &windowHeight);
		game.renderContext =
			std::make_unique<RenderContext>(static_cast<u32>(windowWidth), static_cast<u32>(windowHeight));

		game.content = std::make_unique<ContentManager>(game.renderContext.get(), "Assets");


		auto spriteBatchFramebuffer = game.renderContext->CreateFramebuffer(FramebufferDescriptor{
			.colorAttachment = { Texture2DDescriptor{ .extent = DynamicExtent{},
													  .format = TextureFormat::rgba8,
													  .debugName = "sprites_color_render_target" } },
			.depthAttachment = Texture2DDescriptor{ .extent = DynamicExtent{},
													.format = TextureFormat::d32f,
													.debugName = "sprites_depth_render_target" },
			.debugName = "sprites_fb" });


		const auto defaultSpriteBatchPipeline = game.renderContext->CreateGraphicsPipeline(GraphicsPipelineDescriptor{
			.vertexShaderCode = { game.content->LoadText("Shaders/DefaultSpriteBatch.vert"),
								  "Shaders/DefaultSpriteBatch.vert" },
			.fragmentShaderCode = { game.content->LoadText("Shaders/DefaultSpriteBatch.frag"),
									"Shaders/DefaultSpriteBatch.frag" },
			.debugName = "DefaultSpriteBatchPipeline" });


		SpriteBatch spriteBatch(game.renderContext.get());
		SpriteTexture texture1;
		SpriteTexture texture2;

		const auto fullscreenQuadPipeline = game.renderContext->CreateGraphicsPipeline(
			GraphicsPipelineDescriptor{ .vertexShaderCode = { game.content->LoadText("Shaders/FullscreenBlit.vert"),
															  "Shaders/FullscreenBlit.vert" },
										.fragmentShaderCode = { game.content->LoadText("Shaders/FullscreenBlit.frag"),
																"Shaders/FullscreenBlit.frag" },
										.debugName = "fullscreenQuadPipeline" });


		game.OnLoad();

		bool done = false;
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

			game.OnUpdate(io.DeltaTime);
			game.OnDraw(io.DeltaTime);

			spriteBatch.Begin();
			spriteBatch.Draw(texture1, vec2{ 0.0f, 0.0f });
			spriteBatch.End();


			/*
						ImGui::Image((ImTextureID)huskSpriteTexture.nativeHandle, vec2{ huskSpriteTexture.width,
			   huskSpriteTexture.height } / 8.0f); ImGui::SameLine();*/

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

			glBindProgramPipeline(game.renderContext->Get(defaultSpriteBatchPipeline).nativeHandle);
			glBindVertexArray(spriteBatch.vertexArrayObject);
			// TODO:glBindTextureUnit, glBindSamplers, glBindBufferRange for uniforms
			glDrawArraysInstancedBaseInstance(GL_TRIANGLES, 0, 6 * 3, 1, 0);
			//---------------------------------------------

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
			constexpr auto clearColor = vec4(100.0f / 255.f, 149.f / 255.f, 237.f / 255.f, 1.00f);
			glClearColor(clearColor.x * clearColor.w, clearColor.y * clearColor.w, clearColor.z * clearColor.w,
						 clearColor.w);
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

		game.OnUnload();
		game.renderContext->DestroyGraphicsPipeline(fullscreenQuadPipeline);
		game.renderContext->DestroyGraphicsPipeline(defaultSpriteBatchPipeline);
		game.renderContext->DestroyFramebuffer(spriteBatchFramebuffer);

		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplSDL3_Shutdown();
		ImGui::DestroyContext();

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
