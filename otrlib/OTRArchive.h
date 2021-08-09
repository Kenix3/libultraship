#pragma once

#include <string>
#include <vector>
#include <stdint.h>
#include "Lib/BinaryReader.h"
#include "OTRResource.h"

namespace OtrLib
{
	enum OTRArchiveEntryType
	{
		Folder = 0,
		File = 1,
	};

	class OTRArchiveEntry
	{
	public:
		std::string name;
		OTRArchiveEntryType entryType;
		char* content;
		uint32_t contentSize;

		std::vector<OTRArchiveEntry> children;
	};

	enum class OTRArchiveLoadType
	{
		Streamed = 0,
		Blocking = 1,
	};

	class OTRArchiveV0Entry
	{
	public:
		std::string name;
		OTRArchiveEntryType entryType;
		char* content;
		uint32_t contentSize;

		std::vector<OTRArchiveV0Entry*> children;

		OTRArchiveV0Entry(BinaryReader* reader);
		OTRArchiveEntry* ToArchiveEntry();
	};

	class OTRArchiveV0 : public OTRResourceFile
	{
	public:
		OTRArchiveEntryType loadType;
		OTRArchiveV0Entry* rootEntry;

		void ParseFileBinary(BinaryReader* reader, OTRResource* res) override;
		void WriteFileBinary(BinaryWriter* writer, OTRResource* res) override;
	};

	class OTRArchive : public OTRResource
	{
	public:
		OTRArchiveEntryType loadType;
		OTRArchiveEntry* rootEntry;
	};
}