#include "stdafx.h"
#include "BrowserWindow.h"

#include <gdiplus.h>
using namespace Gdiplus;


/**
 * Construction
 */
BrowserWindow::BrowserWindow(const wstring& uuid, const wstring& url) 
    : hiddenBrowser(this, uuid),
      uuid(uuid), url(url)
{
    logger->debug(L"BrowserWindow::BrowserWindow"
                  L" -> " + uuid);
}


/**
 * Event: BrowserWindow::OnCreate
 */
LRESULT BrowserWindow::OnCreate(UINT msg, WPARAM wparam, LPARAM lparam, BOOL& handled)
{
    if (!this->m_hWnd) {
        logger->debug(L"BrowserWindow::Oncreate says this does not yet exist");
        return 0;
    }

    logger->debug(L"BrowserWindow::OnCreate");

    HRESULT hr;
    HWND hwnd;
    CRect dimensions;

    hr = this->GetClientRect(&dimensions);
    hwnd = hiddenBrowser.Create(this->m_hWnd, 
                                dimensions,
                                CComBSTR(this->url.c_str()), // L"about:blank",
                                WS_CHILD | WS_VISIBLE);
    if (!hwnd) {
        logger->debug(L"BrowserWindow::OnCreate failed to create BrowserControl");
        return 0; 
    }

    hr = hiddenBrowser.ShowWindow(SW_SHOW);
    hr = this->ShowWindow(SW_HIDE);
    
    return 0; 
}


/**
 * Destruction
 */
BrowserWindow::~BrowserWindow() 
{
    logger->debug(L"BrowserWindow::~BrowserWindow");

    HRESULT hr;
    hr = this->DestroyWindow();
    if (FAILED(hr)) {
        logger->warn(L"BrowserWindow::~BrowserWindow failed to destroy window");
    }    
    this->m_hWnd = NULL;

    logger->debug(L"BrowserWindow::~BrowserWindow done");
}


/**
 * Event: BrowserWindow::OnDestroy
 */
LRESULT BrowserWindow::OnDestroy(UINT msg, WPARAM wparam, LPARAM lparam, BOOL& handled)
{
    logger->debug(L"BrowserWindow::OnDestroy");
    return 0;
}

/**
 * Event: BrowserWindow::OnSize
 */
LRESULT BrowserWindow::OnSize(UINT msg, WPARAM wparam, LPARAM lparam, BOOL& handled)
{
    if (!hiddenBrowser) {
        logger->debug(L"BrowserWindow::OnSize hiddenBrowser not initialized");
        return -1;
    }

    RECT rect; 
    WORD width, height;
    this->GetClientRect(&rect);
    width  = LOWORD(lparam);
    height = HIWORD(lparam);
    hiddenBrowser.MoveWindow(rect.left, rect.top, rect.right, rect.bottom);
    handled = TRUE;

    return 0;
}


