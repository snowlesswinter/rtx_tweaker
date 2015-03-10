#pragma once

#include <string>

class Blacklist
{
public:
    static bool IsInBlacklist(const std::wstring& title);

private:
    Blacklist();
    ~Blacklist();
};