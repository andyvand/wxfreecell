/****************************************************************************
 * wx_freecell_game.cpp
 *
 * Pure game logic for wxWidgets FreeCell port.
 * No UI dependencies -- all display and dialog interaction is handled
 * by the caller.
 *
 * Ported faithfully from game.c, game2.c, transfer.c, and display.c
 * (June 91, JimH -- initial code; Oct 91, port to Win32).
 ****************************************************************************/

#include "wx_freecell_game.h"
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <ctime>


/****************************************************************************
 * Init
 *
 * Clear all game state to initial values.
 ****************************************************************************/

void FreeCellGame::Init()
{
    unsigned int col, pos;

    for (col = 0; col < MAXCOL; col++)
        for (pos = 0; pos < MAXPOS; pos++)
            card[col][pos] = EMPTY;

    std::memset(shadow, 0, sizeof(shadow));

    for (int i = 0; i < 4; i++)
    {
        home[i] = EMPTY;
        homesuit[i] = EMPTY;
    }

    gamenumber = 0;
    oldgamenumber = 0;
    wCardCount = 0;
    wFromCol = EMPTY;
    wFromPos = EMPTY;
    wMouseMode = FROM;
    moveindex = 0;
    cUndo = 0;

    bGameInProgress = false;
    bSelecting = false;
    bCheating = false;
    bMoveCol = false;

    bMessages = false;
    bFastMode = false;
    bDblClick = true;

    cWins = 0;
    cLosses = 0;
    cGames = 0;
}


/****************************************************************************
 * ShuffleDeck
 *
 * If seed is non-zero, that number is used as the game number.
 * Otherwise a random game number is generated.
 *
 * The shuffle algorithm is published and relied upon by millions of
 * players who track games by number.  It MUST use the Windows-compatible
 * PRNG (WinRand) so that identical game numbers produce identical layouts
 * across all platforms.
 *
 * Special cases:
 *   gamenumber == -1  -- known unwinnable shuffle #1
 *   gamenumber == -2  -- known unwinnable shuffle #2
 ****************************************************************************/

void FreeCellGame::ShuffleDeck(unsigned int seed)
{
    unsigned int i, j;
    unsigned int col, pos;
    unsigned int wLeft = 52;
    CARD deck[52];

    if (seed == 0)
        gamenumber = (int)GenerateRandomGameNum();
    else
        gamenumber = (int)seed;

    /* Clear the entire layout */

    for (col = 0; col < MAXCOL; col++)
        for (pos = 0; pos < MAXPOS; pos++)
            card[col][pos] = EMPTY;

    /* Build an ordered deck */

    for (i = 0; i < 52; i++)
        deck[i] = (CARD)i;

    /* Handle special unwinnable shuffles */

    if (gamenumber == -1)
    {
        i = 0;

        for (pos = 0; pos < 7; pos++)
        {
            for (col = 1; col < 5; col++)
                card[col][pos] = (CARD)i++;

            i += 4;
        }

        for (pos = 0; pos < 6; pos++)
        {
            i -= 12;

            for (col = 5; col < 9; col++)
                card[col][pos] = (CARD)i++;
        }
    }
    else if (gamenumber == -2)
    {
        i = 3;

        for (col = 1; col < 5; col++)
            card[col][0] = (CARD)i--;

        i = 51;

        for (pos = 1; pos < 7; pos++)
            for (col = 1; col < 5; col++)
                card[col][pos] = (CARD)i--;

        for (pos = 0; pos < 6; pos++)
            for (col = 5; col < 9; col++)
                card[col][pos] = (CARD)i--;
    }
    else
    {
        /*
         * Caution:
         * This shuffle algorithm has been published to people all around.
         * The intention was to let people track the games by game numbers.
         * So far all the games between 1 and 32767 except one have been
         * proved to have a winning solution.  Do not change the shuffling
         * algorithm else you will incur the wrath of people who have
         * invested a huge amount of time solving these games.
         *
         * Uses the Windows-compatible LCG (WinRand) so that every
         * platform produces the same layout for the same game number.
         */

        m_rng.Seed((unsigned int)gamenumber);

        for (i = 0; i < 52; i++)
        {
            j = m_rng.Next() % wLeft;
            wLeft--;
            card[(i % 8) + 1][i / 8] = deck[j];
            deck[j] = deck[wLeft];
        }
    }
}


