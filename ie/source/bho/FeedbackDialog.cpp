#include "stdafx.h"
#include "FeedbackDialog.h"

#include <generated/BHO_h.h>
#include <util.h>
#include "dllmain.h"

bool    FeedbackDialog::feedbackSent = false;
wstring FeedbackDialog::author       = L"";


/**
 * FeedbackDialog::Show - display dialog and return user selection
 */
bool FeedbackDialog::Show(HWND parent)
{
    INT_PTR ret =  ::DialogBoxParamW(_AtlModule.moduleHandle, 
                                     MAKEINTRESOURCE((WORD)IDD_FEEDBACK_DIALOG), 
                                     parent, 
                                     FeedbackDialog::DialogProc, 
                                     NULL);
	return ret != NULL;
}


/**
 *
 */
INT_PTR FeedbackDialog::DialogProc(HWND dialog, UINT msgid, 
                                   WPARAM wparam, LPARAM lparam)
{
	switch (msgid)	{
	case WM_INITDIALOG:
        return TRUE;
    case WM_TIMER:
        //::MessageBoxW(NULL, L"d", L"d", MB_OK);
        if (wparam == 1345 && feedbackSent == true) {
            ::KillTimer(dialog, 1345);
            EndDialog(dialog, _dialogOk);
        }
        break;
	case WM_CLOSE:
        // Close the dialog
		EndDialog(dialog, _dialogCancel);
		return TRUE;
	case WM_COMMAND:
		if (HIWORD(wparam) == BN_CLICKED) {
			switch (LOWORD(wparam)) {
			case IDC_BUTTON_CANCEL:
                // Close the dialog, indicating cancel.
				EndDialog(dialog, _dialogCancel);
				return FALSE;
			case IDC_BUTTON_SEND:
                SendFeedback(dialog);
                return TRUE;
			}
		}
		break;
	}
	return FALSE;
}


/**
 *
 */
void FeedbackDialog::SendFeedback(HWND dialog) 
{
    wchar_t feedbackText[2048];
    ::GetDlgItemText(dialog, IDC_EDIT_FEEDBACK_TEXT, feedbackText, 2048);

    feedbackSent = false;
    FeedbackParam* feedbackParam = new FeedbackParam("23", feedbackText);
    HANDLE thread = CreateThread(NULL, 0, FeedbackDialog::SendFeedbackThreadProc, feedbackParam, 0, NULL);

    ::EnableWindow(GetDlgItem(dialog, IDC_BUTTON_SEND), FALSE);
    ::ShowWindow(GetDlgItem(dialog, IDC_STATUS), SW_NORMAL);
    ::SetTimer(dialog, 1345, 100, NULL);
}


/**
 *
 */
DWORD WINAPI FeedbackDialog::SendFeedbackThreadProc(LPVOID param)
{
    // Initialize COM for this thread.
    HRESULT hr;
    hr = ::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) {
        logger->debug(L"FeedbackDialog::SendFeedbackThreadProc could not initialize COM" 
                      L" -> " + boost::lexical_cast<wstring>(hr));
        return hr;
    }

    // TODO - vendorspec
    //https://trigger.io/uninstall_report?user_id=%257BUSER_NAME%257D&message=IE+%257BMESSAGE%257D
    wstring url = L"https://trigger.io/uninstall_report?";
    wstring query;
	FeedbackParam *feedbackParam;

    CComPtr<IXMLHTTPRequest> request;
    hr = ::CoCreateInstance(CLSID_XMLHTTP60, NULL, 
                            CLSCTX_INPROC, // CLSCTX_ALL,
                            IID_IXMLHTTPRequest, 
                            reinterpret_cast<void**>(&request));
    if (FAILED(hr) || !request) {
        logger->debug(L"FeedbackDialog::SendFeedbackThreadProc xmlhttp60 failed");
		goto cleanup;
    }

    // open request
    hr = request->open(CComBSTR(L"POST"), CComBSTR(url.c_str()), 
                       CComVariant(false), CComVariant(L""), CComVariant(L""));
    if (FAILED(hr)) {
        logger->error(L"FeedbackDialog::SendFeedbackThreadProc request->open failed");
        goto cleanup;
    }

    // build query
	feedbackParam = static_cast<FeedbackParam*>(param);
    if (!feedbackParam) {
        logger->error(L"FeedbackDialog::SendFeedbackThreadProc invalid feedbackParam");
        goto cleanup;
    }
    query = L"user_id=" + wstring().assign(feedbackParam->username.begin(),
                                           feedbackParam->username.end()) + 
            L"&message=IE+" + feedbackParam->message;

    // set request headers
    request->setRequestHeader(CComBSTR(L"Content-type"),
                              CComBSTR(L"application/x-www-form-urlencoded"));
    request->setRequestHeader(CComBSTR(L"Content-length"),
                              CComBSTR(boost::lexical_cast<wstring>(query.length()).c_str()));
    request->setRequestHeader(CComBSTR(L"Connection"), CComBSTR(L"close"));

    // send request
    hr = request->send(CComVariant(query.c_str()));
    if (FAILED(hr)) {
        logger->error(L"FeedbackDialog::SendFeedbackThreadProc request->send failed");
        goto cleanup;
    }

    long status;
    hr = request->get_status(&status);
    if (FAILED(hr) || status != 200) {
        logger->debug(L"FeedbackDialog::SendFeedbackThreadProc status failed");
    }

    // always say thank you!
    ::MessageBoxW(NULL, L"Thank you for your feedback.", FeedbackDialog::author.c_str(), MB_OK);
    
 cleanup:
    delete param;
    ::CoUninitialize();
    feedbackSent = true;
    return 0;
}

