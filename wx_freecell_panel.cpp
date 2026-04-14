/****************************************************************************
 * wx_freecell_panel.cpp
 *
 * Game board panel for wxWidgets FreeCell port.
 * Handles card rendering, mouse interaction, and animation.
 *
 * Ported from: freecell.c (MainWndProc), display.c, glide.c, game.c,
 *              game2.c, transfer.c
 ****************************************************************************/

#include "wx_freecell_panel.h"
#include "wx_freecell_frame.h"
#include "wx_freecell_dialogs.h"

#include <wx/dcbuffer.h>
#include <cmath>
#include <cstring>

#define BGND_RGB    wxColour(255, 255, 255)
#define STEPSIZE    37

#define ICONY       ((int)(m_dyCrd - ICONHEIGHT) / 3)

wxBEGIN_EVENT_TABLE(FreeCellPanel, wxPanel)
    EVT_PAINT(FreeCellPanel::OnPaint)
    EVT_SIZE(FreeCellPanel::OnSize)
    EVT_LEFT_DOWN(FreeCellPanel::OnLeftDown)
    EVT_LEFT_DCLICK(FreeCellPanel::OnLeftDClick)
    EVT_RIGHT_DOWN(FreeCellPanel::OnRightDown)
    EVT_RIGHT_UP(FreeCellPanel::OnRightUp)
    EVT_MOTION(FreeCellPanel::OnMouseMove)
    EVT_CHAR(FreeCellPanel::OnKeyChar)
    EVT_TIMER(FLASH_TIMER_ID, FreeCellPanel::OnTimer)
    EVT_TIMER(FLIP_TIMER_ID, FreeCellPanel::OnTimer)
wxEND_EVENT_TABLE()


FreeCellPanel::FreeCellPanel(FreeCellFrame* parent)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
              wxFULL_REPAINT_ON_RESIZE | wxWANTS_CHARS)
    , m_frame(parent)
    , m_wIconOffset(0)
    , m_wVSpace(10)
    , m_dyTops(0)
    , m_dxCrd(0)
    , m_dyCrd(0)
    , m_wUpdateCol(0)
    , m_wUpdatePos(0)
    , m_bCardRevealed(false)
    , m_kingState(RIGHT)
    , m_flashTimer(this, FLASH_TIMER_ID)
    , m_flipTimer(this, FLIP_TIMER_ID)
    , m_cFlashes(0)
    , m_flipPos(0)
    , m_bEatNextMouseHit(false)
    , m_bFlipping(false)
    , m_bgndColour(0, 127, 0)
    , m_bgndBrush(wxColour(0, 127, 0))
    , m_brightPen(wxColour(0, 255, 0), 1)
{
    std::memset(m_wOffset, 0, sizeof(m_wOffset));
    SetBackgroundColour(m_bgndColour);
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    m_game.Init();
}

FreeCellPanel::~FreeCellPanel()
{
    m_flashTimer.Stop();
    m_flipTimer.Stop();
}

bool FreeCellPanel::InitCards()
{
    if (!m_cards.Init())
        return false;

    m_dxCrd = m_cards.GetCardWidth();
    m_dyCrd = m_cards.GetCardHeight();

    /* Create offscreen bitmaps for glide animation */
    m_bmFgnd  = wxBitmap(m_dxCrd, m_dyCrd);
    m_bmBgnd1 = wxBitmap(m_dxCrd, m_dyCrd);
    m_bmBgnd2 = wxBitmap(m_dxCrd, m_dyCrd);

    CalcOffsets();
    return true;
}

void FreeCellPanel::EnsureBackBuffer()
{
    wxSize sz = GetClientSize();
    if (sz.GetWidth() <= 0 || sz.GetHeight() <= 0)
        return;
    if (!m_backBuffer.IsOk()
        || m_backBuffer.GetWidth() != sz.GetWidth()
        || m_backBuffer.GetHeight() != sz.GetHeight())
    {
        m_backBuffer = wxBitmap(sz.GetWidth(), sz.GetHeight());
    }
}

void FreeCellPanel::FlushBuffer(const wxRect* rect)
{
    if (rect)
        RefreshRect(*rect, false);
    else
        Refresh(false);
    Update();
}

/* ====================================================================
   Layout
   ==================================================================== */

void FreeCellPanel::CalcOffsets()
{
    wxSize sz = GetClientSize();
    unsigned int width = sz.GetWidth();
    unsigned int i;
    unsigned int leftedge;

    m_wOffset[TOPROW] = width - (4 * m_dxCrd);

    leftedge = (width - ((MAXCOL - 1) * m_dxCrd)) / MAXCOL;

    for (i = 1; i < MAXCOL; i++)
        m_wOffset[i] = leftedge + (((i - 1) * (width - leftedge)) / (MAXCOL - 1));

    m_wIconOffset = (width - ICONWIDTH) / 2;
    m_wVSpace = 10;
    m_dyTops = (m_dyCrd * 9) / 46;
}

void FreeCellPanel::Card2Point(unsigned int col, unsigned int pos,
                               unsigned int& x, unsigned int& y)
{
    if (col == TOPROW)
    {
        y = 0;
        x = pos * m_dxCrd;
        if (pos > 3)
            x += m_wOffset[TOPROW] - (4 * m_dxCrd);
    }
    else
    {
        x = m_wOffset[col];
        y = m_dyCrd + m_wVSpace + (pos * m_dyTops);
    }
}

