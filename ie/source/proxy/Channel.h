#pragma once

#include <util.h>


/**
 * Channel
 */
class Channel 
{
public:
    Channel(wchar_t *baseName, DWORDX processId);
    ~Channel();

    BOOL Read(LPVOID data, DWORD dataSize, BOOL response = FALSE);
    BOOL Write(LPVOID data, DWORD dataSize, BOOL response = FALSE);

    BOOL SetReady() { return ::SetEvent(m_events[SERVER_AVAILABLE]); }
    BOOL IsFirst()  { return m_first; }
    
public:
    static const DWORD SECTION_SIZE = 4096;

    enum States {
        REQUEST_AVAILABLE,
        RESPONSE_AVAILABLE,
        SERVER_AVAILABLE,
        STATE_COUNT
    };

private:
    HANDLE m_section;
    HANDLE m_events[STATE_COUNT];
    BOOL   m_first;
};


/**
 * Command
 */
struct Command {
    UINTX type;
    Command() : type(0x00) { }
    Command(UINTX type) : type(type) { }
};

struct MessageCommand : public Command {
    UINTX  msg;
    WPARAM wparam;
    LPARAM lparam;
    MessageCommand() : msg(0), wparam(0), lparam(0) { }
    MessageCommand(UINTX type, UINTX uMsg, WPARAM wparam, LPARAM lparam) : 
        Command(type), msg(uMsg), wparam(wparam), lparam(lparam) { }
    MessageCommand(Channel* channel) { 
        channel->Read(&type, sizeof(MessageCommand)); 
    }
};

struct SendMessageCommand : public MessageCommand {
    static const UINTX COMMAND_TYPE = 0x01;
    SendMessageCommand(UINTX uMsg, WPARAM wparam, LPARAM lparam) : 
        MessageCommand(COMMAND_TYPE, uMsg, wparam, lparam) { }
    SendMessageCommand(Channel* channel) : 
        MessageCommand(channel) { }
};

struct PostMessageCommand : public MessageCommand {
    static const UINTX COMMAND_TYPE = 0x02;
    PostMessageCommand(UINTX uMsg, WPARAM wparam, LPARAM lparam) : 
        MessageCommand(COMMAND_TYPE, uMsg, wparam, lparam) { }
    PostMessageCommand(Channel* channel) : 
        MessageCommand(channel) { }
};

struct TabCommand : public Command {
    DWORDX   processId;
    INT_PTRX proxy;
	HWND toolbar;
    TabCommand(UINTX type) : Command(type) { }
    TabCommand(UINTX type, DWORDX processId, INT_PTRX proxy) : 
        Command(type), processId(processId), proxy(proxy) { }
	TabCommand(UINTX type, DWORDX processId, INT_PTRX proxy, HWND toolbar) : 
        Command(type), processId(processId), proxy(proxy), toolbar(toolbar) { }
    TabCommand(Channel* channel) { 
        channel->Read(&type, sizeof(TabCommand));
    }
};

#pragma warning(disable:4996) // http://www.urbandictionary.com/define.php?term=ffffuuuu

struct ForgeCommand : public TabCommand {
    wchar_t uuid [MAX_PATH];
    ForgeCommand(UINTX type) : TabCommand(type) { }
    ForgeCommand(UINTX type, DWORDX processId, INT_PTRX proxy, const wstring& uuid) 
        : TabCommand(type, processId, proxy) { 
        size_t len = uuid.copy(this->uuid, MAX_PATH, 0);
        this->uuid[len] = L'\0';
    }
    ForgeCommand(Channel* channel) : TabCommand(type) { 
        channel->Read(&type, sizeof(ForgeCommand)); 
    }
};

struct LoadCommand : public ForgeCommand {
    wchar_t title[MAX_PATH];
    wchar_t icon [MAX_PATH];
    HWNDX addressBarWnd;
    HWNDX commandTargetWnd;
    static const UINTX COMMAND_TYPE = 0x03;
    LoadCommand(HWNDX addressBarWnd, HWNDX commandTargetWnd, 
                const wstring& uuid, const wstring& title, const wstring& icon, 
                DWORDX processId, INT_PTRX proxy) 
        : ForgeCommand(COMMAND_TYPE, processId, proxy, uuid),
          addressBarWnd(addressBarWnd), 
          commandTargetWnd(commandTargetWnd) { 
        size_t len = title.copy(this->title, MAX_PATH, 0);
        this->title[len] = L'\0';
        len = icon.copy(this->icon, MAX_PATH, 0);
        this->icon[len] = L'\0';
        logger->debug(L"LoadCommand::LoadCommand"
                      L" -> " + wstring(this->uuid) + 
                      L" -> " + wstring(this->title) +
                      L" -> " + wstring(this->icon));  
    }
    LoadCommand(Channel* channel) : ForgeCommand(COMMAND_TYPE) { 
        channel->Read(&type, sizeof(LoadCommand)); 
    }
};

struct UnloadCommand : public TabCommand {
    static const UINTX COMMAND_TYPE = 0x04;
    UnloadCommand(DWORDX processId, INT_PTRX proxy) : 
        TabCommand(COMMAND_TYPE, processId, proxy) { }
    UnloadCommand(Channel* channel) : 
        TabCommand(channel) { }
};

struct SelectTabCommand : public TabCommand {
    static const UINTX COMMAND_TYPE = 0x05;
    SelectTabCommand(DWORDX processId, INT_PTRX proxy) : 
        TabCommand(COMMAND_TYPE, processId, proxy) { }
	SelectTabCommand(DWORDX processId, INT_PTRX proxy, HWND toolbar) : 
        TabCommand(COMMAND_TYPE, processId, proxy, toolbar) { }
    SelectTabCommand(Channel* channel) : 
        TabCommand(channel) { }
};

struct ForwardedMessage {
    INT_PTRX proxy;
    UINTX    msg;
    WPARAM   wparam;
    LPARAM   lparam;
    static const UINTX COMMAND_TYPE = 0x06;
    ForwardedMessage() { }
    ForwardedMessage(INT_PTRX proxy, UINTX msg, WPARAM wparam, LPARAM lparam) :
        proxy(proxy), 
        msg(msg), 
        wparam(wparam), 
        lparam(lparam) { }
};
