#pragma once

#ifndef __GXTBUILD_H
#define __GXTBUILD_H

#include "enum.h"
#include "crc32keygen.h"

#include <string>
#include <map>
#include <unordered_map>
#include <memory>
#include <strsafe.h>
#include <intrin.h>

class GXTTableBase
{
public:
    std::wstring			szPath;
    std::string				Content;

    GXTTableBase(std::wstring szFilePath)
        : szPath(std::move(szFilePath))
    {}
    GXTTableBase()
    {}
    virtual ~GXTTableBase()
    {}

public:
    virtual bool	InsertEntry(const std::string& entryName, uint32_t offset) = 0;
    virtual bool	InsertEntry(const uint32_t crc32EntryHash, uint32_t offset) = 0;
    virtual bool	ReplaceEntries(const std::unordered_map<std::string, std::string>& entryMap) = 0;
    virtual bool	ReplaceEntries(const std::unordered_map<uint32_t, std::string>& entryMap) = 0;
    virtual bool	UsesHashForEntryName() = 0;
    virtual size_t	GetNumEntries() = 0;
    virtual size_t	GetFormattedContentSize() = 0;
    virtual size_t	GetEntrySize() = 0;
    virtual void	WriteOutEntries(std::ostream& stream) = 0;
    virtual void	WriteOutContent(std::ostream& stream) = 0;
    virtual size_t	ReadTKEYAndTDATBlock(std::ifstream& inputStream, const uint32_t offset);
    virtual void	ReadEntireContent(std::ifstream& inputStream, const uint32_t offset, const size_t size) = 0;
    virtual void	PushFormattedChar(int character) = 0;

    static std::unique_ptr<GXTTableBase> InstantiateGXTTable(GXTEnum::eGXTVersion version);

private:

};


class GXTTableBlockInfo
{
public:
    uint32_t			_absoluteOffset = 0;
    std::string			_tableName;
    std::unique_ptr<GXTTableBase>				_GXTTable;

    GXTTableBlockInfo(std::string tableName, GXTEnum::eGXTVersion fileVersion)
    {
        _tableName = tableName;
        _GXTTable = std::move(GXTTableBase::InstantiateGXTTable(fileVersion));
    }

    GXTTableBlockInfo(std::string tableName, uint32_t absoluteOffset, const GXTEnum::eGXTVersion fileVersion)
    {
        _absoluteOffset = absoluteOffset;
        _tableName = tableName;
        _GXTTable = std::move(GXTTableBase::InstantiateGXTTable(fileVersion));
    }

    GXTTableBlockInfo(GXTTableBlockInfo && rhs)
    {
        _absoluteOffset = rhs._absoluteOffset;
        _tableName = rhs._tableName;
        _GXTTable = std::move(rhs._GXTTable);
    }
};

class GXTTableCollection
{
public:
    GXTTableBlockInfo _mainTable;
    std::map<std::string, std::unique_ptr<GXTTableBlockInfo>> _missionTable;

    GXTTableCollection(std::string& tableName, uint32_t absoluteMainTableOffset, GXTEnum::eGXTVersion fileVersion);

    GXTTableBlockInfo& GetMainTable()
    {
        return _mainTable;
    }
    std::map<std::string, std::unique_ptr<GXTTableBlockInfo>>& GetMissionTableMap()
    {
        return _missionTable;
    }

    void AddNewMissionTable(std::string& tableName, uint32_t absoluteTableOffset);
    void BulkReplaceText(std::wstring& textSourceDirectory, GXTEnum::eTextConvertingMode textConvertingMode, std::ofstream& logFile);

    bool HasAnyMissionTables()
    {
        return _missionTable.size() > 0;
    }
    bool UsesHashForEntryName()
    {
        return (_fileVersion != GXTEnum::eGXTVersion::GXT_VC);
    }

private:
    GXTEnum::eGXTVersion _fileVersion;
};

struct EntryName
{
    static const size_t		GXT_TABLE_NAME_LEN = 8;

    char						cName[GXT_TABLE_NAME_LEN];

    EntryName(const char* pName)
    {
        StringCchCopyNExA(cName, _countof(cName), pName, GXT_TABLE_NAME_LEN, nullptr, nullptr, STRSAFE_FILL_BEHIND_NULL);
    }

    EntryName(const wchar_t* pName)
    {
        size_t numConverted = 0;
        wcstombs_s(&numConverted, cName, pName, _countof(cName));
        std::fill(std::begin(cName) + numConverted, std::end(cName), 0);
    }
};