bool FreeCellPanel::Point2Card(unsigned int x, unsigned int y,
                               unsigned int& col, unsigned int& pos)
{
    if (y < m_dyCrd)    // TOPROW
    {
        if (x < (unsigned int)(4 * m_dxCrd))
        {
            col = TOPROW;
            pos = x / m_dxCrd;
            return true;
        }
        else if (x < m_wOffset[TOPROW])
            return false;

        unsigned int tx = x - m_wOffset[TOPROW];
        if (tx < (unsigned int)(4 * m_dxCrd))
        {
            col = TOPROW;
            pos = (tx / m_dxCrd) + 4;
            return true;
        }
        else
            return false;
    }

    if (y < (m_dyCrd + m_wVSpace))
        return false;

    if (x < m_wOffset[1])
        return false;

    col = (x - m_wOffset[1]) / (m_wOffset[2] - m_wOffset[1]);
    col++;

    if (col >= MAXCOL)
        return false;

    if (x > (m_wOffset[col] + m_dxCrd))
        return false;

    y -= (m_dyCrd + m_wVSpace);

    pos = y / m_dyTops;
    if (pos >= MAXPOS) pos = MAXPOS - 1;

    if (m_game.card[col][0] == EMPTY)
        return false;

    if (pos < (MAXPOS - 1))
    {
        if (m_game.card[col][pos + 1] != EMPTY)
            return true;
    }

    while (m_game.card[col][pos] == EMPTY && pos > 0)
        pos--;

    if (m_game.card[col][pos] == EMPTY)
        return false;

    if (y > (pos * m_dyTops + m_dyCrd))
        return false;

    return true;
}

/* ====================================================================
   Rendering
   ==================================================================== */

void FreeCellPanel::DrawCard(wxDC& dc, unsigned int col, unsigned int pos,
                             CARD c, int mode)
{
    unsigned int x, y;
    Card2Point(col, pos, x, y);

    if (c == IDGHOST && m_cards.GetGhostBitmap().IsOk())
    {
        wxMemoryDC memDC;
        memDC.SelectObject(m_cards.GetGhostBitmap());
        dc.Blit(x, y, m_dxCrd, m_dyCrd, &memDC, 0, 0, wxCOPY);
        memDC.SelectObject(wxNullBitmap);
    }
    else
    {
        m_cards.DrawCard(dc, x, y, m_dxCrd, m_dyCrd, c, mode, BGND_RGB);
    }
}

void FreeCellPanel::DrawCardMem(wxDC& dc, CARD c, int mode)
{
    m_cards.DrawCard(dc, 0, -(int)m_dyTops, m_dxCrd, m_dyCrd, c, mode, BGND_RGB);
}

void FreeCellPanel::DrawKing(wxDC& dc, unsigned int state, bool bDraw)
{
    if (state == m_kingState)
        return;

    if (state == SAME)
        state = m_kingState;

    if (!bDraw)
    {
        m_kingState = state;
        return;
    }

    if (state == NONE)
    {
        dc.SetBrush(m_bgndBrush);
        dc.SetPen(wxPen(m_bgndColour));
        dc.DrawRectangle(m_wIconOffset, ICONY, BMWIDTH, BMHEIGHT);
    }
    else
    {
        wxBitmap& bmp = m_cards.GetKingBitmap(state);
        if (bmp.IsOk())
        {
            wxMemoryDC memDC;
            memDC.SelectObject(bmp);
            dc.Blit(m_wIconOffset, ICONY, BMWIDTH, BMHEIGHT,
                    &memDC, 0, 0, wxCOPY);
            memDC.SelectObject(wxNullBitmap);
        }
    }

    m_kingState = state;
}

void FreeCellPanel::Payoff(wxDC& dc)
{
    DrawKing(dc, NONE, true);

    wxBitmap& bmp = m_cards.GetKingBitmap(SMILE);
    if (bmp.IsOk())
    {
        wxMemoryDC memDC;
        memDC.SelectObject(bmp);
        dc.StretchBlit(10, m_dyCrd + 10, 320, 320,
                       &memDC, 0, 0, BMWIDTH, BMHEIGHT);
        memDC.SelectObject(wxNullBitmap);
    }
}

void FreeCellPanel::OnPaint(wxPaintEvent& WXUNUSED(event))
{
    wxAutoBufferedPaintDC dc(this);
    EnsureBackBuffer();
    if (m_backBuffer.IsOk())
    {
        wxMemoryDC memDC;
        memDC.SelectObject(m_backBuffer);
        PaintMainWindow(memDC);
        memDC.SelectObject(wxNullBitmap);
        dc.DrawBitmap(m_backBuffer, 0, 0);
    }
    else
    {
        PaintMainWindow(dc);
    }
}

