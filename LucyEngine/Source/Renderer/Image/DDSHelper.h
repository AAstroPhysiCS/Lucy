#pragma once

#include "Image.h"

namespace Lucy {

	namespace DDS {
		constexpr uint32_t MAGIC = 0x20534444;

		constexpr uint32_t DDPF_FOURCC = 0x00000004;

		constexpr uint32_t DDSCAPS2_CUBEMAP = 0x00000200;
		constexpr uint32_t DDSCAPS2_VOLUME = 0x00200000;

		constexpr uint32_t DDS_RESOURCE_MISC_TEXTURECUBE = 0x4;
		constexpr uint32_t DDS_DIMENSION_TEXTURE2D = 3;

		struct PixelFormat {
			uint32_t Size;
			uint32_t Flags;
			uint32_t FourCC;
			uint32_t RGBBitCount;
			uint32_t RBitMask;
			uint32_t GBitMask;
			uint32_t BBitMask;
			uint32_t ABitMask;
		};

		struct Header {
			uint32_t Size;
			uint32_t Flags;
			uint32_t Height;
			uint32_t Width;
			uint32_t PitchOrLinearSize;
			uint32_t Depth;
			uint32_t MipMapCount;
			uint32_t Reserved1[11];
			PixelFormat PixelFormat;
			uint32_t Caps;
			uint32_t Caps2;
			uint32_t Caps3;
			uint32_t Caps4;
			uint32_t Reserved2;
		};

		struct HeaderDX10 {
			uint32_t DXGIFormat;
			uint32_t ResourceDimension;
			uint32_t MiscFlag;
			uint32_t ArraySize;
			uint32_t MiscFlags2;
		};

		enum class DXGIFormat : uint32_t {
			BC1Typeless = 70,
			BC1UNorm = 71,
			BC1UNormSRGB = 72,

			BC2Typeless = 73,
			BC2UNorm = 74,
			BC2UNormSRGB = 75,

			BC3Typeless = 76,
			BC3UNorm = 77,
			BC3UNormSRGB = 78,

			BC4Typeless = 79,
			BC4UNorm = 80,
			BC4SNorm = 81,

			BC5Typeless = 82,
			BC5UNorm = 83,
			BC5SNorm = 84,

			BC6HTypeless = 94,
			BC6HUF16 = 95,
			BC6HSF16 = 96,

			BC7Typeless = 97,
			BC7UNorm = 98,
			BC7UNormSRGB = 99
		};

		struct FormatInfo {
			ImageFormat Format = ImageFormat::Unknown;
			uint32_t BlockSize = 0;
		};

		static FormatInfo GetDXGIFormatInfo(uint32_t format) {
			switch (static_cast<DXGIFormat>(format)) {
				case DXGIFormat::BC1Typeless:
				case DXGIFormat::BC1UNorm:
					return FormatInfo{ ImageFormat::BC1_UNORM, 8 };
				case DXGIFormat::BC1UNormSRGB:
					return FormatInfo{ ImageFormat::BC1_SRGB, 8 };
				case DXGIFormat::BC2Typeless:
				case DXGIFormat::BC2UNorm:
					return FormatInfo{ ImageFormat::BC2_UNORM, 16 };
				case DXGIFormat::BC2UNormSRGB:
					return FormatInfo{ ImageFormat::BC2_SRGB, 16 };
				case DXGIFormat::BC3Typeless:
				case DXGIFormat::BC3UNorm:
					return FormatInfo{ ImageFormat::BC3_UNORM, 16 };
				case DXGIFormat::BC3UNormSRGB:
					return FormatInfo{ ImageFormat::BC3_SRGB, 16 };
				case DXGIFormat::BC4Typeless:
				case DXGIFormat::BC4UNorm:
					return FormatInfo{ ImageFormat::BC4_UNORM, 8 };
				case DXGIFormat::BC4SNorm:
					return FormatInfo{ ImageFormat::BC4_SNORM, 8 };
				case DXGIFormat::BC5Typeless:
				case DXGIFormat::BC5UNorm:
					return FormatInfo{ ImageFormat::BC5_UNORM, 16 };
				case DXGIFormat::BC5SNorm:
					return FormatInfo{ ImageFormat::BC5_SNORM, 16 };
				case DXGIFormat::BC6HUF16:
					return FormatInfo{ ImageFormat::BC6H_UFLOAT, 16 };
				case DXGIFormat::BC6HSF16:
					return FormatInfo{ ImageFormat::BC6H_SFLOAT, 16 };
				case DXGIFormat::BC7Typeless:
				case DXGIFormat::BC7UNorm:
					return FormatInfo{ ImageFormat::BC7_UNORM, 16 };
				case DXGIFormat::BC7UNormSRGB:
					return FormatInfo{ ImageFormat::BC7_SRGB, 16 };
			}
			return {};
		}

		static consteval uint32_t MakeFourCC(char a, char b, char c, char d) {
			return a | (b << 8u) | (c << 16u) | (d << 24u);
		};

		static FormatInfo GetFormatInfo(const Header& header, const HeaderDX10* headerDX10) {
			if (!(header.PixelFormat.Flags & DDPF_FOURCC))
				return {};

			switch (header.PixelFormat.FourCC) {
				case MakeFourCC('D', 'X', 'T', '1'):
					return FormatInfo{ ImageFormat::BC1_UNORM, 8 };
				case MakeFourCC('D', 'X', 'T', '2'):
				case MakeFourCC('D', 'X', 'T', '3'):
					return FormatInfo{ ImageFormat::BC2_UNORM, 16 };
				case MakeFourCC('D', 'X', 'T', '4'):
				case MakeFourCC('D', 'X', 'T', '5'):
					return FormatInfo{ ImageFormat::BC3_UNORM, 16 };
				case MakeFourCC('A', 'T', 'I', '1'):
				case MakeFourCC('B', 'C', '4', 'U'):
					return FormatInfo{ ImageFormat::BC4_UNORM, 8 };
				case MakeFourCC('B', 'C', '4', 'S'):
					return FormatInfo{ ImageFormat::BC4_SNORM, 8 };
				case MakeFourCC('A', 'T', 'I', '2'):
				case MakeFourCC('B', 'C', '5', 'U'):
					return FormatInfo{ ImageFormat::BC5_UNORM, 16 };
				case MakeFourCC('B', 'C', '5', 'S'):
					return FormatInfo{ ImageFormat::BC5_SNORM, 16 };
				case MakeFourCC('D', 'X', '1', '0'):
					if (headerDX10)
						return GetDXGIFormatInfo(headerDX10->DXGIFormat);
					return {};
			}
			return {};
		}

		static uint64_t GetMipSize(uint32_t width, uint32_t height, uint32_t blockSize) {
			uint64_t blockCountX = std::max(1U, (width + 3) / 4);
			uint64_t blockCountY = std::max(1U, (height + 3) / 4);

			return blockCountX * blockCountY * blockSize;
		}

		static bool IsDDS(const std::filesystem::path& path) {
			std::string extension = path.extension().string();
			std::ranges::transform(extension, extension.begin(), [](unsigned char character) {
				return static_cast<char>(std::tolower(character));
			});
			return extension == ".dds";
		}
	}
}