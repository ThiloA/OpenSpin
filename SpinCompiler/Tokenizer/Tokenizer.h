//////////////////////////////////////////////////////////////
//                                                          //
// Propeller Spin/PASM Compiler                             //
// (c)2012-2016 Parallax Inc. DBA Parallax Semiconductor.   //
// Adapted from Chip Gracey's x86 asm code by Roy Eltham    //
// Rewritten to modern C++ by Thilo Ackermann               //
// See end of file for terms of use.                        //
//                                                          //
////////////////////////////////////////////////////////////// 

#ifndef SPINCOMPILER_TOKENIZER_H
#define SPINCOMPILER_TOKENIZER_H

#include <string>
#include "SpinCompiler/Tokenizer/TextFileReader.h"
#include "SpinCompiler/Tokenizer/SpinBuiltInSymbolMap.h"
#include "SpinCompiler/Tokenizer/FloatParser.h"

class Tokenizer {
private:
    TextFileReader m_textFileReader;
    const SpinBuiltInSymbolMap &m_builtInSymbols;
    FloatParser m_floatParser;
    int m_sourceFlags;
public:
    static TokenList readTokenList(const SpinBuiltInSymbolMap &builtInSymbols, const std::string& sourceCode, SourcePositionFile file) {
        Tokenizer tokenizer(builtInSymbols,sourceCode,file);
        TokenList tokenList;
        while (true) {
            auto tk = tokenizer.getNextToken();
            if (tk.eof)
                break;
            tokenList.tokens.push_back(tk);
        }
        //generate block jump table
        int lastBlock = -1;
        for (int i=0; i<int(tokenList.tokens.size()); ++i) {
            if (tokenList.tokens[i].type != Token::Block)
                continue;
            if (lastBlock == -1)
                tokenList.indexOfFirstBlock = TokenIndex(i);
            else
                tokenList.tokens[lastBlock].opType = i-lastBlock; //jump amount to next block
            lastBlock = i;
        }
        if (lastBlock>=0)
            tokenList.tokens[lastBlock].opType = -1; //end of tbl
        return tokenList;
    }
private:
    Tokenizer(const SpinBuiltInSymbolMap &builtInSymbols, const std::string& sourceCode, SourcePositionFile file):
        m_textFileReader(sourceCode, file),
        m_builtInSymbols(builtInSymbols),
        m_sourceFlags(0) {
    }
    void updateSourcePositionOfToken(Token &tk) {
        tk.sourcePosition = m_textFileReader.sourcePosition();
    }

    unsigned int readFloatNumber(char *floatTempBuffer, int floatTempBufferPos, const SourcePosition& sourcePosition) {
        if (floatTempBufferPos>=FloatTempBufferSize)
            throw CompilerError(ErrorType::fpcmbw, sourcePosition);
        floatTempBuffer[--floatTempBufferPos] = 0; //end of float string

        //use strtod to convert floatTempBuffer to a float
        //strtod is locale dependent
        float floatValue = 0.0f;
        if (!m_floatParser.stringToFloat(floatTempBuffer, floatTempBufferPos, floatValue))
            throw CompilerError(ErrorType::fpcmbw, sourcePosition);
        return UnaryConstantExpression::floatResult(floatValue);
    }

    unsigned int readFloatNumberAfterPoint(char *floatTempBuffer, int floatTempBufferPos, const SourcePosition &sourcePosition) {
        //[0-9]+ followed by '.' followed by [0-9]
        while(true) {
            if (m_textFileReader.nextCharIf('_')) // skip over _'s
                continue;
            const char currentChar = m_textFileReader.peekChar();
            if (floatTempBufferPos<FloatTempBufferSize)
                floatTempBuffer[floatTempBufferPos++] = currentChar;
            if (currentChar>='0' && currentChar<='9') {
                m_textFileReader.nextChar();
                continue;
            }
            if (currentChar=='e' || currentChar == 'E') {
                m_textFileReader.nextChar();
                return readFloatNumberAfterExponent(floatTempBuffer,floatTempBufferPos,sourcePosition);
            }
            //other character, parse float
            return readFloatNumber(floatTempBuffer,floatTempBufferPos,sourcePosition);
        }
    }

