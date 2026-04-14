/****************************************************************************
 * wx_freecell_app.h
 *
 * wxWidgets application class for FreeCell.
 ****************************************************************************/

#ifndef WX_FREECELL_APP_H
#define WX_FREECELL_APP_H

#include <wx/wx.h>

class FreeCellApp : public wxApp
{
public:
    virtual bool OnInit() override;
};

#endif // WX_FREECELL_APP_H
