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

void __stdcall MessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
							   const GLchar* message, const void* userParam)
{
	std::println(stderr, "GL debug message: {} type = 0x{}, severity = 0x{}, message = {}\n",
				 (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""), type, severity, message);
}

#define glLabel(s) (GLuint) strlen(s), s

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

	// Our state
	bool show_demo_window = true;
	bool show_another_window = false;
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	//OPENGL DSA example: https://github.com/g-truc/ogl-samples/blob/38cada7a9458864265e25415ae61586d500ff5fc/samples/gl-450-direct-state-access.cpp
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

	auto windowWidth = 0;
	auto windowHeight = 0;
	SDL_GetWindowSize(window, &windowWidth, &windowHeight);

	GLuint spriteFramebufferColorTexture;
	glCreateTextures(GL_TEXTURE_2D, 1, &spriteFramebufferColorTexture);
	glTextureParameteri(spriteFramebufferColorTexture, GL_TEXTURE_BASE_LEVEL, 0);
	glTextureParameteri(spriteFramebufferColorTexture, GL_TEXTURE_MAX_LEVEL, 0);
	glTextureStorage2D(spriteFramebufferColorTexture, 1, GL_RGBA8, static_cast<GLsizei>(windowWidth),
					   static_cast<GLsizei>(windowHeight));
	glObjectLabel(GL_TEXTURE, spriteFramebufferColorTexture, glLabel("sprites_color_render_target"));

	GLuint spriteFramebufferDepthTexture;
	glCreateTextures(GL_TEXTURE_2D, 1, &spriteFramebufferDepthTexture);
	glTextureParameteri(spriteFramebufferDepthTexture, GL_TEXTURE_BASE_LEVEL, 0);
	glTextureParameteri(spriteFramebufferDepthTexture, GL_TEXTURE_MAX_LEVEL, 0);
	glTextureStorage2D(spriteFramebufferDepthTexture, 1, GL_DEPTH_COMPONENT32F, static_cast<GLsizei>(windowWidth),
					   static_cast<GLsizei>(windowHeight));
	glObjectLabel(GL_TEXTURE, spriteFramebufferColorTexture, glLabel("sprites_depth_render_target"));


	GLuint spriteRendererFramebuffer;
	glCreateFramebuffers(1, &spriteRendererFramebuffer);
	glObjectLabel(GL_FRAMEBUFFER, spriteRendererFramebuffer, glLabel("sprites_fb"));
	glNamedFramebufferTexture(spriteRendererFramebuffer, GL_COLOR_ATTACHMENT0, spriteFramebufferColorTexture, 0);
	glNamedFramebufferTexture(spriteRendererFramebuffer, GL_DEPTH_ATTACHMENT, spriteFramebufferDepthTexture, 0);
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
	glObjectLabel(GL_PROGRAM, fooVertProgram, glLabel("vs_01"));
	GLuint fooFragProgram = glCreateShaderProgramv(GL_FRAGMENT_SHADER, 1, sourcesFrag.data());
	glObjectLabel(GL_PROGRAM, fooVertProgram, glLabel("fs_01"));


	GLuint fooPipeline;
	glCreateProgramPipelines(1, &fooPipeline);
	glObjectLabel(GL_PROGRAM_PIPELINE, fooVertProgram, glLabel("pipeline_01"));
	glUseProgramStages(fooPipeline, GL_VERTEX_SHADER_BIT, fooVertProgram);
	glUseProgramStages(fooPipeline, GL_FRAGMENT_SHADER_BIT, fooFragProgram);


	struct QuadVertex
	{
		glm::vec2 position;
		glm::vec2 uv;
	};
	auto quadData = std::array{
		QuadVertex{ { 0.0, 0.0 }, { 0.0, 0.0 } },
		QuadVertex{ { 1.0, 0.0 }, { 1.0, 0.0 } },
		QuadVertex{ { 1.0, 1.0 }, { 1.0, 1.0 } },
		QuadVertex{ { 0.0, 0.0 }, { 0.0, 0.0 } },
		QuadVertex{ { 1.0, 1.0 }, { 1.0, 1.0 } },
		QuadVertex{ { 0.0, 1.0 }, { 0.0, 1.0 } },
	};

	GLuint vertexBuffer;
	glCreateBuffers(1, &vertexBuffer);
	glObjectLabel(GL_BUFFER, vertexBuffer, glLabel("vertex_buffer_01"));
	glNamedBufferStorage(vertexBuffer, sizeof(QuadVertex) * quadData.size(), quadData.data(), 0);

	GLuint fooVertexArray;
	glCreateVertexArrays(1, &fooVertexArray);
	glObjectLabel(GL_VERTEX_ARRAY, fooVertexArray, glLabel("vao_01"));
	const GLuint positionAttribute = 0;
	const GLuint texcoordAttribute = 1;

	glVertexArrayVertexBuffer(fooVertexArray, 0, vertexBuffer, 0, sizeof(QuadVertex));

	glEnableVertexArrayAttrib(fooVertexArray, positionAttribute);
	glVertexArrayAttribBinding(fooVertexArray, positionAttribute, 0);
	glVertexArrayAttribFormat(fooVertexArray, positionAttribute, 2, GL_FLOAT, GL_FALSE, offsetof(QuadVertex, position));

	glEnableVertexArrayAttrib(fooVertexArray, texcoordAttribute);
	glVertexArrayAttribBinding(fooVertexArray, texcoordAttribute, 0);
	glVertexArrayAttribFormat(fooVertexArray, texcoordAttribute, 2, GL_FLOAT, GL_FALSE, offsetof(QuadVertex, uv));


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


		ImGui::Image((ImTextureID)texture_handle, ImVec2{ width / 8.0f, height / 8.0f });
		ImGui::SameLine();
		ImGui::Image((ImTextureID)spriteFramebufferColorTexture, ImVec2{ windowWidth / 4.0f, windowHeight / 4.0f });

		ImGui::Render();

		//---------PASS--------------------------------
		glViewport(0, 0, windowWidth, windowHeight);
		glClipControl(GL_LOWER_LEFT, GL_NEGATIVE_ONE_TO_ONE);
		glFrontFace(GL_CCW);
		glCullFace(GL_BACK);

		glEnable(GL_CULL_FACE);

		const auto colorClearValue = std::array{ 1.0f, 1.0f, 1.0f, 0.0f };
		glClearNamedFramebufferfv(spriteRendererFramebuffer, GL_COLOR, 0, colorClearValue.data());
		glClearNamedFramebufferfi(spriteRendererFramebuffer, GL_DEPTH_STENCIL, 0, 0.0f, 0);

		glBindFramebuffer(GL_FRAMEBUFFER, spriteRendererFramebuffer);
		glBindProgramPipeline(fooPipeline);
		glBindVertexArray(fooVertexArray);
		// TODO:glBindTextureUnit, glBindSamplers, glBindBufferRange for uniforms
		glDrawArraysInstancedBaseInstance(GL_TRIANGLES, 0, 6, 1, 0);
		//---------------------------------------------


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


	glDeleteProgramPipelines(1, &fooPipeline);
	glDeleteBuffers(1, &vertexBuffer);
	glDeleteProgram(fooVertProgram);
	glDeleteProgram(fooFragProgram);
	glDeleteTextures(1, &texture_handle);
	glDeleteTextures(1, &spriteFramebufferColorTexture);
	glDeleteTextures(1, &spriteFramebufferDepthTexture);
	glDeleteFramebuffers(1, &spriteRendererFramebuffer);
	glDeleteVertexArrays(1, &fooVertexArray);

	SDL_GL_DestroyContext(gl_context);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}