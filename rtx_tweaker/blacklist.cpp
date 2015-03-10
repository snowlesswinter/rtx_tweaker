#include "stdafx.h"
#include "blacklist.h"

#include <fstream>
#include <string>
#include <vector>

namespace {

std::wstring GetLocalDirectory()
{
    std::vector<wchar_t> buf(MAX_PATH);
    if (GetModuleFileNameW(nullptr, &buf[0], buf.size())) {
        std::wstring module_file(&buf[0]);
        int pos = module_file.find_last_of(L"\\/");
        if (pos != std::wstring::npos)
            return module_file.substr(0, pos + 1);
    }

    return std::wstring();
}

const std::vector<std::wstring>& LoadBlacklist()
{
    static std::vector<std::wstring>* blacklist = nullptr;
    if (!blacklist) {
        blacklist = new std::vector<std::wstring>();

        std::wstring local_dir = GetLocalDirectory();
        if (local_dir.empty())
            return *blacklist;

        std::wstring blacklist_file_path = local_dir += L"config\\blacklist";
        std::ifstream blacklist_file(blacklist_file_path.c_str(),
                                     std::ios::in);

        std::string temp;
        blacklist_file >> temp;
        while (!temp.empty()) {
            std::vector<wchar_t> wide_char(temp.size());
            int count = MultiByteToWideChar(CP_UTF8, 0, temp.c_str(),
                                            temp.size(), &wide_char[0],
                                            wide_char.size());
            std::wstring s(wide_char.begin(), wide_char.begin() + count);

            blacklist->push_back(std::move(s));
            temp.clear();
            blacklist_file >> temp;
        }
    }

    return *blacklist;
}
}

bool Blacklist::IsInBlacklist(const std::wstring& title)
{
    const std::vector<std::wstring>& blacklist = LoadBlacklist();
    for (const auto& i : blacklist) {
        if (title.find(i) != title.npos)
            return true;
    }

    return false;
}