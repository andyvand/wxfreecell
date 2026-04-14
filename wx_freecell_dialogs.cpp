/****************************************************************************
 * wx_freecell_dialogs.cpp
 *
 * Dialog implementations for wxWidgets FreeCell port.
 * All dialogs load their layout from XRC resources (freecell.xrc).
 *
 * Original: June 91, JimH - initial code; Oct 91, port to Win32
 * wxWidgets port preserves dialog behavior from dialog.c.
 ****************************************************************************/

#include "wx_freecell_dialogs.h"

/* =========================================================================
 * MoveColDialog
 * ========================================================================= */

MoveColDialog::MoveColDialog(wxWindow* parent)
    : m_result(-1)
{
    wxXmlResource::Get()->LoadDialog(this, parent, "MoveColDialog");

    Bind(wxEVT_BUTTON, &MoveColDialog::OnMoveCol, this, XRCID("IDC_MOVECOL"));
    Bind(wxEVT_BUTTON, &MoveColDialog::OnSingle,  this, XRCID("IDC_SINGLE"));
    Bind(wxEVT_BUTTON, &MoveColDialog::OnCancel,   this, XRCID("wxID_CANCEL"));
}

void MoveColDialog::OnMoveCol(wxCommandEvent& WXUNUSED(event))
{
    m_result = 1;       // TRUE -- move column
    EndModal(1);
}

void MoveColDialog::OnSingle(wxCommandEvent& WXUNUSED(event))
{
    m_result = 0;       // FALSE -- move single card
    EndModal(0);
}

void MoveColDialog::OnCancel(wxCommandEvent& WXUNUSED(event))
{
    m_result = -1;      // cancel
    EndModal(-1);
}

/* =========================================================================
 * GameNumDialog
 * ========================================================================= */

GameNumDialog::GameNumDialog(wxWindow* parent)
    : m_gameNum(0)
{
    wxXmlResource::Get()->LoadDialog(this, parent, "GameNumDialog");

    Bind(wxEVT_BUTTON, &GameNumDialog::OnOK,     this, XRCID("wxID_OK"));
    Bind(wxEVT_BUTTON, &GameNumDialog::OnCancel,  this, XRCID("wxID_CANCEL"));
}

void GameNumDialog::SetGameNum(int n)
{
    m_gameNum = n;
    wxTextCtrl* tc = XRCCTRL(*this, "IDC_GAMENUM", wxTextCtrl);
    if (tc)
        tc->SetValue(wxString::Format("%d", n));
}

void GameNumDialog::OnOK(wxCommandEvent& WXUNUSED(event))
{
    wxTextCtrl* tc = XRCCTRL(*this, "IDC_GAMENUM", wxTextCtrl);
    if (tc)
    {
        long val = 0;
        wxString text = tc->GetValue();
        if (text.ToLong(&val))
        {
            m_gameNum = (int)val;

            // negative numbers are special cases -- unwinnable shuffles
            if (m_gameNum == -1 || m_gameNum == -2)
            {
                EndModal(wxID_OK);
                return;
            }

            if (m_gameNum >= 1 && m_gameNum <= MAXGAMENUMBER)
            {
                EndModal(wxID_OK);
                return;
            }
        }

        // invalid number -- set to 0 so caller can detect
        m_gameNum = 0;
        EndModal(wxID_OK);
    }
}

void GameNumDialog::OnCancel(wxCommandEvent& WXUNUSED(event))
{
    m_gameNum = CANCELGAME;
    EndModal(wxID_CANCEL);
}

/* =========================================================================
 * YouWinDialog
 * ========================================================================= */

YouWinDialog::YouWinDialog(wxWindow* parent)
{
    wxXmlResource::Get()->LoadDialog(this, parent, "YouWinDialog");

    Bind(wxEVT_BUTTON, &YouWinDialog::OnYes, this, XRCID("wxID_YES"));
    Bind(wxEVT_BUTTON, &YouWinDialog::OnNo,  this, XRCID("wxID_NO"));
}

bool YouWinDialog::GetSelectGame() const
{
    wxCheckBox* cb = XRCCTRL(*this, "IDC_YWSELECT", wxCheckBox);
    return cb ? cb->GetValue() : false;
}

void YouWinDialog::SetSelectGame(bool b)
{
    wxCheckBox* cb = XRCCTRL(*this, "IDC_YWSELECT", wxCheckBox);
    if (cb)
        cb->SetValue(b);
}

void YouWinDialog::OnYes(wxCommandEvent& WXUNUSED(event))
{
    EndModal(wxID_YES);
}

void YouWinDialog::OnNo(wxCommandEvent& WXUNUSED(event))
{
    EndModal(wxID_NO);
}

/* =========================================================================
 * YouLoseDialog
 * ========================================================================= */

YouLoseDialog::YouLoseDialog(wxWindow* parent)
{
    wxXmlResource::Get()->LoadDialog(this, parent, "YouLoseDialog");

    Bind(wxEVT_BUTTON, &YouLoseDialog::OnYes, this, XRCID("wxID_YES"));
    Bind(wxEVT_BUTTON, &YouLoseDialog::OnNo,  this, XRCID("wxID_NO"));
}

