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

#include <assert.h>
#include <cstdint>
#include <iostream>
#include <regex>
#include <stack>
#include <variant>
#include <vector>

void __stdcall MessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
							   const GLchar* message, const void* userParam)
{
	std::println(stderr, "GL debug message: {} type = 0x{}, severity = 0x{}, message = {}\n",
				 (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""), type, severity, message);
}

#define glLabel(s) (GLuint) strlen(s), s

struct Rectangle
{
	glm::vec2 position;
	glm::vec2 extent;
};

struct AnimationFrame
{
	Rectangle sourceSprite;
	glm::vec2 root;
};

enum class AnimationRepeat
{
	once,
	loop
};

struct Animation
{
	std::string name;
	uint32_t animationIndex;
	uint32_t animationFrameCount;
	AnimationRepeat repeat{ AnimationRepeat::once };
};

enum class FrameFlip
{
	none,
	horizontal
};

struct AnimationKey
{
	uint32_t frameIndex{ 0 };
	uint32_t duration{ 1 };
	FrameFlip flip{ FrameFlip::none };
};

static const auto animationFrames = std::array{
	AnimationFrame{ { { 3, 22 }, { 316 - 3, 368 - 22 } }, { 131, 366 } },
	AnimationFrame{ { { 320, 22 }, { 633 - 320, 368 - 22 } }, { 447, 365 } },
	AnimationFrame{ { { 637, 22 }, { 950 - 637, 368 - 22 } }, { 764, 366 } },
	AnimationFrame{ { { 954, 22 }, { 1267 - 954, 368 - 22 } }, { 1080, 366 } },
	AnimationFrame{ { { 1271, 22 }, { 1584 - 1271, 368 - 22 } }, { 1958, 365 } },

	AnimationFrame{ { { 3, 1098 }, { 304 - 3, 1452 - 1098 } }, { 118, 1447 } },
	AnimationFrame{ { { 308, 1098 }, { 609 - 308, 1452 - 1098 } }, { 423, 1449 } },

	AnimationFrame{ { { 3, 391 }, { 317 - 3, 731 - 391 } }, { 191, 725 } },
	AnimationFrame{ { { 321, 391 }, { 635 - 321, 731 - 391 } }, { 491, 727 } },
	AnimationFrame{ { { 639, 391 }, { 953 - 639, 731 - 391 } }, { 771, 730 } },
	AnimationFrame{ { { 957, 391 }, { 1271 - 957, 731 - 391 } }, { 1062, 728 } },
	AnimationFrame{ { { 1275, 391 }, { 1589 - 1275, 731 - 391 } }, { 1358, 725 } },
	AnimationFrame{ { { 1593, 391 }, { 1907 - 1593, 731 - 391 } }, { 1673, 713 } },
	AnimationFrame{ { { 3, 735 }, { 317 - 3, 1075 - 735 } }, { 0, 0 } },
	AnimationFrame{ { { 321, 735 }, { 635 - 321, 1075 - 735 } }, { 0, 0 } },
};

static const auto animationSequences = std::array{
	AnimationKey{ 0, 1, FrameFlip::none },		  AnimationKey{ 1, 1, FrameFlip::none },
	AnimationKey{ 2, 1, FrameFlip::none },		  AnimationKey{ 3, 1, FrameFlip::none },
	AnimationKey{ 4, 1, FrameFlip::none },		  AnimationKey{ 3, 1, FrameFlip::none },
	AnimationKey{ 2, 1, FrameFlip::none },		  AnimationKey{ 1, 1, FrameFlip::none }, // Idle Right

	AnimationKey{ 0, 1, FrameFlip::horizontal },  AnimationKey{ 1, 1, FrameFlip::horizontal },
	AnimationKey{ 2, 1, FrameFlip::horizontal },  AnimationKey{ 3, 1, FrameFlip::horizontal },
	AnimationKey{ 4, 1, FrameFlip::horizontal },  AnimationKey{ 3, 1, FrameFlip::horizontal },
	AnimationKey{ 2, 1, FrameFlip::horizontal },  AnimationKey{ 1, 1, FrameFlip::horizontal }, // Idle Left

	AnimationKey{ 5, 1, FrameFlip::none },		  AnimationKey{ 6, 1, FrameFlip::none }, // Turn Left
	AnimationKey{ 5, 1, FrameFlip::horizontal },  AnimationKey{ 6, 1, FrameFlip::horizontal }, // Turn Right,

	AnimationKey{ 7, 1, FrameFlip::none },		  AnimationKey{ 8, 1, FrameFlip::none },
	AnimationKey{ 9, 1, FrameFlip::none },		  AnimationKey{ 10, 1, FrameFlip::none },
	AnimationKey{ 11, 1, FrameFlip::none },		  AnimationKey{ 12, 1, FrameFlip::none },
	AnimationKey{ 13, 1, FrameFlip::none },		  AnimationKey{ 14, 1, FrameFlip::none },

	AnimationKey{ 7, 1, FrameFlip::horizontal },  AnimationKey{ 8, 1, FrameFlip::horizontal },
	AnimationKey{ 9, 1, FrameFlip::horizontal },  AnimationKey{ 10, 1, FrameFlip::horizontal },
	AnimationKey{ 11, 1, FrameFlip::horizontal }, AnimationKey{ 12, 1, FrameFlip::horizontal },
	AnimationKey{ 13, 1, FrameFlip::horizontal }, AnimationKey{ 14, 1, FrameFlip::horizontal },

};

