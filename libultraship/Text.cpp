#include "Text.h"

void Ship::TextV0::ParseFileBinary(BinaryReader* reader, Resource* res)
{
	Text* txt = (Text*)res;

	ResourceFile::ParseFileBinary(reader, txt);

	uint32_t msgCount = reader->ReadUInt32();

	for (int i = 0; i < msgCount; i++)
	{
		MessageEntry entry;
		entry.id = reader->ReadUInt16();
		entry.textboxType = reader->ReadUByte();
		entry.textboxYPos = reader->ReadUByte();
		entry.msg = reader->ReadString();

		txt->messages.push_back(entry);
	}
}
