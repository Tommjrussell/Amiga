#include "mfm.h"
#include "util/endian.h"

std::pair<uint32_t, uint32_t> am::EncodeMFM(uint32_t value)
{
	const uint32_t mask = 0x55555555;

	uint32_t odd = (value >> 1) & mask;
	uint32_t even = value & mask;

	return { odd, even };
}

void am::EncodeDiskImage(const std::vector<uint8_t>& data, DiskImage& encodedImage)
{
	encodedImage.clear();
	encodedImage.resize(kCylindersPerDisk);

	std::vector<uint8_t>* track = nullptr;
	bool lastBitOne = false;

	auto WriteWord = [&](size_t& off, uint16_t w)
	{
		(*track)[off++] = uint8_t(w >> 8);
		(*track)[off++] = uint8_t(w);
	};

	auto WriteLong = [&](size_t& off, uint32_t l)
	{
		(*track)[off++] = uint8_t(l >> 24);
		(*track)[off++] = uint8_t(l >> 16);
		(*track)[off++] = uint8_t(l >> 8);
		(*track)[off++] = uint8_t(l);
	};

	auto WriteEncodedLong = [&](size_t& off, uint32_t l)
	{
		auto encoded = EncodeMFM(l);
		WriteLong(off, encoded.first);
		WriteLong(off, encoded.second);
	};

	auto AddSyncBits = [&](size_t start, size_t end)
	{
		bool lastBit = false;

		for (size_t p = start; p < end; p++)
		{
			auto& b = (*track)[p];

			for (uint8_t syncBit = 0x40; syncBit != 0; syncBit >>= 2)
			{
				const bool bit = (b & syncBit) != 0;
				if (!lastBit && !bit)
				{
					b |= (syncBit << 1);
				}
				lastBit = bit;
			}
		}
	};

	for (int c = 0; c < kCylindersPerDisk; c++)
	{
		encodedImage[c].resize(kTracksPerCylinder);

		for (int h = 0; h < kTracksPerCylinder; h++)
		{
			track = &encodedImage[c][h];

			track->resize(kMfmSectorSize * kSectorsPerTrack + kMfmGapSize);

			size_t ptr = 0;
			size_t syncStartPtr;

			for (int s = 0; s < kSectorsPerTrack; s++)
			{
				WriteWord(ptr, 0xaaaa);
				WriteWord(ptr, 0xaaaa);
				WriteWord(ptr, 0x4489);
				WriteWord(ptr, 0x4489);

				syncStartPtr = ptr;

				uint32_t info = 0xff000000;
				info |= uint32_t(c * 2 + h) << 16;
				info |= uint32_t(s) << 8;
				info |= uint32_t(11 - s); // sectors until end of track.

				uint32_t headerChecksum = 0;

				auto encodedInfo = EncodeMFM(info);

				bool lastBitOne = false;

				WriteLong(ptr, encodedInfo.first);
				WriteLong(ptr, encodedInfo.second);

				headerChecksum ^= encodedInfo.first;
				headerChecksum ^= encodedInfo.second;

				ptr += 0x20;

				WriteEncodedLong(ptr, headerChecksum);

				uint32_t checkSum = 0;

				const auto adfOffset = (((c * kTracksPerCylinder + h) * kSectorsPerTrack) + s) * kDecodedBytesPerSector;

				auto decodedData = &data[adfOffset];

				size_t oddPtr = ptr + 0x8;
				size_t evenPtr = oddPtr + 0x200;

				for (int d = 0; d < kDecodedBytesPerSector / 4; d++)
				{
					uint32_t data32;
					memcpy(&data32, &decodedData[d * 4], 4);
					data32 = SwapEndian(data32);

					auto encoded = EncodeMFM(data32);

					checkSum ^= encoded.first;
					checkSum ^= encoded.second;

					WriteLong(oddPtr, encoded.first);
					WriteLong(evenPtr, encoded.second);
				}

				WriteEncodedLong(ptr, checkSum);

				ptr += 0x400;

				AddSyncBits(syncStartPtr, ptr);
			}

			ptr += kMfmGapSize;
		}
	}
}