typedef std::map<EntryName, std::unique_ptr<GXTTableBase>, bool(*)(const EntryName&, const EntryName&)>	tableMap_t;

class GXTFileBase
{
public:
    static std::unique_ptr<GXTFileBase> InstantiateBuilder(GXTEnum::eGXTVersion version);

    void ProduceGXTFile(const std::wstring& szLangName, const tableMap_t& TablesMap);

private:
    virtual uint32_t WriteOutHeader(std::ostream& stream) const = 0;
};

struct VersionControlMap
{
    uint32_t					TextHash;
    bool						bLinked;

    VersionControlMap(uint32_t hash = 0)
        : TextHash(hash), bLinked(true)
    {}
};

namespace VC
{
    class GXTTable : public GXTTableBase
    {
    public:
        typedef uint16_t character_t;

        GXTTable(std::wstring szFilePath)
            : GXTTableBase(std::move(szFilePath))
        {}
        GXTTable() : GXTTableBase()
        {}

        virtual size_t	GetNumEntries() override
        {
            return Entries.size();
        }

        virtual size_t GetFormattedContentSize() override
        {
            return FormattedContent.size() * sizeof(character_t);
        }

        virtual size_t GetEntrySize() override
        {
            return GXT_ENTRY_NAME_LEN + sizeof(uint32_t);
        }

        virtual bool UsesHashForEntryName() override
        {
            return false;
        }
        virtual bool InsertEntry(const uint32_t, uint32_t) override
        {
            return false;
        }
        virtual bool ReplaceEntries(const std::unordered_map<uint32_t, std::string>& entryMap) override
        {
            return;
        }

        virtual bool	InsertEntry(const std::string& entryName, uint32_t offset) override;
        virtual bool	ReplaceEntries(const std::unordered_map<std::string, std::string>& entryMap) = 0;
        virtual void	WriteOutEntries(std::ostream& stream) override;
        virtual void	WriteOutContent(std::ostream& stream) override;
        virtual void	ReadEntireContent(std::ifstream& inputStream, const uint32_t offset, const size_t size) override;
        virtual void	PushFormattedChar(int character) override;

    private:
        static const size_t		GXT_ENTRY_NAME_LEN = 8;

        std::map<std::string, uint32_t>	Entries;
        std::basic_string<character_t>	FormattedContent;
    };

    class GXTFile : public GXTFileBase
    {
    private:
        virtual uint32_t WriteOutHeader(std::ostream&) const override
        {
            // No header
            return 0;
        }
    };
};

namespace SA
{
    class GXTTable : public GXTTableBase
    {
    public:
        typedef uint8_t character_t;

        GXTTable(std::wstring szFilePath)
            : GXTTableBase(std::move(szFilePath))
        {}
        GXTTable() : GXTTableBase()
        {}

        virtual size_t	GetNumEntries() override
        {
            return Entries.size();
        }

        virtual size_t GetFormattedContentSize() override
        {
            return FormattedContent.size() * sizeof(character_t);
        }

        virtual size_t GetEntrySize() override
        {
            return sizeof(uint32_t) + sizeof(uint32_t);
        }

        virtual bool UsesHashForEntryName() override
        {
            return true;
        }

        virtual bool ReplaceEntries(const std::unordered_map<std::string, std::string>& entryMap) override
        {
            return;
        }

        virtual bool	InsertEntry(const std::string& entryName, uint32_t offset) override;
        virtual bool	InsertEntry(const uint32_t crc32EntryHash, uint32_t offset) override;
        virtual bool    ReplaceEntries(const std::unordered_map<uint32_t, std::string>& entryMap) override;
        virtual void	WriteOutEntries(std::ostream& stream) override;
        virtual void	WriteOutContent(std::ostream& stream) override;
        virtual void	ReadEntireContent(std::ifstream& inputStream, const uint32_t offset, const size_t size) override;
        virtual void	PushFormattedChar(int character) override;

    private:
        std::map<uint32_t, uint32_t>	Entries;
        std::basic_string<character_t>	FormattedContent;
    };

    class GXTFile : public GXTFileBase
    {


    private:
        virtual uint32_t WriteOutHeader(std::ostream& stream) const override
        {
            const char		header[] = { 0x04, 0x00, 0x08, 0x00 };	// 0x080004
            stream.write(header, sizeof(header));
            return sizeof(header);
        }
    };
};

#endif