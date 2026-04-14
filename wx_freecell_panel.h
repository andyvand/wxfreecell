/****************************************************************************
 * wx_freecell_panel.h
 *
 * Game board panel for wxWidgets FreeCell port.
 * Handles all card rendering, mouse interaction, and animation.
 *
 * Original: MainWndProc (freecell.c), display.c, glide.c
 ****************************************************************************/

#ifndef WX_FREECELL_PANEL_H
#define WX_FREECELL_PANEL_H

#include <wx/wx.h>
#include "wx_freecell.h"
#include "wx_freecell_game.h"
#include "wx_freecell_cards.h"

class FreeCellFrame;

class FreeCellPanel : public wxPanel
{
public:
    FreeCellPanel(FreeCellFrame* parent);
    ~FreeCellPanel();

    bool InitCards();

    /* Game access */
    FreeCellGame& GetGame() { return m_game; }
    CardManager&  GetCards() { return m_cards; }

    /* Game actions (called from frame menu handlers) */
    void StartNewGame(int seed);
    void DoUndo();

    /* Display helpers */
    void UpdateCardCount();

private:
    /* Event handlers */
    void OnPaint(wxPaintEvent& event);
    void OnSize(wxSizeEvent& event);
    void OnLeftDown(wxMouseEvent& event);
    void OnLeftDClick(wxMouseEvent& event);
    void OnRightDown(wxMouseEvent& event);
    void OnRightUp(wxMouseEvent& event);
    void OnMouseMove(wxMouseEvent& event);
    void OnKeyChar(wxKeyEvent& event);
    void OnTimer(wxTimerEvent& event);

    /* Rendering */
    void PaintMainWindow(wxDC& dc);
    void DrawCard(wxDC& dc, unsigned int col, unsigned int pos,
                  CARD c, int mode);
    void DrawCardMem(wxDC& dc, CARD c, int mode);
    void DrawKing(wxDC& dc, unsigned int state, bool bDraw);

    /* Card position mapping */
    void CalcOffsets();
    void Card2Point(unsigned int col, unsigned int pos,
                    unsigned int& x, unsigned int& y);
    bool Point2Card(unsigned int x, unsigned int y,
                    unsigned int& col, unsigned int& pos);
    void Payoff(wxDC& dc);

    /* Game flow */
    void SetFromLoc(unsigned int x, unsigned int y);
    void ProcessMoveRequest(unsigned int x, unsigned int y);
    bool ProcessDoubleClick();
    void MoveCards();
    void Transfer(unsigned int fcol, unsigned int fpos,
                  unsigned int tcol, unsigned int tpos);
    void IsGameLost();

    /* Animation */
    void Glide(unsigned int fcol, unsigned int fpos,
               unsigned int tcol, unsigned int tpos);
    void GlideStep(wxDC& dc, unsigned int x1, unsigned int y1,
                   unsigned int x2, unsigned int y2);
    int IntSqrt(int square);

    /* Card reveal (right-click) */
    void RevealCard(unsigned int x, unsigned int y);
    void RestoreColumn();

    /* Keyboard */
    void KeyboardInput(unsigned int keycode);
    void Flash();
    void Flip();

    /* Cursor */
    void SetCursorShape(unsigned int x, unsigned int y);

    /* Game state */
    FreeCellGame m_game;
    CardManager  m_cards;
    FreeCellFrame* m_frame;

    /* Layout offsets */
    unsigned int m_wOffset[MAXCOL];
    unsigned int m_wIconOffset;
    unsigned int m_wVSpace;
    unsigned int m_dyTops;
    unsigned int m_dxCrd, m_dyCrd;

    /* Reveal state */
    unsigned int m_wUpdateCol, m_wUpdatePos;
    bool m_bCardRevealed;

    /* King state */
    unsigned int m_kingState;

    /* Flash timer */
    wxTimer m_flashTimer;
    wxTimer m_flipTimer;
    int m_cFlashes;

    /* Flip state */
    unsigned int m_flipPos;

    /* Persistent back buffer for GTK3 compatibility (wxClientDC is a no-op) */
    wxBitmap m_backBuffer;
    void EnsureBackBuffer();
    void FlushBuffer(const wxRect* rect = nullptr);

    /* Glide animation bitmaps */
    wxBitmap m_bmBgnd1;
    wxBitmap m_bmBgnd2;
    wxBitmap m_bmFgnd;

    /* Activation flag */
    bool m_bEatNextMouseHit;

    /* Flipping flag */
    bool m_bFlipping;

    /* Background colour */
    wxColour m_bgndColour;
    wxBrush  m_bgndBrush;
    wxPen    m_brightPen;

    wxDECLARE_EVENT_TABLE();
};

#endif // WX_FREECELL_PANEL_H
