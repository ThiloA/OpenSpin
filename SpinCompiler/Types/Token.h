//////////////////////////////////////////////////////////////
//                                                          //
// Propeller Spin/PASM Compiler                             //
// (c)2012-2016 Parallax Inc. DBA Parallax Semiconductor.   //
// Adapted from Chip Gracey's x86 asm code by Roy Eltham    //
// Rewritten to modern C++ by Thilo Ackermann               //
// See end of file for terms of use.                        //
//                                                          //
////////////////////////////////////////////////////////////// 

#ifndef SPINCOMPILER_TOKEN_H
#define SPINCOMPILER_TOKEN_H

#include <vector>
#include <string>
#include "SpinCompiler/Types/StrongTypedefInt.h"
#include "SpinCompiler/Types/Symbols.h"
#include "SpinCompiler/Types/SourcePosition.h"

struct BlockType {
    enum Type {
        BlockCON = 0,
        BlockVAR,
        BlockDAT,
        BlockOBJ,
        BlockPUB,
        BlockPRI,
        BlockDEV
    };
};

struct OperatorType {
    // operator precedences (0=priority)
    // 0= -, !, ||, >|, |<, ^^  (unary)
    // 1= ->, <-, >>, << ~>, ><
    // 2= &
    // 3= |, ^
    // 4= *, **, /, //
    // 5= +, -
    // 6= #>, <#
    // 7= <, >, <>, ==, =<, =>
    // 8= NOT       (unary)
    // 9= AND
    // 10= OR
    enum Type {
        OpRor = 0,
        OpRol,
        OpShr,
        OpShl,
        OpMin,
        OpMax,
        OpNeg,
        OpNot,
        OpAnd,
        OpAbs,
        OpOr,
        OpXor,
        OpAdd,
        OpSub,
        OpSar,
        OpRev,
        OpLogAnd,
        OpNcd,
        OpLogOr,
        OpDcd,
        OpMul,
        OpScl,
        OpDiv,
        OpRem,
        OpSqr,
        OpCmpB,
        OpCmpA,
        OpCmpNe,
        OpCmpE,
        OpCmpBe,
        OpCmpAe,
        OpLogNot,
        OpFloat,
        OpRound,
        OpTrunc
    };
    static std::string toString(Type op, const std::string& left, const std::string& right) {
        switch(op)  {
            case OpRor: return left+"->"+right;
            case OpRol: return left+"<-"+right;
            case OpShr: return left+">>"+right;
            case OpShl: return left+"<<"+right;
            case OpMin: return left+"#>"+right;
            case OpMax: return left+"<#"+right;
            case OpNeg: return "-"+left;
            case OpNot: return "!"+left;
            case OpAnd: return left+"&"+right;
            case OpAbs: return "||"+left;
            case OpOr: return left+"|"+right;
            case OpXor: return left+"^"+right;
            case OpAdd: return left+"+"+right;
            case OpSub: return left+"-"+right;
            case OpSar: return left+"~>"+right;
            case OpRev: return left+"><"+right;
            case OpLogAnd: return left+" AND "+right;
            case OpNcd: return ">|"+left;
            case OpLogOr: return left+" OR "+right;
            case OpDcd: return "|<"+left;
            case OpMul: return left+"*"+right;
            case OpScl: return left+"**"+right;
            case OpDiv: return left+"/"+right;
            case OpRem: return left+"//"+right;
            case OpSqr: return "^"+left;
            case OpCmpB: return left+"<"+right;
            case OpCmpA: return left+">"+right;
            case OpCmpNe: return left+"<>"+right;
            case OpCmpE: return left+"="+right;
            case OpCmpBe: return left+"=<"+right;
            case OpCmpAe: return left+"=>"+right;
            case OpLogNot: return "NOT "+left;
            case OpFloat: return "FLOAT("+left+")";
            case OpRound: return "ROUND("+left+")";
            case OpTrunc: return "TRUNC("+left+")";
        }
        return std::string();
    }
};

struct AsmDirective {
    enum {
        DirOrgX = 0,
        DirOrg = 1,
        DirRes = 2,
        DirFit = 3,
        DirNop = 4
    };
};

struct AsmConditionType {
    enum {
        IfNever     = 0x0,
        IfNcAndNz   = 0x1,
        IfNcAndZ    = 0x2,
        IfNc        = 0x3,
        IfCAndNz    = 0x4,
        IfNz        = 0x5,
        IfCNeZ      = 0x6,
        IfNcOrNz    = 0x7,
        IfCAndZ     = 0x8,
        IfCEqZ      = 0x9,
        IfZ         = 0xA,
        IfNcOrZ     = 0xB,
        IfC         = 0xC,
        IfCOrNz     = 0xD,
        IfCOrZ      = 0xE,
        IfAlways    = 0xF
    };
};