static const auto animations = std::array{
	Animation{ "idle-right", 0, 8, AnimationRepeat::loop },	 Animation{ "idle-left", 8, 8, AnimationRepeat::loop },
	Animation{ "turn-left", 16, 2, AnimationRepeat::once },	 Animation{ "turn-right", 18, 2, AnimationRepeat::once },
	Animation{ "walk-right", 20, 8, AnimationRepeat::loop }, Animation{ "walk-left", 28, 8, AnimationRepeat::loop }
};

struct AnimationInstance
{
	uint32_t currentNodeIndex;
	uint32_t key;
};

struct SequenceItem
{
	uint32_t node;
	int key;

	auto operator<=>(const SequenceItem&) const = default;
};
using AnimationSequence = std::vector<SequenceItem>;

struct AnimationPlayer
{
	AnimationInstance ForwardAnimation(AnimationInstance instance, AnimationSequence& sequence);
	void ForwardTime(const float deltaTime);

	float localTime;
	float frameDuration{ 0.016f };
	bool nextKey{ false };
};

struct AnimationState
{
	std::string name;
};


struct AnimationTransition
{
	uint32_t key;
	uint32_t nodeIndex;
};

struct SyncOnKey
{
	uint32_t key;
};

struct SyncOnLastFrame
{
};

struct SyncImmediate
{
};

namespace animation::sync
{
	inline constexpr SyncOnLastFrame lastFrame{};
	inline constexpr SyncOnLastFrame immediate{};

} // namespace animation::sync

using AnimationSyncBehavior = std::variant<SyncOnKey, SyncOnLastFrame, SyncImmediate>;


struct AnimationGraph
{
	using AnimationIndex = uint32_t;


	void AddNode(const std::string& name, const AnimationIndex animationIndex);
	void AddTransition(const std::string& from, const std::string& to,
					   const AnimationSyncBehavior& syncBehavior = animation::sync::immediate);

	AnimationIndex GetNodeIndex(const std::string& nodeName) const;

	AnimationSequence FindAnimationSequence(const AnimationInstance& instance, const std::string& to);


	std::unordered_map<std::string, AnimationIndex> nameToNodeIndexMap;
	std::unordered_map<AnimationIndex, std::vector<AnimationTransition>> transitions;
};


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

	GLuint fooVertexArray;
	glCreateVertexArrays(1, &fooVertexArray);
	glObjectLabel(GL_VERTEX_ARRAY, fooVertexArray, glLabel("vao_01"));
	const GLuint positionAttribute = 0;
	const GLuint texcoordAttribute = 1;

	//------------------------------------
	// TODO: per frame vertex generation
	glVertexArrayVertexBuffer(fooVertexArray, 0, vertexBuffer, 0, sizeof(QuadVertex));
	//------------------------------------

	glEnableVertexArrayAttrib(fooVertexArray, positionAttribute);
	glVertexArrayAttribBinding(fooVertexArray, positionAttribute, 0);
	glVertexArrayAttribFormat(fooVertexArray, positionAttribute, 2, GL_FLOAT, GL_FALSE, offsetof(QuadVertex, position));

	glEnableVertexArrayAttrib(fooVertexArray, texcoordAttribute);
	glVertexArrayAttribBinding(fooVertexArray, texcoordAttribute, 0);
	glVertexArrayAttribFormat(fooVertexArray, texcoordAttribute, 2, GL_FLOAT, GL_FALSE, offsetof(QuadVertex, uv));


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
		AnimationInstance{ .currentNodeIndex = animationGraph.GetNodeIndex("walk-right"), .key = 0 };

	auto characterAnimationSequence = AnimationSequence{};

	auto player = AnimationPlayer{};

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

		// static uint32_t frameIndex = 0;


		ImGui::SliderFloat("frame duration", &player.frameDuration, 0.01f, 1.0f);
		/*static auto currentAnimation = 0;
		auto animationName = [](void* userData, int idx) -> const char* { return animations[idx].name.c_str(); };*/
		/*if (ImGui::ListBox("##listbox", &currentAnimation, animationName, nullptr, animations.size(), 4))
		{
			frameIndex = 0;
			localTime = 0.0f;
		}*/

		if (ImGui::Button("switch to walk right"))
		{
			characterAnimationSequence = animationGraph.FindAnimationSequence(characterAnimationInstance, "walk-right");
		}
		if (ImGui::Button("switch to walk left"))
		{
			characterAnimationSequence = animationGraph.FindAnimationSequence(characterAnimationInstance, "walk-left");
		}
		if (ImGui::Button("swith to idle"))
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
		characterAnimationInstance = player.ForwardAnimation(characterAnimationInstance, characterAnimationSequence);


		const auto& animation = animations[characterAnimationInstance.currentNodeIndex];


		const auto animationKey = animationSequences[animation.animationIndex + characterAnimationInstance.key];
		const auto& frame = animationFrames[animationKey.frameIndex];


		const auto srcRect = frame.sourceSprite;
		const auto uv0 = (srcRect.position) / glm::vec2{ width, height };

		const auto uv1 = (srcRect.position + srcRect.extent) / glm::vec2{ width, height };

		if (animationKey.flip == FrameFlip::horizontal)
		{
			ImGui::Image((ImTextureID)texture_handle, ImVec2{ srcRect.extent.x, srcRect.extent.y },
						 ImVec2{ uv1.x, uv0.y }, ImVec2{ uv0.x, uv1.y });
		}
		else
		{
			ImGui::Image((ImTextureID)texture_handle, ImVec2{ srcRect.extent.x, srcRect.extent.y },
						 ImVec2{ uv0.x, uv0.y }, ImVec2{ uv1.x, uv1.y });
		}


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

