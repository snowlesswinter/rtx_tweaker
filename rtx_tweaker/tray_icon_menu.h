#pragma once

#include <memory>

class CMenu;
class CWnd;

class TrayIconMenu
{
public:
    TrayIconMenu();
    ~TrayIconMenu();

    void Create();
    void Popup(CWnd* parent, int x, int y);

private:
    std::unique_ptr<CMenu> menu_;
};