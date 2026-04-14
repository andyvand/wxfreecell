/****************************************************************************
 * wx_freecell_cards.cpp
 *
 * Card bitmap manager for wxWidgets FreeCell port.
 * Loads all card bitmaps from compiled XRC resources (embedded in binary).
 *
 * Original Win32 code: cards.c (GDI resource bitmaps)
 ****************************************************************************/

#include "wx_freecell_cards.h"
#include "wx_freecell.h"

#include <wx/xrc/xmlres.h>


/* -----------------------------------------------------------------------
   Constructor / Destructor
   ----------------------------------------------------------------------- */

CardManager::CardManager()
    : m_dxCard(0)
    , m_dyCard(0)
{
}

CardManager::~CardManager()
{
    Term();
}


/* -----------------------------------------------------------------------
   Init
   Loads all bitmaps from XRC resources.  The XRC must already be loaded
   (by the app's OnInit) before this is called.
   Returns false if essential bitmaps cannot be loaded.
   Sets m_dxCard / m_dyCard from the ghost bitmap dimensions.
   ----------------------------------------------------------------------- */

bool CardManager::Init()
{
    wxXmlResource* xrc = wxXmlResource::Get();

    /* ---- Ghost bitmap (required -- used for card dimensions) ---- */
    m_bmpGhost = xrc->LoadBitmap("bmp_ghost");
    if (!m_bmpGhost.IsOk())
    {
        wxLogError("CardManager: failed to load bmp_ghost from XRC");
        return false;
    }
    m_dxCard = m_bmpGhost.GetWidth();
    m_dyCard = m_bmpGhost.GetHeight();

    /* ---- Deck X / O ---- */
    m_bmpDeckX = xrc->LoadBitmap("bmp_x");
    if (!m_bmpDeckX.IsOk())
    {
        wxLogError("CardManager: failed to load bmp_x from XRC");
        return false;
    }

    m_bmpDeckO = xrc->LoadBitmap("bmp_o");
    if (!m_bmpDeckO.IsOk())
    {
        wxLogError("CardManager: failed to load bmp_o from XRC");
        return false;
    }

    /* ---- Facedown card back (optional, fall back to green fill) ---- */
    m_bmpBack = xrc->LoadBitmap("bmp_back");

    /* ---- King bitmaps ---- */
    m_bmpKingRight = xrc->LoadBitmap("bmp_king");
    if (!m_bmpKingRight.IsOk())
    {
        wxLogError("CardManager: failed to load bmp_king from XRC");
        return false;
    }

    m_bmpKingLeft = xrc->LoadBitmap("bmp_kingl");
    if (!m_bmpKingLeft.IsOk())
    {
        wxLogError("CardManager: failed to load bmp_kingl from XRC");
        return false;
    }

    m_bmpKingSmile = xrc->LoadBitmap("bmp_kings");
    if (!m_bmpKingSmile.IsOk())
    {
        wxLogError("CardManager: failed to load bmp_kings from XRC");
        return false;
    }

    /* ---- 52 playing card bitmaps ---- */
    for (int cd = 0; cd < 52; cd++)
    {
        wxString name = wxString::Format("card_%d", cd);
        m_bmpCard[cd] = xrc->LoadBitmap(name);

        if (!m_bmpCard[cd].IsOk())
        {
            wxLogError("CardManager: failed to load %s from XRC", name);
            return false;
        }
    }

    return true;
}


/* -----------------------------------------------------------------------
   Term
   Releases all loaded bitmaps.
   ----------------------------------------------------------------------- */

void CardManager::Term()
{
    for (int i = 0; i < 52; i++)
    {
        if (m_bmpCard[i].IsOk())
            m_bmpCard[i] = wxNullBitmap;
    }

    m_bmpGhost     = wxNullBitmap;
    m_bmpDeckX     = wxNullBitmap;
    m_bmpDeckO     = wxNullBitmap;
    m_bmpBack      = wxNullBitmap;
    m_bmpKingRight = wxNullBitmap;
    m_bmpKingLeft  = wxNullBitmap;
    m_bmpKingSmile = wxNullBitmap;

    m_dxCard = 0;
    m_dyCard = 0;
}


/* -----------------------------------------------------------------------
   DrawCard
   Draws a single card at (x,y) with size (dx,dy).
   mode determines how the card is rendered:
     FACEUP(0)         - draw card face; add black border for red cards
     FACEDOWN(1)       - draw card back (or green fill)
     HILITE(2)         - draw card face inverted
     GHOST(3)          - fill background, then draw ghost with AND
     REMOVE(4)         - fill with background colour
     INVISIBLEGHOST(5) - draw ghost with AND (no background fill)
     DECKX(6)          - draw deck X marker
     DECKO(7)          - draw deck O marker
   ----------------------------------------------------------------------- */

