/****************************************************************************
 * wx_freecell_frame.cpp
 *
 * Main frame for wxWidgets FreeCell port.
 * Handles menus, settings persistence (wxConfig), and game flow.
 *
 * Ported from: freecell.c (WinMain, MainWndProc), dialog.c (ReadOptions,
 *              WriteOptions, GetInt, SetInt, StatsDlg, UpdateLossCount)
 ****************************************************************************/

#include "wx_freecell_frame.h"
#include "wx_freecell_panel.h"
#include "wx_freecell_dialogs.h"

#include <wx/config.h>
#include <wx/xrc/xmlres.h>

wxBEGIN_EVENT_TABLE(FreeCellFrame, wxFrame)
    EVT_CLOSE(FreeCellFrame::OnClose)
wxEND_EVENT_TABLE()


FreeCellFrame::FreeCellFrame()
    : wxFrame(nullptr, wxID_ANY, "FreeCell",
              wxDefaultPosition, wxSize(640, 480))
{
    /* ---- Application icon ---- */
#ifdef __WXMSW__
    SetIcon(wxICON(wxfreecell));
#elif !defined(__WXMAC__)
    /* macOS uses the bundle icon from Info.plist */
    SetIcon(wxICON(wxfreecell));
#endif

    /* ---- Menu bar from XRC (already loaded from memory in OnInit) ---- */
    wxMenuBar* mb = wxXmlResource::Get()->LoadMenuBar("MainMenuBar");
    if (mb)
    {
        SetMenuBar(mb);
    }
    else
    {
        /* Fallback: create menu bar programmatically */
        mb = new wxMenuBar;

        wxMenu* gameMenu = new wxMenu;
        gameMenu->Append(IDM_NEWGAME, "&New Game\tF2");
        gameMenu->Append(IDM_SELECT, "&Select Game\tF3");
        gameMenu->Append(IDM_RESTART, "&Restart Game");
        gameMenu->Enable(IDM_RESTART, false);
        gameMenu->AppendSeparator();
        gameMenu->Append(IDM_STATS, "S&tatistics...\tF4");
        gameMenu->Append(IDM_OPTIONS, "&Options...\tF5");
        gameMenu->AppendSeparator();
        gameMenu->Append(IDM_UNDO, "&Undo\tF10");
        gameMenu->Enable(IDM_UNDO, false);
        gameMenu->AppendSeparator();
        gameMenu->Append(IDM_EXIT, "E&xit");

        wxMenu* helpMenu = new wxMenu;
        helpMenu->Append(IDM_ABOUT, "&About FreeCell...");

        mb->Append(gameMenu, "&Game");
        mb->Append(helpMenu, "&Help");
        SetMenuBar(mb);
    }

    /* Bind menu events — IDM_* macros use XRCID() so they match
       both the XRC-loaded and fallback menus identically. */
    Bind(wxEVT_MENU, &FreeCellFrame::OnNewGame,    this, IDM_NEWGAME);
    Bind(wxEVT_MENU, &FreeCellFrame::OnSelectGame,  this, IDM_SELECT);
    Bind(wxEVT_MENU, &FreeCellFrame::OnRestart,     this, IDM_RESTART);
    Bind(wxEVT_MENU, &FreeCellFrame::OnStats,       this, IDM_STATS);
    Bind(wxEVT_MENU, &FreeCellFrame::OnOptions,     this, IDM_OPTIONS);
    Bind(wxEVT_MENU, &FreeCellFrame::OnUndo,        this, IDM_UNDO);
    Bind(wxEVT_MENU, &FreeCellFrame::OnExit,        this, IDM_EXIT);
    Bind(wxEVT_MENU, &FreeCellFrame::OnAbout,       this, IDM_ABOUT);
    Bind(wxEVT_MENU, &FreeCellFrame::OnCheat,       this, IDM_CHEAT);

    /* ---- Accelerator table ---- */
    wxAcceleratorEntry accel[6];
    accel[0].Set(wxACCEL_NORMAL, WXK_F2, IDM_NEWGAME);
    accel[1].Set(wxACCEL_NORMAL, WXK_F3, IDM_SELECT);
    accel[2].Set(wxACCEL_NORMAL, WXK_F4, IDM_STATS);
    accel[3].Set(wxACCEL_NORMAL, WXK_F5, IDM_OPTIONS);
    accel[4].Set(wxACCEL_NORMAL, WXK_F10, IDM_UNDO);
    accel[5].Set(wxACCEL_CTRL | wxACCEL_SHIFT, WXK_F10, IDM_CHEAT);
    wxAcceleratorTable accelTable(6, accel);
    SetAcceleratorTable(accelTable);

    /* ---- Status bar for card count ---- */
    CreateStatusBar();

    /* ---- Game panel ---- */
    m_panel = new FreeCellPanel(this);

    /* Load card bitmaps from XRC resources (embedded in binary) */
    if (!m_panel->InitCards())
    {
        wxMessageBox("Failed to load card bitmaps from XRC resources.",
                     "FreeCell - Error", wxOK | wxICON_ERROR);
    }

    /* ---- Read options from config ---- */
    ReadOptions();

    /* ---- Start the first game ---- */
    FreeCellGame& game = m_panel->GetGame();
    int seed = game.GenerateRandomGameNum();
    m_panel->StartNewGame(seed);
}


