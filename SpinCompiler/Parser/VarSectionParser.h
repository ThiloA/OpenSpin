//////////////////////////////////////////////////////////////
//                                                          //
// Propeller Spin/PASM Compiler                             //
// (c)2012-2016 Parallax Inc. DBA Parallax Semiconductor.   //
// Adapted from Chip Gracey's x86 asm code by Roy Eltham    //
// Rewritten to modern C++ by Thilo Ackermann               //
// See end of file for terms of use.                        //
//                                                          //
////////////////////////////////////////////////////////////// 

#ifndef SPINCOMPILER_VARSECTIONPARSER_H
#define SPINCOMPILER_VARSECTIONPARSER_H

#include "SpinCompiler/Parser/ConstantExpressionParser.h"
#include "SpinCompiler/Parser/ParserObjectContext.h"
#include "SpinCompiler/Parser/ParsedObject.h"

class VarSectionParser {
public:
private:
    TokenReader &m_reader;
    ParserObjectContext &m_objectContext;
public:
    explicit VarSectionParser(TokenReader& reader, ParserObjectContext& objectContext):m_reader(reader),m_objectContext(objectContext) {}
    void parseVarBlocks() {
        m_reader.reset();
        while (m_reader.getNextBlock(BlockType::BlockVAR)) {
            while (true) {
                const auto tk = m_reader.getNextNonBlockOrNewlineToken();
                if (tk.eof)
                    break;
                if (tk.type != Token::Size)
                    throw CompilerError(ErrorType::ebwol, tk);
                const int nSize = tk.value;
                while(true) {
                    // save a copy of the symbol
                    auto symbolId = m_reader.forceUnusedSymbol(ErrorType::eauvn).symbolId;
                    // see if there is a count
                    AbstractConstantExpressionP countExpression;
                    if (m_reader.checkElement(Token::LeftIndex)) {
                        // get the count value and validate it
                        countExpression = ConstantExpressionParser::tryResolveValueNonAsm(m_reader, m_objectContext.childObjectSymbols, true, true).expression;
                        m_reader.forceElement(Token::RightIndex); // get the closing bracket
                    }
                    else
                        countExpression = ConstantValueExpression::create(tk.sourcePosition,1); //no count was given, exact 1 variable

                    // add the symbol
                    auto varSym = SpinVarSectionSymbolP(new SpinVarSectionSymbol(tk.sourcePosition, countExpression, m_objectContext.currentObject->reserveNextVarSymbolId(), nSize));
                    m_objectContext.currentObject->globalVariables.push_back(varSym);
                    m_objectContext.globalSymbols.addSymbol(symbolId,varSym,0);
                    if (!m_reader.getCommaOrEnd())
                        break;
                }
            }
        }
    }
};

#endif //SPINCOMPILER_VARSECTIONPARSER_H

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
