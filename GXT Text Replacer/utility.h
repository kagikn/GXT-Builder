#pragma once

#include "gxt_text_replacer.h"

#include <string>
#include <map>
#include <memory>
#include <strsafe.h>
#include <intrin.h>
#include <unordered_map>
#include <optional>
#include <any>

class Directory
{
public:
    static bool Exists(const std::wstring& dirName_in);
};

class Encoding
{
public:
    static std::wstring AnsiStringToWString(std::string const& src);
    static std::wstring Utf8ToUtf16(const std::string& utf8);
    static std::string Utf8ToAnsi(const std::string& utf8, int ansiCodePage);

    static void MapUtf8StringToAnsi(std::unordered_map<std::string, std::string>& map, int ansiCodePage);
    static void MapUtf8StringToAnsi(std::unordered_map<uint32_t, std::string>& map, int ansiCodePage);
};

class EntryLoader
{
public:
    static std::unordered_map<std::string, std::string> LoadEntryTextsInDirectory(const std::wstring& textDirectory, std::ofstream& logFile);
    static std::unordered_map<uint32_t, std::string> LoadHashEntryTextsInDirectory(const std::wstring& textDirectory, std::ofstream& logFile);
    static void LoadFileContent(const wchar_t* fileName, std::unordered_map<std::string, std::string>& entryMap, std::ofstream& logFile);
    static void LoadFileContentForHashEntry(const wchar_t* fileName, std::unordered_map<uint32_t, std::string>& entryMap, std::ofstream& logFile);

private:
    static std::optional<uint32_t> HexStringToUInt32(const std::string& hexString);
};

class Utf8Validator
{
public:
    static bool IsValid(std::ifstream& file);
};

class StringExtension
{
public:
    static std::vector<std::string> SplitString(const std::string &txt, const char separator, bool allowEmptyString);
    static std::vector<std::wstring> SplitWString(const std::wstring &txt, const wchar_t separator);

};

static const size_t CHARACTER_MAP_WIDTH = 16;
static const size_t CHARACTER_MAP_HEIGHT = 14;
static const size_t CHARACTER_MAP_SIZE = CHARACTER_MAP_WIDTH * CHARACTER_MAP_HEIGHT;

typedef std::array<uint32_t, CHARACTER_MAP_SIZE> CharMapArray;

class CharMap
{
public:
    static void ApplyCharacterMap(std::unordered_map<std::string, std::string>& entryMap, const CharMapArray& characterMap);
    static void ApplyCharacterMap(std::unordered_map<uint32_t, std::string>& entryMap, const CharMapArray& characterMap);
    static CharMapArray ParseCharacterMap(const std::wstring& szFileName);
};