/* ====================================================================
   Menu Handlers
   ==================================================================== */

void FreeCellFrame::OnNewGame(wxCommandEvent& WXUNUSED(event))
{
    FreeCellGame& game = m_panel->GetGame();

    if (game.bGameInProgress)
    {
        int resp = wxMessageBox("Do you want to resign this game?",
                                "FreeCell", wxYES_NO | wxICON_QUESTION, this);
        if (resp == wxNO)
            return;
        SaveLossStats();
    }

    game.bSelecting = false;
    int seed = game.GenerateRandomGameNum();
    m_panel->StartNewGame(seed);
}

void FreeCellFrame::OnSelectGame(wxCommandEvent& WXUNUSED(event))
{
    FreeCellGame& game = m_panel->GetGame();

    if (game.bGameInProgress)
    {
        int resp = wxMessageBox("Do you want to resign this game?",
                                "FreeCell", wxYES_NO | wxICON_QUESTION, this);
        if (resp == wxNO)
            return;
        SaveLossStats();
    }

    game.bSelecting = true;
    m_panel->StartNewGame(0);  // 0 = show dialog for game number
}

void FreeCellFrame::OnRestart(wxCommandEvent& event)
{
    FreeCellGame& game = m_panel->GetGame();

    if (game.bGameInProgress)
    {
        int resp = wxMessageBox("Do you want to resign this game?",
                                "FreeCell", wxYES_NO | wxICON_QUESTION, this);
        if (resp == wxNO)
            return;
        SaveLossStats();
    }

    int num = game.bGameInProgress ? game.gamenumber : game.oldgamenumber;
    if (event.GetInt() != 0)
        num = event.GetInt();

    m_panel->StartNewGame(num);
}

void FreeCellFrame::OnStats(wxCommandEvent& WXUNUSED(event))
{
    FreeCellGame& game = m_panel->GetGame();

    wxConfigBase* cfg = wxConfigBase::Get();

    unsigned int cTWon    = cfg->ReadLong("/FreeCell/won", 0);
    unsigned int cTLost   = cfg->ReadLong("/FreeCell/lost", 0);
    unsigned int cTWins   = cfg->ReadLong("/FreeCell/wins", 0);
    unsigned int cTLosses = cfg->ReadLong("/FreeCell/losses", 0);
    unsigned int wStreak  = cfg->ReadLong("/FreeCell/streak", 0);
    unsigned int wSType   = cfg->ReadLong("/FreeCell/stype", 0);

    unsigned int sessionPct = CalcPercentage(game.cWins, game.cLosses);
    unsigned int totalPct   = CalcPercentage(cTWon, cTLost);

    wxString currentStreak;
    if (wStreak == 0)
        currentStreak = "0";
    else if (wStreak == 1)
        currentStreak = (wSType == WON) ? "1 win" : "1 loss";
    else
        currentStreak = wxString::Format("%u %s", wStreak,
                            (wSType == WON) ? "wins" : "losses");

    StatsDialog dlg(this);
    dlg.SetStats(sessionPct, game.cWins, game.cLosses,
                 totalPct, cTWon, cTLost,
                 cTWins, cTLosses, currentStreak);

    int result = dlg.ShowModal();
    if (result == StatsDialog::CLEAR_STATS)
    {
        cfg->DeleteEntry("/FreeCell/won");
        cfg->DeleteEntry("/FreeCell/lost");
        cfg->DeleteEntry("/FreeCell/wins");
        cfg->DeleteEntry("/FreeCell/losses");
        cfg->DeleteEntry("/FreeCell/streak");
        cfg->DeleteEntry("/FreeCell/stype");
        cfg->Flush();

        game.cWins = 0;
        game.cLosses = 0;
    }
}

void FreeCellFrame::OnOptions(wxCommandEvent& WXUNUSED(event))
{
    FreeCellGame& game = m_panel->GetGame();

    OptionsDialog dlg(this);
    dlg.SetOptions(game.bMessages, game.bFastMode, game.bDblClick);

    if (dlg.ShowModal() == wxID_OK)
    {
        game.bMessages = dlg.GetMessages();
        game.bFastMode = dlg.GetFastMode();
        game.bDblClick = dlg.GetDblClick();
    }
}

void FreeCellFrame::OnUndo(wxCommandEvent& WXUNUSED(event))
{
    m_panel->DoUndo();
}

void FreeCellFrame::OnExit(wxCommandEvent& WXUNUSED(event))
{
    Close(false);
}

void FreeCellFrame::OnAbout(wxCommandEvent& WXUNUSED(event))
{
    wxMessageBox("FreeCell\n\n"
                 "Originally by Jim Horne (June 1991)\n"
                 "Ported to wxWidgets for cross-platform play.\n\n"
                 "Game numbers produce identical layouts\n"
                 "across all platforms.",
                 "About FreeCell",
                 wxOK | wxICON_INFORMATION, this);
}

