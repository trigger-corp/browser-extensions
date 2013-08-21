#include "stdafx.h"
#include "WindowsMessage.h"
#include <util.h>


/**
 * Helper: GetToolbar
 *
 * Retrieves the ToolBar hwnd from IE hwnd
 */
HWND WindowsMessage::GetToolbar(HWND ieframe, HWND *out_toolbar, HWND *out_target)
{
    *out_toolbar = NULL;
    *out_target  = NULL;

    HWND commandbar = ::FindWindowEx(ieframe, NULL, _T("CommandBarClass"),  NULL);
	if (!commandbar) {
        logger->error(L"WindowsMessage::GetToolbar failed to get CommandBarClass");
        return NULL;
    }
    HWND rebar = ::FindWindowEx(commandbar, NULL, _T("ReBarWindow32"), NULL);
	if (!rebar) {
        logger->error(L"WindowsMessage::GetToolbar failed to get ReBarWindow32");
        return NULL;
    }
    HWND commandtoolbar = ::FindWindowEx(rebar, NULL, _T("CommandToolbarBand"), NULL); // Only >= IE8 ?
	if (!commandtoolbar) {
        logger->error(L"WindowsMessage::GetToolbar failed to get CommandToolbarBand");
        return NULL;
    }
    HWND toolbar = ::FindWindowEx(commandtoolbar, NULL, _T("ToolbarWindow32"), NULL);
    if (!toolbar) {
        logger->error(L"WindowsMessage::GetToolbar failed to get ToolbarWindow32");
        return NULL;
    }
    
    if (out_toolbar) *out_toolbar = toolbar;
    if (out_target)  *out_target  = commandtoolbar;
    
    return toolbar;
}


/**
 * Message: TB_BUTTONCOUNT
 */
int WindowsMessage::tb_buttoncount(HWND hwnd)
{
    return static_cast<int>(::SendMessage(hwnd, TB_BUTTONCOUNT, 0, 0));
}


/**
 * Message: TB_GETBUTTON
 */
bool WindowsMessage::tb_getbutton(HWND hwnd, int index, 
                                  TBBUTTON *out)
{
    if (!::SendMessage(hwnd, TB_GETBUTTON, index, (LPARAM)out)) {
        logger->error(L"WindowsMessage::tb_getbutton"
                      L" -> SendMessage failed"
                      L" -> " + boost::lexical_cast<wstring>(index));
        return false;
    }

    return true;
}


/**
 * Message: TB_GETBUTTONINFO
 */
bool WindowsMessage::tb_getbuttoninfo(HWND hwnd, int index, TBBUTTONINFO *out)
{
    if (!out || !out->pszText) {
        logger->error(L"WindowsMessage::tb_getbuttoninfo invalid out argument");
        return false;
    }

    size_t bytes = static_cast<int>(::SendMessage(hwnd, TB_GETBUTTONINFO, index, (LPARAM)out));
    if (bytes == -1) {
        logger->error(L"WindowsMessage::tb_getbuttoninfo failed to send message");
        return false;
    }

    return true;
}


/**
 * Message: TB_SETBUTTONINFO
 */
bool WindowsMessage::tb_setbuttoninfo(HWND hwnd, int index, 
                                      TBBUTTONINFO *buttoninfo)
{
    if (!buttoninfo) {
        logger->error(L"WindowsMessage::tb_setbuttoninfo invalid buttoninfo");
        return false;
    }

    if (!::SendMessage(hwnd, TB_SETBUTTONINFO, index, (LPARAM)buttoninfo)) {
        logger->error(L"WindowsMessage::tb_setbuttoninfo failed to send message");
        return false;
    }
    
    return true;   
}


/**
 * Message: TB_GETITEMRECT
 */
