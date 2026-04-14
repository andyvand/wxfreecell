/****************************************************************************
 * wx_freecell.h
 *
 * Common header for wxWidgets FreeCell port.
 * Contains game constants, types, and macros.
 *
 * Original: June 91, JimH - initial code; Oct 91, port to Win32
 * wxWidgets port preserves game logic and shuffle algorithm exactly.
 ****************************************************************************/

#ifndef WX_FREECELL_H
#define WX_FREECELL_H

#include <wx/wx.h>
#include <cstdint>
#include <cstring>

/* ----- Game Constants ----- */

#define FACEUP          0
#define FACEDOWN        1
#define HILITE          2
#define GHOST           3
#define REMOVE          4
#define INVISIBLEGHOST  5
#define DECKX           6
#define DECKO           7

#define EMPTY           0xFFFFFFFF

#define IDGHOST         53

#define MAXPOS          21
#define MAXCOL          9           // includes top row as column 0
#define MAXMOVELIST     150

#define TOPROW          0           // column 0 is the top row

#define BLACK           0           // COLOUR(card)
#define RED             1

#define ACE             0           // VALUE(card)
#define DEUCE           1

#define CLUB            0           // SUIT(card)
#define DIAMOND         1
#define HEART           2
#define SPADE           3

#define FROM            0           // wMouseMode
#define TO              1

#define ICONWIDTH       32
#define ICONHEIGHT      32

#define MAXGAMENUMBER   1000000
#define CANCELGAME      (MAXGAMENUMBER + 1)

#define NONE            0           // king bitmap identifiers
#define SAME            1
#define RIGHT           2
#define LEFT            3
#define SMILE           4

#define BMWIDTH         32
#define BMHEIGHT        32

#define LOST            0           // used for streaks
#define WON             1

#define FLASH_TIMER_ID  2
#define FLASH_INTERVAL  400
#define FLIP_TIMER_ID   3
#define FLIP_INTERVAL   300

#define CHEAT_LOSE      1
#define CHEAT_WIN       2

/* ----- Card Macros ----- */

#define SUIT(card)      ((card) % 4)
#define VALUE(card)     ((card) / 4)
#define COLOUR(card)    (SUIT(card) == 1 || SUIT(card) == 2)

/* ----- Types ----- */

typedef int CARD;

struct MOVE {
    unsigned int fcol;
    unsigned int fpos;
    unsigned int tcol;
    unsigned int tpos;
};

/* ----- Windows-compatible PRNG -----
 * The original FreeCell uses srand/rand from the Windows C runtime.
 * For game number compatibility across platforms, we replicate the
 * exact Windows LCG algorithm: seed = seed * 214013 + 2531011
 * Returns values in [0, 32767].
 */
class WinRand {
public:
    void Seed(unsigned int s) { m_seed = s; }
    int Next() {
        m_seed = m_seed * 214013 + 2531011;
        return (int)((m_seed >> 16) & 0x7FFF);
    }
private:
    unsigned int m_seed = 0;
};

/* ----- Menu/control IDs -----
   Use XRCID() so these match the IDs assigned when loading the menu
   from compiled XRC resources.  XRCID is safe to call before the XRC
   is loaded — it auto-registers the string and returns a stable int. */

#include <wx/xrc/xmlres.h>

#define IDM_ABOUT      XRCID("IDM_ABOUT")
#define IDM_NEWGAME    XRCID("IDM_NEWGAME")
#define IDM_SELECT     XRCID("IDM_SELECT")
#define IDM_MOVECARD   XRCID("IDM_MOVECARD")
#define IDM_STATS      XRCID("IDM_STATS")
#define IDM_HELP_FC    XRCID("IDM_HELP_FC")
#define IDM_RESTART    XRCID("IDM_RESTART")
#define IDM_EXIT       XRCID("IDM_EXIT")
#define IDM_OPTIONS    XRCID("IDM_OPTIONS")
#define IDM_HOWTOPLAY  XRCID("IDM_HOWTOPLAY")
#define IDM_HELPONHELP XRCID("IDM_HELPONHELP")
#define IDM_FAKETIMER  XRCID("IDM_FAKETIMER")
#define IDM_CHEAT      XRCID("IDM_CHEAT")
#define IDM_UNDO       XRCID("IDM_UNDO")
#define IDM_MENU       XRCID("IDM_MENU")

#endif // WX_FREECELL_H