void FreeCellFrame::OnCheat(wxCommandEvent& WXUNUSED(event))
{
    FreeCellGame& game = m_panel->GetGame();

    int resp = wxMessageBox(
        "Choose Yes to Win,\nNo to Lose,\nor Cancel to Cancel.",
        "User-Friendly User Interface",
        wxYES_NO | wxCANCEL | wxICON_QUESTION, this);

    if (resp == wxYES)
        game.bCheating = true;  // CHEAT_WIN
    else if (resp == wxNO)
        game.bCheating = true;  // CHEAT_LOSE
    else
        game.bCheating = false;
}

void FreeCellFrame::OnClose(wxCloseEvent& event)
{
    FreeCellGame& game = m_panel->GetGame();

    if (game.bGameInProgress)
    {
        int resp = wxMessageBox("Do you want to resign this game?",
                                "FreeCell", wxYES_NO | wxICON_QUESTION, this);
        if (resp == wxNO && event.CanVeto())
        {
            event.Veto();
            return;
        }
        SaveLossStats();
    }

    WriteOptions();
    Destroy();
}


/* ====================================================================
   Settings Persistence
   ==================================================================== */

void FreeCellFrame::ReadOptions()
{
    wxConfigBase* cfg = wxConfigBase::Get();
    FreeCellGame& game = m_panel->GetGame();

    game.bMessages = cfg->ReadBool("/FreeCell/messages", true);
    game.bFastMode = cfg->ReadBool("/FreeCell/quick", false);
    game.bDblClick = cfg->ReadBool("/FreeCell/dblclick", true);
}

void FreeCellFrame::WriteOptions()
{
    wxConfigBase* cfg = wxConfigBase::Get();
    FreeCellGame& game = m_panel->GetGame();

    cfg->Write("/FreeCell/messages", game.bMessages);
    cfg->Write("/FreeCell/quick", game.bFastMode);
    cfg->Write("/FreeCell/dblclick", game.bDblClick);
    cfg->Flush();
}


/* ====================================================================
   Statistics Persistence
   ==================================================================== */

void FreeCellFrame::SaveWinStats()
{
    FreeCellGame& game = m_panel->GetGame();

    if (game.gamenumber <= 0 || game.gamenumber == game.oldgamenumber)
        return;

    wxConfigBase* cfg = wxConfigBase::Get();

    long cTWon = cfg->ReadLong("/FreeCell/won", 0);
    cTWon++;
    cfg->Write("/FreeCell/won", cTWon);

    long wSType = cfg->ReadLong("/FreeCell/stype", LOST);
    long wStreak;
    if (wSType == LOST)
    {
        cfg->Write("/FreeCell/stype", (long)WON);
        wStreak = 1;
        cfg->Write("/FreeCell/streak", wStreak);
    }
    else
    {
        wStreak = cfg->ReadLong("/FreeCell/streak", 0);
        wStreak++;
        cfg->Write("/FreeCell/streak", wStreak);
    }

    long wWins = cfg->ReadLong("/FreeCell/wins", 0);
    if (wWins < wStreak)
    {
        wWins = wStreak;
        cfg->Write("/FreeCell/wins", wWins);
    }

    cfg->Flush();
}

void FreeCellFrame::SaveLossStats()
{
    FreeCellGame& game = m_panel->GetGame();

    if (game.gamenumber <= 0 || game.gamenumber == game.oldgamenumber)
        return;

    wxConfigBase* cfg = wxConfigBase::Get();

    long cTLost = cfg->ReadLong("/FreeCell/lost", 0);
    cTLost++;
    game.cLosses++;
    game.cGames++;
    cfg->Write("/FreeCell/lost", cTLost);

    long wSType = cfg->ReadLong("/FreeCell/stype", WON);
    long wStreak;
    if (wSType == WON)
    {
        cfg->Write("/FreeCell/stype", (long)LOST);
        wStreak = 1;
        cfg->Write("/FreeCell/streak", wStreak);
    }
    else
    {
        wStreak = cfg->ReadLong("/FreeCell/streak", 0);
        wStreak++;
        cfg->Write("/FreeCell/streak", wStreak);
    }

    long wLosses = cfg->ReadLong("/FreeCell/losses", 0);
    if (wLosses < wStreak)
    {
        wLosses = wStreak;
        cfg->Write("/FreeCell/losses", wLosses);
    }

    cfg->Flush();
    game.oldgamenumber = game.gamenumber;
}

unsigned int FreeCellFrame::CalcPercentage(unsigned int cWins,
                                           unsigned int cLosses)
{
    unsigned int wPct = 0;
    unsigned int lDenom = cWins + cLosses;

    if (lDenom != 0)
        wPct = (((cWins * 200) + lDenom) / (2 * lDenom));

    if (wPct >= 100 && cLosses != 0)
        wPct = 99;

    return wPct;
}