    unsigned int readFloatNumberAfterExponent(char *floatTempBuffer, int floatTempBufferPos, const SourcePosition &sourcePosition) {
        //[0-9]+(.[0-9]+)? followed by [eE]
        while(m_textFileReader.nextCharIf('_')) { //skip _
        }
        //read sign (optional)
        if (floatTempBufferPos<FloatTempBufferSize && m_textFileReader.nextCharIf('-'))
            floatTempBuffer[floatTempBufferPos++] = '-';
        else
            m_textFileReader.nextCharIf('+');
        //read exponent
        while(true) {
            if (m_textFileReader.nextCharIf('_')) // skip over _'s
                continue;
            const char currentChar = m_textFileReader.peekChar();
            if (floatTempBufferPos<FloatTempBufferSize)
                floatTempBuffer[floatTempBufferPos++] = currentChar;
            if (currentChar<'0' || currentChar>'9')
                break;
            m_textFileReader.nextChar();
        }
        return readFloatNumber(floatTempBuffer, floatTempBufferPos, sourcePosition);
    }

    void readNumber(int constantBase, Token& tk) {
        // this handles reading in a constant of base 2, 4, 10, or 16
        // the constantBase value is set based on prefix characters handled below
        char floatTempBuffer[FloatTempBufferSize+1]; //one extra for null char
        int floatTempBufferPos = 0;
        bool isInteger = true;
        bool integerOverflow = false;
        unsigned int tmpValue = 0;
        while(true) {
            if (m_textFileReader.nextCharIf('_')) // skip over _'s
                continue;
            const char currentChar = m_textFileReader.peekChar();
            if (floatTempBufferPos<FloatTempBufferSize)
                floatTempBuffer[floatTempBufferPos++] = currentChar;
            const int digitValue = parseDigit(currentChar, constantBase);
            if (digitValue>=0) {
                m_textFileReader.nextChar();
                unsigned int oldValue = tmpValue;
                tmpValue = tmpValue*constantBase+digitValue; // multiply accumulate the constant
                if (tmpValue<oldValue) // check for overflow
                    integerOverflow = true;
                continue;
            }
            if (constantBase == 10 && currentChar == '.' && parseDigit(m_textFileReader.peekChar(1), constantBase)>=0) {
                //[0-9]+ followed by '.' followed by [0-9]
                isInteger = false;
                m_textFileReader.nextChar();
                tmpValue = readFloatNumberAfterPoint(floatTempBuffer, floatTempBufferPos, tk.sourcePosition);
                break;
            }
            if (constantBase == 10 && (currentChar == 'e' || currentChar == 'E')) {
                //[0-9]+ followed by [eE]
                isInteger = false;
                m_textFileReader.nextChar();
                tmpValue = readFloatNumberAfterExponent(floatTempBuffer, floatTempBufferPos, tk.sourcePosition);
                break;
            }
            //read integer
            if (integerOverflow)
                throw CompilerError(ErrorType::ce32b, tk.sourcePosition);
            break;
        }
        tk.type = Token::DefinedSymbol;
        tk.resolvedSymbol = SpinAbstractSymbolP(new SpinConstantSymbol(ConstantValueExpression::create(tk.sourcePosition, tmpValue), isInteger));
    }

    void skipMultiLineComment(Token& tk) {
        // brace comment
        // read the whole comment, handling doc comments as needed
        int braceCommentLevel = 1;
        bool bDocComment = false;
        if (m_textFileReader.nextCharIf('{')) { // skip over second {
            bDocComment = true;
            m_textFileReader.nextCharIf(13);
        }
        while (true) {
            const char currentChar = m_textFileReader.nextChar();
            if (currentChar == 0) {
                updateSourcePositionOfToken(tk);
                throw CompilerError(bDocComment ? ErrorType::erbb : ErrorType::erb, tk.sourcePosition);
            }
            else if (!bDocComment && currentChar == '{')
                braceCommentLevel++;
            else if (currentChar == '}') {
                if (bDocComment && m_textFileReader.nextCharIf('}')) // skip over second }
                    break;
                else if (!bDocComment) {
                    braceCommentLevel--;
                    if (braceCommentLevel < 1)
                        break;
                }
            }
        }
        updateSourcePositionOfToken(tk);
    }

