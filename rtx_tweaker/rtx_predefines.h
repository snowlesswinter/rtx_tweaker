#pragma once

class RTXPredefines
{
public:
    ~RTXPredefines();

    static const wchar_t* GetChatWindowClassName();
    static const wchar_t* GetChatWindowTitleKeyword();
    static const wchar_t* GetChatWindowRichEditClassName();

private:
    RTXPredefines();
};