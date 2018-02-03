#include "gxtbuild.h"
#include "utf8.h"

#include "DelimStringReader.h"
#include "utility.h"

#include <fstream>
#include <iostream>
#include <forward_list>
#include <Shlwapi.h>
#include <ctime>
#include <sstream>
#include <array>
#include <vector>
#include <unordered_map>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "ScopedCurrentDirectory.h"

#ifndef UNICODE
#error GXT Builder must be compiled with Unicode character set
#endif

#ifdef _DEBUG
#define DEBUG_COUT(str) do { std::cout << str; } while( false )
#else
#define DEBUG_COUT(str) do { } while ( false )
#endif
#ifdef _DEBUG
#define DEBUG_WCOUT(str) do { std::wcout << str; } while( false )
#else
#define DEBUG_WCOUT(str) do { } while ( false )
#endif

static const uint32_t crc32table[256] =
{
    0x00000000UL, 0x77073096UL, 0xee0e612cUL, 0x990951baUL, 0x076dc419UL,
    0x706af48fUL, 0xe963a535UL, 0x9e6495a3UL, 0x0edb8832UL, 0x79dcb8a4UL,
    0xe0d5e91eUL, 0x97d2d988UL, 0x09b64c2bUL, 0x7eb17cbdUL, 0xe7b82d07UL,
    0x90bf1d91UL, 0x1db71064UL, 0x6ab020f2UL, 0xf3b97148UL, 0x84be41deUL,
    0x1adad47dUL, 0x6ddde4ebUL, 0xf4d4b551UL, 0x83d385c7UL, 0x136c9856UL,
    0x646ba8c0UL, 0xfd62f97aUL, 0x8a65c9ecUL, 0x14015c4fUL, 0x63066cd9UL,
    0xfa0f3d63UL, 0x8d080df5UL, 0x3b6e20c8UL, 0x4c69105eUL, 0xd56041e4UL,
    0xa2677172UL, 0x3c03e4d1UL, 0x4b04d447UL, 0xd20d85fdUL, 0xa50ab56bUL,
    0x35b5a8faUL, 0x42b2986cUL, 0xdbbbc9d6UL, 0xacbcf940UL, 0x32d86ce3UL,
    0x45df5c75UL, 0xdcd60dcfUL, 0xabd13d59UL, 0x26d930acUL, 0x51de003aUL,
    0xc8d75180UL, 0xbfd06116UL, 0x21b4f4b5UL, 0x56b3c423UL, 0xcfba9599UL,
    0xb8bda50fUL, 0x2802b89eUL, 0x5f058808UL, 0xc60cd9b2UL, 0xb10be924UL,
    0x2f6f7c87UL, 0x58684c11UL, 0xc1611dabUL, 0xb6662d3dUL, 0x76dc4190UL,
    0x01db7106UL, 0x98d220bcUL, 0xefd5102aUL, 0x71b18589UL, 0x06b6b51fUL,
    0x9fbfe4a5UL, 0xe8b8d433UL, 0x7807c9a2UL, 0x0f00f934UL, 0x9609a88eUL,
    0xe10e9818UL, 0x7f6a0dbbUL, 0x086d3d2dUL, 0x91646c97UL, 0xe6635c01UL,
    0x6b6b51f4UL, 0x1c6c6162UL, 0x856530d8UL, 0xf262004eUL, 0x6c0695edUL,
    0x1b01a57bUL, 0x8208f4c1UL, 0xf50fc457UL, 0x65b0d9c6UL, 0x12b7e950UL,
    0x8bbeb8eaUL, 0xfcb9887cUL, 0x62dd1ddfUL, 0x15da2d49UL, 0x8cd37cf3UL,
    0xfbd44c65UL, 0x4db26158UL, 0x3ab551ceUL, 0xa3bc0074UL, 0xd4bb30e2UL,
    0x4adfa541UL, 0x3dd895d7UL, 0xa4d1c46dUL, 0xd3d6f4fbUL, 0x4369e96aUL,
    0x346ed9fcUL, 0xad678846UL, 0xda60b8d0UL, 0x44042d73UL, 0x33031de5UL,
    0xaa0a4c5fUL, 0xdd0d7cc9UL, 0x5005713cUL, 0x270241aaUL, 0xbe0b1010UL,
    0xc90c2086UL, 0x5768b525UL, 0x206f85b3UL, 0xb966d409UL, 0xce61e49fUL,
    0x5edef90eUL, 0x29d9c998UL, 0xb0d09822UL, 0xc7d7a8b4UL, 0x59b33d17UL,
    0x2eb40d81UL, 0xb7bd5c3bUL, 0xc0ba6cadUL, 0xedb88320UL, 0x9abfb3b6UL,
    0x03b6e20cUL, 0x74b1d29aUL, 0xead54739UL, 0x9dd277afUL, 0x04db2615UL,
    0x73dc1683UL, 0xe3630b12UL, 0x94643b84UL, 0x0d6d6a3eUL, 0x7a6a5aa8UL,
    0xe40ecf0bUL, 0x9309ff9dUL, 0x0a00ae27UL, 0x7d079eb1UL, 0xf00f9344UL,
    0x8708a3d2UL, 0x1e01f268UL, 0x6906c2feUL, 0xf762575dUL, 0x806567cbUL,
    0x196c3671UL, 0x6e6b06e7UL, 0xfed41b76UL, 0x89d32be0UL, 0x10da7a5aUL,
    0x67dd4accUL, 0xf9b9df6fUL, 0x8ebeeff9UL, 0x17b7be43UL, 0x60b08ed5UL,
    0xd6d6a3e8UL, 0xa1d1937eUL, 0x38d8c2c4UL, 0x4fdff252UL, 0xd1bb67f1UL,
    0xa6bc5767UL, 0x3fb506ddUL, 0x48b2364bUL, 0xd80d2bdaUL, 0xaf0a1b4cUL,
    0x36034af6UL, 0x41047a60UL, 0xdf60efc3UL, 0xa867df55UL, 0x316e8eefUL,
    0x4669be79UL, 0xcb61b38cUL, 0xbc66831aUL, 0x256fd2a0UL, 0x5268e236UL,
    0xcc0c7795UL, 0xbb0b4703UL, 0x220216b9UL, 0x5505262fUL, 0xc5ba3bbeUL,
    0xb2bd0b28UL, 0x2bb45a92UL, 0x5cb36a04UL, 0xc2d7ffa7UL, 0xb5d0cf31UL,
    0x2cd99e8bUL, 0x5bdeae1dUL, 0x9b64c2b0UL, 0xec63f226UL, 0x756aa39cUL,
    0x026d930aUL, 0x9c0906a9UL, 0xeb0e363fUL, 0x72076785UL, 0x05005713UL,
    0x95bf4a82UL, 0xe2b87a14UL, 0x7bb12baeUL, 0x0cb61b38UL, 0x92d28e9bUL,
    0xe5d5be0dUL, 0x7cdcefb7UL, 0x0bdbdf21UL, 0x86d3d2d4UL, 0xf1d4e242UL,
    0x68ddb3f8UL, 0x1fda836eUL, 0x81be16cdUL, 0xf6b9265bUL, 0x6fb077e1UL,
    0x18b74777UL, 0x88085ae6UL, 0xff0f6a70UL, 0x66063bcaUL, 0x11010b5cUL,
    0x8f659effUL, 0xf862ae69UL, 0x616bffd3UL, 0x166ccf45UL, 0xa00ae278UL,
    0xd70dd2eeUL, 0x4e048354UL, 0x3903b3c2UL, 0xa7672661UL, 0xd06016f7UL,
    0x4969474dUL, 0x3e6e77dbUL, 0xaed16a4aUL, 0xd9d65adcUL, 0x40df0b66UL,
    0x37d83bf0UL, 0xa9bcae53UL, 0xdebb9ec5UL, 0x47b2cf7fUL, 0x30b5ffe9UL,
    0xbdbdf21cUL, 0xcabac28aUL, 0x53b39330UL, 0x24b4a3a6UL, 0xbad03605UL,
    0xcdd70693UL, 0x54de5729UL, 0x23d967bfUL, 0xb3667a2eUL, 0xc4614ab8UL,
    0x5d681b02UL, 0x2a6f2b94UL, 0xb40bbe37UL, 0xc30c8ea1UL, 0x5a05df1bUL,
    0x2d02ef8dUL
};

