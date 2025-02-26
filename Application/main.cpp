#include <SDL3/SDL.h>
#include <glad/glad.h>
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl3.h>
#define TINYDDS_IMPLEMENTATION
#include <tinydds.h>

#include <gli/gli.hpp>

#include <array>
#include <fstream>
#include <print>

void APIENTRY DebugCallback(uint32_t uiSource, uint32_t uiType, uint32_t uiID, uint32_t uiSeverity, int32_t iLength,
							const char* p_cMessage, void* p_UserParam)
{
	std::println("{}", p_cMessage);
}

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[])
{
	// Setup SDL
	if (!SDL_Init(SDL_INIT_VIDEO))
	{
		std::println("Error: SDL_Init(): {}\n", SDL_GetError());
		return -1;
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
		return -1;
	}
	SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

	SDL_GLContext gl_context = SDL_GL_CreateContext(window);


	if (gl_context == nullptr)
	{
		printf("Error: SDL_GL_CreateContext(): %s\n", SDL_GetError());
		return -1;
	}

	int version = gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress);
	std::println("GL {}.{}\n", version, version);

	SDL_GL_MakeCurrent(window, gl_context);
	SDL_GL_SetSwapInterval(1); // Enable vsync
	SDL_ShowWindow(window);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

	// Set up the debug info callback
	glDebugMessageCallback((GLDEBUGPROC)&DebugCallback, NULL);

	// Set up the type of debug information we want to receive
	uint32_t uiUnusedIDs = 0;
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, &uiUnusedIDs, GL_TRUE); // Enable all
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, NULL,
						  GL_FALSE); // Disable notifications

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	(void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;


	ImGui::StyleColorsDark();
	ImGui_ImplSDL3_InitForOpenGL(window, gl_context);
	ImGui_ImplOpenGL3_Init(glsl_version);

	// Our state
	bool show_demo_window = true;
	bool show_another_window = false;
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	//---------------------------------------------------
	//		Create Compressed MipMapped Texture
	//---------------------------------------------------
	const auto texture_data = gli::texture2d(gli::load(
		"D:/Downloads/SpeedTree_v2/SpeedTree_v2/White Oak/LowPoly/Textures/T_White_Oak_Leaves_Mobile_BaseColor.dds"));
	gli::gl GL(gli::gl::PROFILE_GL33);
	const auto format = GL.translate(texture_data.format(), texture_data.swizzles());
	const auto mips = texture_data.levels();
	const auto width = texture_data.extent().x;
	const auto height = texture_data.extent().y;

	GLuint texture_handle;
	glGenTextures(1, &texture_handle);

	glBindTexture(GL_TEXTURE_2D, texture_handle);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, GLint(mips - 1));
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, mips == 1 ? GL_LINEAR : GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	for (auto i = 0; i < mips; i++)
	{
		glCompressedTexImage2D(GL_TEXTURE_2D, GLint(i), format.Internal,
							   static_cast<GLsizei>(texture_data[i].extent().x),
							   static_cast<GLsizei>(texture_data[i].extent().y), 0,
							   static_cast<GLsizei>(texture_data[i].size()), texture_data[i].data());
	}

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);
	//---------------------------------------------------

	auto windowWidth = 0;
	auto windowHeight = 0;
	SDL_GetWindowSize(window, &windowWidth, &windowHeight);

	GLuint spriteFramebufferColorTexture;
	glGenTextures(1, &spriteFramebufferColorTexture);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, spriteFramebufferColorTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
	glTexImage2D(GL_TEXTURE_2D, GLint(0), GL_RGBA8, GLsizei(windowWidth), GLsizei(windowHeight), 0, GL_RGBA,
				 GL_UNSIGNED_BYTE, NULL);

	GLuint spriteFramebufferDepthTexture;
	glGenTextures(1, &spriteFramebufferDepthTexture);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, spriteFramebufferDepthTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
	glTexImage2D(GL_TEXTURE_2D, GLint(0), GL_DEPTH_COMPONENT24, GLsizei(windowWidth), GLsizei(windowHeight), 0,
				 GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, NULL);


	GLuint spriteRendererFramebuffer;
	glGenFramebuffers(1, &spriteRendererFramebuffer);

	glBindFramebuffer(GL_FRAMEBUFFER, spriteRendererFramebuffer);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, spriteFramebufferColorTexture, 0);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, spriteFramebufferDepthTexture, 0);
	const auto buffersRender = GLenum{ GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, &buffersRender);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	//---------------------------------------------------------------
	const auto vertexShaderCode = R"(#
	#version 460

	layout(location = 0) in vec2 Position;
	layout(location = 1) in vec2 Texcoord;

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

	layout(location = 0) out vec4 Color;

	void main()
	{
		Color = vec4(In.Texcoord,0.0f,1.0f);
	}
)";

	const auto sourcesVert = std::array{ vertexShaderCode };
	const auto sourcesFrag = std::array{ fragmentShaderCode };

	GLuint fooVertProgram = glCreateShaderProgramv(GL_VERTEX_SHADER, 1, sourcesVert.data());
	GLint t;
	GLuint fooFragProgram = glCreateShaderProgramv(GL_FRAGMENT_SHADER, 1, sourcesFrag.data());

	GLuint fooPipeline;
	glGenProgramPipelines(1, &fooPipeline);

	glBindProgramPipeline(fooPipeline);

	glUseProgramStages(fooPipeline, GL_VERTEX_SHADER_BIT, fooVertProgram);
	glUseProgramStages(fooPipeline, GL_FRAGMENT_SHADER_BIT, fooFragProgram);


	struct QuadVertex
	{
		glm::vec2 position;
		glm::vec2 uv;
	};
	auto quadData = std::array{
		QuadVertex{ { 0.0, 0.0 }, { 0.0, 0.0 } },
		QuadVertex{ { 0.5, 0.0 }, { 1.0, 0.0 } },
		QuadVertex{ { 0.5, 0.5 }, { 1.0, 1.0 } },
		QuadVertex{ { 0.0, 0.5 }, { 0.0, 1.0 } },
	};

	GLuint vertexBuffer;
	glGenBuffers(1, &vertexBuffer);

	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(QuadVertex) * quadData.size(), quadData.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	GLuint fooVertexArray;
	glCreateVertexArrays(1, &fooVertexArray);

	const GLuint positionAttribute = 0;
	const GLuint texcoordAttribute = 1;


	glVertexArrayAttribBinding(fooVertexArray, positionAttribute, 0);
	glVertexArrayAttribFormat(fooVertexArray, positionAttribute, 2, GL_FLOAT, GL_FALSE, 0);
	glEnableVertexArrayAttrib(fooVertexArray, positionAttribute);


	glVertexArrayAttribBinding(fooVertexArray, texcoordAttribute, 0);
	glVertexArrayAttribFormat(fooVertexArray, texcoordAttribute, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2));
	glEnableVertexArrayAttrib(fooVertexArray, texcoordAttribute);

	
	// Main loop
	bool done = false;

	while (!done)
	{
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			ImGui_ImplSDL3_ProcessEvent(&event);
			if (event.type == SDL_EVENT_QUIT)
				done = true;
			if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(window))
				done = true;
		}
		if (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED)
		{
			SDL_Delay(10);
			continue;
		}

		// Start the Dear ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplSDL3_NewFrame();
		ImGui::NewFrame();


		if (show_demo_window)
			ImGui::ShowDemoWindow(&show_demo_window);


		ImGui::Image((ImTextureID)texture_handle, ImVec2{ 400, 400 });

		ImGui::Render();

		// glBindFramebuffer(GL_FRAMEBUFFER, spriteRendererFramebuffer);
		glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
		glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w,
					 clear_color.w);
		glClearDepth(0.5f);
		glBindProgramPipeline(fooPipeline);
		{
			const auto error = glGetError();
			std::println("GL Error: {}", error);
		}
		glBindVertexArray(fooVertexArray);
		{
			const auto error = glGetError();
			std::println("GL Error: {}", error);
		}

		glDrawArraysInstancedBaseInstance(GL_TRIANGLES, 0, 3, 1, 0);
		{
			const auto error = glGetError();
			std::println("GL Error: {}", error);
		}
		glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
	{
		const auto error = glGetError();
		std::println("GL Error: {}", error);
	}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
		glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w,
					 clear_color.w);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		SDL_GL_SwapWindow(window);
	}

	// Cleanup
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL3_Shutdown();
	ImGui::DestroyContext();

	SDL_GL_DestroyContext(gl_context);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}