void FreeCellPanel::PaintMainWindow(wxDC& dc)
{
    wxSize sz = GetClientSize();
    dc.SetBackground(m_bgndBrush);
    dc.Clear();

    if (m_dxCrd == 0)
        return;

    /* Draw icon area with 3D box */
    unsigned int iconY = ICONY;

    dc.SetPen(m_brightPen);
    dc.DrawLine(m_wIconOffset - 3, iconY + ICONHEIGHT + 1,
                m_wIconOffset - 3, iconY - 3);
    dc.DrawLine(m_wIconOffset - 3, iconY - 3,
                m_wIconOffset + ICONWIDTH + 2, iconY - 3);

    dc.SetPen(*wxBLACK_PEN);
    dc.DrawLine(m_wIconOffset + ICONWIDTH + 2, iconY - 2,
                m_wIconOffset + ICONWIDTH + 2, iconY + ICONHEIGHT + 2);
    dc.DrawLine(m_wIconOffset + ICONWIDTH + 2, iconY + ICONHEIGHT + 2,
                m_wIconOffset - 3, iconY + ICONHEIGHT + 2);

    DrawKing(dc, m_kingState == NONE ? RIGHT : SAME, true);

    /* Top row */
    for (unsigned int pos = 0; pos < 8; pos++)
    {
        int mode = FACEUP;
        CARD c = m_game.card[TOPROW][pos];
        if (c == EMPTY)
            c = IDGHOST;
        else if (m_game.wMouseMode == TO && pos == m_game.wFromPos
                 && TOPROW == m_game.wFromCol)
            mode = HILITE;

        DrawCard(dc, TOPROW, pos, c, mode);
    }

    /* Columns 1-8 */
    for (unsigned int col = 1; col < MAXCOL; col++)
    {
        for (unsigned int pos = 0; pos < MAXPOS; pos++)
        {
            CARD c = m_game.card[col][pos];
            if (c == EMPTY)
                break;

            int mode = FACEUP;
            if (m_game.wMouseMode == TO && pos == m_game.wFromPos
                && col == m_game.wFromCol)
                mode = HILITE;

            DrawCard(dc, col, pos, c, mode);
        }
    }

    if (m_game.wCardCount == 0 && m_game.gamenumber != 0
        && !m_game.bGameInProgress)
        Payoff(dc);
}

void FreeCellPanel::OnSize(wxSizeEvent& event)
{
    CalcOffsets();
    UpdateCardCount();
    Refresh();
    event.Skip();
}

void FreeCellPanel::UpdateCardCount()
{
    if (m_frame)
    {
        unsigned int count = m_game.wCardCount;
        if (count == 0xFFFF) count = 0;
        m_frame->SetStatusText(wxString::Format("Cards Left: %u", count));
    }
}

/* ====================================================================
   New Game
   ==================================================================== */

void FreeCellPanel::StartNewGame(int seed)
{
    m_game.bGameInProgress = false;
    m_game.wFromCol = EMPTY;
    m_game.wMouseMode = FROM;
    m_game.moveindex = 0;
    for (int i = 0; i < 4; i++)
    {
        m_game.homesuit[i] = EMPTY;
        m_game.home[i] = EMPTY;
    }

    if (seed == 0)
    {
        /* Show game number dialog */
        m_game.gamenumber = m_game.GenerateRandomGameNum();

        for (;;)
        {
            GameNumDialog dlg(this);
            dlg.SetGameNum(m_game.gamenumber);
            if (dlg.ShowModal() == wxID_OK)
            {
                m_game.gamenumber = dlg.GetGameNum();
                if (m_game.gamenumber != 0)
                    break;
            }
            else
            {
                m_game.gamenumber = CANCELGAME;
                break;
            }
        }

        if (m_game.gamenumber == CANCELGAME)
            return;

        m_game.ShuffleDeck((unsigned int)m_game.gamenumber);
    }
    else
    {
        m_game.ShuffleDeck((unsigned int)seed);
    }

    m_game.wCardCount = 52;
    m_game.bGameInProgress = true;
    m_game.cUndo = 0;

    /* Update window title */
    if (m_frame)
    {
        m_frame->SetTitle(wxString::Format("FreeCell Game #%d",
                                           m_game.gamenumber));
        m_frame->GetMenuBar()->Enable(IDM_RESTART, true);
        m_frame->GetMenuBar()->Enable(IDM_UNDO, false);
    }

    UpdateCardCount();
    m_kingState = NONE;  // Force redraw
    Refresh();
}

/* ====================================================================
   Mouse Handlers
   ==================================================================== */

void FreeCellPanel::OnLeftDown(wxMouseEvent& event)
{
    if (m_bEatNextMouseHit)
    {
        m_bEatNextMouseHit = false;
        return;
    }

    if (m_bFlipping)
        return;
    if (m_game.moveindex != 0)
        return;
    if (m_game.gamenumber == 0)
        return;

    unsigned int mx = (unsigned int)event.GetX();
    unsigned int my = (unsigned int)event.GetY();

    if (m_game.wMouseMode == FROM)
        SetFromLoc(mx, my);
    else
        ProcessMoveRequest(mx, my);
}

void FreeCellPanel::OnLeftDClick(wxMouseEvent& event)
{
    if (m_game.moveindex != 0 || m_game.gamenumber == 0 || m_bFlipping)
        return;

    unsigned int mx = (unsigned int)event.GetX();
    unsigned int my = (unsigned int)event.GetY();

    if (m_game.bDblClick && m_game.wFromCol > TOPROW
        && m_game.wFromCol < MAXCOL)
    {
        if (m_game.wMouseMode == TO)
        {
            unsigned int col, pos;
            Point2Card(mx, my, col, pos);
            if (col == m_game.wFromCol)
            {
                if (ProcessDoubleClick())
                    return;
            }
        }
    }

    /* Fall through to single click */
    OnLeftDown(event);
}

void FreeCellPanel::OnRightDown(wxMouseEvent& event)
{
    CaptureMouse();
    if (m_bFlipping)
        return;
    if (m_game.gamenumber != 0)
        RevealCard((unsigned int)event.GetX(), (unsigned int)event.GetY());
}

void FreeCellPanel::OnRightUp(wxMouseEvent& WXUNUSED(event))
{
    if (HasCapture())
        ReleaseMouse();
    RestoreColumn();
}

