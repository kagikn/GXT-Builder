#include <string>
#include <map>
#include <memory>
#include <strsafe.h>
#include <intrin.h>

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
    static void LoadFileContent(const wchar_t* fileName, std::unordered_map<std::string, std::string>& entryMap, std::ofstream& logFile);
};

class Utf8Validator
{
public:
    static bool MakeSureFileIsValid(std::ifstream& file);
};