#include "utility.h"
#include "utf8.h"

#include <fstream>
#include <iostream>
#include <forward_list>
#include <Shlwapi.h>
#include <ctime>
#include <sstream>
#include <array>
#include <vector>
#include <unordered_map>

#include <windows.h>
#include <io.h>
#include <fcntl.h>


bool Directory::Exists(const std::wstring& dirName_in)
{
    DWORD ftyp = GetFileAttributesW(dirName_in.c_str());
    if (ftyp == INVALID_FILE_ATTRIBUTES)
        return false;  //something is wrong with your path!

    if (ftyp & FILE_ATTRIBUTE_DIRECTORY)
        return true;   // this is a directory!

    return false;    // this is not a directory!
}


std::wstring Encoding::AnsiStringToWString(std::string const& src)
{
    int iBufferSize = MultiByteToWideChar(CP_ACP, 0, src.c_str(), -1, (wchar_t*)NULL, 0);

    std::vector<wchar_t> dest(iBufferSize, L'\0');

    MultiByteToWideChar(CP_ACP, 0, src.c_str(), -1, dest.data(), iBufferSize);

    return std::wstring(dest.data(), dest.data() + iBufferSize - 1);
}

std::wstring Encoding::Utf8ToUtf16(const std::string& utf8)
{
    int iBufferSize = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, (wchar_t*)NULL, 0);

    std::vector<wchar_t> dest(iBufferSize, L'\0');

    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, dest.data(), iBufferSize);

    return std::wstring(dest.data(), dest.data() + iBufferSize - 1);
}

void EntryLoader::LoadFileContent(const wchar_t* fileName, std::unordered_map<std::string, std::string>& entryMap, std::ofstream& logFile)
{
    std::ifstream		InputFile(fileName, std::ifstream::in);

    if (InputFile.is_open())
    {
        std::wcout << L"Reading entries from " << fileName << L"...\n";

        if (!Utf8Validator::IsValid(InputFile))
        {
            std::wcerr << L"ERROR: File " << fileName << " contains invalid UTF-8 characters!\n";
            return;
        }

        uint64_t lineCount = 0;
        std::string	fileLine;
        while (std::getline(InputFile, fileLine))
        {
            lineCount++;

            if (!fileLine.empty() && fileLine[0] != '#')
            {
                // Extract entry name
                std::string::size_type tabPos = fileLine.find_first_of('\t');
                if (tabPos == std::string::npos) continue;

                std::string		EntryName(fileLine.begin(), fileLine.begin() + tabPos);
                std::string		EntryContent(fileLine.begin() + fileLine.find_first_not_of('\t', tabPos), fileLine.end());

                for (char& c : EntryName)
                {
                    if (c > 0x7e)
                    {
                        _setmode(_fileno(stdout), _O_WTEXT);
                        std::wcerr << L"ERROR: the entry name " << Encoding::Utf8ToUtf16(EntryName) << "at line" << lineCount << "contains non-ASCII characters!" << "Only ASCII characters can be used for entry names.";
                        continue;
                    }
                }
                if (EntryName.length() >= 8)
                {
                    std::wcerr << L"ERROR: the entry name " << Encoding::Utf8ToUtf16(EntryName) << "at line" << lineCount << "is too long!" << "Entry names must be less than 8 characters.";
                    continue;
                }
                // Push entry into table map
                if (!entryMap.emplace(EntryName, EntryContent).second)
                {
                    if (logFile.is_open())
                    {
                        std::wstring wideFileName(fileName);
                        logFile << "Entry " << EntryName << " duplicated in " << std::string(wideFileName.begin(), wideFileName.end()) << " file!\n";
                    }
                }
            }
        }
    }
    else
    {
        std::wstring tmp(fileName);
        throw std::runtime_error(std::string(tmp.begin(), tmp.end()) + " not found!");
    }
}

bool Utf8Validator::IsValid(std::ifstream& file)
{
    std::istreambuf_iterator<char> it(file.rdbuf());
    std::istreambuf_iterator<char> eos;
    if (!utf8::is_valid(it, eos))
        return false;

    file.seekg(0, std::ios_base::beg);

    // Skip BOM (if exists)
    // starts_with_bom advances the pointer all the time, so we have to call seekg anyway
    file.seekg(utf8::starts_with_bom(it, eos) ? 3 : 0, std::ios_base::beg);
    return true;
}

