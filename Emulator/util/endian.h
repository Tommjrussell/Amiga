#pragma once

#include <stdint.h>

#ifdef _MSC_VER

#include <stdlib.h>

__forceinline uint16_t SwapEndian(uint16_t v)
{
	return _byteswap_ushort(v);
}

__forceinline uint32_t SwapEndian(uint32_t v)
{
	return _byteswap_ulong(v);
}

#else

// TODO : intrinsic endian swap functions for other compilers/architectures

inline SwapEndian(uint16_t v)
{
	return (((v >> 8) & 0xff) | ((v & 0xff) << 8));
}

inline uint32_t SwapEndian(uint32_t v)
{
	return (((v & 0xff000000) >> 24) | ((v & 0x00ff0000) >> 8) | ((v & 0x0000ff00) << 8) | ((v & 0x000000ff) << 24));
}

#endif