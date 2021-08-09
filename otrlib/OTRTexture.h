#pragma once

#include "OTRResource.h"

namespace OtrLib
{
	enum class TextureType
	{
		RGBA32bpp = 0,
		RGBA16bpp = 1,
		Palette4bpp = 2,
		Palette8bpp = 3,
		Grayscale4bpp = 4,
		Grayscale8bpp = 5,
		GrayscaleAlpha4bpp = 6,
		GrayscaleAlpha8bpp = 7,
		GrayscaleAlpha16bpp = 8,
	};

	class OTRTextureV0 : public OTRResourceFile
	{
	public:
		TextureType texType;
		uint16_t width, height;
		uint32_t offsetToImageData;
		uint32_t offsetToPaletteData;

		void ParseFileBinary(BinaryReader* reader, OTRResource* res) override;
	};

	class OTRTexture : public OTRResource
	{
	public:
		TextureType texType;
		uint16_t width, height;
		uint8_t* imageData;
		uint8_t* paletteData;
	};
}