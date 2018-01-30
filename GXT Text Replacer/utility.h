#include "gxtbuild.h"

#include <string>
#include <map>
#include <memory>
#include <strsafe.h>
#include <intrin.h>
#include <unordered_map>

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
};

class EntryLoader
{
public:
    static std::unordered_map<std::string, std::string> LoadTextsInDirectory(const std::wstring& textDirectory, GXTEnum::eTextConvertingMode textConvertingMode, std::ofstream& logFile);
    static void LoadFileContent(const wchar_t* fileName, std::unordered_map<std::string, std::string>& entryMap, std::ofstream& logFile);
    static void LoadFileContentForHashEntry(const wchar_t* fileName, std::unordered_map<uint32_t, std::string>& entryMap, std::ofstream& logFile);
};

class Utf8Validator
{
public:
    static bool IsValid(std::ifstream& file);
};