// Hashes a string
uint32_t crc32FromString(const char *Str)
{
    uint32_t hash = 0xFFFFFFFF;
    while (*Str)
        hash = crc32table[(hash ^ *Str++) & 0xFF] ^ (hash >> 8);
    return hash;
}

// Hashes a string as it is in upper case
uint32_t crc32FromUpcaseString(const char* Str)
{
    uint32_t hash = 0xFFFFFFFF;
    while (*Str)
        hash = crc32table[(hash^toupper(*Str++)) & 0xFF] ^ (hash >> 8);
    return hash;
}

// Continues hashing, takes a hash (to be continued) and a string (to be hashed)
uint32_t crc32Continue(uint32_t hash, const char* Str)
{
    while (*Str)
        hash = crc32table[(hash ^ *Str++) & 0xFF] ^ (hash >> 8);
    return hash;
}

GXTTableCollection::GXTTableCollection(std::string& tableName, uint32_t absoluteMainTableOffset, GXTEnum::eGXTVersion fileVersion)
    :_mainTable(std::move(GXTTableBlockInfo(tableName, absoluteMainTableOffset, fileVersion))), _fileVersion(fileVersion)
{
}

void GXTTableCollection::AddNewMissionTable(std::string& tableName, uint32_t absoluteTableOffset)
{
    auto tableInfo = GXTTableBlockInfo(tableName, absoluteTableOffset, _fileVersion);
    _missionTable[tableName] = std::move(std::unique_ptr<GXTTableBlockInfo>(new GXTTableBlockInfo(tableName, absoluteTableOffset, _fileVersion)));
}

