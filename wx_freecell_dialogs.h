/****************************************************************************
 * wx_freecell_dialogs.h
 *
 * Dialog classes for wxWidgets FreeCell port.
 * All dialogs load their layout from XRC resources (freecell.xrc).
 *
 * Original: June 91, JimH - initial code; Oct 91, port to Win32
 * wxWidgets port preserves dialog behavior from dialog.c.
 ****************************************************************************/

#ifndef WX_FREECELL_DIALOGS_H
#define WX_FREECELL_DIALOGS_H

#include <wx/wx.h>
#include <wx/xrc/xmlres.h>
#include "wx_freecell.h"

/****************************************************************************
 * MoveColDialog
 *
 * If there is ambiguity about whether the user intends to move a single card
 * or a column to an empty column, this dialog lets the user decide.
 *
 * GetResult() returns:
 *   -1    user chose cancel
 *    0    user chose to move a single card (FALSE)
 *    1    user chose to move a column (TRUE)
 ****************************************************************************/

class MoveColDialog : public wxDialog
{
public:
    MoveColDialog(wxWindow* parent);

    int GetResult() const { return m_result; }

private:
    void OnMoveCol(wxCommandEvent& event);
    void OnSingle(wxCommandEvent& event);
    void OnCancel(wxCommandEvent& event);

    int m_result;
};

/****************************************************************************
 * GameNumDialog
 *
 * The game number must be set before showing.  The user can accept or
 * change it.  Valid range is 1 to MAXGAMENUMBER, with -1 and -2 as
 * special cases (unwinnable shuffles).
 ****************************************************************************/

class GameNumDialog : public wxDialog
{
public:
    GameNumDialog(wxWindow* parent);

    void SetGameNum(int n);
    int  GetGameNum() const { return m_gameNum; }

private:
    void OnOK(wxCommandEvent& event);
    void OnCancel(wxCommandEvent& event);

    int m_gameNum;
};

/****************************************************************************
 * YouWinDialog
 *
 * Shows congratulations message.  The user can choose to play again
 * (Yes/No) and optionally check "Select game" to pick a game number.
 ****************************************************************************/

class YouWinDialog : public wxDialog
{
public:
    YouWinDialog(wxWindow* parent);

    bool GetSelectGame() const;
    void SetSelectGame(bool b);

private:
    void OnYes(wxCommandEvent& event);
    void OnNo(wxCommandEvent& event);
};

/****************************************************************************
 * YouLoseDialog
 *
 * Shows loss message.  The user can choose to play again (Yes/No)
 * and optionally check "Same game" to replay the same shuffle.
 * The checkbox defaults to checked (set in XRC).
 ****************************************************************************/

class YouLoseDialog : public wxDialog
{
public:
    YouLoseDialog(wxWindow* parent);

    bool GetSameGame() const;

private:
    void OnYes(wxCommandEvent& event);
    void OnNo(wxCommandEvent& event);
};

/****************************************************************************
 * StatsDialog
 *
 * Shows current session and total statistics, including streaks.
 * The Clear button asks for confirmation before clearing all stats.
 *
 * Return codes:
 *   wxID_OK   - user pressed OK
 *   100       - user pressed Clear and confirmed (stats should be cleared)
 ****************************************************************************/

class StatsDialog : public wxDialog
{
public:
    static const int CLEAR_STATS = 100;

    StatsDialog(wxWindow* parent);

    void SetStats(unsigned int sessionPct, unsigned int sessionWins,
                  unsigned int sessionLosses,
                  unsigned int totalPct, unsigned int totalWon,
                  unsigned int totalLost,
                  unsigned int winStreak, unsigned int lossStreak,
                  const wxString& currentStreak);

private:
    void OnOK(wxCommandEvent& event);
    void OnClear(wxCommandEvent& event);
};

/****************************************************************************
 * OptionsDialog
 *
 * Allows the user to set display and play options:
 *   - Display messages on illegal moves
 *   - Quick play (no animation)
 *   - Double click moves card to free cell
 ****************************************************************************/

class OptionsDialog : public wxDialog
{
public:
    OptionsDialog(wxWindow* parent);

    void SetOptions(bool messages, bool fastMode, bool dblClick);
    bool GetMessages() const;
    bool GetFastMode() const;
    bool GetDblClick() const;

private:
    void OnOK(wxCommandEvent& event);
    void OnCancel(wxCommandEvent& event);

    bool m_messages;
    bool m_fastMode;
    bool m_dblClick;
};

#endif // WX_FREECELL_DIALOGS_H