void FreeCellPanel::OnMouseMove(wxMouseEvent& event)
{
    SetCursorShape((unsigned int)event.GetX(), (unsigned int)event.GetY());
}

void FreeCellPanel::OnKeyChar(wxKeyEvent& event)
{
    if (!m_bFlipping)
        KeyboardInput((unsigned int)event.GetKeyCode());
    event.Skip();
}

/* ====================================================================
   Timer
   ==================================================================== */

void FreeCellPanel::OnTimer(wxTimerEvent& event)
{
    if (event.GetId() == FLASH_TIMER_ID)
        Flash();
    else if (event.GetId() == FLIP_TIMER_ID)
        Flip();
}

void FreeCellPanel::Flash()
{
    m_cFlashes--;
    if (m_cFlashes <= 0)
    {
        m_flashTimer.Stop();
    }
}

void FreeCellPanel::Flip()
{
    EnsureBackBuffer();
    wxMemoryDC dc;
    dc.SelectObject(m_backBuffer);
    DrawCard(dc, m_game.wFromCol, m_flipPos,
             m_game.card[m_game.wFromCol][m_flipPos], FACEUP);
    dc.SelectObject(wxNullBitmap);
    FlushBuffer();
    m_flipPos++;

    if (m_game.card[m_game.wFromCol][m_flipPos] == EMPTY)
    {
        m_flipPos = 0;
        m_flipTimer.Stop();
        m_bFlipping = false;

        /* Cancel move */
        unsigned int x, y;
        Card2Point(m_game.wFromCol, 0, x, y);

        wxMouseEvent fakeEvent(wxEVT_LEFT_DOWN);
        fakeEvent.SetX(x);
        fakeEvent.SetY(y);
        wxPostEvent(this, fakeEvent);
    }
}

/* ====================================================================
   Game Actions
   ==================================================================== */

void FreeCellPanel::SetFromLoc(unsigned int x, unsigned int y)
{
    unsigned int col, pos;

    m_game.wFromCol = EMPTY;
    m_game.wFromPos = EMPTY;
    m_game.cUndo = 0;

    if (m_frame)
        m_frame->GetMenuBar()->Enable(IDM_UNDO, false);

    if (Point2Card(x, y, col, pos))
    {
        if (col == TOPROW)
        {
            if (m_game.card[TOPROW][pos] == EMPTY || pos > 3)
                return;
        }
        else
        {
            pos = m_game.FindLastPos(col);
            if (pos == EMPTY)
                return;
        }
    }
    else
        return;

    m_game.wFromCol = col;
    m_game.wFromPos = pos;
    m_game.wMouseMode = TO;

    EnsureBackBuffer();
    wxMemoryDC dc;
    dc.SelectObject(m_backBuffer);
    DrawCard(dc, col, pos, m_game.card[col][pos], HILITE);
    if (col == TOPROW && pos < 4)
        DrawKing(dc, LEFT, true);
    dc.SelectObject(wxNullBitmap);
    FlushBuffer();
}

void FreeCellPanel::ProcessMoveRequest(unsigned int x, unsigned int y)
{
    unsigned int tcol, tpos;
    unsigned int freecells;
    unsigned int trans;
    unsigned int maxtrans;

    m_game.SaveState();
    Point2Card(x, y, tcol, tpos);

    if (tcol >= MAXCOL)
    {
        tpos = m_game.wFromPos;
        tcol = m_game.wFromCol;
    }

    if (tcol == TOPROW)
    {
        if (tpos > 7)
        {
            tpos = m_game.wFromPos;
            tcol = m_game.wFromCol;
        }
    }
    else
    {
        tpos = m_game.FindLastPos(tcol);
        if (tpos == EMPTY)
            tpos = 0;
    }

    /* Moving between non-empty columns */
    if (m_game.wFromCol != TOPROW && tcol != TOPROW
        && m_game.card[tcol][0] != EMPTY)
    {
        freecells = 0;
        for (int i = 0; i < 4; i++)
            if (m_game.card[TOPROW][i] == EMPTY)
                freecells++;

        trans = m_game.NumberToTransfer(m_game.wFromCol, tcol);

        if (trans > 0)
        {
            maxtrans = m_game.MaxTransfer();
            if (trans <= maxtrans)
            {
                m_game.MultiMove(m_game.wFromCol, tcol);
            }
            else if (m_game.bMessages)
            {
                wxMessageBox(
                    wxString::Format(
                        "That move requires moving %u cards. "
                        "You only have enough free space to move %u.",
                        trans, maxtrans),
                    "FreeCell", wxOK | wxICON_INFORMATION, this);
                m_game.QueueTransfer(m_game.wFromCol, m_game.wFromPos,
                                     m_game.wFromCol, m_game.wFromPos);
            }
            else
                return;
        }
        else
        {
            if (m_game.bMessages)
            {
                wxMessageBox("That move is not allowed.", "FreeCell",
                             wxOK | wxICON_INFORMATION, this);
                m_game.QueueTransfer(m_game.wFromCol, m_game.wFromPos,
                                     m_game.wFromCol, m_game.wFromPos);
            }
            else
                return;
        }
    }
    else  /* Move involves TOPROW or to empty column */
    {
        m_game.bMoveCol = false;
        bool valid = m_game.IsValidMove(tcol, tpos);

        /* If disambiguation needed (bMoveCol == true after IsValidMove) */
        if (valid && m_game.bMoveCol)
        {
            MoveColDialog dlg(this);
            int result = dlg.ShowModal();
            if (result == -1)
            {
                m_game.QueueTransfer(m_game.wFromCol, m_game.wFromPos,
                                     m_game.wFromCol, m_game.wFromPos);
                valid = false;
            }
            else
            {
                m_game.bMoveCol = (result == 1);
            }
        }

        if (valid)
        {
            if (m_game.bMoveCol)
                m_game.MoveCol(m_game.wFromCol, tcol);
            else
                m_game.QueueTransfer(m_game.wFromCol, m_game.wFromPos,
                                     tcol, tpos);
        }
        else
        {
            if (m_game.bMessages)
            {
                wxMessageBox("That move is not allowed.", "FreeCell",
                             wxOK | wxICON_INFORMATION, this);
                m_game.QueueTransfer(m_game.wFromCol, m_game.wFromPos,
                                     m_game.wFromCol, m_game.wFromPos);
            }
            else
                return;
        }
    }

    m_game.Cleanup();
    MoveCards();
    m_game.wMouseMode = FROM;
}

