#pragma once

#include "Common.hpp"

struct Color
{
	u8 r;
	u8 g;
	u8 b;
	u8 a;
};
namespace Colors
{
	inline constexpr Color White = Color{ 255, 255, 255, 255 };
	inline constexpr Color Black = Color{ 0, 0, 0, 255 };
	inline constexpr Color CornflowerBlue = Color{ 100, 149, 237, 255 };
} // namespace Colors