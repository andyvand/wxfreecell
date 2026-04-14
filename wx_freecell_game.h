/****************************************************************************
 * wx_freecell_game.h
 *
 * Pure game logic for wxWidgets FreeCell port.
 * No UI dependencies -- all display and dialog interaction is handled
 * by the caller (the frame / panel class).
 *
 * Original: June 91, JimH - initial code; Oct 91, port to Win32
 * wxWidgets port preserves game logic and shuffle algorithm exactly.
 ****************************************************************************/

#ifndef WX_FREECELL_GAME_H
#define WX_FREECELL_GAME_H

#include "wx_freecell.h"

class FreeCellGame {
public:
    /* ----- Card layout ----- */
    CARD card[MAXCOL][MAXPOS];      // current layout (col 0 = top row)
    CARD shadow[MAXCOL][MAXPOS];    // backup for multi-moves / cleanup
    CARD home[4];                   // value of top card in each home pile
    CARD homesuit[4];               // which home-cell position (4-7) each suit uses

    /* ----- Game identification ----- */
    int  gamenumber;                // current game number (rand seed)
    int  oldgamenumber;             // previous game (repeats don't count)

    /* ----- Counts and indices ----- */
    unsigned int wCardCount;        // cards not yet in home cells (0 == win)
    unsigned int wFromCol;          // column user selected to transfer FROM
    unsigned int wFromPos;          // position within that column
    unsigned int wMouseMode;        // selecting FROM or TO
    unsigned int moveindex;         // index to end of movelist
    MOVE movelist[MAXMOVELIST];     // queued move requests
    int  cUndo;                     // number of moves available for undo

    /* ----- Flags ----- */
    bool bGameInProgress;
    bool bSelecting;                // user is selecting game numbers?
    bool bCheating;                 // cheat mode active (CHEAT_WIN / CHEAT_LOSE)
    bool bMoveCol;                  // user chose column move (vs. single card)?

    /* ----- Game options (persisted) ----- */
    bool bMessages;                 // show illegal-move messages?
    bool bFastMode;                 // skip glide animations?
    bool bDblClick;                 // honor double-click?

    /* ----- Session statistics ----- */
    unsigned int cWins;
    unsigned int cLosses;
    unsigned int cGames;

    /* ----- Methods ----- */

    /** Clear all state to initial values. */
    void Init();

    /**
     * Shuffle the deck using the given seed.
     * If seed is 0, generates a random game number internally.
     * MUST use WinRand (Windows-compatible PRNG) for the shuffle itself
     * so that game numbers produce identical layouts across platforms.
     * Special cases: gamenumber == -1 and -2 produce known unwinnable shuffles.
     */
    void ShuffleDeck(unsigned int seed);

    /** Position of last card in column, or EMPTY if column is empty. */
    unsigned int FindLastPos(unsigned int col);

    /** Returns true if fcard fits under tcard (alternating colour, descending). */
    bool FitsUnder(CARD fcard, CARD tcard);

    /** Maximum number of cards transferable given current free cells / columns. */
    unsigned int MaxTransfer();

    /** Recursive helper for MaxTransfer. */
    unsigned int MaxTransfer2(unsigned int freecells, unsigned int freecols);

    /** Number of cards needed to move from fcol to tcol, or 0 if illegal. */
    unsigned int NumberToTransfer(unsigned int fcol, unsigned int tcol);

    /** Queue a single card move into movelist and update card array. */
    void QueueTransfer(unsigned int fcol, unsigned int fpos,
                       unsigned int tcol, unsigned int tpos);

    /** Copy card[][] into shadow[][] (save state before multi-move). */
    void SaveState();

    /** Copy shadow[][] back into card[][] (restore after queuing). */
    void RestoreState();

    /** Can this card be safely auto-moved to a home cell? */
    bool Useless(CARD c);

    /** Auto-move all safe cards to home cells (queues transfers). */
    void Cleanup();

    /** Multi-card move to an empty column using free cells as temp. */
    void MoveCol(unsigned int fcol, unsigned int tcol);

    /** Multi-card move between two non-empty columns. */
    void MultiMove(unsigned int fcol, unsigned int tcol);

    /**
     * Execute a single transfer: update the card array.
     * This is the non-animated Transfer -- it just does the bookkeeping
     * that the original Transfer() did (minus the HDC / Glide calls).
     */
    void DoTransfer(unsigned int fcol, unsigned int fpos,
                    unsigned int tcol, unsigned int tpos);

    /**
     * Check whether moving from wFromCol/wFromPos to tcol/tpos is valid.
     * Does NOT show dialogs -- the caller handles UI.
     * Sets bMoveCol: true = user wants column move, false = single card,
     * -1 (cast) = cancel.
     * Returns true if the move is legal, false otherwise.
     */
    bool IsValidMove(unsigned int tcol, unsigned int tpos);

    /**
     * Double-click handler: finds an empty free cell and queues the move.
     * Returns true if a move was queued, false if no free cell available.
     */
    bool ProcessDoubleClick();

    /** Generate a random game number from 1 to MAXGAMENUMBER. */
    unsigned int GenerateRandomGameNum();

private:
    WinRand m_rng;  // Windows-compatible PRNG for shuffle
};

#endif // WX_FREECELL_GAME_H
