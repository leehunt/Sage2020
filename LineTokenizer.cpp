#include "pch.h"

#include <cassert>
#include "LineTokenizer.h"

#define CCH_MAX_MACRO 1024

#define SYM 9

// clang-format off
const int _vchtblAnsi[256] =
    //  0   1   2   3   4   5   6   7   8   9   a   b   c   d   e   f
    {
        //  nul soh stx etx eot enq ack bel bs  tab lf  vt  np  cr  so  si 0
        // FUTURE:  The Ox000b entry is bogus because we lose keyword lookup on
        // SYM
        //            Change was made for W96 bug#127554
        7, 7, 7, SYM, SYM, 7, SYM, 7, 7, 8, 5, 6, 5, 5, 5, SYM,

        //  dle dc1 dc2 dc3 dc4 nak syn etb can em  eof esc fs  gs  rs  us 1
        SYM, 7, SYM, 7, 7, 7, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM,

        //  sp  !   "   #   $   %   &   '   (   )   *   +   ,   -   .   / 2
        0, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 4, 6, 3, 6,

        //  0   1   2   3   4   5   6   7   8   9   :   ;   <   =   >   ? 3
        2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 6, 6, 6, 6, 6, 6,

        //  @   A   B   C   D   E   F   G   H   I   J   K   L   M   N   O 4
        6, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,

        //  P   Q   R   S   T   U   V   W   X   Y   Z   [   \   ]   ^   _ 5
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 6, 6, 6, 6, 6,

        //  `   a   b   c   d   e   f   g   h   i   j   k   l   m   n   o 6
        6, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,

        //  p   q   r   s   t   u   v   w   x   y   z   {   |   }   ~   del 7
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 6, 6, 6, 6, SYM,

        //                                                                        8
        SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM,
        SYM, SYM,

        //                                                                        9
        SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM,
        SYM, SYM,

        //                                                                        a
        SYM, SYM, SYM, SYM, 7, SYM, SYM, SYM, SYM, SYM, SYM, 6, SYM, SYM, SYM,
        SYM,

        //                                                                        b
        SYM, SYM, SYM, SYM, SYM, SYM, 5, 7, SYM, SYM, SYM, 6, SYM, SYM, SYM,
        SYM,

        //                                                                        c
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,

        //                                                                        d
        1, 1, 1, 1, 1, 1, 1, SYM, 1, 1, 1, 1, 1, 1, 1, 1,

        //                                                                        e
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,

        //                                                                        f
        1, 1, 1, 1, 1, 1, 1, SYM, 1, 1, 1, 1, 1, 1, 1, 1,

        //  0   1   2   3   4   5   6   7   8   9   a   b   c   d   e   f
};

static tkCH __inline ItkFromState(int uState) {
  return (tkCH)(uState >> 8);
}

// Final state last-character inclusion flag values
#define uINCLUDE (0U)  // Include last char
#define uEXCLUDE (1U)  // Exclude last char

// Final state values
#define WIX (((int)tkCH::uWSPACE_IND << 8) | uEXCLUDE)
#define TII (((int)tkCH::uTAB_IND << 8) | uINCLUDE)
#define NLI (((int)tkCH::uNEWLINE << 8) | uINCLUDE)
#define IDX (((int)tkCH::uIDENT << 8) | uEXCLUDE)
#define INX (((int)tkCH::uINTEGER << 8) | uEXCLUDE)
#define PDI (((int)tkCH::uPERIOD << 8) | uINCLUDE)
#define CMI (((int)tkCH::uCOMMA << 8) | uINCLUDE)
#define DLI (((int)tkCH::uDELIM << 8) | uINCLUDE)
#define SPI (((int)tkCH::uNIL << 8) | uEXCLUDE)
#define IGX (((int)tkCH::uIGNORE << 8) | uEXCLUDE)
#define SYI (((int)tkCH::uSYMBOL << 8) | uINCLUDE)
#define WSX (((int)tkCH::uWHITESPACE << 8) | uEXCLUDE)
#define FSI (((int)tkCH::uFSPEC << 8) | uINCLUDE)
#define FEI (((int)tkCH::uFECHAR << 8) | uINCLUDE)
#define FEX (((int)tkCH::uFECHAR << 8) | uEXCLUDE)
#define FSX (((int)tkCH::uFE_SPCIND << 8) | uEXCLUDE)
#define FYI (((int)tkCH::uFESYMBOL << 8) | uINCLUDE)

// State Transition Table, Tabs are whitespace, return whitespace indirectly
#define WSttblNumRows 6   // Num rows in Sttbl
#define WSttblNumCols 16  // Num cols in Sttbl

