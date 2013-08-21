#pragma once

#include <util.h>

namespace WindowsMessage {
    // helpers
    HWND   GetToolbar(HWND ieframe, HWND *toolbar = NULL, HWND *target = NULL);

    // messages
    int  tb_buttoncount(HWND hwnd);
    bool tb_getbutton(HWND hwnd, int index, TBBUTTON *out);
    bool tb_getbuttoninfo(HWND hwnd, int index, TBBUTTONINFO *out);
    bool tb_setbuttoninfo(HWND hwnd, int index, TBBUTTONINFO *buttoninfo);
    bool tb_getitemrect(HWND hwnd, int index, RECT *out);
    bool tb_getrect(HWND hwnd, int idCommand, RECT *out);
    bool tb_getmetrics(HWND hwnd, TBMETRICS *out);
    bool tb_getimagelist(HWND hwnd, HIMAGELIST *out);
    bool tb_gethotimagelist(HWND hwnd, HIMAGELIST *out);
    bool tb_getpressedimagelist(HWND hwnd, HIMAGELIST *out);
    bool tb_setimagelist(HWND hwnd, HIMAGELIST imagelist);
    bool tb_sethotimagelist(HWND hwnd, HIMAGELIST imagelist);
    bool tb_setpressedimagelist(HWND hwnd, HIMAGELIST imagelist);
    bool tb_changebitmap(HWND hwnd, int idCommand, int index);
};

