#pragma once

#include <memory>
#include <string>

class TrayIconHelper
{
public:
    class Delegate
    {
    public:
        virtual void OnTrayMenuCallback(int message) = 0;
    };

    explicit TrayIconHelper(Delegate* delegate);
    ~TrayIconHelper();

    int GetCallbackMessageId() const;
    void OnTrayMenuCallback(int message);
    void Remove();
    bool Setup();
    void ShowBalloon(const std::wstring& title, const std::wstring& text);

private:
    class MessageCallbackWindow;

    Delegate* delegate_;
    int tray_icon_callback_message_;
    std::unique_ptr<void, void(*) (void*)> icon_;
    std::unique_ptr<MessageCallbackWindow> callback_handler_;
};