namespace VC
{
    bool GXTTable::InsertEntry(const std::string& entryName, uint32_t offset)
    {
        return Entries.emplace(entryName, static_cast<uint32_t>(offset * sizeof(character_t))).second != false;
    }

    void GXTTable::WriteOutEntries(std::ostream& stream)
    {
        for (auto& it : Entries)
        {
            // Pad string to 8 bytes
            char buf[GXT_ENTRY_NAME_LEN];
            StringCchCopyNExA(buf, _countof(buf), it.first.c_str(), GXT_ENTRY_NAME_LEN, nullptr, nullptr, STRSAFE_FILL_BEHIND_NULL);

            stream.write(reinterpret_cast<const char*>(&it.second), sizeof(it.second));
            stream.write(buf, GXT_ENTRY_NAME_LEN);
        }
    }

    void GXTTable::WriteOutContent(std::ostream& stream)
    {
        stream.write(reinterpret_cast<const char*>(FormattedContent.c_str()), FormattedContent.size() * sizeof(character_t));
    }

    bool GXTTable::ReplaceEntries(const std::unordered_map<std::string, std::wstring>& entryMap)
    {
        if (entryMap.size() == 0)
        {
            return false;
        }

        using offset = uint32_t;
        using entryName = std::string;

        std::vector<std::pair<offset, entryName>> tempPairs;
        tempPairs.reserve(Entries.size());

        for (auto entryPair : Entries)
        {
            tempPairs.push_back(std::make_pair(entryPair.second, entryPair.first));
        }

        std::sort(tempPairs.begin(), tempPairs.end());

        const auto originalContentStrings = StringExtension::SplitWString(FormattedContent, '0');

        uint32_t index = 0;
        std::wstring newFormattedStr;
        newFormattedStr.reserve(FormattedContent.size());
        for (auto pair : tempPairs)
        {
            auto itr = entryMap.find(pair.second);
            if (itr != entryMap.end())
            {
                Entries[pair.second] = static_cast<uint32_t>(newFormattedStr.size() * sizeof(wchar_t));

                auto contentStr = itr->second;
                contentStr += '0';
                newFormattedStr += contentStr;
            }
            else
            {
                Entries[pair.second] = static_cast<uint32_t>(newFormattedStr.size());

                auto contentStr = originalContentStrings.at(index);
                contentStr += '0';
                newFormattedStr += contentStr;
            }

            index++;
        }
        FormattedContent = newFormattedStr;

        return true;
    }

    void GXTTable::ReadEntireContent(std::ifstream& inputStream, const uint32_t offset, const size_t size)
    {
        std::vector<uint16_t> buffer;
        buffer.resize(size / sizeof(uint16_t));

        inputStream.seekg(offset, std::ios_base::beg);
        inputStream.read(reinterpret_cast<char *>(buffer.data()), size);

        FormattedContent = std::wstring{ buffer.begin(), buffer.end() };
    }

    void GXTTable::PushFormattedChar(int character)
    {
        FormattedContent.push_back(static_cast<character_t>(character));
    }
};

namespace SA
{
    bool GXTTable::InsertEntry(const std::string& entryName, uint32_t offset)
    {
        uint32_t entryHash = crc32FromUpcaseString(entryName.c_str());
        return Entries.emplace(entryHash, offset).second != false;
    }
    bool GXTTable::InsertEntry(const uint32_t crc32EntryHash, uint32_t offset)
    {
        return Entries.emplace(crc32EntryHash, offset).second != false;
    }

    bool GXTTable::ReplaceEntries(const std::unordered_map<uint32_t, std::string>& entryMap)
    {

        if (entryMap.size() == 0)
        {
            return false;
        }

        using hash = uint32_t;
        using offset = uint32_t;

        std::vector<std::pair<offset, hash>> tempPairs;
        tempPairs.reserve(Entries.size());

        for (auto entryPair : Entries)
        {
            tempPairs.push_back(std::make_pair(entryPair.second, entryPair.first));
        }

        std::sort(tempPairs.begin(), tempPairs.end());

        const auto originalContentStrings = StringExtension::SplitString(FormattedContent, '\0');

        uint32_t index = 0;
        std::string newFormattedStr;
        newFormattedStr.reserve(FormattedContent.size());
        for (auto pair : tempPairs)
        {
            auto itr = entryMap.find(pair.second);
            if (itr != entryMap.end())
            {
                Entries[pair.second] = static_cast<uint32_t>(newFormattedStr.size());

                auto contentStr = itr->second;
                contentStr += '\0';
                newFormattedStr += contentStr;
            }
            else
            {
                Entries[pair.second] = static_cast<uint32_t>(newFormattedStr.size());
                auto contentStr = originalContentStrings[index];
                contentStr += '\0';

                newFormattedStr += contentStr;
            }
            index++;
        }
        FormattedContent = newFormattedStr;

        return true;
    }

