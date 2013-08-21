#ifndef __FEEDBACKDIALOG_H__
#define __FEEDBACKDIALOG_H__

using namespace ATL;

#define IDD_FEEDBACK_DIALOG 1000
#define IDC_EDIT_FEEDBACK_TEXT 1001
#define IDC_BUTTON_SEND 1003
#define IDC_BUTTON_CANCEL 1004
#define IDC_STATUS 1002
#define IDC_IMG1 1005

#include <util.h>

class FeedbackDialog {
 public:
    FeedbackDialog(const wstring& author) {
        FeedbackDialog::author = author;
    }
	
	bool Show(HWND parent);
    
 private:
	static INT_PTR CALLBACK DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    
    static const INT_PTR _dialogCancel = 1;
	static const INT_PTR _dialogOk = 2;
    
    static void SendFeedback(HWND hWnd);
    
    struct FeedbackParam {
        FeedbackParam(const std::string& username, 
                      const std::wstring& message) 
          : username(username), 
            message(message) {
        }
        std::string username;
        std::wstring message;
    };
    
    static DWORD WINAPI SendFeedbackThreadProc(LPVOID param);

    static wstring author;
    static bool feedbackSent;
};


#endif /* __FEEDBACKDIALOG_H__ */
