#include "SpriteBatch.hpp"

#include <glm/glm.hpp>

/*

----------------------------------------------------------------------------
q1-t1 q2-t2 q3-t1 ..........................................................

q1-t1 q2-t1 q3-t2 ..........................................................

batch1 - q1-q2 	- t1
batch2 - q1		- t2

glBindProgramPipeline(...);
glBindVertexArray(...);
for(batch in batches)
{
	glBindTextureUnit(batch.texture);
	glDrawArraysInstancedBaseInstance(GL_TRIANGLES, batch.vertexOffsetAndSize);
}





*/

#define glLabel(s) (GLuint) strlen(s), s

struct SpriteQuadVertex
{
	glm::vec2 position;
	glm::vec2 uv;
	glm::vec4 color;
};

SpriteBatch::SpriteBatch()
{
	glCreateBuffers(1, &vertexBuffer);
	glObjectLabel(GL_BUFFER, vertexBuffer, glLabel("sprite_batch_buffer"));
	glNamedBufferStorage(vertexBuffer, defaultBufferSize, nullptr, GL_DYNAMIC_STORAGE_BIT);

	glCreateVertexArrays(1, &vertexArrayObject);
	//glObjectLabel(GL_VERTEX_ARRAY, vertexArrayObject, glLabel("sprite_batch_vao"));
	const GLuint positionAttribute = 0;
	const GLuint textureCoordinateAttribute = 1;
	const GLuint colorAttribute = 2;


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
}

SpriteBatch::~SpriteBatch()
{
}

void SpriteBatch::Begin()
{
}

void SpriteBatch::End()
{
	const auto hasSomeWork = true;
	if (hasSomeWork)
	{
		std::vector<SpriteQuadVertex> generatedVertices;
		generatedVertices.reserve(spriteInfos.size() * 6);

		for (const auto& spriteInfo : spriteInfos)
		{
			const auto position = spriteInfo.position;
			const auto extent = glm::vec2{ 1.0f, 1.0f };
			const auto color =
				glm::vec4{ spriteInfo.color.r, spriteInfo.color.g, spriteInfo.color.b, spriteInfo.color.a };

			generatedVertices.push_back(SpriteQuadVertex{ position, { 0.0, 0.0 }, color });
			generatedVertices.push_back(SpriteQuadVertex{ position + glm::vec2{ extent.x, 0 }, { 1.0, 0.0 }, color });
			generatedVertices.push_back(
				SpriteQuadVertex{ position + glm::vec2{ extent.x, extent.y }, { 1.0, 1.0 }, color });
			generatedVertices.push_back(SpriteQuadVertex{ position, { 0.0, 0.0 }, color });
			generatedVertices.push_back(
				SpriteQuadVertex{ position + glm::vec2{ extent.x, extent.y }, { 1.0, 1.0 }, color });
			generatedVertices.push_back(SpriteQuadVertex{ position + glm::vec2{ 0, extent.y }, { 0.0, 1.0 }, color });
		}

		glNamedBufferSubData(vertexBuffer, 0, generatedVertices.size() * sizeof(SpriteQuadVertex),
							 generatedVertices.data());
	}

	spriteInfos.clear();
}

void SpriteBatch::Draw(const SpriteTexture texture, const glm::vec2& postion, const Color& color)
{
	spriteInfos.push_back(SpriteInfo{ .texture = texture, .position = postion, .color = color });
}