    void GXTTable::WriteOutEntries(std::ostream& stream)
    {
        for (auto& it : Entries)
        {
            stream.write(reinterpret_cast<const char*>(&it.second), sizeof(it.second));
            stream.write(reinterpret_cast<const char*>(&it.first), sizeof(it.first));
        }
    }

    void GXTTable::WriteOutContent(std::ostream& stream)
    {
        stream.write(reinterpret_cast<const char*>(FormattedContent.c_str()), FormattedContent.size() * sizeof(character_t));
    }

    void GXTTable::ReadEntireContent(std::ifstream& inputStream, const uint32_t offset, const size_t size)
    {
        std::vector<char> buffer;
        buffer.resize(size);

        inputStream.seekg(offset, std::ios_base::beg);
        inputStream.read(buffer.data(), size);

        FormattedContent = std::move(std::string{ buffer.begin(), buffer.end() });
    }

    void GXTTable::PushFormattedChar(int character)
    {
        FormattedContent.push_back(static_cast<character_t>(character));
    }
};

std::unique_ptr<GXTTableBase> GXTTableBase::InstantiateGXTTable(GXTEnum::eGXTVersion version)
{
    std::unique_ptr<GXTTableBase> ptr;
    switch (version)
    {
    case GXTEnum::eGXTVersion::GXT_VC:
        ptr = std::make_unique<VC::GXTTable>();
        break;
    case GXTEnum::eGXTVersion::GXT_SA:
        ptr = std::make_unique<SA::GXTTable>();
        break;
    default:
        throw std::runtime_error(std::string("Trying to instantiate an unsupported GXT table version " + version) + "!");
        break;
    }

    return ptr;
}

static std::pair<std::string, uint32_t> ReadTableBlock(std::ifstream& inputStream, const uint32_t offset)
{
    constexpr uint32_t TABLE_NAME_SIZE = 8;
    constexpr uint32_t OFFSET_STORAGE_SIZE = 4;

    static std::string tableName(TABLE_NAME_SIZE, NULL);
    static std::array<char, OFFSET_STORAGE_SIZE> offsetBuf;

    inputStream.seekg(offset, std::ios_base::beg);

    inputStream.read(&tableName[0], TABLE_NAME_SIZE);
    inputStream.seekg(TABLE_NAME_SIZE, std::ios_base::cur);

    inputStream.read(offsetBuf.data(), OFFSET_STORAGE_SIZE);
    const uint32_t tableOffset = *(uint32_t*)offsetBuf.data();

    return std::make_pair(tableName, tableOffset);
}

