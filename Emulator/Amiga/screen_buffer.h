#pragma once

namespace am
{
	constexpr auto kScreenBufferWidth = 672;
	constexpr auto kScreenBufferHeight = 272;

	using ColourRef = uint32_t;
	using ScreenBuffer = std::array<ColourRef, kScreenBufferWidth* kScreenBufferHeight>;

	// Construct a 32-bit ABGR packed Colour value for use in textures
	inline ColourRef MakeColourRef(uint32_t r, uint32_t g, uint32_t b)
	{
		return (r | (g << 8) | (b << 16) | 0xff000000);
	};

}