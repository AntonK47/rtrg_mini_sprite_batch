#include "Game.hpp"

#include <SDL3/SDL.h>

#include <assert.h>
#include <print>
#include <chrono>

#include "Animation.hpp"
#include "Common.hpp"
#include "ContentManager.hpp"
#include "ImGui.hpp"
#include "RenderContext.hpp"

#include <tracy/Tracy.hpp>

#ifdef TRACY_ENABLE
void* operator new(std::size_t count)
{
	auto ptr = malloc(count);
	TracyAllocS(ptr, count, 30);
	return ptr;
}
void operator delete(void* ptr) noexcept
{
	TracyFreeS(ptr, 30);
	free(ptr);
}
#endif
namespace
{
	void __stdcall MessageCallback(GLenum, GLenum type, GLuint, GLenum severity, GLsizei, const GLchar* message,
								   const void*)
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
		for (auto i = 0; i < argc; i++)
		{
			if (!strcmp(argv[i], "--disable_resource_download"))
			{
				EnableResourceFileDownload = false;
			}
		}


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
		// TODO: ther is an issue with vsync, domehow it is not working in a windowed mode
		//SDL_SetWindowFullscreen(window, true);

		SDL_GLContext gl_context = SDL_GL_CreateContext(window);


		if (gl_context == nullptr)
		{
			printf("Error: SDL_GL_CreateContext(): %s\n", SDL_GetError());
			return;
		}

		gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress);

		SDL_GL_MakeCurrent(window, gl_context);
		SDL_GL_SetSwapInterval(1);
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

		auto time = std::chrono::high_resolution_clock::now();

		game.OnLoad();

		bool done = false;
		while (!done)
		{
			const auto newTime = std::chrono::high_resolution_clock::now();
			const auto frameTime = (newTime - time);
			const auto frameTimeInSeconds = std::chrono::duration<f32>(frameTime).count();
			time = newTime;
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

			game.OnUpdate(frameTimeInSeconds);
			{
				ZoneScopedNS("Draw", 30);
				
				game.OnDraw(frameTimeInSeconds);
			}

			ImGui::LabelText("Delta Time", "%f", frameTimeInSeconds);
			ImGui::Render();
			
			//glFinish();
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
			SDL_GL_SwapWindow(window);
			FrameMark;
		}

		game.OnUnload();


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
