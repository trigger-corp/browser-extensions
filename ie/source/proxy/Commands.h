#pragma once

#include <util.h>
#include "Channel.h"
#include "FrameServer.h"

#pragma warning(disable:4996) // http://www.urbandictionary.com/define.php?term=ffffuuuu

struct button_addCommand : public ForgeCommand {
    wchar_t title[MAX_PATH];
    wchar_t icon [MAX_PATH];
    static const UINTX COMMAND_TYPE = 0x100;
    button_addCommand(const std::wstring& uuid, const std::wstring& title, const std::wstring& icon,
                      DWORDX processId = NULL, INT_PTRX proxy = NULL) 
        : ForgeCommand(COMMAND_TYPE, processId, proxy, uuid) {
        size_t len = title.copy(this->title, MAX_PATH, 0);
        this->title[len] = L'\0';
        len = icon.copy(this->icon, MAX_PATH, 0);
        this->icon[len] = L'\0';
    }
    button_addCommand(Channel* channel) : ForgeCommand(COMMAND_TYPE) {
        channel->Read(&type, sizeof(button_addCommand)); 
    }
    HRESULT exec(HWND toolbar, HWND target, FrameServer::Button *out_button);
};


struct button_setIconCommand : public ForgeCommand {
    wchar_t url [MAX_PATH];
    static const UINTX COMMAND_TYPE = 0x101;
    button_setIconCommand(const std::wstring& uuid, const std::wstring& url, 
                          DWORDX processId = NULL, INT_PTRX proxy = NULL) 
        : ForgeCommand(COMMAND_TYPE, processId, proxy, uuid) {
        size_t len = url.copy(this->url, MAX_PATH, 0);
        this->url[len] = L'\0';
    }
    button_setIconCommand(Channel* channel) : ForgeCommand(COMMAND_TYPE) {
        channel->Read(&type, sizeof(button_setIconCommand)); 
    }
    HRESULT exec(HWND toolbar, HWND target, int idCommand, int iBitmap);
};


struct button_setTitleCommand : public ForgeCommand {
    wchar_t title[MAX_PATH];
    static const UINTX COMMAND_TYPE = 0x102;
    button_setTitleCommand(const std::wstring& uuid, const std::wstring& title, 
                           DWORDX processId = NULL, INT_PTRX proxy = NULL) 
        : ForgeCommand(COMMAND_TYPE, processId, proxy, uuid) {
        size_t len = title.copy(this->title, MAX_PATH, 0);
        this->title[len] = L'\0';
    }
    button_setTitleCommand(Channel* channel) : ForgeCommand(COMMAND_TYPE) {
        channel->Read(&type, sizeof(button_setTitleCommand)); 
    }
    HRESULT exec(HWND toolbar, HWND target, int idCommand);
};


struct button_setBadgeCommand : public ForgeCommand {
    INT32     number;
    static const UINTX COMMAND_TYPE = 0x103;
    button_setBadgeCommand(const std::wstring& uuid, INT32 number, 
                           DWORDX processId = NULL, INT_PTRX proxy = NULL) 
        : ForgeCommand(COMMAND_TYPE, processId, proxy, uuid), 
          number(number) { }
    button_setBadgeCommand(Channel* channel) : ForgeCommand(COMMAND_TYPE) {
        channel->Read(&type, sizeof(button_setBadgeCommand)); 
    }
    HRESULT exec(HWND toolbar, HWND target, int idCommand);
};


struct button_setBadgeBackgroundColorCommand : public ForgeCommand {
    BYTE    r, g, b, a;
    static const UINTX COMMAND_TYPE = 0x104;
    button_setBadgeBackgroundColorCommand(const std::wstring& uuid, 
                                          BYTE r, BYTE g, BYTE b, BYTE a, 
                                          DWORDX processId = NULL, INT_PTRX proxy = NULL) 
        : ForgeCommand(COMMAND_TYPE, processId, proxy, uuid), 
          r(r), g(g), b(b), a(a) { }
    button_setBadgeBackgroundColorCommand(Channel* channel) : ForgeCommand(COMMAND_TYPE) {
        channel->Read(&type, sizeof(button_setBadgeCommand)); 
    }
    HRESULT exec(HWND toolbar, HWND target, int idCommand);
};


struct button_onClickCommand : public ForgeCommand {
    POINT point;
    static const UINTX COMMAND_TYPE = 0x105;
    button_onClickCommand(const std::wstring& uuid, 
                          POINT point,
                          DWORDX processId = NULL, INT_PTRX proxy = NULL) 
        : ForgeCommand(COMMAND_TYPE, processId, proxy, uuid), 
          point(point) { }
    button_onClickCommand(Channel* channel) : ForgeCommand(COMMAND_TYPE) {
        channel->Read(&type, sizeof(button_onClickCommand)); 
    }
    HRESULT exec();
};
