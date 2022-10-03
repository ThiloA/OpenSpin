//////////////////////////////////////////////////////////////
//                                                          //
// Propeller Spin/PASM Compiler                             //
// (c)2012-2016 Parallax Inc. DBA Parallax Semiconductor.   //
// Adapted from Chip Gracey's x86 asm code by Roy Eltham    //
// Rewritten to modern C++ by Thilo Ackermann               //
// See end of file for terms of use.                        //
//                                                          //
////////////////////////////////////////////////////////////// 

#ifndef SPINCOMPILER_TEXTFILEREADER_H
#define SPINCOMPILER_TEXTFILEREADER_H

#include <string>
#include "SpinCompiler/Types/SourcePosition.h"

class TextFileReader {
private:
    SourcePosition m_currentSourcePosition;
    const std::string& m_sourceCode;
    int m_sourceIndex;
public:
    TextFileReader(const std::string& sourceCode, SourcePositionFile file):m_currentSourcePosition(1,1,file),m_sourceCode(sourceCode),m_sourceIndex(0) {}
    const SourcePosition& sourcePosition() const {
        return m_currentSourcePosition;
    }
    char peekChar(int offset=0) {
        return m_sourceCode[m_sourceIndex+offset];
    }

    char nextChar() {
        char c = m_sourceCode[m_sourceIndex];
        if (c == 13) {
            m_currentSourcePosition.line++;
            m_currentSourcePosition.column = 0;
        }
        else if (c == 9) { //tab char
            int col = m_currentSourcePosition.column;
            if (col == 0)
                col = 8;
            else if ((col &7) == 0) {
                //col++;
            }
            else {
                col |= 7;
                col++;
            }
            m_currentSourcePosition.column = col;
        }
        if (c != 0) {
            ++m_sourceIndex;
            m_currentSourcePosition.column++;
        }
        return c;
    }

    bool nextCharIf(char c) {
        if (c != m_sourceCode[m_sourceIndex])
            return false;
        nextChar();
        return true;
    }
};

#endif //SPINCOMPILER_TEXTFILEREADER_H

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