bool FreeCellPanel::ProcessDoubleClick()
{
    if (m_game.ProcessDoubleClick())
    {
        MoveCards();
        return true;
    }
    return false;
}

/* ====================================================================
   Transfer / MoveCards
   ==================================================================== */

void FreeCellPanel::Transfer(unsigned int fcol, unsigned int fpos,
                             unsigned int tcol, unsigned int tpos)
{
    CARD c;

    if (fcol == TOPROW)
    {
        if ((fpos > 3) || (m_game.card[TOPROW][fpos] == IDGHOST))
            return;
    }
    else
    {
        fpos = m_game.FindLastPos(fcol);
        if (fpos == EMPTY)
            return;

        if (fcol == tcol)
        {
            EnsureBackBuffer();
            wxMemoryDC dc;
            dc.SelectObject(m_backBuffer);
            DrawCard(dc, fcol, fpos, m_game.card[fcol][fpos], FACEUP);
            dc.SelectObject(wxNullBitmap);
            FlushBuffer();
            return;
        }
    }

    if (tcol == TOPROW)
    {
        if (tpos > 3)
        {
            m_game.wCardCount--;
            UpdateCardCount();
            c = m_game.card[fcol][fpos];
            m_game.home[SUIT(c)] = VALUE(c);
        }
    }
    else
    {
        tpos = m_game.FindLastPos(tcol) + 1;
    }

    Glide(fcol, fpos, tcol, tpos);

    c = m_game.card[fcol][fpos];
    m_game.card[fcol][fpos] = EMPTY;
    m_game.card[tcol][tpos] = c;

    if (VALUE(c) == ACE && tcol == TOPROW && tpos > 3)
        m_game.homesuit[SUIT(c)] = tpos;

    if (tcol == TOPROW)
    {
        EnsureBackBuffer();
        wxMemoryDC dc;
        dc.SelectObject(m_backBuffer);
        DrawKing(dc, tpos < 4 ? LEFT : RIGHT, true);
        dc.SelectObject(wxNullBitmap);
        FlushBuffer();
    }
}

void FreeCellPanel::MoveCards()
{
    if (m_game.moveindex == 0)
        return;

    m_game.RestoreState();

    SetCursor(wxCursor(wxCURSOR_WAIT));

    for (unsigned int i = 0; i < m_game.moveindex; i++)
        Transfer(m_game.movelist[i].fcol, m_game.movelist[i].fpos,
                 m_game.movelist[i].tcol, m_game.movelist[i].tpos);

    if (m_frame)
    {
        if ((m_game.moveindex > 1)
            || (m_game.movelist[0].fcol != m_game.movelist[0].tcol))
        {
            m_game.cUndo = m_game.moveindex;
            m_frame->GetMenuBar()->Enable(IDM_UNDO, true);
        }
        else
        {
            m_game.cUndo = 0;
            m_frame->GetMenuBar()->Enable(IDM_UNDO, false);
        }
    }

    m_game.moveindex = 0;
    SetCursor(wxCursor(wxCURSOR_ARROW));

    if (m_game.wCardCount == 0)
    {
        /* Game won */
        m_game.cUndo = 0;
        if (m_frame)
            m_frame->GetMenuBar()->Enable(IDM_UNDO, false);

        m_game.bGameInProgress = false;
        m_game.bCheating = false;

        if (m_game.gamenumber != m_game.oldgamenumber
            && m_game.gamenumber > 0)
        {
            m_game.cWins++;
            m_game.cGames++;
            m_frame->SaveWinStats();
        }

        {
            EnsureBackBuffer();
            wxMemoryDC dc;
            dc.SelectObject(m_backBuffer);
            Payoff(dc);
            dc.SelectObject(wxNullBitmap);
            FlushBuffer();
        }

        YouWinDialog dlg(this);
        dlg.SetSelectGame(m_game.bSelecting);
        int response = dlg.ShowModal();

        m_game.bSelecting = dlg.GetSelectGame();

        if (response == wxID_YES)
        {
            wxCommandEvent evt(wxEVT_MENU,
                m_game.bSelecting ? IDM_SELECT : IDM_NEWGAME);
            wxPostEvent(m_frame, evt);
        }

        m_game.oldgamenumber = m_game.gamenumber;
        m_game.gamenumber = 0;
    }
    else
    {
        IsGameLost();
    }
}