/****************************************************************************
 * FindLastPos
 *
 * Returns position of last card in column, or EMPTY if column is empty.
 ****************************************************************************/

unsigned int FreeCellGame::FindLastPos(unsigned int col)
{
    unsigned int pos = 0;

    if (col > 9)
        return EMPTY;

    if (card[col][0] == EMPTY)
        return EMPTY;

    while (pos < MAXPOS && card[col][pos] != EMPTY)
        pos++;

    pos--;
    return pos;
}


/****************************************************************************
 * FitsUnder
 *
 * Returns true if fcard fits under tcard (alternating colour, descending).
 ****************************************************************************/

bool FreeCellGame::FitsUnder(CARD fcard, CARD tcard)
{
    if ((VALUE(tcard) - VALUE(fcard)) != 1)
        return false;

    if (COLOUR(fcard) == COLOUR(tcard))
        return false;

    return true;
}


/****************************************************************************
 * MaxTransfer / MaxTransfer2
 *
 * Determine the maximum number of cards that could be transferred given
 * the current number of free cells and empty columns.
 ****************************************************************************/

unsigned int FreeCellGame::MaxTransfer()
{
    unsigned int freecells = 0;
    unsigned int freecols  = 0;
    unsigned int col, pos;

    for (pos = 0; pos < 4; pos++)
        if (card[TOPROW][pos] == EMPTY)
            freecells++;

    for (col = 1; col <= 8; col++)
        if (card[col][0] == EMPTY)
            freecols++;

    return MaxTransfer2(freecells, freecols);
}

unsigned int FreeCellGame::MaxTransfer2(unsigned int freecells,
                                        unsigned int freecols)
{
    if (freecols == 0)
        return (freecells + 1);

    return (freecells + 1 + MaxTransfer2(freecells, freecols - 1));
}


/****************************************************************************
 * NumberToTransfer
 *
 * Given a from column and a to column, returns the number of cards
 * required to do the transfer, or 0 if there is no legal move.
 *
 * If the transfer is from a column to an empty column, returns the
 * maximum number of cards that could transfer.
 ****************************************************************************/

unsigned int FreeCellGame::NumberToTransfer(unsigned int fcol,
                                            unsigned int tcol)
{
    unsigned int fpos;
    CARD tcard;
    unsigned int number = 0;

    assert(fcol > 0 && fcol < 9);
    assert(tcol > 0 && tcol < 9);
    assert(card[fcol][0] != EMPTY);

    if (fcol == tcol)
        return 1;                       // cancellation takes one move

    fpos = FindLastPos(fcol);

    if (card[tcol][0] == EMPTY)         // transfer to empty column
    {
        while (fpos > 0)
        {
            if (!FitsUnder(card[fcol][fpos], card[fcol][fpos - 1]))
                break;
            fpos--;
            number++;
        }
        return (number + 1);
    }
    else
    {
        unsigned int tpos = FindLastPos(tcol);
        tcard = card[tcol][tpos];
        for (;;)
        {
            number++;
            if (FitsUnder(card[fcol][fpos], tcard))
                return number;
            if (fpos == 0)
                return 0;
            if (!FitsUnder(card[fcol][fpos], card[fcol][fpos - 1]))
                return 0;
            fpos--;
        }
    }
}


/****************************************************************************
 * QueueTransfer
 *
 * Queue a single-card move into the movelist array and update the card
 * array to reflect the pending move.  (The card array is restored from
 * shadow before the moves are actually animated.)
 *
 * This matches the original Transfer.c QueueTransfer() logic exactly.
 ****************************************************************************/

