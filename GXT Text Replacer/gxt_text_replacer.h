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
    virtual bool	ReplaceEntries(const std::unordered_map<std::string, std::wstring>& entryMap) = 0;
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

    bool WriteGXTFile(const std::wstring& fileName);
    void AddNewMissionTable(std::string& tableName, uint32_t absoluteTableOffset);
    void BulkReplaceText(std::wstring& textSourceDirectory, GXTEnum::eTextConvertingMode textConvertingMode, int ansiCodePage, std::ofstream& logFile);

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
        virtual bool ReplaceEntries(const std::unordered_map<uint32_t, std::string>&) override
        {
            return false;
        }

        virtual bool	InsertEntry(const std::string& entryName, uint32_t offset) override;
        virtual bool	ReplaceEntries(const std::unordered_map<std::string, std::wstring>& entryMap) override;
        virtual void	WriteOutEntries(std::ostream& stream) override;
        virtual void	WriteOutContent(std::ostream& stream) override;
        virtual void	ReadEntireContent(std::ifstream& inputStream, const uint32_t offset, const size_t size) override;
        virtual void	PushFormattedChar(int character) override;

    private:
        static const size_t	GXT_ENTRY_NAME_LEN = 8;

        std::map<std::string, uint32_t>	Entries;
        std::wstring FormattedContent;
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

        virtual bool ReplaceEntries(const std::unordered_map<std::string, std::wstring>&) override
        {
            return false;
        }

        virtual bool	InsertEntry(const std::string& entryName, uint32_t offset) override;
        virtual bool	InsertEntry(const uint32_t crc32EntryHash, uint32_t offset) override;
        virtual bool    ReplaceEntries(const std::unordered_map<uint32_t, std::string>& entryMap) override;
        virtual void	WriteOutEntries(std::ostream& stream) override;
        virtual void	WriteOutContent(std::ostream& stream) override;
        virtual void	ReadEntireContent(std::ifstream& inputStream, const uint32_t offset, const size_t size) override;
        virtual void	PushFormattedChar(int character) override;

    private:
        std::map<uint32_t, uint32_t> Entries;
        std::string	FormattedContent;
    };
};

#endif