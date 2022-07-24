#pragma once

#include <vector>
#include <utility>
#include <stdint.h>

namespace am
{
	const int kCylindersPerDisk = 80;
	const int kTracksPerCylinder = 2;
	const int kSectorsPerTrack = 11;
	const int kDecodedBytesPerSector = 512;
	const int kMfmSectorHeaderSize = 64;
	const int kMfmGapSize = 30;
	const int kMfmSectorSize = kMfmSectorHeaderSize + kDecodedBytesPerSector * 2;
	const uint16_t kGapPattern = 0x5050;

	using DiskImage = std::vector<std::vector<std::vector<uint8_t>>>;

	std::pair<uint32_t, uint32_t> EncodeMFM(uint32_t value);
	void EncodeDiskImage(const std::vector<uint8_t>& data, DiskImage& encodedImage);

}
