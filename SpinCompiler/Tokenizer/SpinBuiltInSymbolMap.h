//////////////////////////////////////////////////////////////
//                                                          //
// Propeller Spin/PASM Compiler                             //
// (c)2012-2016 Parallax Inc. DBA Parallax Semiconductor.   //
// Adapted from Chip Gracey's x86 asm code by Roy Eltham    //
// Rewritten to modern C++ by Thilo Ackermann               //
// See end of file for terms of use.                        //
//                                                          //
////////////////////////////////////////////////////////////// 

#ifndef SPINCOMPILER_SPINBUILTINSYMBOLMAP_H
#define SPINCOMPILER_SPINBUILTINSYMBOLMAP_H

#include "SpinCompiler/Tokenizer/StringMap.h"
#include "SpinCompiler/Types/ConstantExpression.h"

class SpinBuiltInSymbolMap {
public:
    StringMap &stringMap;
private:
    struct InitEntry {
        Token::Type type;                   // what type of symbol is it?
        int             value;                  // value is type dependant
        const char*     name;                   // the string of the symbol
        unsigned char   operator_type_or_asm;   // operator type for op symbols, or asm value for dual symbols
        bool            dual;
    };
    struct BuiltInToken {
        BuiltInToken():type(Token::Undefined),value(0),opType(-1),asmOp(-1),dual(false) {}
        SpinAbstractSymbolP resolvedSymbol;
        SpinSymbolId symbolId;
        Token::Type type;
        int value;
        int opType;
        int asmOp;
        bool dual;
    };

