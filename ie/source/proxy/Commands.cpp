#include "stdafx.h"
#include <WindowsMessage.h>
#include "Commands.h"


/**
 * Command: button_addCommand::exec
 */
HRESULT button_addCommand::exec(HWND toolbar, HWND target, FrameServer::Button *out_button) 
{
    logger->debug(L"button_addCommand::exec"
                  L" -> " + wstring(this->uuid) +
                  L" -> " + wstring(this->title) +
                  L" -> " + wstring(this->icon) +
                  L" -> " + boost::lexical_cast<wstring>(toolbar) +
                  L" -> " + boost::lexical_cast<wstring>(target));

    // load bitmap 
    bfs::wpath path(this->icon);
    if (!bfs::exists(path)) {
        logger->debug(L"button_addCommand::exec invalid file"
                      L" -> " + path.wstring());
        return E_FAIL;
    }

    HICON icon;
    icon = (HICON)::LoadImage(NULL, path.wstring().c_str(),
                              IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_LOADFROMFILE);
    if (!icon) {
        logger->error(L"button_addCommand::exec failed to load icon"
                      L" -> " + path.wstring());
        return E_FAIL;
    }

    // get toolbar image list
    HIMAGELIST imagelist;
    if (!WindowsMessage::tb_getimagelist(toolbar, &imagelist)) {
        logger->error(L"button_addCommand::exec failed to get image lists"
                     L" -> " + path.wstring());
        return E_FAIL;
    }

    // add button
    int index = ImageList_AddIcon(imagelist, icon);
    TBBUTTON button;
    memset(&button, 0, sizeof(TBBUTTON));
    button.fsState = TBSTATE_ENABLED;
    button.idCommand = 0xc001; // TODO
    button.iBitmap = index;
    button.fsStyle = 0;
    button.iString = (INT_PTR)this->title;
    ::SendMessage(toolbar, TB_INSERTBUTTON, 0, (LPARAM)&button);

    // refresh the size of the toolbar
    ::SendMessage(toolbar, WM_SIZE, 0, 0);
    ::SendMessage(target,  WM_SIZE, 0, 0);

    (*out_button).uuid      = this->uuid;
    (*out_button).idCommand = 0xc001;     // TODO
    (*out_button).iBitmap   = index;
    (*out_button).icon      = icon;
    (*out_button).toolbar   = toolbar;
    (*out_button).target    = target;

    return S_OK;
}


/**
 * Command: button_setIconCommand::exec 
 */
HRESULT button_setIconCommand::exec(HWND toolbar, HWND target, int idCommand, int iBitmap) 
{
    wstring url(this->url);

    logger->debug(L"button_setIconCommand::exec"
                  L" -> " + wstring(this->uuid) +
                  L" -> " + url +
                  L" -> " + boost::lexical_cast<wstring>(toolbar) +
                  L" -> " + boost::lexical_cast<wstring>(target) +
                  L" -> " + boost::lexical_cast<wstring>(idCommand) +
                  L" -> " + boost::lexical_cast<wstring>(iBitmap));

    // TODO support network URLs
    bfs::wpath path(url);

    // load bitmap from URL
    HICON icon;
    icon = (HICON)::LoadImage(NULL, path.wstring().c_str(),
                              IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_LOADFROMFILE);
    if (!icon) {
        logger->debug(L"button_setIconCommand::exec failed to load icon"
                      L" -> " + path.wstring());
    }

    // get toolbar image list
    HIMAGELIST imagelist;
    if (!WindowsMessage::tb_getimagelist(toolbar, &imagelist)) {
        logger->error(L"button_setIconCommand::exec failed to get image lists"
                     L" -> " + path.wstring());
        return E_FAIL;
    }

    // replace icon 
    if (ImageList_ReplaceIcon(imagelist, iBitmap, icon) == -1) {
        logger->error(L"button_setIconCommand::exec failed to replace icon"
                     L" -> " + path.wstring());
        return E_FAIL;    }

    if (!WindowsMessage::tb_setimagelist(toolbar, imagelist)) {
        logger->error(L"button_setIconCommand::exec failed to set imagelist"
                     L" -> " + path.wstring());
        return E_FAIL;
    }
    if (!WindowsMessage::tb_changebitmap(toolbar, idCommand, iBitmap)) {
        logger->error(L"button_setIconCommand::exec failed to change bitmap"
                     L" -> " + path.wstring());
        return E_FAIL;
    }
      
    return S_OK;
}


/**
 * Command: button_setTitleCommand::exec 
 */
