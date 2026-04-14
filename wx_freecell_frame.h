/****************************************************************************
 * wx_freecell_frame.h
 *
 * Main frame window for wxWidgets FreeCell port.
 * Handles menus, accelerators, and delegates to the game panel.
 *
 * Original: freecell.c (WinMain, InitApplication, InitInstance, MainWndProc)
 ****************************************************************************/

#ifndef WX_FREECELL_FRAME_H
#define WX_FREECELL_FRAME_H

#include <wx/wx.h>
#include "wx_freecell.h"

class FreeCellPanel;

class FreeCellFrame : public wxFrame
{
public:
    FreeCellFrame();

    FreeCellPanel* GetPanel() { return m_panel; }

    /* Statistics persistence (called from panel on win/loss) */
    void SaveWinStats();
    void SaveLossStats();
    void ReadOptions();
    void WriteOptions();

private:
    /* Menu event handlers */
    void OnNewGame(wxCommandEvent& event);
    void OnSelectGame(wxCommandEvent& event);
    void OnRestart(wxCommandEvent& event);
    void OnStats(wxCommandEvent& event);
    void OnOptions(wxCommandEvent& event);
    void OnUndo(wxCommandEvent& event);
    void OnExit(wxCommandEvent& event);
    void OnAbout(wxCommandEvent& event);
    void OnCheat(wxCommandEvent& event);
    void OnClose(wxCloseEvent& event);

    /* Statistics helpers */
    unsigned int CalcPercentage(unsigned int wins, unsigned int losses);

    FreeCellPanel* m_panel;

    wxDECLARE_EVENT_TABLE();
};

#endif // WX_FREECELL_FRAME_H