// State Transition Table, Tabs are whitespace, return whitespace indirectly
static unsigned int const _vrgsttblWsIndirect[WSttblNumRows][WSttblNumCols] =
    {
/* SCANNING 0   1   2   3   4   5   6   7   8   9   10  11  12  13  14  15   */
/* FOR:     sp  a-z 0-9 .   ,   nl  del spl tab sym fsp dbc dsp dsy ch1 khl  */

/* 0     */ 1,  2,  3,  PDI,CMI,NLI,DLI,SPI,TII,SYI,FSI,FEI,4,  FYI,0,  5,
/* 1:ws  */ 1,  WIX,WIX,WIX,WIX,WIX,WIX,WIX,WIX,WIX,WIX,WIX,4,  WIX,WIX,WIX,
/* 2:id  */ IDX,2,  IDX,IDX,IDX,IDX,IDX,IDX,IDX,IDX,IDX,IDX,IDX,IDX,IDX,IDX,
/* 3:nn  */ INX,INX,3,  INX,INX,INX,INX,INX,INX,INX,INX,INX,INX,INX,INX,INX,
/* 4:ds  */ 4,  FSX,FSX,FSX,FSX,FSX,FSX,FSX,FSX,FSX,FSX,FSX,4,  FSX,FSX,FSX,
/* 5:hid */ FEX,FEX,FEX,FEX,FEX,FEX,FEX,FEX,FEX,FEX,FEX,FEX,FEX,FEX,FEX,5,
    };

// State Transition Table, Tabs are separate tokens, return whitespace as tks
static unsigned int const _vrgsttblWsDirect[WSttblNumRows][WSttblNumCols]=
    {
/* SCANNING 0   1   2   3   4   5   6   7   8   9   10  11  12  13  14  15   */
/* FOR:     sp  a-z 0-9 .   ,   nl  del spl tab sym fsp dbc dsp dsy ch1 khl  */

/* 0     */ 1,  2,  3,  PDI,CMI,NLI,DLI,SPI,DLI,SYI,FSI,FEI,4,  FYI,0,  5,
/* 1:ws  */ 1,  WSX,WSX,WSX,WSX,WSX,WSX,WSX,WSX,WSX,WSX,WSX,4,  WIX,WIX,WIX,
/* 2:id  */ IDX,2,  IDX,IDX,IDX,IDX,IDX,IDX,IDX,IDX,IDX,IDX,IDX,IDX,IDX,IDX,
/* 3:nn  */ INX,INX,3,  INX,INX,INX,INX,INX,INX,INX,INX,INX,INX,INX,INX,INX,
/* 4:ds  */ 4,  WSX,WSX,WSX,WSX,WSX,WSX,WSX,WSX,WSX,WSX,WSX,4,  WSX,WSX,FSX,
/* 5:hid */ FEX,FEX,FEX,FEX,FEX,FEX,FEX,FEX,FEX,FEX,FEX,FEX,FEX,FEX,FEX,5,
    };
// clang-format on

static enum tkCH _TkchLexText(
    __in_z char** pszLine,
    unsigned int const rgsttblWs[WSttblNumRows][WSttblNumCols]) {
  char* pch = *pszLine;
  unsigned int iuState = 0;

  /* DFA: Scan for the limits of the token (rough scan) */
  while (TRUE) {
    unsigned char ch = *pch;
    /* Read and translate a character into a column in psttblStateTable */
    int isttCol = _vchtblAnsi[ch];

    /* Find new state (or final state) from current state and column */
    if ((iuState = rgsttblWs[iuState][isttCol]) > WSttblNumRows) {
      break;
    }
    pch++;
  }

  if (!(iuState & uEXCLUDE))
    pch++;
  *pszLine = pch;

  return ItkFromState(iuState);
}

static BOOL FGetCharTok(CHTOK* ptok) {
  if (ptok->tkch == tkCH::uNIL || ptok->tkchPrev == tkCH::uNill)
    return FALSE;  // out of string

  ptok->tkchPrev = ptok->tkch;

  // restore NIL
  *ptok->pchEnd = ptok->chSav;
  ptok->szVal = ptok->pchEnd;

  ptok->tkch = _TkchLexText(&ptok->pchEnd, _vrgsttblWsIndirect);

  // terminate
  ptok->chSav = *ptok->pchEnd;
  *ptok->pchEnd = L'\0';

  return TRUE;
}

static BOOL FGetCharTokDirect(CHTOK* ptok) {
  if (ptok->tkchPrev == tkCH::uNill)
    return FALSE;  // out of string

  ptok->tkchPrev = ptok->tkch;

  // restore NIL
  *ptok->pchEnd = ptok->chSav;
  ptok->szVal = ptok->pchEnd;

  ptok->tkch = _TkchLexText(&ptok->pchEnd, _vrgsttblWsDirect);

  // terminate
  ptok->chSav = *ptok->pchEnd;
  *ptok->pchEnd = L'\0';

  return TRUE;
}