    std::map<SpinSymbolId, BuiltInToken> m_map;
public:
    void hasSymbol(SpinSymbolId symbolId, Token& tk) const {
        tk.eof = false;
        auto it = m_map.find(symbolId);
        if (it == m_map.end()) {
            tk.type = Token::Undefined;
            tk.symbolId = symbolId;
            return;
        }

        tk.resolvedSymbol = it->second.resolvedSymbol;
        tk.symbolId = it->second.symbolId;
        tk.type = it->second.type;
        tk.value = it->second.value;
        tk.opType = OperatorType::Type(it->second.opType);
        tk.asmOp = it->second.asmOp;
        tk.dual = it->second.dual;
    }
    explicit SpinBuiltInSymbolMap(StringMap &stringMap):stringMap(stringMap) {
        std::vector<InitEntry> builtInSymbols = {
            {Token::LeftBracket,             0,                  "(",            0,                  false}, //miscellaneous
            {Token::RightBracket,            0,                  ")",            0,                  false},
            {Token::LeftIndex,            0,                  "[",            0,                  false},
            {Token::RightIndex,           0,                  "]",            0,                  false},
            {Token::Comma,            0,                  ",",            0,                  false},
            {Token::Equal,            0,                  "=",            0,                  false},
            {Token::Pound,            0,                  "#",            0,                  false},
            {Token::Colon,            0,                  ":",            0,                  false},
            {Token::Backslash,             0,                  "\\",           0,                  false},
            {Token::Dot,              0,                  ".",            0,                  false},
            {Token::DotDot,           0,                  "..",           0,                  false},
            {Token::At,               0,                  "@",            0,                  false},
            {Token::AtAt,             0,                  "@@",           0,                  false},
            {Token::Tilde,              0,                  "~",            0,                  false},
            {Token::TildeTilde,           0,                  "~~",           0,                  false},
            {Token::Random,              0,                  "?",            0,                  false},
            {Token::Inc,              0,                  "++",           0,                  false},
            {Token::Dec,              0,                  "--",           0,                  false},
            {Token::Assign,           0,                  ":=",           0,                  false},
            {Token::SPR,              0,                  "SPR",          0,                  false},

            {Token::Binary,           1,                  "->",           OperatorType::OpRor,             false}, // math operators
            {Token::Binary,           1,                  "<-",           OperatorType::OpRol,             false},
            {Token::Binary,           1,                  ">>",           OperatorType::OpShr,             false},
            {Token::Binary,           1,                  "<<",           OperatorType::OpShl,             false},
            {Token::Binary,           6,                  "#>",           OperatorType::OpMin,             false},
            {Token::Binary,           6,                  "<#",           OperatorType::OpMax,             false},
        //  {Token::type_unary,            0,                  "-",            OperatorType::op_neg,             false}, // (uses op_sub symbol)
            {Token::Unary,            0,                  "!",            OperatorType::OpNot,             false},
            {Token::Binary,           2,                  "&",            OperatorType::OpAnd,             false},
            {Token::Unary,            0,                  "||",           OperatorType::OpAbs,             false},
            {Token::Binary,           3,                  "|",            OperatorType::OpOr,              false},
            {Token::Binary,           3,                  "^",            OperatorType::OpXor,             false},
            {Token::Binary,           5,                  "+",            OperatorType::OpAdd,             false},
            {Token::Binary,           5,                  "-",            OperatorType::OpSub,             false},
            {Token::Binary,           1,                  "~>",           OperatorType::OpSar,             false},
            {Token::Binary,           1,                  "><",           OperatorType::OpRev,             false},
            {Token::Binary,           9,                  "AND",          OperatorType::OpLogAnd,         false},
            {Token::Unary,            0,                  ">|",           OperatorType::OpNcd,             false},
            {Token::Binary,           10,                 "OR",           OperatorType::OpLogOr,          false},
            {Token::Unary,            0,                  "|<",           OperatorType::OpDcd,             false},
            {Token::Binary,           4,                  "*",            OperatorType::OpMul,             false},
            {Token::Binary,           4,                  "**",           OperatorType::OpScl,             false},
            {Token::Binary,           4,                  "/",            OperatorType::OpDiv,             false},
            {Token::Binary,           4,                  "//",           OperatorType::OpRem,             false},
            {Token::Unary,            0,                  "^^",           OperatorType::OpSqr,             false},
            {Token::Binary,           7,                  "<",            OperatorType::OpCmpB,           false},
            {Token::Binary,           7,                  ">",            OperatorType::OpCmpA,           false},
            {Token::Binary,           7,                  "<>",           OperatorType::OpCmpNe,          false},
            {Token::Binary,           7,                  "==",           OperatorType::OpCmpE,           false},
            {Token::Binary,           7,                  "=<",           OperatorType::OpCmpBe,          false},
            {Token::Binary,           7,                  "=>",           OperatorType::OpCmpAe,          false},
            {Token::Unary,            8,                  "NOT",          OperatorType::OpLogNot,         false},

            {Token::Float,            0,                  "FLOAT",        0,                  false}, //floating-point operators
            {Token::Round,            0,                  "ROUND",        0,                  false},
            {Token::Trunc,            0,                  "TRUNC",        0,                  false},

            {Token::ConExpr,           0,                  "CONSTANT",     0,                  false}, //constant and string expressions
            {Token::ConStr,           0,                  "STRING",       0,                  false},

            {Token::Block,            BlockType::BlockCON,          "CON",          0,                  false}, //block designators
            {Token::Block,            BlockType::BlockVAR,          "VAR",          0,                  false},
            {Token::Block,            BlockType::BlockDAT,          "DAT",          0,                  false},
            {Token::Block,            BlockType::BlockOBJ,          "OBJ",          0,                  false},
            {Token::Block,            BlockType::BlockPUB,          "PUB",          0,                  false},
            {Token::Block,            BlockType::BlockPRI,          "PRI",          0,                  false},
            {Token::Block,            BlockType::BlockDEV,          "DEV",          0,                  false},

            {Token::Size,             0,                  "BYTE",         0,                  false}, //sizes
            {Token::Size,             1,                  "WORD",         0,                  false},
            {Token::Size,             2,                  "LONG",         0,                  false},

            {Token::PreCompile,       0,                  "PRECOMPILE",   0,                  false}, //file-related
            {Token::Archive,          0,                  "ARCHIVE",      0,                  false},
            {Token::File,             0,                  "FILE",         0,                  false},

            {Token::If,               0,                  "IF",           0,                  false}, //high-level structures
            {Token::IfNot,            0,                  "IFNOT",        0,                  false},
            {Token::ElseIf,           0,                  "ELSEIF",       0,                  false},
            {Token::ElseIfNot,        0,                  "ELSEIFNOT",    0,                  false},
            {Token::Else,             0,                  "ELSE",         0,                  false},
            {Token::Case,             0,                  "CASE",         0,                  false},
            {Token::Other,            0,                  "OTHER",        0,                  false},
            {Token::Repeat,           0,                  "REPEAT",       0,                  false},
            {Token::While,            0,                  "WHILE",        0,                  false},
            {Token::Until,            0,                  "UNTIL",        0,                  false},
            {Token::From,             0,                  "FROM",         0,                  false},
            {Token::To,               0,                  "TO",           0,                  false},
            {Token::Step,             0,                  "STEP",         0,                  false},

            {Token::NextQuit,      0,                  "NEXT",         0,                  false}, //high-level instructions
            {Token::NextQuit,      1,                  "QUIT",         0,                  false},
            {Token::Abort,   0,               "ABORT",        0,                  false},
            {Token::Return,   0,               "RETURN",       0,                  false},
            {Token::Look,           0x10,               "LOOKUP",       0,                  false},
            {Token::Look,           0x10 + 0x80,        "LOOKUPZ",      0,                  false},
            {Token::Look,           0x11,               "LOOKDOWN",     0,                  false},
            {Token::Look,           0x11 + 0x80,        "LOOKDOWNZ",    0,                  false},
            {Token::ClkMode,        0,                  "CLKMODE",      0,                  false},
            {Token::ClkFreq,        0,                  "CLKFREQ",      0,                  false},
            {Token::ChipVer,        0,                  "CHIPVER",      0,                  false},
            {Token::Reboot,         0,                  "REBOOT",       0,                  false},
            {Token::CogNew,         0 + (2 * 0x40),  "COGNEW",       0,                  false},
            {Token::InstAlwaysReturn,             0x16 + (1 * 0x40),  "STRSIZE",      0,                  false},
            {Token::InstAlwaysReturn,             0x17 + (2 * 0x40),  "STRCOMP",      0,                  false},
            {Token::InstNeverReturn,             0x18 + (3 * 0x40),  "BYTEFILL",     0,                  false},
            {Token::InstNeverReturn,             0x19 + (3 * 0x40),  "WORDFILL",     0,                  false},
            {Token::InstNeverReturn,             0x1A + (3 * 0x40),  "LONGFILL",     0,                  false},
            {Token::InstNeverReturn,             0x1C + (3 * 0x40),  "BYTEMOVE",     0,                  false},
            {Token::InstNeverReturn,             0x1D + (3 * 0x40),  "WORDMOVE",     0,                  false},
            {Token::InstNeverReturn,             0x1E + (3 * 0x40),  "LONGMOVE",     0,                  false},

            {Token::InstNeverReturn,             0x1B + (3 * 0x40),  "WAITPEQ",      0x3C,               true},  // dual mode instructions (spin and asm)
            {Token::InstNeverReturn,             0x1F + (3 * 0x40),  "WAITPNE",      0x3D,               true},
            {Token::InstNeverReturn,             0x23 + (1 * 0x40),  "WAITCNT",      0x3E + 0x40,        true},
            {Token::InstNeverReturn,             0x27 + (2 * 0x40),  "WAITVID",      0x3F,               true},
            {Token::InstNeverReturn,             0x20 + (2 * 0x40),  "CLKSET",       0 + 0x80,           true},
            {Token::CogId,          0,                  "COGID",        1 + 0x80 + 0x40,    true},
            {Token::CogInit,        0x2C + (3 * 0x40),  "COGINIT",      2 + 0x80,           true},
            {Token::InstNeverReturn,             0x21 + (1 * 0x40),  "COGSTOP",      3 + 0x80,           true},
            {Token::InstCanReturn,             0x29 + (0 * 0x40),  "LOCKNEW",      4 + 0x80 + 0x40,    true},
            {Token::InstNeverReturn,             0x22 + (1 * 0x40),  "LOCKRET",      5 + 0x80,           true},
            {Token::InstCanReturn,             0x2A + (1 * 0x40),  "LOCKSET",      6 + 0x80,           true},
            {Token::InstCanReturn,             0x2B + (1 * 0x40),  "LOCKCLR",      7 + 0x80,           true},

            {Token::AsmDir,          AsmDirective::DirOrgX,           "ORGX",         0,                  false}, //assembly directives
            {Token::AsmDir,          AsmDirective::DirOrg,            "ORG",          0,                  false},
            {Token::AsmDir,          AsmDirective::DirRes,            "RES",          0,                  false},
            {Token::AsmDir,          AsmDirective::DirFit,            "FIT",          0,                  false},
            {Token::AsmDir,          AsmDirective::DirNop,            "NOP",          0,                  false},

            {Token::AsmCond,         AsmConditionType::IfNcAndNz,       "IF_NC_AND_NZ", 0,                  false}, //assembly conditionals
            {Token::AsmCond,         AsmConditionType::IfNcAndNz,       "IF_NZ_AND_NC", 0,                  false},
            {Token::AsmCond,         AsmConditionType::IfNcAndNz,       "IF_A",         0,                  false},
            {Token::AsmCond,         AsmConditionType::IfNcAndZ,        "IF_NC_AND_Z",  0,                  false},
            {Token::AsmCond,         AsmConditionType::IfNcAndZ,        "IF_Z_AND_NC",  0,                  false},
            {Token::AsmCond,         AsmConditionType::IfNc,              "IF_NC",        0,                  false},
            {Token::AsmCond,         AsmConditionType::IfNc,              "IF_AE",        0,                  false},
            {Token::AsmCond,         AsmConditionType::IfCAndNz,        "IF_C_AND_NZ",  0,                  false},
            {Token::AsmCond,         AsmConditionType::IfCAndNz,        "IF_NZ_AND_C",  0,                  false},
            {Token::AsmCond,         AsmConditionType::IfNz,              "IF_NZ",        0,                  false},
            {Token::AsmCond,         AsmConditionType::IfNz,              "IF_NE",        0,                  false},
            {Token::AsmCond,         AsmConditionType::IfCNeZ,          "IF_C_NE_Z",    0,                  false},
            {Token::AsmCond,         AsmConditionType::IfCNeZ,          "IF_Z_NE_C",    0,                  false},
            {Token::AsmCond,         AsmConditionType::IfNcOrNz,        "IF_NC_OR_NZ",  0,                  false},
            {Token::AsmCond,         AsmConditionType::IfNcOrNz,        "IF_NZ_OR_NC",  0,                  false},
            {Token::AsmCond,         AsmConditionType::IfCAndZ,         "IF_C_AND_Z",   0,                  false},
            {Token::AsmCond,         AsmConditionType::IfCAndZ,         "IF_Z_AND_C",   0,                  false},
            {Token::AsmCond,         AsmConditionType::IfCEqZ,          "IF_C_EQ_Z",    0,                  false},
            {Token::AsmCond,         AsmConditionType::IfCEqZ,          "IF_Z_EQ_C",    0,                  false},
            {Token::AsmCond,         AsmConditionType::IfZ,               "IF_Z",         0,                  false},
            {Token::AsmCond,         AsmConditionType::IfZ,               "IF_E",         0,                  false},
            {Token::AsmCond,         AsmConditionType::IfNcOrZ,         "IF_NC_OR_Z",   0,                  false},
            {Token::AsmCond,         AsmConditionType::IfNcOrZ,         "IF_Z_OR_NC",   0,                  false},
            {Token::AsmCond,         AsmConditionType::IfC,               "IF_C",         0,                  false},
            {Token::AsmCond,         AsmConditionType::IfC,               "IF_B",         0,                  false},
            {Token::AsmCond,         AsmConditionType::IfCOrNz,         "IF_C_OR_NZ",   0,                  false},
            {Token::AsmCond,         AsmConditionType::IfCOrNz,         "IF_NZ_OR_C",   0,                  false},
            {Token::AsmCond,         AsmConditionType::IfCOrZ,          "IF_C_OR_Z",    0,                  false},
            {Token::AsmCond,         AsmConditionType::IfCOrZ,          "IF_Z_OR_C",    0,                  false},
            {Token::AsmCond,         AsmConditionType::IfCOrZ,          "IF_BE",        0,                  false},
            {Token::AsmCond,         AsmConditionType::IfAlways,          "IF_ALWAYS",    0,                  false},
            {Token::AsmCond,         AsmConditionType::IfNever,           "IF_NEVER",     0,                  false},

            {Token::AsmInst,         0,                  "WRBYTE",       0,                  false}, //assembly instructions
            {Token::AsmInst,         0x00 + 0x40,        "RDBYTE",       0,                  false},
            {Token::AsmInst,         0x01,               "WRWORD",       0,                  false},
            {Token::AsmInst,         0x01 + 0x40,        "RDWORD",       0,                  false},
            {Token::AsmInst,         0x02,               "WRLONG",       0,                  false},
            {Token::AsmInst,         0x02 + 0x40,        "RDLONG",       0,                  false},
            {Token::AsmInst,         0x03,               "HUBOP",        0,                  false},
            {Token::AsmInst,         0x04 + 0x40,        "MUL",          0,                  false},
            {Token::AsmInst,         0x05 + 0x40,        "MULS",         0,                  false},
            {Token::AsmInst,         0x06 + 0x40,        "ENC",          0,                  false},
            {Token::AsmInst,         0x07 + 0x40,        "ONES",         0,                  false},
            {Token::AsmInst,         0x08 + 0x40,        "ROR",          0,                  false},
            {Token::AsmInst,         0x09 + 0x40,        "ROL",          0,                  false},
            {Token::AsmInst,         0x0A + 0x40,        "SHR",          0,                  false},
            {Token::AsmInst,         0x0B + 0x40,        "SHL",          0,                  false},
            {Token::AsmInst,         0x0C + 0x40,        "RCR",          0,                  false},
            {Token::AsmInst,         0x0D + 0x40,        "RCL",          0,                  false},
            {Token::AsmInst,         0x0E + 0x40,        "SAR",          0,                  false},
            {Token::AsmInst,         0x0F + 0x40,        "REV",          0,                  false},
            {Token::AsmInst,         0x10 + 0x40,        "MINS",         0,                  false},
            {Token::AsmInst,         0x11 + 0x40,        "MAXS",         0,                  false},
            {Token::AsmInst,         0x12 + 0x40,        "MIN",          0,                  false},
            {Token::AsmInst,         0x13 + 0x40,        "MAX",          0,                  false},
            {Token::AsmInst,         0x14 + 0x40,        "MOVS",         0,                  false},
            {Token::AsmInst,         0x15 + 0x40,        "MOVD",         0,                  false},
            {Token::AsmInst,         0x16 + 0x40,        "MOVI",         0,                  false},
            {Token::AsmInst,         0x17 + 0x40,        "JMPRET",       0,                  false},
        //  {Token::type_asm_inst,         0x18 + 0x40,        "AND",          0,                  false}, //({type_binary_bool)
            {Token::AsmInst,         0x19 + 0x40,        "ANDN",         0,                  false},
        //  {Token::type_asm_inst,         0x1A + 0x40,        "OR",           0,                  false}, //({type_binary_bool)
            {Token::AsmInst,         0x1B + 0x40,        "XOR",          0,                  false},
            {Token::AsmInst,         0x1C + 0x40,        "MUXC",         0,                  false},
            {Token::AsmInst,         0x1D + 0x40,        "MUXNC",        0,                  false},
            {Token::AsmInst,         0x1E + 0x40,        "MUXZ",         0,                  false},
            {Token::AsmInst,         0x1F + 0x40,        "MUXNZ",        0,                  false},
            {Token::AsmInst,         0x20 + 0x40,        "ADD",          0,                  false},
            {Token::AsmInst,         0x21 + 0x40,        "SUB",          0,                  false},
            {Token::AsmInst,         0x22 + 0x40,        "ADDABS",       0,                  false},
            {Token::AsmInst,         0x23 + 0x40,        "SUBABS",       0,                  false},
            {Token::AsmInst,         0x24 + 0x40,        "SUMC",         0,                  false},
            {Token::AsmInst,         0x25 + 0x40,        "SUMNC",        0,                  false},
            {Token::AsmInst,         0x26 + 0x40,        "SUMZ",         0,                  false},
            {Token::AsmInst,         0x27 + 0x40,        "SUMNZ",        0,                  false},
            {Token::AsmInst,         0x28 + 0x40,        "MOV",          0,                  false},
            {Token::AsmInst,         0x29 + 0x40,        "NEG",          0,                  false},
            {Token::AsmInst,         0x2A + 0x40,        "ABS",          0,                  false},
            {Token::AsmInst,         0x2B + 0x40,        "ABSNEG",       0,                  false},
            {Token::AsmInst,         0x2C + 0x40,        "NEGC",         0,                  false},
            {Token::AsmInst,         0x2D + 0x40,        "NEGNC",        0,                  false},
            {Token::AsmInst,         0x2E + 0x40,        "NEGZ",         0,                  false},
            {Token::AsmInst,         0x2F + 0x40,        "NEGNZ",        0,                  false},
            {Token::AsmInst,         0x30,               "CMPS",         0,                  false},
            {Token::AsmInst,         0x31,               "CMPSX",        0,                  false},
            {Token::AsmInst,         0x32 + 0x40,        "ADDX",         0,                  false},
            {Token::AsmInst,         0x33 + 0x40,        "SUBX",         0,                  false},
            {Token::AsmInst,         0x34 + 0x40,        "ADDS",         0,                  false},
            {Token::AsmInst,         0x35 + 0x40,        "SUBS",         0,                  false},
            {Token::AsmInst,         0x36 + 0x40,        "ADDSX",        0,                  false},
            {Token::AsmInst,         0x37 + 0x40,        "SUBSX",        0,                  false},
            {Token::AsmInst,         0x38 + 0x40,        "CMPSUB",       0,                  false},
            {Token::AsmInst,         0x39 + 0x40,        "DJNZ",         0,                  false},
            {Token::AsmInst,         0x3A,               "TJNZ",         0,                  false},
            {Token::AsmInst,         0x3B,               "TJZ",          0,                  false},
            {Token::AsmInst,         0x15,               "CALL",         0,                  false}, //converts to 17h (jmpret symbol_ret,#symbol)
            {Token::AsmInst,         0x16,               "RET",          0,                  false}, //converts to 17h (jmp #0)
            {Token::AsmInst,         0x17,               "JMP",          0,                  false},
            {Token::AsmInst,         0x18,               "TEST",         0,                  false},
            {Token::AsmInst,         0x19,               "TESTN",        0,                  false},
            {Token::AsmInst,         0x21,               "CMP",          0,                  false},
            {Token::AsmInst,         0x33,               "CMPX",         0,                  false},

            {Token::AsmEffect,       0x04,               "WZ",           0,                  false}, //assembly effects
            {Token::AsmEffect,       0x02,               "WC",           0,                  false},
            {Token::AsmEffect,       0x01,               "WR",           0,                  false},
            {Token::AsmEffect,       0x08,               "NR",           0,                  false},

            {Token::AsmReg,              0x10,               "PAR",          0,                  false}, //registers
            {Token::AsmReg,              0x11,               "CNT",          0,                  false},
            {Token::AsmReg,              0x12,               "INA",          0,                  false},
            {Token::AsmReg,              0x13,               "INB",          0,                  false},
            {Token::AsmReg,              0x14,               "OUTA",         0,                  false},
            {Token::AsmReg,              0x15,               "OUTB",         0,                  false},
            {Token::AsmReg,              0x16,               "DIRA",         0,                  false},
            {Token::AsmReg,              0x17,               "DIRB",         0,                  false},
            {Token::AsmReg,              0x18,               "CTRA",         0,                  false},
            {Token::AsmReg,              0x19,               "CTRB",         0,                  false},
            {Token::AsmReg,              0x1A,               "FRQA",         0,                  false},
            {Token::AsmReg,              0x1B,               "FRQB",         0,                  false},
            {Token::AsmReg,              0x1C,               "PHSA",         0,                  false},
            {Token::AsmReg,              0x1D,               "PHSB",         0,                  false},
            {Token::AsmReg,              0x1E,               "VCFG",         0,                  false},
            {Token::AsmReg,              0x1F,               "VSCL",         0,                  false},

            {Token::Result,           0,                  "RESULT",       0,                  false}, //variables

            {Token::BuiltInIntegerConstant,              0,                  "FALSE",        0,                  false}, //constants
            {Token::BuiltInIntegerConstant,              -1,                 "TRUE",         0,                  false},
            {Token::BuiltInIntegerConstant,              ~0x7FFFFFFF,        "NEGX",         0,                  false},
            {Token::BuiltInIntegerConstant,              0x7FFFFFFF,         "POSX",         0,                  false},
            {Token::BuiltInFloatConstant,        0x40490FDB,         "PI",           0,                  false},

            {Token::BuiltInIntegerConstant,              0x00000001,         "RCFAST",       0,                  false},
            {Token::BuiltInIntegerConstant,              0x00000002,         "RCSLOW",       0,                  false},
            {Token::BuiltInIntegerConstant,              0x00000004,         "XINPUT",       0,                  false},
            {Token::BuiltInIntegerConstant,              0x00000008,         "XTAL1",        0,                  false},
            {Token::BuiltInIntegerConstant,              0x00000010,         "XTAL2",        0,                  false},
            {Token::BuiltInIntegerConstant,              0x00000020,         "XTAL3",        0,                  false},
            {Token::BuiltInIntegerConstant,              0x00000040,         "PLL1X",        0,                  false},
            {Token::BuiltInIntegerConstant,              0x00000080,         "PLL2X",        0,                  false},
            {Token::BuiltInIntegerConstant,              0x00000100,         "PLL4X",        0,                  false},
            {Token::BuiltInIntegerConstant,              0x00000200,         "PLL8X",        0,                  false},
            {Token::BuiltInIntegerConstant,              0x00000400,         "PLL16X",       0,                  false}
        };
        for (const auto& item:builtInSymbols) {
            auto symbolId = stringMap.getOrPutSymbolName(std::string(item.name));
            BuiltInToken tk;
            tk.symbolId = symbolId;
            tk.type = item.type;
            if (item.type == Token::BuiltInIntegerConstant || item.type == Token::BuiltInFloatConstant) {
                tk.type = Token::DefinedSymbol;
                tk.symbolId = SpinSymbolId();
                tk.resolvedSymbol = SpinAbstractSymbolP(new SpinConstantSymbol(ConstantValueExpression::create(SourcePosition(),item.value), item.type == Token::BuiltInIntegerConstant));
            }
            else
                tk.value = item.value;
            tk.dual = item.dual;
            if (tk.dual)
                tk.asmOp = item.operator_type_or_asm;
            else {
                tk.opType = item.operator_type_or_asm;
                // fixup for AND and OR to have asm also
                if (tk.type == Token::Binary && tk.opType == OperatorType::OpLogAnd)
                    tk.asmOp = 0x18 + 0x40;
                if (tk.type == Token::Binary && tk.opType == OperatorType::OpLogOr)
                    tk.asmOp = 0x1A + 0x40;
            }
            m_map[symbolId] = tk;
        }
    }
};

#endif //SPINCOMPILER_SPINBUILTINSYMBOLMAP_H

///////////////////////////////////////////////////////////////////////////////////////////
//                           TERMS OF USE: MIT License                                   //
///////////////////////////////////////////////////////////////////////////////////////////
// Permission is hereby granted, free of charge, to any person obtaining a copy of this  //
// software and associated documentation files (the "Software"), to deal in the Software //
// without restriction, including without limitation the rights to use, copy, modify,    //
// merge, publish, distribute, sublicense, and/or sell copies of the Software, and to    //
// permit persons to whom the Software is furnished to do so, subject to the following   //
// conditions:                                                                           //
//                                                                                       //
// The above copyright notice and this permission notice shall be included in all copies //
// or substantial portions of the Software.                                              //
//                                                                                       //
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,   //
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A         //
// PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT    //
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION     //
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE        //
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                                //
///////////////////////////////////////////////////////////////////////////////////////////