struct Token {
    enum Type {
        Undefined = 0,              // (undefined symbol, must be 0)
        LeftBracket,                // (
        RightBracket,               // )
        LeftIndex,                  // [
        RightIndex,                 // ]
        Comma,                      // ,
        Equal,                      // =
        Pound,                      // #
        Colon,                      // :
        Backslash,                  /* \  */
        Dot,                        // .                                //10
        DotDot,                     // ..
        At,                         // @
        AtAt,                       // @@
        Tilde,                      // ~
        TildeTilde,                 // ~~
        Random,                     // ?
        Inc,                        // ++
        Dec,                        // --
        Assign,                     // :=
        SPR,                        // SPR                              //20
        Unary,                      // -, !, ||, etc.
        Binary,                     // +, -, *, /, etc.
        Float,                      // FLOAT
        Round,                      // ROUND
        Trunc,                      // TRUNC
        ConExpr,                    // CONSTANT
        ConStr,                     // STRING
        Block,                      // CON, VAR, DAT, OBJ, PUB, PRI
        Size,                       // BYTE, WORD, LONG
        PreCompile,                 // PRECOMPILE                       //30
        Archive,                    // ARCHIVE
        File,                       // FILE
        If,                         // IF
        IfNot,                      // IFNOT
        ElseIf,                     // ELSEIF
        ElseIfNot,                  // ELSEIFNOT
        Else,                       // ELSE
        Case,                       // CASE
        Other,                      // OTHER
        Repeat,                     // REPEAT                           //40
        RepeatCount,                // REPEAT count - different QUIT method
        While,                      // WHILE
        Until,                      // UNTIL
        From,                       // FROM
        To,                         // TO
        Step,                       // STEP
        NextQuit,                   // NEXT/QUIT
        Abort,                      // ABORT/RETURN
        Return,                     // ABORT/RETURN
        Look,                       // LOOKUP/LOOKDOWN
        ClkMode,                    // CLKMODE                          //50
        ClkFreq,                    // CLKFREQ
        ChipVer,                    // CHIPVER
        Reboot,                     // REBOOT
        CogId,                      // COGID
        CogNew,                     // COGNEW
        CogInit,                    // COGINIT
        InstAlwaysReturn,           // STRSIZE, STRCOMP - always returns value
        InstCanReturn,              // LOCKNEW, LOCKCLR, LOCKSET - can return value
        InstNeverReturn,            // BYTEFILL, WORDFILL, LONGFILL, etc. - never returns value
        InstDualOperation,          // WAITPEQ, WAITPNE, etc. - type_asm_inst or type_i_???     //60
        AsmOrg,                     // $ (without a hex digit following)
        AsmDir,                     // ORGX, ORG, RES, FIT, NOP
        AsmCond,                    // IF_C, IF_Z, IF_NC, etc
        AsmInst,                    // RDBYTE, RDWORD, RDLONG, etc.
        AsmEffect,                  // WZ, WC, WR, NR
        AsmReg,                     // PAR, CNT, INA, etc.
        Result,                     // RESULT
        BuiltInIntegerConstant,     // user constant integer (must be followed by type_con_float)
        BuiltInFloatConstant,       // user constant float
        End,                        // end-of-line c=0, end-of-file c=1     //70
        DefinedSymbol               // symbol listed in symbol table
    };

    //Token():type(Undefined),value(0),opType(-1),asmOp(-1),eof(false),dual(false) {}
    Token(SpinSymbolId symbolId, Type type, int value, const SourcePosition& sourcePosition):sourcePosition(sourcePosition),symbolId(symbolId),type(type),value(value),opType(-1),asmOp(-1),eof(false),dual(false) {}
    SpinAbstractSymbolP resolvedSymbol;
    SourcePosition sourcePosition;
    SpinSymbolId symbolId;
    Type type;
    int value;
    int opType;
    int asmOp;
    bool eof;
    bool dual;

    bool subToNeg() {
        if (type != Binary || opType != OperatorType::OpSub)
            return false;
        type = Unary;
        opType = OperatorType::OpNeg;
        value = 0;
        return true;
    }
    static int packSubroutineParameterCountAndIndexForInterpreter(int parameterCount, int subroutineIndex) {
        //both integers will be packed into a single for the propeller spin interpreter, format is fixed
        return (parameterCount<<8) | subroutineIndex;
    }
    int unpackBuiltInParameterCount() const {
        return (value>>6)&0xFF;
    }
    int unpackBuiltInByteCode() const {
        return value&0x3F;
    }
    bool isValidWordSymbol() const {
        return symbolId.valid();
    }
};

struct TokenList {
    std::vector<Token> tokens;
    TokenIndex indexOfFirstBlock;
};

#endif //SPINCOMPILER_TOKEN_H

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
