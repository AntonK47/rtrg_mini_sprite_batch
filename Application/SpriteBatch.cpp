#include "SpriteBatch.hpp"

#include "ContentManager.hpp"
#include "Effect.hpp"
#include "RenderContext.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>

#include <tracy/Tracy.hpp>

namespace
{
	TextAsset LoadText(const std::filesystem::path& asset)
	{
		auto fullPath = std::filesystem::path{ "Assets" } / asset;
		auto file = std::ifstream{ fullPath };
		auto stream = std::ostringstream{};
		stream << file.rdbuf();
		return TextAsset{ stream.str() };
	}
} // namespace

SpriteBatch::SpriteBatch(RenderContext* context) : renderContext(context)
{
	glCreateBuffers(1, &vertexBuffer);
	glNamedBufferStorage(vertexBuffer, defaultBufferSize, nullptr, GL_DYNAMIC_STORAGE_BIT);

	glCreateVertexArrays(1, &vertexArrayObject);
	const auto positionAttribute = GLuint{ 0 };
	const auto textureCoordinateAttribute = GLuint{ 1 };
	const auto colorAttribute = GLuint{ 2 };


	glVertexArrayVertexBuffer(vertexArrayObject, 0, vertexBuffer, 0, sizeof(SpriteQuadVertex));


	glEnableVertexArrayAttrib(vertexArrayObject, positionAttribute);
	glVertexArrayAttribBinding(vertexArrayObject, positionAttribute, 0);
	glVertexArrayAttribFormat(vertexArrayObject, positionAttribute, 2, GL_FLOAT, GL_FALSE,
							  offsetof(SpriteQuadVertex, position));

	glEnableVertexArrayAttrib(vertexArrayObject, textureCoordinateAttribute);
	glVertexArrayAttribBinding(vertexArrayObject, textureCoordinateAttribute, 0);
	glVertexArrayAttribFormat(vertexArrayObject, textureCoordinateAttribute, 2, GL_FLOAT, GL_FALSE,
							  offsetof(SpriteQuadVertex, uv));

	glEnableVertexArrayAttrib(vertexArrayObject, colorAttribute);
	glVertexArrayAttribBinding(vertexArrayObject, colorAttribute, 0);
	glVertexArrayAttribFormat(vertexArrayObject, colorAttribute, 4, GL_FLOAT, GL_FALSE,
							  offsetof(SpriteQuadVertex, color));


	auto UniformBufferOffset = GLint{ 0 };
	glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &UniformBufferOffset);
	uniformConstantsSize = glm::max(GLint(sizeof(SpriteBatchConstants)), UniformBufferOffset);

	glCreateBuffers(1, &uniformBuffer.nativeHandle);


	glNamedBufferStorage(uniformBuffer.nativeHandle, uniformConstantsSize, nullptr,
						 GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);

	uniformBuffer.mappedPtr =
		static_cast<void*>(glMapNamedBufferRange(uniformBuffer.nativeHandle, 0, uniformConstantsSize,
												 GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT));


	defaultSpriteBatchPipeline = renderContext->CreateGraphicsPipeline(GraphicsPipelineDescriptor{
		.vertexShaderCode = { LoadText("Shaders/DefaultSpriteBatch.vert"), "Shaders/DefaultSpriteBatch.vert" },
		.fragmentShaderCode = { LoadText("Shaders/DefaultSpriteBatch.frag"), "Shaders/DefaultSpriteBatch.frag" },
		.debugName = "DefaultSpriteBatchPipeline" });
}

SpriteBatch::~SpriteBatch()
{
	glUnmapNamedBuffer(uniformBuffer.nativeHandle);
	renderContext->DestroyGraphicsPipeline(defaultSpriteBatchPipeline);
}