void CardManager::DrawCard(wxDC& dc, int x, int y, int dx, int dy,
                           int cd, int mode, wxColour bgnd)
{
    wxBitmap* pBmp = NULL;
    bool bInvert = false;
    bool bAndBlit = false;

    switch (mode)
    {
    case FACEUP:
        if (cd >= 0 && cd < 52)
            pBmp = &m_bmpCard[cd];
        break;

    case FACEDOWN:
        if (m_bmpBack.IsOk())
        {
            pBmp = &m_bmpBack;
        }
        else
        {
            // Fallback: fill with a green rectangle.
            dc.SetBrush(wxBrush(wxColour(0, 128, 0)));
            dc.SetPen(*wxBLACK_PEN);
            dc.DrawRectangle(x, y, dx, dy);
            return;
        }
        break;

    case HILITE:
        if (cd >= 0 && cd < 52)
            pBmp = &m_bmpCard[cd];
        bInvert = true;
        break;

    case GHOST:
        // Fill with background colour first, then draw ghost with AND.
        {
            dc.SetBrush(wxBrush(bgnd));
            dc.SetPen(wxPen(bgnd));
            dc.DrawRectangle(x, y, dx, dy);
        }
        pBmp = &m_bmpGhost;
        bAndBlit = true;
        break;

    case REMOVE:
        // Just fill with background colour.
        {
            dc.SetBrush(wxBrush(bgnd));
            dc.SetPen(wxPen(bgnd));
            dc.DrawRectangle(x, y, dx, dy);
        }
        return;

    case INVISIBLEGHOST:
        pBmp = &m_bmpGhost;
        bAndBlit = true;
        break;

    case DECKX:
        pBmp = &m_bmpDeckX;
        break;

    case DECKO:
        pBmp = &m_bmpDeckO;
        break;

    default:
        return;
    }

    if (pBmp == NULL || !pBmp->IsOk())
        return;

    // Prepare the source bitmap (optionally scaled, optionally inverted).
    wxBitmap srcBmp = *pBmp;

    // Scale if the requested size differs from the native card size.
    if (dx != m_dxCard || dy != m_dyCard)
    {
        wxImage img = srcBmp.ConvertToImage();
        img.Rescale(dx, dy, wxIMAGE_QUALITY_BILINEAR);
        srcBmp = wxBitmap(img);
    }

    if (bInvert)
    {
        // Invert the bitmap image for HILITE mode.
        wxImage img = srcBmp.ConvertToImage();
        unsigned char* data = img.GetData();
        int len = img.GetWidth() * img.GetHeight() * 3;
        for (int i = 0; i < len; i++)
            data[i] = 255 - data[i];
        srcBmp = wxBitmap(img);
    }

    wxMemoryDC memDC;
    memDC.SelectObject(srcBmp);

    if (bAndBlit)
    {
        // AND blit: use wxAND logical function to combine ghost with
        // the existing background, preserving the background through
        // the transparent parts of the ghost bitmap.
        dc.Blit(x, y, dx, dy, &memDC, 0, 0, wxAND);
    }
    else
    {
        dc.Blit(x, y, dx, dy, &memDC, 0, 0, wxCOPY);
    }

    memDC.SelectObject(wxNullBitmap);

    // For FACEUP red cards (Diamonds and Hearts), draw a black border.
    // This matches the original Win32 code which draws borders for cards
    // whose suit is Diamond (1) or Heart (2).
    if (mode == FACEUP && cd >= 0 && cd < 52)
    {
        int suit = cd % 4;
        if (suit == DIAMOND || suit == HEART)
        {
            wxPen blackPen(*wxBLACK, 1);
            dc.SetPen(blackPen);

            // Top edge (2px inset from left and right)
            dc.DrawLine(x + 2, y, x + dx - 2, y);

            // Right edge (2px inset from top and bottom)
            dc.DrawLine(x + dx - 1, y + 2, x + dx - 1, y + dy - 2);

            // Bottom edge (2px inset from left and right)
            dc.DrawLine(x + 2, y + dy - 1, x + dx - 2, y + dy - 1);

            // Left edge (2px inset from top and bottom)
            dc.DrawLine(x, y + 2, x, y + dy - 2);

            // Corner pixels
            dc.DrawPoint(x + 1, y + 1);            // top left
            dc.DrawPoint(x + dx - 2, y + 1);       // top right
            dc.DrawPoint(x + dx - 2, y + dy - 2);  // bottom right
            dc.DrawPoint(x + 1, y + dy - 2);        // bottom left
        }
    }
}


/* -----------------------------------------------------------------------
   DrawCardSimple
   Convenience wrapper that draws a card at its native size.
   ----------------------------------------------------------------------- */

void CardManager::DrawCardSimple(wxDC& dc, int x, int y,
                                 int cd, int mode, wxColour bgnd)
{
    DrawCard(dc, x, y, m_dxCard, m_dyCard, cd, mode, bgnd);
}


/* -----------------------------------------------------------------------
   GetKingBitmap
   Returns the king bitmap for the given state.
     RIGHT(2) - right-facing king (default)
     LEFT(3)  - left-facing king
     SMILE(4) - smiling king
   Falls back to the right-facing king for any other state.
   ----------------------------------------------------------------------- */

wxBitmap& CardManager::GetKingBitmap(int state)
{
    switch (state)
    {
    case LEFT:
        return m_bmpKingLeft;
    case SMILE:
        return m_bmpKingSmile;
    case RIGHT:
    default:
        return m_bmpKingRight;
    }
}