void FreeCellGame::QueueTransfer(unsigned int fcol, unsigned int fpos,
                                 unsigned int tcol, unsigned int tpos)
{
    CARD c;
    MOVE move;

    assert(moveindex < MAXMOVELIST);
    assert(fpos < MAXPOS);
    assert(tpos < MAXPOS);

    move.fcol = fcol;
    move.fpos = fpos;
    move.tcol = tcol;
    move.tpos = tpos;
    movelist[moveindex++] = move;

    /* Update card array to reflect the queued move. */

    if (fcol == TOPROW)
    {
        if ((fpos > 3) || (card[TOPROW][fpos] == IDGHOST))
            return;
    }
    else
    {
        fpos = FindLastPos(fcol);
        if (fpos == EMPTY)
            return;

        if (fcol == tcol)               // click and release on same column
            return;
    }

    if (tcol == TOPROW)
    {
        if (tpos > 3)                   // move to home cell
        {
            c = card[fcol][fpos];
            home[SUIT(c)] = VALUE(c);
        }
    }
    else
    {
        tpos = FindLastPos(tcol) + 1;
    }

    c = card[fcol][fpos];
    card[fcol][fpos] = EMPTY;
    card[tcol][tpos] = c;

    if (VALUE(c) == ACE && tcol == TOPROW && tpos > 3)
        homesuit[SUIT(c)] = tpos;
}


/****************************************************************************
 * SaveState / RestoreState
 ****************************************************************************/

void FreeCellGame::SaveState()
{
    std::memcpy(&shadow[0][0], &card[0][0], sizeof(card));
}

void FreeCellGame::RestoreState()
{
    std::memcpy(&card[0][0], &shadow[0][0], sizeof(card));
}


/****************************************************************************
 * Useless
 *
 * Returns true if a card can be safely discarded to a home cell
 * (no existing cards could possibly play on it).
 ****************************************************************************/

bool FreeCellGame::Useless(CARD c)
{
    CARD limit;

    if (c == EMPTY)
        return false;

    if (bCheating)
        return true;

    if (VALUE(c) == ACE)
        return true;

    else if (VALUE(c) == DEUCE)
    {
        if (home[SUIT(c)] == ACE)
            return true;
        else
            return false;
    }
    else
    {
        limit = VALUE(c) - 1;

        if (home[SUIT(c)] != limit)
            return false;

        if (COLOUR(c) == RED)
        {
            if (home[CLUB] == EMPTY || home[SPADE] == EMPTY)
                return false;
            else
                return (home[CLUB] >= limit && home[SPADE] >= limit);
        }
        else
        {
            if (home[DIAMOND] == EMPTY || home[HEART] == EMPTY)
                return false;
            else
                return (home[DIAMOND] >= limit && home[HEART] >= limit);
        }
    }
}


/****************************************************************************
 * Cleanup
 *
 * Auto-move exposed cards that are no longer needed to home cells.
 * Keeps looping until a full pass finds no new useless cards.
 ****************************************************************************/

void FreeCellGame::Cleanup()
{
    unsigned int col, pos;
    unsigned int i;
    CARD c;
    bool bMore = true;

    while (bMore)
    {
        bMore = false;

        /* Do TOPROW (free cells) first */

        for (pos = 0; pos < 4; pos++)
        {
            c = card[TOPROW][pos];
            if (Useless(c))
            {
                bMore = true;

                if (homesuit[SUIT(c)] == EMPTY)
                {
                    i = 4;
                    while (card[TOPROW][i] != EMPTY)
                        i++;
                    homesuit[SUIT(c)] = i;
                }
                QueueTransfer(TOPROW, pos, TOPROW, homesuit[SUIT(c)]);
            }
        }

        /* Do columns 1-8 next */

        for (col = 1; col <= 8; col++)
        {
            pos = FindLastPos(col);
            if (pos != EMPTY)
            {
                c = card[col][pos];
                if (Useless(c))
                {
                    bMore = true;

                    if (homesuit[SUIT(c)] == EMPTY)
                    {
                        i = 4;
                        while (card[TOPROW][i] != EMPTY)
                            i++;
                        homesuit[SUIT(c)] = i;
                    }
                    QueueTransfer(col, pos, TOPROW, homesuit[SUIT(c)]);
                }
            }
        }
    }
}


/****************************************************************************
 * MoveCol
 *
 * Multi-card move to an empty column, using free cells as temporary
 * storage.  Mirrors the original transfer.c MoveCol() exactly.
 ****************************************************************************/

