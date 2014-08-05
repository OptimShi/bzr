#include "Palette.h"
#include "BlobReader.h"

Palette::Palette(const void* data, size_t size)
{
	BlobReader reader(data, size);

	auto fileId = reader.read<uint32_t>();
	assert((fileId & 0xFF000000) == 0x04000000);

	auto numColors = reader.read<uint32_t>();
	assert(numColors == 2048);
	_colors.resize(numColors);

	for(auto i = 0u; i < numColors; i++)
	{
		_colors[i].red = reader.read<uint8_t>();
		_colors[i].green = reader.read<uint8_t>();
		_colors[i].blue = reader.read<uint8_t>();
		_colors[i].alpha = reader.read<uint8_t>();
	}

	reader.assertEnd();
}