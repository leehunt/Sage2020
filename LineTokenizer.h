#pragma once

#include <specstrings.h>

// Lexer tokens: Plain Text and Delimiters
enum class TK {
  tk_RESCAN_ = -2,  // Dummy: force rescan
  tkERROR,          // Lexer error

  tkNil = 0,  // No token at all
  tkEND_OBJ,  // End of object

  tkPARA,      // 0xB6 (Para mark)
  tkNEWLINE,   // \n
  tkWSPC,      // Blanks, tabs
  tkWSPCMULT,  // Multiple Blanks
  tkTAB,       // Tab character
  tkWORD,      // E.g. abc
  tkINTEGER,   // E.g. 123
  tkPUNCT,     // @#$%^&*()!

  tkCOMMA = 10,   // ,
  tkPERIOD,       // .
  tkEXCLAIM,      // !
  tkPOUND,        // #
  tkDOLLAR,       // $
  tkPERCENT,      // %
  tkAMPER,        // &
  tkLPAREN,       // (
  tkRPAREN,       // )
  tkASTER,        // *
  tkPLUS = 20,    // +
  tkMINUS,        // -
  tkSLASH,        // /
  tkCOLON,        // :
  tkSEMI,         // ;
  tkLESSTHAN,     // <
  tkEQUAL,        // =
  tkGREATERTHAN,  // >
  tkQUEST,        // ?
  tkATSIGN,       // @
  tkLBRACK = 30,  // [
  tkRBRACK,       // ]
  tkBSLASH,       // "\\"
  tkCARET,        // ^
  tkUSCORE,       // _
  tkBQUOTE,       // `
  tkLBRACE,       // {
  tkRBRACE,       // }
  tkVBAR,         // |
  tkTILDE,        // ~
  tkDQUOTE = 40,  // "
  tkLDQUOTE,      // " left curly dbl
  tkRDQUOTE,      // " right curly dbl
  tkQUOTE,        // '
  tkLQUOTE,       // ' left curly sgl
  tkRQUOTE,       // ' right curly sgl
  tkLCHEVRON,     // << French LDQuote
  tkRCHEVRON,     // >> French RDQuote
  tkENDASH,       // - en-dash
  tkEMDASH,       // -- em-dash

  tkLESSTHANEQUAL = 50,  // <=
  tkGREATERTHANEQUAL,    // >=
  tkEQ,                  // ==
  tkNEQ,                 // !=
  tkLOGICALOR,           // ||
  tkLOGICALAND,          // &&
  tkLSHIFT,              // <<
  tkRSHIFT,              // >>

  tkOTHER = 58,  // anything else
};

// Character translation table for ANSI_CHARSET
enum class tkCH : int {
  uNill = 0,        // No token at all
  uWSPACE_IND = 1,  // Blanks, CRs
  uTAB_IND,         // Tabs
  uNEWLINE,         // Newline
  uIDENT,           // Identifiers, words
  uINTEGER,         // Integer numbers
  uPERIOD,          // .
  uCOMMA,           // ,
  uDELIM,           // Delimiters eg :?
  uNIL,             // '\0'
  uIGNORE,          // Ignore token (WS)
  uSYMBOL,          // Some unknown char
  uWHITESPACE,      // Blanks as tks
  uFSPEC,           // fSpec character
  uFECHAR,          // Far East char
  uFE_SPCIND,       // Far East Space
  uFESYMBOL,        // Far East symbol
};

// NOTE: these two structures must have the first four elements the same!
struct TOK {
  enum TK tk;
  char* szVal;
  char* pchEnd;
  char chSav;  // internal: saved character replaced with NIL
  enum TK tkPrev;
};

struct CHTOK {
  enum tkCH tkch;
  const char* szVal;
  char* pchEnd;
  char chSav;  // internal: saved character replaced with NIL
  enum tkCH tkchPrev;
};

bool FGetTokDirect(TOK* ptok);
bool FGetTok(TOK* ptok);
void AttachTokToLine(TOK* ptok, __in_z char* szLine);
void UnattachTok(TOK* ptok);
