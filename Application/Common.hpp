#pragma once
#include <cstdint>
#include <glad/glad.h>

using u32 = uint32_t;
using i32 = int32_t;

#define glLabel(s) (GLuint) strlen(s), s