HRESULT button_setTitleCommand::exec(HWND toolbar, HWND target, int idCommand) 
{
    logger->debug(L"button_setTitleCommand::exec"
                  L" -> " + wstring(this->uuid) +
                  L" -> " + wstring(this->title) +
                  L" -> " + boost::lexical_cast<wstring>(toolbar) +
                  L" -> " + boost::lexical_cast<wstring>(target));

    // init button info
    TBBUTTONINFO buttoninfo;
    memset(&buttoninfo, 0, sizeof(TBBUTTONINFO));
    buttoninfo.cbSize  = sizeof(TBBUTTONINFO);
    buttoninfo.dwMask  = TBIF_COMMAND | TBIF_TEXT;
    buttoninfo.pszText = new wchar_t[MAX_PATH];
    buttoninfo.cchText = MAX_PATH;
    if (!WindowsMessage::tb_getbuttoninfo(toolbar, idCommand, &buttoninfo)) {
        logger->error(L"    button"
                      L" -> " + boost::lexical_cast<wstring>(idCommand) +
                      L" -> tb_fetbuttoninfo failed");
        return S_OK;
    } 
    delete buttoninfo.pszText;

    CComBSTR btitle(title);
    buttoninfo.pszText = btitle;  // TODO
    buttoninfo.cchText = btitle.Length() * sizeof(TCHAR); // TODO

    // set title
    if (!WindowsMessage::tb_setbuttoninfo(toolbar, idCommand, &buttoninfo)) {
        logger->error(L"    button"
                      L" -> " + boost::lexical_cast<wstring>(idCommand) +
                      L" -> tb_setbuttoninfo failed");
    }

    return S_OK;
}


/**
 * Command: button_setBadgeCommand::exec 
 */
HRESULT button_setBadgeCommand::exec(HWND toolbar, HWND target, int idCommand) 
{
    logger->debug(L"button_setBadgeCommand::exec"
                  L" -> " + wstring(this->uuid) +
                  L" -> " + boost::lexical_cast<wstring>(this->number) +
                  L" -> " + boost::lexical_cast<wstring>(toolbar) +
                  L" -> " + boost::lexical_cast<wstring>(target));

    // get button bitmap
    
    // draw badge over button bitmap

    // replace button bitmap
    
    return S_OK;
}


/**
 * Command: button_setBadgeBackgroundColorCommand::exec
 */
HRESULT button_setBadgeBackgroundColorCommand::exec(HWND toolbar, HWND target, int idCommand) 
{
    logger->debug(L"button_setBadgeBackgroundColorCommand::exec"
                  L" -> " + wstring(this->uuid) +
                  L" -> " + boost::lexical_cast<wstring>(this->r) +
                  L" -> " + boost::lexical_cast<wstring>(this->g) +
                  L" -> " + boost::lexical_cast<wstring>(this->b) +
                  L" -> " + boost::lexical_cast<wstring>(this->a) +
                  L" -> " + boost::lexical_cast<wstring>(toolbar) +
                  L" -> " + boost::lexical_cast<wstring>(target));
    
    return S_OK;
}


#include <generated/Forge_i.h> // For INativeControls

/**
 * Event: button_onClickCommand::exec
 */
HRESULT button_onClickCommand::exec() 
{
    HRESULT hr;

    logger->debug(L"button_onClickCommand::exec"
                  L" -> " + wstring(this->uuid) +
                  L" -> " + boost::lexical_cast<wstring>(this->point.x) +
                  L" -> " + boost::lexical_cast<wstring>(this->point.y));

    // invoke NativeControls
    logger->debug(L"button_onClickCommand::exec invoking NativeControls");
    hr = ::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED); 
    CComPtr<INativeControls> nativeControls;
    hr = ::CoCreateInstance(CLSID_NativeControls, 
                            NULL,
                            CLSCTX_LOCAL_SERVER, 
                            IID_INativeControls,
                            (LPVOID*)&nativeControls);
    if (FAILED(hr) || !nativeControls) {
        logger->error(L"button_onClickCommand::exec"
                      L"failed to create NativeControls instance"
                      L" -> " + logger->parse(hr));
        ::CoUninitialize();
        return S_OK;
    }

    hr = nativeControls->button_click(CComBSTR(this->uuid), this->point);
    if (FAILED(hr)) {
        logger->error(L"button_onClickCommand::exec"
                      L"failed to invoke NativeControls::button_click"
                      L" -> " + logger->parse(hr));
    }

    // Force popup to the foreground for IE9
    int major, minor;
    hr = GET_MSIE_VERSION(&major, &minor);
    if (FAILED(hr)) {
        logger->error(L"button_onClickCommand::exec "
                      L"failed to get browser version");
        ::CoUninitialize();
        return S_OK;
    }

    if (major >= 9) {
        BOOL visible = FALSE;
        HWND popup = NULL;
        hr = nativeControls->popup_hwnd(CComBSTR(this->uuid), &visible, (ULONG*)&popup);
        if (FAILED(hr) || !popup) {
            logger->error(L"button_onClickCommand::exec"
                          L"failed to invoke NativeControls::popup_hwnd"
                          L" -> " + logger->parse(hr));
            ::CoUninitialize();
            return S_OK;
        }
        
        if (visible && popup) {
            logger->debug(L"Forcing popup to foreground");
            if (::SetForegroundWindow(popup) == 0) {
                logger->error(L"button_onClickCommand::exec failed to set fg win"
                              L"-> " + logger->GetLastError());
            } 
        }
    }

    ::CoUninitialize();
    
    return S_OK;
}

