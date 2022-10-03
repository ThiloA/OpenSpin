//////////////////////////////////////////////////////////////
//                                                          //
// Propeller Spin/PASM Compiler                             //
// (c)2012-2016 Parallax Inc. DBA Parallax Semiconductor.   //
// Adapted from Chip Gracey's x86 asm code by Roy Eltham    //
// Rewritten to modern C++ by Thilo Ackermann               //
// See end of file for terms of use.                        //
//                                                          //
////////////////////////////////////////////////////////////// 

#ifndef SPINCOMPILER_PARSEROBJECTCONTEXT_H
#define SPINCOMPILER_PARSEROBJECTCONTEXT_H

#include "SpinCompiler/Types/Token.h"
#include "SpinCompiler/Types/CompilerError.h"
#include "SpinCompiler/Tokenizer/SymbolMap.h"

typedef std::shared_ptr<class ParsedObject> ParsedObjectP;
class AbstractParser;

struct ParserObjectContext {
    explicit ParserObjectContext(AbstractParser *parser, ParsedObjectP currentObject):
        parser(parser),
        currentObject(currentObject) {}
    AbstractParser *parser;
    ParsedObjectP currentObject;
    SymbolMap childObjectSymbols; //con and pub of child objects
    SymbolMap globalSymbols; //con, pub, pri, var, dat of this object

    std::shared_ptr<SpinSubSymbol> getObjMethod(const Token& tkin, ObjectClassId objClass) {
        if (auto ptr = childObjectSymbols.hasSpecificSymbol<SpinSubSymbol>(tkin.symbolId, objClass.value()))
            return ptr;
        throw CompilerError(ErrorType::easn, tkin);
    }

    std::shared_ptr<SpinConstantSymbol> getObjConstant(const Token& tkin, ObjectClassId objClass) {
        if (tkin.symbolId.valid()) { //nur echte symbole
            if (auto conSym = childObjectSymbols.hasSpecificSymbol<SpinConstantSymbol>(tkin.symbolId, objClass.value()))
                return conSym;
        }
        throw CompilerError(ErrorType::eacn, tkin);
    }
};

#endif //SPINCOMPILER_PARSEROBJECTCONTEXT_H

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