void FreeCellGame::MoveCol(unsigned int fcol, unsigned int tcol)
{
    unsigned int freecells;
    CARD free[4];
    unsigned int trans;
    int i;

    assert(fcol != TOPROW);
    assert(tcol != TOPROW);
    assert(card[fcol][0] != EMPTY);

    /* Count free cells and record their positions */

    freecells = 0;
    for (i = 0; i < 4; i++)
    {
        if (card[TOPROW][i] == EMPTY)
        {
            free[freecells] = i;
            freecells++;
        }
    }

    /* Find number of cards to transfer */

    if (fcol == TOPROW || tcol == TOPROW)
        trans = 1;
    else
        trans = NumberToTransfer(fcol, tcol);

    if (trans > (freecells + 1))
        trans = freecells + 1;

    /* Move cards to free cells */

    trans--;
    for (i = 0; i < (int)trans; i++)
        QueueTransfer(fcol, 0, TOPROW, free[i]);

    /* Transfer last card directly */

    QueueTransfer(fcol, 0, tcol, 0);

    /* Transfer from free cells back to column */

    for (i = (int)trans - 1; i >= 0; i--)
        QueueTransfer(TOPROW, free[i], tcol, 0);
}


/****************************************************************************
 * MultiMove
 *
 * Move from one non-empty column to another.  If the number of cards
 * exceeds what can be moved directly, uses free columns as temporary
 * storage.  Mirrors the original transfer.c MultiMove() exactly.
 ****************************************************************************/

void FreeCellGame::MultiMove(unsigned int fcol, unsigned int tcol)
{
    unsigned int freecol[MAXCOL];
    unsigned int freecells;
    unsigned int trans;
    unsigned int col, pos;
    int i;

    assert(fcol != TOPROW);
    assert(tcol != TOPROW);
    assert(card[fcol][0] != EMPTY);

    /* Count free cells */

    freecells = 0;
    for (pos = 0; pos < 4; pos++)
    {
        if (card[TOPROW][pos] == EMPTY)
            freecells++;
    }

    /* Find how many cards to move.  If too many for a direct transfer,
       push partial results into available empty columns. */

    trans = NumberToTransfer(fcol, tcol);

    if (trans > (freecells + 1))
    {
        i = 0;
        for (col = 1; col < MAXCOL; col++)
            if (card[col][0] == EMPTY)
                freecol[i++] = col;

        /* Transfer into free columns until a direct transfer can be made */

        i = 0;
        while (trans > (freecells + 1))
        {
            MoveCol(fcol, freecol[i]);
            trans -= (freecells + 1);
            i++;
        }

        MoveCol(fcol, tcol);

        /* Gather cards back from free columns */

        for (i--; i >= 0; i--)
            MoveCol(freecol[i], tcol);
    }
    else
    {
        MoveCol(fcol, tcol);
    }
}


/****************************************************************************
 * DoTransfer
 *
 * Execute a single transfer: update the card array bookkeeping.
 * This is the non-animated equivalent of the original Transfer() in
 * transfer.c -- it does everything except the HDC / Glide calls.
 ****************************************************************************/

void FreeCellGame::DoTransfer(unsigned int fcol, unsigned int fpos,
                              unsigned int tcol, unsigned int tpos)
{
    CARD c;

    assert(fpos < MAXPOS);
    assert(tpos < MAXPOS);

    if (fcol == TOPROW)
    {
        if ((fpos > 3) || (card[TOPROW][fpos] == IDGHOST))
            return;
    }
    else
    {
        fpos = FindLastPos(fcol);
        if (fpos == EMPTY)
            return;

        if (fcol == tcol)
            return;
    }

    if (tcol == TOPROW)
    {
        if (tpos > 3)                       // move to home cell
        {
            wCardCount--;
            c = card[fcol][fpos];
            home[SUIT(c)] = VALUE(c);
        }
    }
    else
    {
        tpos = FindLastPos(tcol) + 1;
    }

    c = card[fcol][fpos];
    card[fcol][fpos] = EMPTY;
    card[tcol][tpos] = c;

    if (VALUE(c) == ACE && tcol == TOPROW && tpos > 3)
        homesuit[SUIT(c)] = tpos;
}