void FreeCellPanel::IsGameLost()
{
    unsigned int col, pos;
    unsigned int fcol, tcol;
    CARD lastcard[MAXCOL];
    CARD c;
    unsigned int cMoves = 0;

    /* Any free cells? */
    for (pos = 0; pos < 4; pos++)
        if (m_game.card[TOPROW][pos] == EMPTY)
            return;

    /* Any free columns? */
    for (col = 1; col < MAXCOL; col++)
        if (m_game.card[col][0] == EMPTY)
            return;

    /* Check bottom cards */
    for (col = 1; col < MAXCOL; col++)
    {
        lastcard[col] = m_game.card[col][m_game.FindLastPos(col)];
        c = lastcard[col];
        if (VALUE(c) == ACE)
            cMoves++;
        if (m_game.home[SUIT(c)] == (VALUE(c) - 1))
            cMoves++;
    }

    /* Check free cell cards */
    for (pos = 0; pos < 4; pos++)
    {
        c = m_game.card[TOPROW][pos];
        if (m_game.home[SUIT(c)] == (VALUE(c) - 1))
            cMoves++;
    }

    /* Free cell cards under columns? */
    for (pos = 0; pos < 4; pos++)
        for (col = 1; col < MAXCOL; col++)
            if (m_game.FitsUnder(m_game.card[TOPROW][pos], lastcard[col]))
                cMoves++;

    /* Bottom cards under other bottom cards? */
    for (fcol = 1; fcol < MAXCOL; fcol++)
        for (tcol = 1; tcol < MAXCOL; tcol++)
            if (tcol != fcol)
                if (m_game.FitsUnder(lastcard[fcol], lastcard[tcol]))
                    cMoves++;

    if (cMoves > 0)
    {
        if (cMoves == 1)
        {
            m_cFlashes = 4;
            m_flashTimer.Start(FLASH_INTERVAL);
        }
        return;
    }

    /* No legal moves remaining */
    m_game.cUndo = 0;
    if (m_frame)
        m_frame->GetMenuBar()->Enable(IDM_UNDO, false);

    m_game.bGameInProgress = false;

    if (m_game.gamenumber != m_game.oldgamenumber
        && m_game.gamenumber > 0)
    {
        m_game.cLosses++;
        m_game.cGames++;
        m_frame->SaveLossStats();
    }

    YouLoseDialog dlg(this);
    int response = dlg.ShowModal();

    if (response == wxID_YES)
    {
        bool same = dlg.GetSameGame();
        if (same)
        {
            wxCommandEvent evt(wxEVT_MENU, IDM_RESTART);
            evt.SetInt(m_game.gamenumber);
            wxPostEvent(m_frame, evt);
        }
        else
        {
            wxCommandEvent evt(wxEVT_MENU,
                m_game.bSelecting ? IDM_SELECT : IDM_NEWGAME);
            wxPostEvent(m_frame, evt);
        }
    }

    m_game.oldgamenumber = m_game.gamenumber;
    m_game.gamenumber = 0;
}

/* ====================================================================
   Glide Animation
   ==================================================================== */

void FreeCellPanel::Glide(unsigned int fcol, unsigned int fpos,
                          unsigned int tcol, unsigned int tpos)
{
    EnsureBackBuffer();

    if (fcol == tcol && fpos == tpos)
    {
        wxMemoryDC dc;
        dc.SelectObject(m_backBuffer);
        DrawCard(dc, tcol, tpos, m_game.card[fcol][fpos], FACEUP);
        dc.SelectObject(wxNullBitmap);
        FlushBuffer();
        return;
    }

    /* Prepare foreground bitmap (the card to move) */
    {
        wxMemoryDC memF;
        memF.SelectObject(m_bmFgnd);
        m_cards.DrawCard(memF, 0, 0, m_dxCrd, m_dyCrd,
                         m_game.card[fcol][fpos], FACEUP, wxColour(0, 0, 0));
        memF.SelectObject(wxNullBitmap);
    }

    /* Prepare background bitmap (what's under the source card) */
    {
        wxMemoryDC memB1;
        memB1.SelectObject(m_bmBgnd1);

        if (fcol == TOPROW)
        {
            if (fpos > 3 && VALUE(m_game.card[fcol][fpos]) != ACE)
            {
                m_cards.DrawCard(memB1, 0, 0, m_dxCrd, m_dyCrd,
                                 m_game.card[fcol][fpos] - 4, FACEUP,
                                 BGND_RGB);
            }
            else
            {
                wxMemoryDC memGhost;
                memGhost.SelectObject(m_cards.GetGhostBitmap());
                memB1.Blit(0, 0, m_dxCrd, m_dyCrd, &memGhost, 0, 0, wxCOPY);
                memGhost.SelectObject(wxNullBitmap);
            }
        }
        else
        {
            memB1.SetBrush(m_bgndBrush);
            memB1.SetPen(wxPen(m_bgndColour));
            memB1.DrawRectangle(0, 0, m_dxCrd, m_dyCrd);

            if (fpos != 0)
            {
                m_cards.DrawCard(memB1, 0, -(int)m_dyTops, m_dxCrd, m_dyCrd,
                                 m_game.card[fcol][fpos - 1], FACEUP,
                                 BGND_RGB);
            }
        }
        memB1.SelectObject(wxNullBitmap);
    }

    unsigned int xStart, yStart, xEnd, yEnd;
    Card2Point(fcol, fpos, xStart, yStart);
    Card2Point(tcol, tpos, xEnd, yEnd);

    int dx = (int)xEnd - (int)xStart;
    int dy = (int)yEnd - (int)yStart;
    int distance = IntSqrt(dx * dx + dy * dy);

    int steps;
    if (m_game.bFastMode)
        steps = 1;
    else
        steps = distance / STEPSIZE;
    if (steps < 1) steps = 1;

    unsigned int x1 = xStart, y1 = yStart;

    for (int i = 1; i < steps; i++)
    {
        unsigned int x2 = xStart + ((i * dx) / steps);
        unsigned int y2 = yStart + ((i * dy) / steps);

        wxMemoryDC dc;
        dc.SelectObject(m_backBuffer);
        GlideStep(dc, x1, y1, x2, y2);
        dc.SelectObject(wxNullBitmap);
        FlushBuffer();

        x1 = x2;
        y1 = y2;
    }

    /* Erase last position and draw final card */
    {
        wxMemoryDC dc;
        dc.SelectObject(m_backBuffer);

        wxMemoryDC memB1;
        memB1.SelectObject(m_bmBgnd1);
        dc.Blit(x1, y1, m_dxCrd, m_dyCrd, &memB1, 0, 0, wxCOPY);
        memB1.SelectObject(wxNullBitmap);

        DrawCard(dc, tcol, tpos, m_game.card[fcol][fpos], FACEUP);
        dc.SelectObject(wxNullBitmap);
        FlushBuffer();
    }
}