size_t GXTTableBase::ReadTKEYAndTDATBlock(std::ifstream& inputStream, const uint32_t offset)
{
    constexpr uint32_t HEADER_SIZE = 4;
    constexpr uint32_t BLOCK_SIZE_STORAGE_SIZE = 4;

    static std::array<char, HEADER_SIZE> headerBuf;
    static const std::array<const char, HEADER_SIZE> HEADER_TKEY = { 'T', 'K', 'E', 'Y' };
    static std::array<char, BLOCK_SIZE_STORAGE_SIZE> sizeBuf;

    const bool usesHashForEntryName = UsesHashForEntryName();
    const size_t	ONE_ENTRY_SIZE = GetEntrySize();

    inputStream.seekg(offset, std::ios_base::beg);

    DEBUG_WCOUT(L"TKEY Block Offset" << inputStream.tellg() << "\n");
    inputStream.read(headerBuf.data(), HEADER_SIZE);

    if (!std::equal(headerBuf.cbegin(), headerBuf.cend(), HEADER_TKEY.cbegin()))
    {
        throw std::runtime_error("The TKEY header wasn't found!");
    }

    inputStream.read(sizeBuf.data(), BLOCK_SIZE_STORAGE_SIZE);
    const uint32_t	TKEYBlockSize = *(uint32_t*)sizeBuf.data();

    uint32_t entryOffsetBuf;
    uint32_t entryHashBuf;
    std::string entryBuf(8, NULL);

    DEBUG_WCOUT(L"TKEY Block Offset" << inputStream.tellg() << "\n");

    for (size_t i = 0; i < TKEYBlockSize; i += ONE_ENTRY_SIZE)
    {
        if (usesHashForEntryName)
        {
            inputStream.read(reinterpret_cast<char *>(&entryOffsetBuf), 4);
            const uint32_t entryOffset = entryOffsetBuf;

            inputStream.read(reinterpret_cast<char *>(&entryHashBuf), 4);
            const uint32_t entryHash = entryHashBuf;

            InsertEntry(entryHash, entryOffset);
        }
        else
        {
            inputStream.read(reinterpret_cast<char *>(&entryOffsetBuf), 4);
            const uint32_t entryOffset = entryOffsetBuf;

            inputStream.read(&entryBuf[0], 8);

            InsertEntry(entryBuf, entryOffset);
        }
    }

    static const std::array<const char, HEADER_SIZE> HEADER_TDAT = { 'T', 'D', 'A', 'T' };
    inputStream.read(headerBuf.data(), HEADER_SIZE);

    if (!std::equal(headerBuf.cbegin(), headerBuf.cend(), HEADER_TDAT.cbegin()))
    {
        std::string errorStr = std::string("The TDAT header wasn't found! Offset: ");
        errorStr.append(std::to_string(inputStream.tellg()));
        errorStr.append("\n");
        throw std::runtime_error(errorStr);
        return 0;
    }

    inputStream.read(sizeBuf.data(), BLOCK_SIZE_STORAGE_SIZE);
    const uint32_t	TDATBlockSize = *(uint32_t*)sizeBuf.data();

    const uint32_t	totalSize = TKEYBlockSize + TDATBlockSize + (HEADER_SIZE + BLOCK_SIZE_STORAGE_SIZE) * 2;

    ReadEntireContent(inputStream, static_cast<uint32_t>(inputStream.tellg()), TDATBlockSize);
    DEBUG_WCOUT(L"Table Entry count " << GetNumEntries() << L"\n");

    return totalSize;
}

