#pragma once
#include <box2d/box2d.h>
#include <cstdint>
#include <glad/glad.h>
#include <glm/glm.hpp>

using u32 = uint32_t;
using i32 = int32_t;
using u8 = uint8_t;

using vec2 = glm::vec2;
using vec4 = glm::vec4;

using f32 = float;

template <typename T1, typename T2>
inline T1 CastTo(const T2& value)
{
	return T1{};
}

template<>
inline b2Vec2 CastTo(const vec2& value)
{
	return b2Vec2{ value.x, value.y };
}

template<>
inline vec2 CastTo(const b2Vec2& value)
{
	return vec2{ value.x, value.y };
}

inline b2Vec2 ToBox2Vector(const vec2& f)
{
	return b2Vec2{ f.x, f.y };
}


struct Rectangle
{
	vec2 position;
	vec2 extent;
};

#define glLabel(s) (GLuint) strlen(s), s