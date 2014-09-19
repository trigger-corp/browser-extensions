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
    if (out_toolbar) *out_toolbar = NULL;
    if (out_target)  *out_target = NULL;

    // List of window classes to traverse from IEFrame window to toolbar window where we place the icon:
    // IE7 Command bar right of tabs
    static const wchar_t* ie7_cmd_bar[] = { L"CommandBarClass", L"ReBarWindow32", L"ToolbarWindow32" };
    // IE8+ Command bar right of tabs. Hidden by default on IE9+
    static const wchar_t* ie8_cmd_bar[] = { L"CommandBarClass", L"ReBarWindow32", L"CommandToolbarBand", L"ToolbarWindow32" };
    // IE9+ Favorites and Tools bar right of address bar
    static const wchar_t* ie9_tools_bar[] = { L"WorkerW", L"ReBarWindow32", L"ControlBandClass", L"ToolbarWindow32" };

    int ie_major = 0, ie_minor = 0;
    if (FAILED(GET_MSIE_VERSION(&ie_major, &ie_minor))) {
        logger->error(L"WindowsMessage::GetToolbar failed to determine IE version, assuming IE 9");
        ie_major = 9;
    }

    wstringvector class_list;
    if (ie_major >= 9) {
        class_list = wstringvector(ie9_tools_bar, staticarray_end(ie9_tools_bar));
    }
    else if (ie_major == 8) {
        class_list = wstringvector(ie8_cmd_bar, staticarray_end(ie8_cmd_bar));
    }
    else {
        class_list = wstringvector(ie7_cmd_bar, staticarray_end(ie7_cmd_bar));
    }

    logger->debug(L"WindowsMessage::GetToolbar class list"
                  L" -> " + boost::algorithm::join(class_list, L", "));

    HWND window = ieframe;
    HWND parent = ieframe;
    wstringvector::const_iterator window_class = class_list.begin();
    for (; window_class != class_list.end(); window_class++) {
        parent = window;
		if (ie_major >= 9 && (*window_class).c_str() == L"ToolbarWindow32") {
			window = ::FindWindowEx(parent, NULL, (*window_class).c_str(), L"Favorites and Tools Bar");
		} else {
			window = ::FindWindowEx(parent, NULL, (*window_class).c_str(), NULL);
		}
        if (!window) {
            logger->error(L"WindowsMessage::GetToolbar failed to get " + *window_class);
            return NULL;
        }
    }

    if (out_toolbar) *out_toolbar = window;
    if (out_target)  *out_target  = parent;
    
    return window;
}

/**
* Helper: AddToolbarIcon
*
* Adds or replaces an icon on all image lists used by the toolbar.
* If index is -1 the icon will be added to the end of the lists and its new
* index returned.
* If index is specified and an icon already exists at that index the
* previous icon will be replaced.
*/
bool WindowsMessage::AddToolbarIcon(HWND toolbar, HICON icon, int *index)
{
    if (!index) {
        logger->error(L"WindowsMessage::AddToolbarIcon invalid index argument");
        return false;
    }

    // Deal with main image list first. If this fails we error.
    HIMAGELIST main_list = reinterpret_cast<HIMAGELIST>(::SendMessage(toolbar, TB_GETIMAGELIST, 0, 0));
    if (!main_list) {
        logger->error(L"WindowsMessage::AddToolbarIcon failed to get main image list"
                      L" -> " + boost::lexical_cast<wstring>(toolbar));
        return false;
    }

    int main_index = ::ImageList_ReplaceIcon(main_list, *index, icon);
    if (main_index == -1) {
        logger->error(L"WindowsMessage::AddToolbarIcon failed to add/replace main icon"
                      L" -> " + boost::lexical_cast<wstring>(toolbar) +
                      L" -> " + boost::lexical_cast<wstring>(index));
        return false;
    }

    // Add the icon to all secondary image lists the toolbar has
    static const DWORD secondary_lists[] = { TB_GETHOTIMAGELIST,
                                             TB_GETPRESSEDIMAGELIST,
                                             TB_GETDISABLEDIMAGELIST };

    for (int i = 0; i < sizeof(secondary_lists) / sizeof(secondary_lists[0]); i++) {
        HIMAGELIST secondary_list = reinterpret_cast<HIMAGELIST>(::SendMessage(toolbar, secondary_lists[i], 0, 0));
        if (!secondary_list) {
            // Failure to get the a secondary image list is not fatal. In fact it is
            // excpected for the IE Command Bar and we only need it when using the IE9+
            // Tools Bar. See WindowsMessage::GetToolbar() for reference.
            /*
            logger->debug(L"button_addCommand::exec failed to get secondary image list"
                          L" -> " + boost::lexical_cast<wstring>(i));
            */
            continue;
        }

        int secondary_index = ::ImageList_ReplaceIcon(secondary_list, *index, icon);
        if (secondary_index == -1 || secondary_index != main_index) {
            // The image lists have gone out of sync somehow and the user will might
            // the wrong icon in some cases. Not much we can do about it.
            logger->error(L"WindowsMessage::AddToolbarIcon failed to add/replace secondary icon"
                          L" -> " + boost::lexical_cast<wstring>(i) +
                          L" -> " + boost::lexical_cast<wstring>(index) +
                          L" -> " + boost::lexical_cast<wstring>(main_index) +
                          L" -> " + boost::lexical_cast<wstring>(secondary_index));
            // Continue to hopefully get the other lists right
            continue;
        }
    }

    *index = main_index;
    return true;
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