static std::unique_ptr<GXTTableCollection> ReadGXTFile(const std::wstring& fileName, const GXTEnum::eGXTVersion fileVersion)
{
    std::ifstream	inputFile(fileName, std::ifstream::binary);

    if (inputFile.is_open())
    {
        uint32_t		dwCurrentOffset = 0;
        uint32_t		headerValue = 0;

        constexpr uint32_t HEADER_SIZE = 4;
        constexpr uint32_t BLOCK_SIZE_STORAGE_SIZE = 4;
        std::array<char, HEADER_SIZE> headerBuf;
        std::array<char, BLOCK_SIZE_STORAGE_SIZE> sizeBuf;

#pragma region "Header"
        if (fileVersion == GXTEnum::eGXTVersion::GXT_SA || fileVersion == GXTEnum::eGXTVersion::GXT_SA_MOBILE)
        {
            inputFile.read(headerBuf.data(), HEADER_SIZE);

            headerValue = *(uint32_t*)headerBuf.data();

            if (headerValue != 0x080004 && headerValue != 0x100004)
            {
                throw std::runtime_error("Incorrect GXT version!");
            }

            dwCurrentOffset += HEADER_SIZE;
            inputFile.seekg(dwCurrentOffset, std::ios_base::beg);
        }
#pragma endregion

#pragma region "Read TABL section"
        std::array<const char, 4> HEADER_TABL = { 'T', 'A', 'B', 'L' };

        inputFile.read(headerBuf.data(), HEADER_SIZE);
        if (!std::equal(headerBuf.cbegin(), headerBuf.cend(), HEADER_TABL.cbegin()))
        {
            throw std::runtime_error("The TABL header wasn't found!");
        }

        dwCurrentOffset += HEADER_SIZE;
        inputFile.seekg(dwCurrentOffset, std::ios_base::beg);

        inputFile.read(sizeBuf.data(), BLOCK_SIZE_STORAGE_SIZE);
        const uint32_t	dwBlockSize = *(uint32_t*)sizeBuf.data();

        if (dwBlockSize < 12)
        {
            throw std::runtime_error("The GXT file is corrupted!");
        }

        dwCurrentOffset += 4;
        inputFile.seekg(dwCurrentOffset, std::ios_base::beg);

        auto mainBlocktableTuple = ReadTableBlock(inputFile, static_cast<uint32_t>(inputFile.tellg()));
        std::string mainTableName = std::get<std::string>(mainBlocktableTuple);
        uint32_t mainTableOffset = std::get<uint32_t>(mainBlocktableTuple);

        const uint32_t	ONE_TABLE_BLOCK_SIZE = 12;

        dwCurrentOffset += ONE_TABLE_BLOCK_SIZE;
        inputFile.seekg(dwCurrentOffset, std::ios_base::beg);

        auto tableCollection = std::make_unique<GXTTableCollection>(mainTableName, mainTableOffset, fileVersion);

        for (uint32_t i = 12; i < dwBlockSize; i += ONE_TABLE_BLOCK_SIZE)
        {
            auto tableTuple = ReadTableBlock(inputFile, static_cast<uint32_t>(inputFile.tellg()));
            std::string tableName = std::get<std::string>(tableTuple);
            uint32_t offset = std::get<uint32_t>(tableTuple);

            tableCollection->AddNewMissionTable(tableName, offset);

            dwCurrentOffset += ONE_TABLE_BLOCK_SIZE;
            inputFile.seekg(dwCurrentOffset, std::ios_base::beg);
        }
#pragma endregion

        //#pragma region "Read TKEY and TDAT sections"
        auto& mainGXTTable = tableCollection->GetMainTable()._GXTTable;

        dwCurrentOffset += static_cast<uint32_t>(mainGXTTable->ReadTKEYAndTDATBlock(inputFile, dwCurrentOffset));
        // Align to 4 bytes
        dwCurrentOffset = (dwCurrentOffset + 4 - 1) & ~(4 - 1);
        inputFile.seekg(dwCurrentOffset, std::ios_base::beg);

        DEBUG_WCOUT(L"Main table entry count " << mainGXTTable->GetNumEntries() << L"\n");
        DEBUG_WCOUT(L"Main Table content size " << mainGXTTable->GetFormattedContentSize() << L"\n");

        auto& missionGXTTables = tableCollection->GetMissionTableMap();

        for (const auto& table : missionGXTTables)
        {
            std::array<char, 8> tableNameBuf;

            inputFile.seekg(dwCurrentOffset, std::ios_base::beg);
            inputFile.read(tableNameBuf.data(), 8);

            auto& tableName = table.first;

            if (!std::equal(tableNameBuf.cbegin(), tableNameBuf.cend(), tableName.cbegin()))
            {
                std::string errorStr = std::string("The table name and TKEY header name does not equal! Offset: ");
                errorStr.append(std::to_string(inputFile.tellg()));
                errorStr.append("\n");
                throw std::runtime_error(errorStr);
            }

            dwCurrentOffset += 8;

            auto& missionGXTTable = table.second->_GXTTable;
            dwCurrentOffset += static_cast<uint32_t>(missionGXTTable->ReadTKEYAndTDATBlock(inputFile, dwCurrentOffset));

            // Align to 4 bytes
            dwCurrentOffset = (dwCurrentOffset + 4 - 1) & ~(4 - 1);

            auto debugTableName = table.first;
            debugTableName.push_back(':');

            DEBUG_COUT(debugTableName);
            DEBUG_WCOUT(L" table entry count " << missionGXTTable->GetNumEntries() << L"\n");
            DEBUG_COUT(debugTableName);
            DEBUG_WCOUT(L" table content size " << missionGXTTable->GetFormattedContentSize() << L"\n");
        }

        DEBUG_WCOUT(L"Table counts " << 1 + missionGXTTables.size() << L"\n");

        return tableCollection;

        //#pragma endregion
    }
    else
    {
        throw std::runtime_error("Can't open " + std::string(fileName.begin(), fileName.end()) + ".gxt!");
    }
}

