//////////////////////////////////////////////////////////////
//                                                          //
// Propeller Spin/PASM Compiler                             //
// (c)2012-2016 Parallax Inc. DBA Parallax Semiconductor.   //
// Adapted from Chip Gracey's x86 asm code by Roy Eltham    //
// Rewritten to modern C++ by Thilo Ackermann               //
// See end of file for terms of use.                        //
//                                                          //
////////////////////////////////////////////////////////////// 

#ifndef SPINCOMPILER_TOKENREADER_H
#define SPINCOMPILER_TOKENREADER_H

#include "SpinCompiler/Tokenizer/SymbolMap.h"
#include "SpinCompiler/Types/Token.h"
#include "SpinCompiler/Types/CompilerError.h"
#include "SpinCompiler/Types/ConstantExpression.h"

class TokenReader {
private:
    const SymbolMap &m_globalSymbols;
    SymbolMap *m_localSymbols;
    const TokenList& m_tokenList;
    TokenIndex m_tokenIndex;
private:
public:
    TokenReader(const TokenList &tokenList, const SymbolMap &globalSymbols):
          m_globalSymbols(globalSymbols),
          m_localSymbols(nullptr),
          m_tokenList(tokenList),
          m_tokenIndex(0)
    {
    }

    void reset() {
        m_tokenIndex = TokenIndex(0);
    }

    void goBack() {
        if (m_tokenIndex.value()>0)
            m_tokenIndex = TokenIndex(m_tokenIndex.value()-1);
    }

    void skipToken() {
        getNextToken();
    }

    Token forceUnusedSymbol(ErrorType err) {
        Token tk = getNextToken();
        if (tk.type != Token::Undefined)
            throw CompilerError(err, tk);
        return tk;
    }

    SourcePosition getSourcePosition() const {
        if (m_tokenIndex.value()<int(m_tokenList.tokens.size()))
            return m_tokenList.tokens[m_tokenIndex.value()].sourcePosition;
        if (!m_tokenList.tokens.empty())
            return m_tokenList.tokens.back().sourcePosition;
        return SourcePosition();
    }

    Token getNextToken() {
        if (m_tokenIndex.value()>=int(m_tokenList.tokens.size())) {
            m_tokenIndex = TokenIndex(m_tokenIndex.value()+1);
            Token tk(SpinSymbolId(),Token::End,0,m_tokenList.tokens.empty() ? SourcePosition() : m_tokenList.tokens.back().sourcePosition);
            tk.eof = true;
            return tk;
        }

        auto tk = m_tokenList.tokens[m_tokenIndex.value()];
        m_tokenIndex = TokenIndex(m_tokenIndex.value()+1);
        if (tk.type == Token::Undefined && m_localSymbols) {
            if (auto symbol = m_localSymbols->hasSymbol(tk.symbolId, 0)) {
                tk.type = Token::DefinedSymbol;
                tk.resolvedSymbol = symbol;
            }
        }
        if (tk.type == Token::Undefined) {
            if (auto symbol = m_globalSymbols.hasSymbol(tk.symbolId, 0)) {
                tk.type = Token::DefinedSymbol;
                tk.resolvedSymbol = symbol;
            }
        }
        return tk;
    }

    void forceElement(Token::Type type) {
        Token tk = getNextToken();
        if (tk.type != type) {
            ErrorType errorNum = ErrorType::internal;
            switch (type) {
                case Token::LeftBracket: errorNum = ErrorType::eleft; break;
                case Token::RightBracket: errorNum = ErrorType::eright; break;
                case Token::LeftIndex: errorNum = ErrorType::eleftb; break;
                case Token::RightIndex: errorNum = ErrorType::erightb; break;
                case Token::Comma: errorNum = ErrorType::ecomma; break;
                case Token::Pound: errorNum = ErrorType::epound; break;
                case Token::Colon: errorNum = ErrorType::ecolon; break;
                case Token::Dot: errorNum = ErrorType::edot; break;
                case Token::End: errorNum = ErrorType::eeol; break;
                default: break; //should not happen
            }
            throw CompilerError(errorNum, tk.sourcePosition);
        }
    }

    Token getNextNonNewlineToken() {
        while (true) {
            Token tk = getNextToken();
            if (tk.type != Token::End || tk.eof) //ignore newlines
                return tk;
        }
    }

    Token getNextNonBlockOrNewlineToken() {
        while (true) {
            Token tk = getNextToken();
            if (tk.type == Token::End && !tk.eof) //ignore newlines
                continue;
            if (tk.type == Token::Block) {
                goBack();
                tk = Token(SpinSymbolId(), Token::End, 0, tk.sourcePosition);
                tk.eof = true;
            }
            return tk;
        }
    }

    bool getNextBlock(BlockType::Type type) {
        if (m_tokenIndex.value() == 0) {
            if (!m_tokenList.indexOfFirstBlock.valid())
                return false;
            m_tokenIndex = m_tokenList.indexOfFirstBlock;
        }
        while(true) {
            auto tk = getNextToken();
            if (tk.eof)
                return false;
            if (tk.type != Token::Block)
                continue;
            if (tk.value == type) {
                if (tk.sourcePosition.column != 1)
                    throw CompilerError(ErrorType::bdmbifc, tk);
                return true;
            }
            const int jumpBy = tk.opType;
            if (jumpBy<0)
                return false;
            m_tokenIndex = TokenIndex(m_tokenIndex.value()+jumpBy-1); //getNextToken() already incremented by 1
        }
    }

    // check if next element is of the given type, if so return true, if not, backup and return false
    bool checkElement(Token::Type type) {
        if (getNextToken().type == type)
            return true;
        goBack();
        return false;
    }

    bool getCommaOrEnd() {
        auto tk = getNextToken();
        if (tk.type == Token::Comma)
            return true;
        if (tk.type != Token::End)
            throw CompilerError(ErrorType::ecoeol, tk);
        return false;
    }

    bool getCommaOrRight() {
        auto tk = getNextToken();
        if (tk.type == Token::Comma)
            return true;
        if (tk.type != Token::RightBracket)
            throw CompilerError(ErrorType::ecor, tk);
        return false;
    }

    bool getPipeOrEnd() {
        auto tk = getNextToken();
        if (tk.type == Token::Binary && tk.opType == OperatorType::OpOr)
            return true;
        if (tk.type != Token::End)
            throw CompilerError(ErrorType::epoeol, tk);
        return false;
    }

    std::string readFileNameString() {
        std::string result;
        while (true) {
            auto tk = getNextToken();
            auto conSym = std::dynamic_pointer_cast<SpinConstantSymbol>(tk.resolvedSymbol);
            int chrCode = 0;
            if (!conSym || !conSym->isInteger || !conSym->expression->isConstant(&chrCode))
                throw CompilerError(ErrorType::ifufiq, tk);

            // check for illegal characters in filename
            if (chrCode > 127 || chrCode < 32 || chrCode == '\\' || chrCode == '/' ||
                chrCode == ':' || chrCode == '*' || chrCode == '?' || chrCode == '\"' ||
                chrCode == '<' || chrCode == '>' || chrCode == '|') {
                throw CompilerError(ErrorType::ifc, tk);
            }
            result.push_back(chrCode); // add character
            if (!checkElement(Token::Comma))
                return result;
        }
        return result;
    }

    void setupLocalSymbolMap(SymbolMap *localSymbols) { //TODO weg
        m_localSymbols = localSymbols;
    }
};


#endif //SPINCOMPILER_TOKENREADER_H

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