void SpriteBatch::Begin(const mat3& transform, Effect* effect)
{
	// ZoneScoped;
	auto fbo = FramebufferHandle{};
	auto pso = GraphicsPipelineHandle{};
	if (effect)
	{
		fbo = effect->fbo;
		pso = effect->pso;
	}
	else
	{
		fbo = renderContext->GetDefaultFramebuffer();
		pso = defaultSpriteBatchPipeline;
	}
	const auto& framebuffer = renderContext->Get(fbo);
	const auto& framebufferTexture = renderContext->Get(framebuffer.colorAttachment[0]);
	glViewport(0, 0, framebufferTexture.width, framebufferTexture.height);
	glClipControl(GL_LOWER_LEFT, GL_NEGATIVE_ONE_TO_ONE);
	glFrontFace(GL_CCW);
	glCullFace(GL_BACK);

	glEnable(GL_CULL_FACE);

	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.nativeHandle);

	glBindProgramPipeline(renderContext->Get(pso).nativeHandle);
	glBindVertexArray(vertexArrayObject);
	// TODO:glBindTextureUnit, glBindSamplers, glBindBufferRange for uniforms
	const auto uniformConstants =
		SpriteBatchConstants{ .viewportSize = vec2{ static_cast<float>(framebufferTexture.width),
													static_cast<float>(framebufferTexture.height) },
							  .transform = transform };
	std::memcpy(uniformBuffer.mappedPtr, &uniformConstants, sizeof(SpriteBatchConstants));
	glBindBufferRange(GL_UNIFORM_BUFFER, 0, uniformBuffer.nativeHandle, 0, uniformConstantsSize);
	glMemoryBarrier(GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void SpriteBatch::End()
{
	// ZoneScoped;
	const auto hasSomeWork = true;
	if (hasSomeWork)
	{
		std::sort(spriteInfos.begin(), spriteInfos.end(),
				  [](const SpriteBatch::SpriteInfo& a, const SpriteBatch::SpriteInfo& b)
				  { return a.texture > b.texture; });

		auto lastTexture = !spriteInfos.empty() ? spriteInfos.front().texture : Texture2DHandle{};
		auto vertexOffset = u32{ 0 };
		auto vertexCount = u32{ 0 };


		for (const auto& spriteInfo : spriteInfos)
		{
			const auto position = spriteInfo.destination.position;
			const auto extent = spriteInfo.destination.extent + vec2{ 1.0f, 1.0f }; // TODO: investigate
			const auto color = vec4{ spriteInfo.color.r / 255.0f, spriteInfo.color.g / 255.0f,
									 spriteInfo.color.b / 255.0f, spriteInfo.color.a / 255.0f };

			const auto& textureData = renderContext->Get(spriteInfo.texture);
			const auto textureExtent = vec2{ textureData.width, textureData.height };

			const auto srcRect = spriteInfo.source;
			auto uv0 = (srcRect.position) / textureExtent;
			auto uv1 = (srcRect.position + srcRect.extent) / textureExtent;


			if (spriteInfo.flip == FlipSprite::horizontal or spriteInfo.flip == FlipSprite::horizontalAndVertical)
			{
				const auto x = uv0.x;
				uv0.x = uv1.x;
				uv1.x = x;
			}
			if (spriteInfo.flip == FlipSprite::vertical or spriteInfo.flip == FlipSprite::horizontalAndVertical)
			{
				const auto y = uv0.y;
				uv0.y = uv1.y;
				uv1.y = y;
			}

			generatedVertices.push_back(SpriteQuadVertex{ position + vec2{ extent.x, 0 }, { uv1.x, uv0.y }, color });
			generatedVertices.push_back(SpriteQuadVertex{ position, uv0, color });
			generatedVertices.push_back(SpriteQuadVertex{ position + vec2{ extent.x, extent.y }, uv1, color });

			generatedVertices.push_back(SpriteQuadVertex{ position + vec2{ extent.x, extent.y }, uv1, color });
			generatedVertices.push_back(SpriteQuadVertex{ position, uv0, color });
			generatedVertices.push_back(SpriteQuadVertex{ position + vec2{ 0, extent.y }, { uv0.x, uv1.y }, color });

			if (lastTexture != spriteInfo.texture)
			{
				batches.push_back(Batch{ lastTexture, vertexOffset, vertexCount });
				lastTexture = spriteInfo.texture;
				vertexOffset += vertexCount;
				vertexCount = 0;
			}
			vertexCount += 6;
		}

		batches.push_back(Batch{ lastTexture, vertexOffset, vertexCount });

		glNamedBufferSubData(vertexBuffer, 0, generatedVertices.size() * sizeof(SpriteQuadVertex),
							 generatedVertices.data());

		glMemoryBarrier(GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT);


		for (const auto& batch : batches)
		{
			const auto& texture = renderContext->Get(batch.texture);
			glBindTextureUnit(0, texture.nativeHandle);

			// TODO: we need a proper way to set up a texture sampler
			glTextureParameteri(texture.nativeHandle, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTextureParameteri(texture.nativeHandle, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			/*glTextureParameteri(texture.nativeHandle, GL_TEXTURE_MAX_LEVEL, 0);
			glTextureParameteri(texture.nativeHandle, GL_TEXTURE_MIN_LOD, 0);
			glTextureParameteri(texture.nativeHandle, GL_TEXTURE_MAX_LOD, 0);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);*/
			glDrawArraysInstancedBaseInstance(GL_TRIANGLES, batch.vertexOffset, batch.vertexCount, 1, 0);
		}
	}
	glDisable(GL_BLEND);
	spriteInfos.clear();
	generatedVertices.clear();
	batches.clear();
}

void SpriteBatch::Draw(const Texture2DHandle texture, const vec2& postion, const Color& color)
{
	const auto& textureData = renderContext->Get(texture);
	const auto source = Rectangle{ { 0.0f, 0.0f }, { textureData.width, textureData.height } };
	const auto destination = Rectangle{ postion, { textureData.width, textureData.height } };

	Draw(texture, source, destination, color);
}

void SpriteBatch::Draw(const Texture2DHandle texture, const Rectangle& destination, const Color& color)
{
	const auto& textureData = renderContext->Get(texture);
	const auto source = Rectangle{ { 0.0f, 0.0f }, { textureData.width, textureData.height } };

	Draw(texture, source, destination, color);
}

void SpriteBatch::Draw(const Texture2DHandle texture, const Rectangle& source, const Rectangle& destination,
					   const Color& color, const FlipSprite flip, const vec2& origin, float rotation, float layer)
{
	// ZoneScoped;
	spriteInfos.push_back(SpriteInfo{ .texture = texture,
									  .source = source,
									  .destination = destination,
									  .flip = flip,
									  .origin = origin,
									  .rotation = rotation,
									  .layer = layer,
									  .color = color });
}