bool YouLoseDialog::GetSameGame() const
{
    wxCheckBox* cb = XRCCTRL(*this, "IDC_YLSAME", wxCheckBox);
    return cb ? cb->GetValue() : true;
}

void YouLoseDialog::OnYes(wxCommandEvent& WXUNUSED(event))
{
    EndModal(wxID_YES);
}

void YouLoseDialog::OnNo(wxCommandEvent& WXUNUSED(event))
{
    EndModal(wxID_NO);
}

/* =========================================================================
 * StatsDialog
 * ========================================================================= */

StatsDialog::StatsDialog(wxWindow* parent)
{
    wxXmlResource::Get()->LoadDialog(this, parent, "StatsDialog");

    Bind(wxEVT_BUTTON, &StatsDialog::OnOK,    this, XRCID("wxID_OK"));
    Bind(wxEVT_BUTTON, &StatsDialog::OnClear,  this, XRCID("IDC_CLEAR"));
}

void StatsDialog::SetStats(unsigned int sessionPct, unsigned int sessionWins,
                            unsigned int sessionLosses,
                            unsigned int totalPct, unsigned int totalWon,
                            unsigned int totalLost,
                            unsigned int winStreak, unsigned int lossStreak,
                            const wxString& currentStreak)
{
    wxStaticText* st1 = XRCCTRL(*this, "IDC_STEXT1", wxStaticText);
    wxStaticText* st2 = XRCCTRL(*this, "IDC_STEXT2", wxStaticText);
    wxStaticText* st3 = XRCCTRL(*this, "IDC_STEXT3", wxStaticText);

    if (st1)
    {
        st1->SetLabel(wxString::Format(
            "This session\t\t\t%u%%\n\twon:\t\t%u\n\tlost:\t\t%u\n",
            sessionPct, sessionWins, sessionLosses));
        st1->InvalidateBestSize();
    }

    if (st2)
    {
        st2->SetLabel(wxString::Format(
            "Total\t\t\t\t%u%%\n\twon:\t\t%u\n\tlost:\t\t%u\n",
            totalPct, totalWon, totalLost));
        st2->InvalidateBestSize();
    }

    if (st3)
    {
        st3->SetLabel(wxString::Format(
            "Streaks\n\twins:\t\t%u\n\tlosses:\t\t%u\n\tcurrent:\t\t%s",
            winStreak, lossStreak, currentStreak));
        st3->InvalidateBestSize();
    }

    Fit();
    Centre();
}

void StatsDialog::OnOK(wxCommandEvent& WXUNUSED(event))
{
    EndModal(wxID_OK);
}

void StatsDialog::OnClear(wxCommandEvent& WXUNUSED(event))
{
    wxMessageDialog confirm(this,
        "Are you sure you want to delete all statistics?",
        "FreeCell",
        wxYES_NO | wxICON_QUESTION);

    if (confirm.ShowModal() == wxID_YES)
    {
        EndModal(CLEAR_STATS);
    }
    // If user said No, stay in the dialog
}

/* =========================================================================
 * OptionsDialog
 * ========================================================================= */

OptionsDialog::OptionsDialog(wxWindow* parent)
    : m_messages(true),
      m_fastMode(false),
      m_dblClick(true)
{
    wxXmlResource::Get()->LoadDialog(this, parent, "OptionsDialog");

    Bind(wxEVT_BUTTON, &OptionsDialog::OnOK,     this, XRCID("wxID_OK"));
    Bind(wxEVT_BUTTON, &OptionsDialog::OnCancel,  this, XRCID("wxID_CANCEL"));
}

void OptionsDialog::SetOptions(bool messages, bool fastMode, bool dblClick)
{
    m_messages = messages;
    m_fastMode = fastMode;
    m_dblClick = dblClick;

    wxCheckBox* cbMsg = XRCCTRL(*this, "IDC_MESSAGES", wxCheckBox);
    wxCheckBox* cbFast = XRCCTRL(*this, "IDC_QUICK", wxCheckBox);
    wxCheckBox* cbDbl = XRCCTRL(*this, "IDC_DBLCLICK", wxCheckBox);

    if (cbMsg)  cbMsg->SetValue(messages);
    if (cbFast) cbFast->SetValue(fastMode);
    if (cbDbl)  cbDbl->SetValue(dblClick);
}

bool OptionsDialog::GetMessages() const
{
    return m_messages;
}

bool OptionsDialog::GetFastMode() const
{
    return m_fastMode;
}

bool OptionsDialog::GetDblClick() const
{
    return m_dblClick;
}

void OptionsDialog::OnOK(wxCommandEvent& WXUNUSED(event))
{
    wxCheckBox* cbMsg = XRCCTRL(*this, "IDC_MESSAGES", wxCheckBox);
    wxCheckBox* cbFast = XRCCTRL(*this, "IDC_QUICK", wxCheckBox);
    wxCheckBox* cbDbl = XRCCTRL(*this, "IDC_DBLCLICK", wxCheckBox);

    if (cbMsg)  m_messages = cbMsg->GetValue();
    if (cbFast) m_fastMode = cbFast->GetValue();
    if (cbDbl)  m_dblClick = cbDbl->GetValue();

    EndModal(wxID_OK);
}

void OptionsDialog::OnCancel(wxCommandEvent& WXUNUSED(event))
{
    EndModal(wxID_CANCEL);
}