    void skipSingleLineComment(Token& tk) {
        // comment
        // read until end of line or file, handle doc comment
        bool bDocComment = false;
        if (m_textFileReader.nextCharIf('\'')) // skip over second '
            bDocComment = true;
        while (true) {
            const char currentChar = m_textFileReader.nextChar();
            if (currentChar == 0) {
                tk.type = Token::End;
                tk.eof = true;
                break;
            }
            if (currentChar == 13) {
                tk.type = Token::End;
                break;
            }
        }
    }

    void readStringSingleCharacter(Token &tk) {
        // old string? (continue parsing a string)
        // for strings, m_sourceFlags will start out 0, and then cycle between 1 and 2 for
        // each character of the string, when it is 1, a type_comma is returned, when it is
        // 2 the next character is returned
        // return a comma element between each character of the string
        if (m_sourceFlags == 1) {
            m_sourceFlags++;
            tk.type = Token::Comma;
            return;
        }
        const char firstChar = m_textFileReader.nextChar();
        // reset flag
        m_sourceFlags = 0;

        // check for errors
        if (firstChar == '\"')
            throw CompilerError(ErrorType::es, tk.sourcePosition);
        if (firstChar == 0)
            throw CompilerError(ErrorType::eatq, tk.sourcePosition);
        if (firstChar == 13)
            throw CompilerError(ErrorType::eatq, tk.sourcePosition);

        // return the character

        // check the next character, if it's not a " then setup so the next
        // call returns a type_comma, if it is a ", then we are done with this string
        // and we leave the offset pointing after the "
        if (!m_textFileReader.nextCharIf('\"'))
            m_sourceFlags++;

        // return the character constant
        tk.type = Token::DefinedSymbol;
        tk.resolvedSymbol = SpinAbstractSymbolP(new SpinConstantSymbol(ConstantValueExpression::create(tk.sourcePosition, firstChar & 0xFF), true));
    }

    void readStringStart(Token &tk) {
        // new string (start parsing a string)
        // we got here because m_sourceFlags was 0 and the character is a "
        // get first character of string
        const char chr = m_textFileReader.nextChar();

        // check for errors
        if (chr == '\"')
            throw CompilerError(ErrorType::es, tk.sourcePosition);
        if (chr == 0)
            throw CompilerError(ErrorType::eatq, tk.sourcePosition);
        if (chr == 13)
            throw CompilerError(ErrorType::eatq, tk.sourcePosition);

        // return the character in value
        tk.value = chr & 0x000000FF;

        // check the next character, it's it's not a " then setup so the next
        // call returns a type_comma, if it is a " then it means it's a one character
        // string and we leave the offset pointing after the "
        if (!m_textFileReader.nextCharIf('\"'))
            m_sourceFlags = 1; // cause the next call to return a type_comma
        // return the character constant
        tk.type = Token::DefinedSymbol;
        tk.resolvedSymbol = SpinAbstractSymbolP(new SpinConstantSymbol(ConstantValueExpression::create(tk.sourcePosition, tk.value & 0xFF), true));
    }