void FreeCellPanel::GlideStep(wxDC& dc, unsigned int x1, unsigned int y1,
                              unsigned int x2, unsigned int y2)
{
    /* Save background at new location */
    {
        wxMemoryDC memB2;
        memB2.SelectObject(m_bmBgnd2);
        memB2.Blit(0, 0, m_dxCrd, m_dyCrd, &dc, x2, y2, wxCOPY);

        wxMemoryDC memB1;
        memB1.SelectObject(m_bmBgnd1);
        memB2.Blit((int)x1 - (int)x2, (int)y1 - (int)y2,
                   m_dxCrd, m_dyCrd, &memB1, 0, 0, wxCOPY);
        memB2.SelectObject(wxNullBitmap);
        memB1.SelectObject(wxNullBitmap);
    }

    /* Draw old background, then card at new position */
    {
        wxMemoryDC memB1;
        memB1.SelectObject(m_bmBgnd1);
        dc.Blit(x1, y1, m_dxCrd, m_dyCrd, &memB1, 0, 0, wxCOPY);
        memB1.SelectObject(wxNullBitmap);
    }
    {
        wxMemoryDC memF;
        memF.SelectObject(m_bmFgnd);
        dc.Blit(x2, y2, m_dxCrd, m_dyCrd, &memF, 0, 0, wxCOPY);
        memF.SelectObject(wxNullBitmap);
    }

    /* Swap backgrounds */
    wxBitmap temp = m_bmBgnd1;
    m_bmBgnd1 = m_bmBgnd2;
    m_bmBgnd2 = temp;
}

int FreeCellPanel::IntSqrt(int square)
{
    int guess, lastguess;

    if (square <= 0) return 0;

    lastguess = square;
    guess = square / 2;
    if (guess > 1024) guess = 1024;
    if (guess < 1) guess = 1;

    while (abs(guess - lastguess) > 3)
    {
        lastguess = guess;
        guess -= ((guess * guess) - square) / (2 * guess);
        if (guess <= 0) guess = 1;
    }

    return guess;
}

/* ====================================================================
   Card Reveal (right-click)
   ==================================================================== */

void FreeCellPanel::RevealCard(unsigned int x, unsigned int y)
{
    unsigned int col, pos;
    m_bCardRevealed = false;

    if (Point2Card(x, y, col, pos))
    {
        m_wUpdateCol = col;
        m_wUpdatePos = pos;
    }
    else
        return;

    if (col == 0 || pos == (MAXPOS - 1))
        return;

    if (m_game.card[col][pos + 1] == EMPTY)
        return;

    EnsureBackBuffer();
    wxMemoryDC dc;
    dc.SelectObject(m_backBuffer);
    DrawCard(dc, col, pos, m_game.card[col][pos], FACEUP);
    dc.SelectObject(wxNullBitmap);
    FlushBuffer();
    m_bCardRevealed = true;
}

void FreeCellPanel::RestoreColumn()
{
    if (!m_bCardRevealed)
        return;

    unsigned int lastpos = EMPTY;
    if (m_game.wMouseMode == TO)
        lastpos = m_game.FindLastPos(m_wUpdateCol);

    EnsureBackBuffer();
    wxMemoryDC dc;
    dc.SelectObject(m_backBuffer);
    int mode = FACEUP;

    for (unsigned int pos = m_wUpdatePos + 1; pos < MAXPOS; pos++)
    {
        if (m_game.card[m_wUpdateCol][pos] == EMPTY)
            break;

        if (m_game.wMouseMode == TO && pos == lastpos
            && m_wUpdateCol == m_game.wFromCol)
            mode = HILITE;

        DrawCard(dc, m_wUpdateCol, pos,
                 m_game.card[m_wUpdateCol][pos], mode);
    }
    dc.SelectObject(wxNullBitmap);
    FlushBuffer();
}

/* ====================================================================
   Keyboard Input
   ==================================================================== */