bool GXTTableCollection::WriteGXTFile(const std::wstring& fileName)
{
    std::ofstream	outputFile(fileName, std::ofstream::binary);
    if (outputFile.is_open())
    {
        uint32_t		currentOffset = 0;

        // Header
        if (_fileVersion == GXTEnum::eGXTVersion::GXT_SA)
        {
            const char		header[] = { 0x04, 0x00, 0x08, 0x00 };
            outputFile.write(header, sizeof(header));
            currentOffset += 4;
        }

        // Write TABL section
        {
            const char		header[] = { 'T', 'A', 'B', 'L' };
            outputFile.write(header, sizeof(header));

            const uint32_t	dwBlockSize = static_cast<uint32_t>(12 + _missionTable.size() * 12);
            outputFile.write(reinterpret_cast<const char*>(&dwBlockSize), sizeof(dwBlockSize));

            currentOffset += sizeof(header) + sizeof(dwBlockSize) + dwBlockSize;

            {
                outputFile.write(_mainTable._tableName.c_str(), 8);
                outputFile.write(reinterpret_cast<const char*>(&currentOffset), sizeof(currentOffset));
                currentOffset += static_cast<uint32_t>(16 + (_mainTable._GXTTable->GetNumEntries() * _mainTable._GXTTable->GetEntrySize()) + _mainTable._GXTTable->GetFormattedContentSize());

                // Align to 4 bytes
                currentOffset = (currentOffset + 4 - 1) & ~(4 - 1);
            }

            for (auto& ite : _missionTable)
            {
                outputFile.write(ite.second->_tableName.c_str(), 8);
                outputFile.write(reinterpret_cast<const char*>(&currentOffset), sizeof(currentOffset));
                currentOffset += static_cast<uint32_t>(16 + 8 + (ite.second->_GXTTable->GetNumEntries() * ite.second->_GXTTable->GetEntrySize()) + ite.second->_GXTTable->GetFormattedContentSize());

                // Align to 4 bytes
                currentOffset = (currentOffset + 4 - 1) & ~(4 - 1);
            }
        }

        // Write TKEY and TDAT sections

        {
            {
                const char		header[] = { 'T', 'K', 'E', 'Y' };
                outputFile.write(header, sizeof(header));
                const uint32_t	dwBlockSize = static_cast<uint32_t>(_mainTable._GXTTable->GetNumEntries() * _mainTable._GXTTable->GetEntrySize());
                outputFile.write(reinterpret_cast<const char*>(&dwBlockSize), sizeof(dwBlockSize));

                // Write TKEY entries
                _mainTable._GXTTable->WriteOutEntries(outputFile);
            }

            {
                const char		header[] = { 'T', 'D', 'A', 'T' };
                outputFile.write(header, sizeof(header));
                const uint32_t	dwBlockSize = static_cast<uint32_t>(_mainTable._GXTTable->GetFormattedContentSize());
                outputFile.write(reinterpret_cast<const char*>(&dwBlockSize), sizeof(dwBlockSize));

                _mainTable._GXTTable->WriteOutContent(outputFile);
            }

            // Align to 4 bytes
            if (outputFile.tellp() % 4)
                outputFile.seekp(4 - (outputFile.tellp() % 4), std::ios_base::cur);
        }
        for (const auto& ite : _missionTable)
        {
            outputFile.write(ite.second->_tableName.c_str(), 8);

            {
                const char		header[] = { 'T', 'K', 'E', 'Y' };
                outputFile.write(header, sizeof(header));
                const uint32_t	dwBlockSize = static_cast<uint32_t>(ite.second->_GXTTable->GetNumEntries() * ite.second->_GXTTable->GetEntrySize());
                outputFile.write(reinterpret_cast<const char*>(&dwBlockSize), sizeof(dwBlockSize));

                // Write TKEY entries
                ite.second->_GXTTable->WriteOutEntries(outputFile);
            }

            {
                const char		header[] = { 'T', 'D', 'A', 'T' };
                outputFile.write(header, sizeof(header));
                const uint32_t	dwBlockSize = static_cast<uint32_t>(ite.second->_GXTTable->GetFormattedContentSize());
                outputFile.write(reinterpret_cast<const char*>(&dwBlockSize), sizeof(dwBlockSize));

                ite.second->_GXTTable->WriteOutContent(outputFile);
            }

            // Align to 4 bytes
            if (outputFile.tellp() % 4)
                outputFile.seekp(4 - (outputFile.tellp() % 4), std::ios_base::cur);
        }

        std::wcout << L"Finished writing " << fileName << L"!\n";
        return true;
    }
    else
    {
        throw std::runtime_error("Can't create " + std::string(fileName.begin(), fileName.end()) + "!");
        return false;
    }
}

void GXTTableCollection::BulkReplaceText(std::wstring& textSourceDirectory, GXTEnum::eTextConvertingMode textConvertingMode, std::ofstream& logFile)
{
    namespace fs = std::experimental::filesystem::v1;
    constexpr auto directorySeparatorChar = L"\\";

    std::optional<CharMapArray> charMap;
    if (textConvertingMode == GXTEnum::eTextConvertingMode::UseCharacterMap)
    {
        charMap = CharMap::ParseCharacterMap(L"charmap.txt");
    }

    const std::wstring mainTableName = Encoding::AnsiStringToWString(_mainTable._tableName);
    const std::wstring textDirectoryForMainTable(textSourceDirectory + directorySeparatorChar + mainTableName);

    if (Directory::Exists(textDirectoryForMainTable))
    {
        if (UsesHashForEntryName())
        {
            auto entryMap = EntryLoader::LoadHashEntryTextsInDirectory(textDirectoryForMainTable, logFile);

            switch (textConvertingMode)
            {
                case GXTEnum::eTextConvertingMode::UseCharacterMap:
                {
                    CharMap::ApplyCharacterMap(entryMap, charMap.value());
                }
                    break;
                case GXTEnum::eTextConvertingMode::UseAnsi:
                {
                    Encoding::MapUtf8StringToAnsi(entryMap);
                }
                    break;
                default:
                    break;
            }

            _mainTable._GXTTable->ReplaceEntries(entryMap);
        }
        else
        {
            //Not implemented
        }
    }

    auto& missionGXTTables = GetMissionTableMap();
    for (auto& missionTable : missionGXTTables)
    {
        const std::wstring missionTableName = Encoding::AnsiStringToWString(missionTable.second->_tableName);
        const std::wstring textDirectoryForMissionTable(textSourceDirectory + directorySeparatorChar + missionTableName);
        if (Directory::Exists(textDirectoryForMissionTable))
        {
            if (UsesHashForEntryName())
            {
                auto entryMap = EntryLoader::LoadHashEntryTextsInDirectory(textDirectoryForMainTable, logFile);

                switch (textConvertingMode)
                {
                    case GXTEnum::eTextConvertingMode::UseCharacterMap:
                    {
                        CharMap::ApplyCharacterMap(entryMap, charMap.value());
                    }
                        break;
                    case GXTEnum::eTextConvertingMode::UseAnsi:
                    {
                        Encoding::MapUtf8StringToAnsi(entryMap);
                    }
                        break;
                    default:
                        break;
                }

                missionTable.second->_GXTTable->ReplaceEntries(entryMap);
            }
            else
            {
                //Not implemented
            }
        }
    }
}