static BOOL FExpandTokenChar(__in TOK* ptokBegin /*in/out*/,
                             __in const TOK* ptokEnd) {
  char* szValT;
  assert(ptokBegin);
  assert(ptokEnd);
  szValT = ptokBegin->szVal;

  *ptokBegin = *ptokEnd;
  ptokBegin->szVal = szValT;

  return TRUE;
}

static BOOL FRestoreBuf(const TOK* ptokOld, const TOK* ptokNew) {
  *ptokNew->pchEnd = ptokNew->chSav;
  *ptokOld->pchEnd = L'\0';

  return TRUE;
}

static BOOL FGetTokCore(TOK* ptok, BOOL fDirect) {
  BOOL fRet;
LNextTok:
  if (fDirect)
    fRet = FGetCharTokDirect((CHTOK*)ptok);
  else
    fRet = FGetCharTok((CHTOK*)ptok);
  if (!fRet)
    goto LDone;

  switch ((tkCH)ptok->tk) {
    case tkCH::uWSPACE_IND:
    case tkCH::uTAB_IND:
      goto LNextTok;

    case tkCH::uIDENT:
      ptok->tk = TK::tkWORD;
      break;
    case tkCH::uINTEGER:
      ptok->tk = TK::tkINTEGER;
      break;

    case tkCH::uWHITESPACE:
      ptok->tk = TK::tkWSPC;
      break;

    case tkCH::uSYMBOL:
    case tkCH::uFECHAR:
    case tkCH::uFESYMBOL:
    case tkCH::uFE_SPCIND:
      ptok->tk = TK::tkWORD;  // REVIEW: correct?
      break;

    case tkCH::uFSPEC:
      ptok->tk = TK::tkOTHER;
      break;

    case tkCH::uNEWLINE:
      ptok->tk = TK::tkNEWLINE;
      break;

    case tkCH::uPERIOD:
      ptok->tk = TK::tkPERIOD;
      break;

    case tkCH::uCOMMA:
      ptok->tk = TK::tkCOMMA;
      break;

    case 0:  // REVIEW: is this the only symbol?
    case tkCH::uNIL:
      ptok->tk = TK::tkNil;
      break;

    case tkCH::uDELIM:
      switch (*ptok->szVal) {
        // dual-character tokens
        case L'<': {
          TOK tokNext = *ptok;

          fRet = FGetTokCore(&tokNext, fDirect);
          if (!fRet)
            goto LDone;

          switch (tokNext.tk) {
            case TK::tkEQUAL:
              fRet = FExpandTokenChar(ptok, &tokNext);
              if (!fRet)
                goto LDone;
              ptok->tk = TK::tkLESSTHANEQUAL;
              break;
            case TK::tkLESSTHAN:
              fRet = FExpandTokenChar(ptok, &tokNext);
              if (!fRet)
                goto LDone;
              ptok->tk = TK::tkLSHIFT;
              break;

            default:
              fRet = FRestoreBuf(ptok, &tokNext);
              if (!fRet)
                goto LDone;
              ptok->tk = TK::tkLESSTHAN;
              break;
          }
          break;
        }
        case L'>': {
          TOK tokNext = *ptok;

          fRet = FGetTokCore(&tokNext, fDirect);
          if (!fRet)
            goto LDone;

          switch (tokNext.tk) {
            case TK::tkEQUAL:
              fRet = FExpandTokenChar(ptok, &tokNext);
              if (!fRet)
                goto LDone;
              ptok->tk = TK::tkGREATERTHANEQUAL;
              break;
            case TK::tkGREATERTHAN:
              fRet = FExpandTokenChar(ptok, &tokNext);
              if (!fRet)
                goto LDone;
              ptok->tk = TK::tkLSHIFT;
              break;

            default:
              fRet = FRestoreBuf(ptok, &tokNext);
              if (!fRet)
                goto LDone;
              ptok->tk = TK::tkGREATERTHAN;
              break;
          }
          break;
        }
        case L'=': {
          TOK tokNext = *ptok;

          fRet = FGetTokCore(&tokNext, fDirect);
          if (!fRet)
            goto LDone;

          if (tokNext.tk == TK::tkEQUAL) {
            fRet = FExpandTokenChar(ptok, &tokNext);
            if (!fRet)
              goto LDone;
            ptok->tk = TK::tkEQ;
          } else {
            fRet = FRestoreBuf(ptok, &tokNext);
            if (!fRet)
              goto LDone;
            ptok->tk = TK::tkEQUAL;
          }
          break;
        }
        case L'!': {
          TOK tokNext = *ptok;

          fRet = FGetTokCore(&tokNext, fDirect);
          if (!fRet)
            goto LDone;

          if (tokNext.tk == TK::tkEQUAL) {
            fRet = FExpandTokenChar(ptok, &tokNext);
            if (!fRet)
              goto LDone;
            ptok->tk = TK::tkNEQ;
          } else {
            fRet = FRestoreBuf(ptok, &tokNext);
            if (!fRet)
              goto LDone;
            ptok->tk = TK::tkEXCLAIM;
          }
          break;
        }
        case L'|': {
          TOK tokNext = *ptok;

          fRet = FGetTokCore(&tokNext, fDirect);
          if (!fRet)
            goto LDone;

          if (tokNext.tk == TK::tkVBAR) {
            fRet = FExpandTokenChar(ptok, &tokNext);
            if (!fRet)
              goto LDone;
            ptok->tk = TK::tkLOGICALOR;
          } else {
            fRet = FRestoreBuf(ptok, &tokNext);
            if (!fRet)
              goto LDone;
            ptok->tk = TK::tkVBAR;
          }
          break;
        }
        case L'&': {
          TOK tokNext = *ptok;

          fRet = FGetTokCore(&tokNext, fDirect);
          if (!fRet)
            goto LDone;

          if (tokNext.tk == TK::tkAMPER) {
            fRet = FExpandTokenChar(ptok, &tokNext);
            if (!fRet)
              goto LDone;
            ptok->tk = TK::tkLOGICALAND;
          } else {
            fRet = FRestoreBuf(ptok, &tokNext);
            if (!fRet)
              goto LDone;
            ptok->tk = TK::tkAMPER;
          }
          break;
        }
        case L'\\': {
          TOK tokNext = *ptok;

          fRet = FGetTokCore(&tokNext, fDirect);
          if (!fRet)
            goto LDone;

          switch (tokNext.tk) {
            case TK::tkDQUOTE:
            case TK::tkBSLASH:
              *ptok = tokNext;
              break;

            default:
              fRet = FRestoreBuf(ptok, &tokNext);
              if (!fRet)
                goto LDone;
              break;
          }
          break;
        }

        // single character tokens
        case L'%':
          ptok->tk = TK::tkPERCENT;
          break;
        case L'(':
          ptok->tk = TK::tkLPAREN;
          break;
        case L')':
          ptok->tk = TK::tkRPAREN;
          break;
        case L'*':
          ptok->tk = TK::tkASTER;
          break;
        case L'+':
          ptok->tk = TK::tkPLUS;
          break;
        case L'-':
          ptok->tk = TK::tkMINUS;
          break;
        case L'/':
          ptok->tk = TK::tkSLASH;
          break;
        case L':':
          ptok->tk = TK::tkCOLON;
          break;
        case L';':
          ptok->tk = TK::tkSEMI;
          break;
        case L'?':
          ptok->tk = TK::tkQUEST;
          break;
        case L'@':
          ptok->tk = TK::tkATSIGN;
          break;
        case L'[':
          ptok->tk = TK::tkLBRACK;
          break;
        case L']':
          ptok->tk = TK::tkRBRACK;
          break;
        case L'`':
          ptok->tk = TK::tkBQUOTE;
          break;
        case L'{':
          ptok->tk = TK::tkLBRACE;
          break;
        case L'}':
          ptok->tk = TK::tkRBRACE;
          break;
        case L'~':
          ptok->tk = TK::tkTILDE;
          break;
        case L'"':
          ptok->tk = TK::tkDQUOTE;
          break;
        case L'$':
          ptok->tk = TK::tkDOLLAR;
          break;
        case L'_':
          ptok->tk = TK::tkUSCORE;
          break;
        case L'#':
          ptok->tk = TK::tkPOUND;
          break;
        case L'\'':
          ptok->tk = TK::tkQUOTE;
          break;
        case L'^':
          ptok->tk = TK::tkCARET;
          break;
        case L'\t':
          assert(fDirect);
          ptok->tk = TK::tkTAB;  // direct only
          break;

        default:
          assert(FALSE);
          break;
      }
      break;

    default:
      assert(FALSE);
      break;
  }

LDone:
  return fRet;
}

BOOL FGetTokDirect(TOK* ptok) {
  return FGetTokCore(ptok, TRUE /*fDirect*/);
}

BOOL FGetTok(TOK* ptok) {
  return FGetTokCore(ptok, FALSE /*fDirect*/);
}

void AttachTokToLine(TOK* ptok, __in_z char* szLine) {
  ptok->tk = TK::tkERROR;
  ptok->tkPrev = TK::tkERROR;
  ptok->pchEnd = szLine;
  ptok->chSav = *szLine;
}

void UnattachTok(TOK* ptok) {
#if _DEBUG
  ptok->szVal = NULL;
#endif                          // DBG
  *ptok->pchEnd = ptok->chSav;  // restore NIL
#if _DEBUG
  ptok->pchEnd = NULL;
  ptok->tk = TK::tkERROR;
#endif  // DBG
}