void FreeCellPanel::KeyboardInput(unsigned int keycode)
{
    unsigned int x, y;
    unsigned int col = TOPROW;
    unsigned int pos = 0;

    if (keycode < '0' || keycode > '9')
        return;

    switch (keycode)
    {
    case '0':   // free cell
        if (m_game.wMouseMode == FROM)
        {
            for (pos = 0; pos < 4; pos++)
                if (m_game.card[TOPROW][pos] != EMPTY)
                    break;
            if (pos == 4)
                return;
        }
        else
        {
            if (m_game.wFromCol == TOPROW)
            {
                bool bSave = m_game.bMessages;
                m_game.bMessages = false;

                Card2Point(TOPROW, m_game.wFromPos, x, y);

                wxMouseEvent fakeEvt(wxEVT_LEFT_DOWN);
                fakeEvt.SetX(x);
                fakeEvt.SetY(y);
                ProcessEvent(fakeEvt);

                for (pos = m_game.wFromPos + 1; pos < 4; pos++)
                    if (m_game.card[TOPROW][pos] != EMPTY)
                        break;

                m_game.bMessages = bSave;
                if (pos == 4)
                    return;
            }
            else
            {
                for (pos = 0; pos < 4; pos++)
                    if (m_game.card[TOPROW][pos] == EMPTY)
                        break;
                if (pos == 4)
                    pos = 0;
            }
        }
        break;

    case '9':   // home cell
        if (m_game.wMouseMode == FROM)
            return;
        {
            CARD c = m_game.card[m_game.wFromCol][m_game.wFromPos];
            pos = m_game.homesuit[SUIT(c)];
            if (pos == EMPTY)
                pos = 4;
        }
        break;

    default:    // columns 1-8
        col = keycode - '0';
        break;
    }

    Card2Point(col, pos, x, y);

    wxMouseEvent fakeEvt(wxEVT_LEFT_DOWN);
    fakeEvt.SetX(x);
    fakeEvt.SetY(y);
    wxPostEvent(this, fakeEvt);
}

/* ====================================================================
   Cursor Shape
   ==================================================================== */

void FreeCellPanel::SetCursorShape(unsigned int x, unsigned int y)
{
    if (m_bFlipping)
    {
        SetCursor(wxCursor(wxCURSOR_WAIT));
        return;
    }

    unsigned int tcol = 0, tpos = 0;
    bool bFound = Point2Card(x, y, tcol, tpos);

    if (bFound && tcol == TOPROW)
    {
        unsigned int newState = (tpos < 4) ? LEFT : RIGHT;
        if (newState != m_kingState)
        {
            EnsureBackBuffer();
            wxMemoryDC dc;
            dc.SelectObject(m_backBuffer);
            DrawKing(dc, newState, true);
            dc.SelectObject(wxNullBitmap);
            wxRect kingRect(m_wIconOffset, ICONY, BMWIDTH, BMHEIGHT);
            FlushBuffer(&kingRect);
        }
    }

    if (m_game.wMouseMode != TO)
    {
        SetCursor(wxCursor(wxCURSOR_ARROW));
        return;
    }

    if (!bFound)
    {
        if ((tcol > 0 && tcol < 9) && (m_game.card[tcol][0] == EMPTY))
            SetCursor(wxCursor(wxCURSOR_POINT_LEFT));
        else
            SetCursor(wxCursor(wxCURSOR_ARROW));
        return;
    }

    if (tcol != TOPROW)
        tpos = m_game.FindLastPos(tcol);

    if (m_game.wFromCol == tcol && m_game.wFromPos == tpos)
    {
        SetCursor(wxCursor(wxCURSOR_ARROW));
        return;
    }

    if (m_game.wFromCol == TOPROW || tcol == TOPROW)
    {
        if (m_game.IsValidMove(tcol, tpos))
            SetCursor(wxCursor(wxCURSOR_HAND));
        else
            SetCursor(wxCursor(wxCURSOR_ARROW));
        return;
    }

    unsigned int trans = m_game.NumberToTransfer(m_game.wFromCol, tcol);
    if ((trans > 0) && (trans <= m_game.MaxTransfer()))
        SetCursor(wxCursor(wxCURSOR_HAND));
    else
        SetCursor(wxCursor(wxCURSOR_ARROW));
}

/* ====================================================================
   Undo
   ==================================================================== */

void FreeCellPanel::DoUndo()
{
    if (m_game.cUndo == 0)
        return;

    SetCursor(wxCursor(wxCURSOR_WAIT));

    for (int i = m_game.cUndo - 1; i >= 0; i--)
    {
        int fcol = m_game.movelist[i].tcol;
        int fpos = m_game.movelist[i].tpos;
        int tcol = m_game.movelist[i].fcol;
        int tpos = m_game.movelist[i].fpos;

        if (fcol != TOPROW && fcol == tcol)
            break;

        if (fcol != TOPROW)
            fpos = m_game.FindLastPos(fcol);

        if (tcol != TOPROW)
            tpos = m_game.FindLastPos(tcol) + 1;

        Glide(fcol, fpos, tcol, tpos);

        CARD c = m_game.card[fcol][fpos];

        if (fcol == TOPROW && fpos > 3)
        {
            m_game.wCardCount++;
            UpdateCardCount();
            m_game.home[SUIT(c)]--;

            if (VALUE(c) == ACE)
            {
                m_game.card[fcol][fpos] = EMPTY;
                m_game.homesuit[SUIT(c)] = EMPTY;
            }
            else
            {
                m_game.card[fcol][fpos] -= 4;
            }
        }
        else
        {
            m_game.card[fcol][fpos] = EMPTY;
        }

        m_game.card[tcol][tpos] = c;
    }

    m_game.cUndo = 0;
    if (m_frame)
        m_frame->GetMenuBar()->Enable(IDM_UNDO, false);

    SetCursor(wxCursor(wxCURSOR_ARROW));
}