/****************************************************************************
 * IsValidMove
 *
 * Determines if moving from wFromCol/wFromPos to tcol/tpos is valid.
 * Assumes wFromCol and tcol are not both non-empty columns (that case
 * is handled by MultiMove in the caller).
 *
 * Sets bMoveCol:
 *   true  -- user wants to move entire column
 *   false -- user wants to move a single card
 *   The caller should interpret (int)bMoveCol == -1 as CANCEL.
 *
 * Returns true if the move is legal, false otherwise.
 * Does NOT show any dialogs -- the caller is responsible for UI.
 ****************************************************************************/

bool FreeCellGame::IsValidMove(unsigned int tcol, unsigned int tpos)
{
    CARD fcard, tcard;
    unsigned int trans;
    unsigned int freecells;
    unsigned int pos;

    assert(tpos < MAXPOS);

    bMoveCol = false;

    /* Allow cancel: move from top row back to same slot */

    if (wFromCol == TOPROW && tcol == TOPROW && wFromPos == tpos)
        return true;

    fcard = card[wFromCol][wFromPos];
    tcard = card[tcol][tpos];

    /* Transfer to empty column (from a column, not TOPROW) */

    if ((wFromCol != TOPROW) && (tcol != TOPROW) && (card[tcol][0] == EMPTY))
    {
        trans = NumberToTransfer(wFromCol, tcol);
        freecells = 0;
        for (pos = 0; pos < 4; pos++)
            if (card[TOPROW][pos] == EMPTY)
                freecells++;

        if (freecells == 0 && trans > 1)
            trans = 1;

        if (trans == 0)
            return false;
        else if (trans == 1)
            return true;

        /*
         * Multiple cards can move -- the caller must ask the user to
         * disambiguate (single card vs. column move).  We return true
         * here; the caller should present the MoveCol dialog and set
         * bMoveCol accordingly before proceeding.
         *
         * Convention: caller sets bMoveCol to true (column), false
         * (single), or casts -1 for cancel.
         */
        bMoveCol = true;    // signal: disambiguation needed
        return true;
    }

    /* Transfer to TOPROW */

    if (tcol == TOPROW)
    {
        if (tpos < 4)                   // free cells
        {
            return (tcard == EMPTY);
        }
        else                            // home cells
        {
            if (VALUE(fcard) == ACE && tcard == EMPTY)
                return true;

            if (SUIT(fcard) == SUIT(tcard))
            {
                if (VALUE(fcard) == (VALUE(tcard) + 1))
                    return true;
                else
                    return false;
            }
            return false;
        }
    }
    else                                // tcol is not TOPROW
    {
        if (card[tcol][0] == EMPTY)
            return true;

        return FitsUnder(fcard, tcard);
    }
}


/****************************************************************************
 * ProcessDoubleClick
 *
 * Finds the first empty free cell and queues a move of the selected
 * card to it.  Returns true if a move was queued, false if no free
 * cell is available.
 ****************************************************************************/

bool FreeCellGame::ProcessDoubleClick()
{
    unsigned int freecell = EMPTY;
    int i;

    for (i = 3; i >= 0; i--)
        if (card[TOPROW][i] == EMPTY)
            freecell = (unsigned int)i;

    if (freecell == EMPTY)
        return false;

    SaveState();

    wFromPos = FindLastPos(wFromCol);
    QueueTransfer(wFromCol, wFromPos, TOPROW, freecell);
    Cleanup();
    wMouseMode = FROM;
    return true;
}


/****************************************************************************
 * GenerateRandomGameNum
 *
 * Returns a random game number from 1 to MAXGAMENUMBER.
 * Uses the platform's srand/rand (not WinRand), since this value is
 * only used as a seed for the user, not for the published shuffle.
 ****************************************************************************/

unsigned int FreeCellGame::GenerateRandomGameNum()
{
    unsigned int wGameNum;

    std::srand((unsigned int)std::time(nullptr));
    std::rand();
    std::rand();
    wGameNum = (unsigned int)std::rand();

    while (wGameNum < 1 || wGameNum > MAXGAMENUMBER)
        wGameNum = (unsigned int)std::rand();

    return wGameNum;
}
