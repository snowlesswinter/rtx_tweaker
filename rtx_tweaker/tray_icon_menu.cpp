#include "stdafx.h"
#include "tray_icon_menu.h"

#include "resource.h"


TrayIconMenu::TrayIconMenu()
    : menu_()
{

}

TrayIconMenu::~TrayIconMenu()
{

}

void TrayIconMenu::Create()
{
    if (!menu_) {
        menu_.reset(new CMenu());
        menu_->LoadMenu(IDR_MENU1);
    }
}

void TrayIconMenu::Popup(CWnd* parent, int x, int y)
{
    if (menu_) {
        CMenu* sub_menu = menu_->GetSubMenu(0);
        if (sub_menu) {
            sub_menu->TrackPopupMenu(0, x, y, parent);
        }
    }
}