const wchar_t* GetFormatName(GXTEnum::eGXTVersion version)
{
    switch (version)
    {
    case GXTEnum::eGXTVersion::GXT_VC:
        return L"GTA Vice City";
    case GXTEnum::eGXTVersion::GXT_SA:
        return L"GTA San Andreas";
    }
    return L"Unsupported";
}

std::wstring GetFileNameNoExtension(std::wstring path)
{
    std::wstring::size_type namePos = path.find_last_of(L"/\\");
    std::wstring::size_type extPos = path.find_last_of(L'.');
    if (namePos == std::wstring::npos)
        path = path.substr(0, extPos);
    else
        path = path.substr(namePos + 1, extPos);
    return path;
}

std::wstring GetFileExtension(std::wstring path)
{
    std::wstring::size_type extPos = path.find_last_of(L'.');
    if (extPos != std::wstring::npos)
        path = path.substr(extPos, path.length() - extPos);
    else
        path = std::wstring();
    return path;
}

static std::vector<std::wstring> MakeStringArgv(wchar_t* argv[])
{
    std::vector<std::wstring> result;
    while (*argv != nullptr)
    {
        result.emplace_back(*argv++);
    }
    return result;
}

static const char* const helpText = "Usage:\tgxtbuilder.exe path\\to\\ini.ini [-vc] [-sa] [additional langs...]\n"
"\t-vc - build GXT in Vice City format\n\t-sa - build GXT in San Andreas format\n"
"\tadditional langs... - ADVANCED USAGE ONLY - names of other language INI files you want to notify "
"about the changes\n\t\tfor each file from the list appends information about changed/added GXT entries "
"to [langname]_changes.txt\n\n\tgxtbuilder.exe --help - displays this help message\n\n"
"Please refer to doc\\american.ini for an example of input INI file\n";

int wmain(int argc, wchar_t* argv[])
{
    std::ios_base::sync_with_stdio(false);
    std::wcout << L"GXT Text Replacer v1.0\nMade by kagikn, Special thanks to Silent\n";

    const std::vector<std::wstring> argvStr = MakeStringArgv(argv);

    setlocale(LC_CTYPE, "");

    if (argc >= 3)
    {
        if (argvStr[1] == L"--help")
        {
            std::cout << helpText;
            return 0;
        }

        // A map of GXT tables
        std::wstring GXTName(argvStr[1]);
        std::wstring TextDirectoryToReplace(argvStr[2]);
        std::ofstream LogFile;

        // Parse commandline arguments
        GXTEnum::eGXTVersion fileVersion = GXTEnum::eGXTVersion::GXT_SA;
        GXTEnum::eTextConvertingMode textConvMode = GXTEnum::eTextConvertingMode::UseAnsi;

        int	firstStream = 2;
        for (int i = 2; i < argc; ++i)
        {
            if (argvStr[i][0] == '-')
            {
                const std::wstring&	tmp = argvStr[i];
                firstStream++;
                if (tmp == L"-sa")
                    fileVersion = GXTEnum::eGXTVersion::GXT_SA;
            }
            else
                break;
        }

        if (GetFileExtension(GXTName).empty())
        {
            GXTName += L".gxt";
        }

        try
        {
            auto gxt = ReadGXTFile(GXTName, fileVersion);
            LogFile.open(GetFileNameNoExtension(GXTName) + L"_replace.log");
            gxt->BulkReplaceText(TextDirectoryToReplace, textConvMode, LogFile);
            gxt->WriteGXTFile(GXTName);
        }
        catch (std::exception& e)
        {
            std::cerr << "ERROR: " << e.what();
            return 1;
        }

        return 0;
    }
    else
    {
        std::cout << helpText;
        return 0;
    }

    return 0;
}