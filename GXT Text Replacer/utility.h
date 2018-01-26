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