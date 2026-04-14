/****************************************************************************
 * wx_freecell_cards.h
 *
 * Card bitmap manager for wxWidgets FreeCell port.
 * Loads card bitmaps from compiled XRC resources.
 *
 * Original Win32 code: cards.c / cdt.h
 ****************************************************************************/

#ifndef WX_FREECELL_CARDS_H
#define WX_FREECELL_CARDS_H

#include <wx/wx.h>
#include "wx_freecell.h"

class CardManager
{
public:
    CardManager();
    ~CardManager();

    /* Initialization and teardown */
    bool Init();
    void Term();

    /* Card dimensions (set from ghost bitmap after Init) */
    int GetCardWidth() const  { return m_dxCard; }
    int GetCardHeight() const { return m_dyCard; }

    /* Drawing */
    void DrawCard(wxDC& dc, int x, int y, int dx, int dy,
                  int cd, int mode, wxColour bgnd);
    void DrawCardSimple(wxDC& dc, int x, int y,
                        int cd, int mode, wxColour bgnd);

    /* Accessors */
    wxBitmap& GetGhostBitmap()          { return m_bmpGhost; }
    wxBitmap& GetKingBitmap(int state);

private:
    /* Card face bitmaps: indexed by card number (value*4 + suit) */
    wxBitmap m_bmpCard[52];

    /* Special bitmaps */
    wxBitmap m_bmpGhost;        // empty cell placeholder
    wxBitmap m_bmpDeckX;        // deck X marker
    wxBitmap m_bmpDeckO;        // deck O marker
    wxBitmap m_bmpBack;         // facedown card back

    /* King bitmaps (used in the status area between free/home cells) */
    wxBitmap m_bmpKingRight;    // RIGHT (default, facing right)
    wxBitmap m_bmpKingLeft;     // LEFT  (facing left)
    wxBitmap m_bmpKingSmile;    // SMILE (happy)

    /* Card dimensions in pixels */
    int m_dxCard;
    int m_dyCard;
};

#endif // WX_FREECELL_CARDS_H