bool WindowsMessage::tb_getitemrect(HWND hwnd, int index, RECT *out)
{
    if (!out) {
        logger->error(L"WindowsMessage::tb_getitemrect invalid out argument");
        return false;
    }

    if (!::SendMessage(hwnd, TB_GETITEMRECT, index, (LPARAM)out)) {
        logger->error(L"WindowsMessage::tb_getitemrect failed to send message");
        return false;
    }

    return true;
}


/**
 * Message: TB_GETRECT
 */
bool WindowsMessage::tb_getrect(HWND hwnd, int idCommand, RECT *out)
{
    if (!out) {
        logger->error(L"WindowsMessage::tb_getrect invalid out argument");
        return false;
    }

    if (!::SendMessage(hwnd, TB_GETRECT, idCommand, (LPARAM)out)) {
        logger->error(L"WindowsMessage::tb_getrect failed to send message");
        return false;
    }

    return true;
}


/**
 * Message: TB_GETMETRICS
 */
bool WindowsMessage::tb_getmetrics(HWND hwnd, TBMETRICS *out)
{
    if (!out) {
        logger->error(L"WindowsMessage::tb_getmetrics invalid out argument");
        return false;
    }

    ::SendMessage(hwnd, TB_GETMETRICS, 0, (LPARAM)out);

    return true;
}


/**
 * Message: TB_GETIMAGELIST
 */
bool WindowsMessage::tb_getimagelist(HWND hwnd, HIMAGELIST *out)
{
    if (!out) {
        logger->error(L"WindowsMessage::tb_getimagelist invalid out argument");
        return false;
    }

    *out = NULL;
    *out = reinterpret_cast<HIMAGELIST>(::SendMessage(hwnd, TB_GETIMAGELIST, 0, 0));

    return *out != NULL;
}


/**
 * Message: TB_GETHOTIMAGELIST
 */
bool WindowsMessage::tb_gethotimagelist(HWND hwnd, HIMAGELIST *out)
{
    if (!out) {
        logger->error(L"WindowsMessage::tb_gethotimagelist invalid out argument");
        return false;
    }
    
    *out = NULL;
    *out = reinterpret_cast<HIMAGELIST>(::SendMessage(hwnd, TB_GETHOTIMAGELIST, 0, 0));
    
    return *out != NULL;
}


/**
 * Message: TB_GETPRESSEDIMAGELIST
 */
bool WindowsMessage::tb_getpressedimagelist(HWND hwnd, HIMAGELIST *out)
{
    if (!out) {
        logger->error(L"WindowsMessage::tb_getpressedimagelist invalid out argument");
        return false;
    }
    
    *out = NULL;
    *out = reinterpret_cast<HIMAGELIST>(::SendMessage(hwnd, TB_GETPRESSEDIMAGELIST, 0, 0));
    
    return *out != NULL;
}


/**
 * Message: TB_SETIMAGELIST
 */
bool WindowsMessage::tb_setimagelist(HWND hwnd, HIMAGELIST imagelist)
{
    return ::SendMessage(hwnd, TB_SETIMAGELIST, 0, (LPARAM)imagelist) != NULL;
}


/**
 * Message: TB_SETHOTIMAGELIST
 */
bool WindowsMessage::tb_sethotimagelist(HWND hwnd, HIMAGELIST imagelist)
{
    return ::SendMessage(hwnd, TB_SETHOTIMAGELIST, 0, (LPARAM)imagelist) != NULL;
}


/**
 * Message: TB_SETPRESSEDIMAGELIST
 */
bool WindowsMessage::tb_setpressedimagelist(HWND hwnd, HIMAGELIST imagelist)
{
    return ::SendMessage(hwnd, TB_SETPRESSEDIMAGELIST, 0, (LPARAM)imagelist) != NULL;
}


/**
 * Message: TB_CHANGEBITMAP
 */
bool WindowsMessage::tb_changebitmap(HWND hwnd, int idCommand, int index)
{
    return ::SendMessage(hwnd, TB_CHANGEBITMAP, (WPARAM)idCommand, (LPARAM)index) == TRUE;
}