    Token getNextToken() {
        // default to type_undefined
        Token tk(SpinSymbolId(),Token::Undefined,0,m_textFileReader.sourcePosition());

        // setup source and symbol pointers
        while(true) {
            if (m_sourceFlags != 0) {
                readStringSingleCharacter(tk);
                return tk;
            }
            const char firstChar = m_textFileReader.peekChar();
            if (firstChar >= '0' && firstChar <= '9') { // dec
                readNumber(10, tk);
                return tk;
            }
            m_textFileReader.nextChar();
            if (firstChar == '\"') {
                readStringStart(tk);
                return tk;
            }
            if (firstChar == 0) { // eof
                tk.type = Token::End;
                tk.eof = true;
                updateSourcePositionOfToken(tk);
                return tk;
            }
            if (firstChar == 13) { // eol
                updateSourcePositionOfToken(tk);
                tk.type = Token::End;
                return tk;
            }
            if (firstChar <= ' ') { // space or tab?
                updateSourcePositionOfToken(tk);
                continue;
            }
            if (firstChar == '\'') {
                skipSingleLineComment(tk);
                return tk;
            }
            if (firstChar == '{') {
                skipMultiLineComment(tk);
                updateSourcePositionOfToken(tk); //TODO already done in skipMultiLineComment?
                continue;
            }
            if (firstChar == '}') // unmatched brace comment end
                throw CompilerError(ErrorType::bmbpbb, tk.sourcePosition);
            if (firstChar == '%') {
                // binary
                const int constantBase = m_textFileReader.nextCharIf('%') ? 4 : 2;
                if (parseDigit(m_textFileReader.peekChar(), constantBase)<0)
                    throw CompilerError(ErrorType::idbn, tk.sourcePosition);
                readNumber(constantBase, tk);
                return tk;
            }
            if (firstChar == '$') { // hex
                if (parseDigit(m_textFileReader.peekChar(), 16)<0) {
                    tk.type = Token::AsmOrg;
                    return tk;
                }
                readNumber(16, tk);
                return tk;
            }
            if (checkWordChar(uppercase(firstChar))) { // symbol
                readWordSymbol(tk,uppercase(firstChar));
                return tk;
            }
            readNonWordSymbol(tk, firstChar);
            return tk;
        }
    }

    void readWordSymbol(Token &tk, char firstChar) {
        std::string symbolName;
        symbolName.reserve(64);
        symbolName.push_back(firstChar);
        while (true) {
            const char currentChar = uppercase(m_textFileReader.peekChar());
            if (!checkWordChar(currentChar))
                break;
            symbolName.push_back(currentChar);
            m_textFileReader.nextChar();
        }
        auto symbolId = m_builtInSymbols.stringMap.getOrPutSymbolName(symbolName);
        m_builtInSymbols.hasSymbol(symbolId, tk);
    }

    void readNonWordSymbol(Token &tk, char firstChar) {
        std::string symbolName;
        symbolName.reserve(4);
        symbolName.push_back(uppercase(firstChar));
        const char c2 = m_textFileReader.peekChar();
        const char c3 = m_textFileReader.peekChar(1);
        if (c2>' ') {
            symbolName.push_back(uppercase(c2));
            if (c3)
                symbolName.push_back(uppercase(c3));
        }
        while (symbolName.size()>0) {
            m_builtInSymbols.hasSymbol(m_builtInSymbols.stringMap.hasSymbolName(symbolName), tk);
            tk.symbolId = SpinSymbolId();
            if (tk.type != Token::Undefined) {
                if (symbolName.size()>1)
                    m_textFileReader.nextChar();
                if (symbolName.size()>2)
                    m_textFileReader.nextChar();
                return;
            }
            symbolName.erase(symbolName.end()-1,symbolName.end());
        }
        throw CompilerError(ErrorType::uc, tk.sourcePosition);
    }
    static int parseDigit(const char c, const int base) {
        int digitValue=-1;
        if (c>='0' && c<='9')
            digitValue = c-'0';
        else if (c>='A' && c<='F')
            digitValue = c-'A'+10;
        else if (c>='a' && c<='f')
            digitValue = c-'a'+10;
        else
            return -1;
        if (digitValue>=base)
            return -1;
        return digitValue;
    }
    static char uppercase(const char theChar) {
        if (theChar >= 'a' && theChar <= 'z')
            return theChar - ('a' - 'A');
        return theChar;
    }
    // assumes theChar has been uppercased.
    static bool checkWordChar(const char theChar) {
        return ((theChar >= '0' && theChar <= '9') || (theChar == '_') || (theChar >= 'A' && theChar <= 'Z'));
    }

    enum { FloatTempBufferSize = 127 };
};

#endif //SPINCOMPILER_TOKENIZER_H

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