void AnimationGraph::AddNode(const std::string& name, const AnimationIndex animationIndex)
{
	assert(not nameToNodeIndexMap.contains(name));
	nameToNodeIndexMap[name] = animationIndex;
}

void AnimationGraph::AddTransition(const std::string& from, const std::string& to,
								   const AnimationSyncBehavior& syncBehavior)
{
	const auto fromIndex = nameToNodeIndexMap[from];
	const auto toIndex = nameToNodeIndexMap[to];
	auto syncKey = uint32_t{ 0 };
	if (std::holds_alternative<SyncOnKey>(syncBehavior))
	{
		const auto sync = std::get<SyncOnKey>(syncBehavior);
		syncKey = sync.key;
	}
	if (std::holds_alternative<SyncOnLastFrame>(syncBehavior))
	{
		const auto lastFrame = animations[fromIndex].animationFrameCount - 1;
		syncKey = lastFrame;
	}
	if (std::holds_alternative<SyncImmediate>(syncBehavior))
	{
		syncKey = -1;
	}
	transitions[fromIndex].push_back(AnimationTransition{ syncKey, toIndex });
}

AnimationGraph::AnimationIndex AnimationGraph::GetNodeIndex(const std::string& nodeName) const
{
	assert(nameToNodeIndexMap.contains(nodeName));
	return nameToNodeIndexMap.at(nodeName);
}

AnimationSequence AnimationGraph::FindAnimationSequence(const AnimationInstance& instance, const std::string& to)
{
	const auto targetNodeIndex = nameToNodeIndexMap.at(to);
	const auto startNodeIndex = instance.currentNodeIndex;

	auto animationSequence = AnimationSequence{};

	animationSequence.push_back(SequenceItem{ 4, -1 });
	animationSequence.push_back(SequenceItem{ 2, 1 });
	animationSequence.push_back(SequenceItem{ 5, -1 });

	return animationSequence;
}


AnimationInstance AnimationPlayer::ForwardAnimation(AnimationInstance instance, AnimationSequence& sequence)
{
	AnimationInstance newInstance;
	newInstance.currentNodeIndex = instance.currentNodeIndex;
	newInstance.key = instance.key;

	if (nextKey)
	{
		const auto& animation = animations[instance.currentNodeIndex];


		if (animation.repeat == AnimationRepeat::loop)
		{
			newInstance.key = (newInstance.key + 1) % animation.animationFrameCount;
		}
		else
		{
			newInstance.key = std::min((newInstance.key + 1), animation.animationFrameCount - 1);
		}

		if (not sequence.empty())
		{
			const auto item = sequence.front();
			if (item.node == newInstance.currentNodeIndex)
			{
				if (item.key == -1 or item.key == newInstance.key)
				{
					sequence.erase(std::remove(sequence.begin(), sequence.end(), item), sequence.end());
					if (not sequence.empty())
					{
						newInstance.currentNodeIndex = sequence.front().node;
						newInstance.key = 0;
					}
				}
			}
		}
	}
	return newInstance;
}

void AnimationPlayer::ForwardTime(const float deltaTime)
{
	nextKey = false;
	localTime += deltaTime;
	if (localTime > frameDuration)
	{
		nextKey = true;
		localTime = fmodf(localTime, frameDuration);
	}
}
