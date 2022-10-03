//////////////////////////////////////////////////////////////
//                                                          //
// Propeller Spin/PASM Compiler                             //
// (c)2012-2016 Parallax Inc. DBA Parallax Semiconductor.   //
// Adapted from Chip Gracey's x86 asm code by Roy Eltham    //
// Rewritten to modern C++ by Thilo Ackermann               //
// See end of file for terms of use.                        //
//                                                          //
////////////////////////////////////////////////////////////// 

#ifndef SPINCOMPILER_MACROPREPROCESSOR_H
#define SPINCOMPILER_MACROPREPROCESSOR_H

#include "SpinCompiler/Types/CompilerError.h"
#include <string>
#include <map>

class MacroPreProcessor {
public:
    enum EndOfBlockType { EndOfFile,EndIf,Else,ElseIfDef,ElseIfNDef };
    enum MessageType {MessageError,MessageWarning,MessageInfo};
    explicit MacroPreProcessor(const std::string& source, std::string&dest, std::map<std::string,std::string>& macros, SourcePositionFile file):m_file(file),m_source(source),m_dest(dest),m_macros(macros),m_idx(0),m_lineNumber(1) {}
    void runFile() {
        if (runBlock(true) != EndOfFile)
            throw CompilerError(ErrorType::maceif,getSourcePosition());
    }
private:
    enum {NewLineChar=13};
    static bool isSpecifierStartChar(const char c) {
        return (c>='a' && c<='z') || (c>='A' && c<='Z') || c=='_';
    }
    static bool isSpecifierFollowingChar(const char c) {
        return isSpecifierStartChar(c) || (c>='0' && c<='9');
    }
    EndOfBlockType runBlock(bool writeOutput) { //return true on endif, otherwise false
        while (m_idx<m_source.size()) {
            if (isCommand("#ifdef"))
                runIfBlock(writeOutput, false);
            else if (isCommand("#ifndef"))
                runIfBlock(writeOutput, true);
            else if (isCommand("#else"))
                return Else;
            else if (isCommand("#elseifdef"))
                return ElseIfDef;
            else if (isCommand("#elseifndef"))
                return ElseIfNDef;
            else if (isCommand("#endif"))
                return EndIf;
            else if (writeOutput && isCommand("#define"))
                handleDefine();
            else if (writeOutput && isCommand("#undef"))
                handleUndef();
            else if (writeOutput && (isCommand("#warn") || isCommand("#warning")))
                handleMessage(MessageWarning);
            else if (writeOutput && isCommand("#info"))
                handleMessage(MessageInfo);
            else if (writeOutput && isCommand("#error"))
                handleMessage(MessageError);
            else if (writeOutput && isCommand("#include"))
                handleInclude();
            else
                runUntilEndOrCommand(writeOutput);
        }
        return EndOfFile;
    }
    void handleMessage(MessageType messageType) {
        skipWhitespace();
        auto pos = getSourcePosition();
        auto msg = readUntilEndOfLine();
        throw CompilerError(ErrorType::macerr,pos,msg); //TODO correct code, warning/info
    }
    void handleInclude() {
        throw CompilerError(ErrorType::internal,getSourcePosition()); //TODO implement include
    }
    void handleDefine() {
        skipWhitespace();
        auto macroName = readWord();
        skipWhitespace();
        auto valueBeforeReplace = readUntilEndOfLine();
        //replace macros
        std::string valueAfterReplace;
        MacroPreProcessor subPreProc(valueBeforeReplace, valueAfterReplace, m_macros, m_file);
        subPreProc.runUntilEnd();
        //TODO error/warning on redefine?
        m_macros[macroName] = valueAfterReplace;
    }
    void handleUndef() {
        skipWhitespace();
        auto macroName = readWord();
        skipUntilEndOfLine();
        m_macros.erase(macroName);
    }
    bool isCommand(const char*cmd) {
        int p=0;
        const int origIdx = m_idx;
        while (true) {
            const char c = cmd[p++];
            if (c == 0) {
                if (m_idx>=m_source.size() || (m_source[m_idx]>0 && m_source[m_idx]<=' '))
                    return true;
                m_idx = origIdx;
                return false;
            }
            if (m_idx>=m_source.size() || m_source[m_idx] != c) {
                m_idx = origIdx;
                return false;
            }
            m_idx++;
        }
    }
    void skipWhitespace() {
        while (m_idx<m_source.size() && m_source[m_idx]>0 && m_source[m_idx]<=' ' && m_source[m_idx] != NewLineChar)
            m_idx++;
    }
    void skipUntilEndOfLine() {
        while (m_idx<m_source.size()) {
            if (m_source[m_idx++]==NewLineChar) {
                m_dest.push_back(NewLineChar);
                break;
            }
        }
    }
    void runUntilEnd() {
        unsigned int lastWriteOutIndex = m_idx;
        while (m_idx<m_source.size()) {
            const char c = m_source[m_idx++];
            if (c == '\'') {
                --m_idx;
                break;
            }
            else if (c == '"')
                skipString(false);
            else if (isSpecifierStartChar(c)) //replacement only in active output
                handleSpecifier(lastWriteOutIndex);
        }
        m_dest.insert(m_dest.end(), m_source.begin()+lastWriteOutIndex, m_source.begin()+m_idx);
    }
    std::string readUntilEndOfLine() {
        const unsigned int startIdx = m_idx;
        while (m_idx<m_source.size()) {
            if (m_source[m_idx++]==NewLineChar) {
                m_dest.push_back(NewLineChar);
                return m_source.substr(startIdx, m_idx-startIdx-1);
            }
        }
        return m_source.substr(startIdx, m_idx-startIdx);
    }
    std::string readWord() {
        if (m_idx>=m_source.size() || !isSpecifierStartChar(m_source[m_idx]))
            throw CompilerError(ErrorType::macsp, getSourcePosition());
        const unsigned int startIdx = m_idx;
        m_idx++;
        while (m_idx<m_source.size() && isSpecifierFollowingChar(m_source[m_idx]))
            ++m_idx;
        return m_source.substr(startIdx, m_idx-startIdx);
    }
    bool evaluateMacroCondition() {
        skipWhitespace();
        const bool isDefinded = m_macros.find(readWord()) != m_macros.end();
        skipUntilEndOfLine();
        return isDefinded;
    }
    void runIfBlock(bool writeOutput, bool isInverted) {
        bool condition = evaluateMacroCondition() != isInverted;
        while (true) {
            auto nextIfCmd = runBlock(writeOutput && condition);
            if (condition) //if one block was true, all others are false
                writeOutput = false;
            if (nextIfCmd == ElseIfDef)
                condition = evaluateMacroCondition();
            else if (nextIfCmd == ElseIfNDef)
                condition = !evaluateMacroCondition();
            else if (nextIfCmd == EndIf) {
                skipUntilEndOfLine();
                return;
            }
            else if (nextIfCmd == Else) {
                if (runBlock(writeOutput) != EndIf)
                    throw CompilerError(ErrorType::maceif, getSourcePosition());
                return;
            }
        }
    }
    //runs until end of file or newline followed by #
    //if writeOutput is true, characters will be written to output, otherwise only newlines will be forwared to output
    void runUntilEndOrCommand(bool writeOutput) {
        unsigned int lastWriteOutIndex = m_idx;
        bool startOfLine=false;
        while (m_idx<m_source.size()) {
            const char c = m_source[m_idx++];
            if (c == NewLineChar) { //newline
                m_lineNumber++;
                startOfLine = true;
                if (!writeOutput)
                    m_dest.push_back(NewLineChar); //newlines will be added, if output is disabled
                continue;
            }
            if (c == '#' && startOfLine) {
                m_idx--;
                break;
            }
            startOfLine = false;
            if (c == '\'')
                skipLineComment();
            else if (c == '{')
                skipMultiLineComment();
            else if (c == '"')
                skipString(true);
            else if (writeOutput && isSpecifierStartChar(c)) //replacement only in active output
                handleSpecifier(lastWriteOutIndex);
        }
        if (writeOutput)
            m_dest.insert(m_dest.end(), m_source.begin()+lastWriteOutIndex, m_source.begin()+m_idx);
    }
    void skipLineComment() {
        while (m_idx<m_source.size() && m_source[m_idx]!=NewLineChar)
            ++m_idx;
    }
    void skipString(bool throwError) {
        while (m_idx<m_source.size() && m_source[m_idx] != NewLineChar) {
            if (m_source[m_idx++] == '"')
                return;
        }
        if (throwError)
            throw CompilerError(ErrorType::macstr, getSourcePosition());
    }
    void skipMultiLineComment() {
        int depth=1;
        while(depth>0 && m_idx<m_source.size()) {
            const char c = m_source[m_idx++];
            if (c == '{')
                depth++;
            else if (c == '}')
                depth--;
            else if (c == NewLineChar)
                m_lineNumber++;
        }
    }
    void handleSpecifier(unsigned int &lastWriteOutIndex) {
        int startOfSpecifier = m_idx-1; //read one specifier char already
        while (m_idx<m_source.size() && isSpecifierFollowingChar(m_source[m_idx]))
            m_idx++;
        auto m = m_macros.find(m_source.substr(startOfSpecifier,m_idx-startOfSpecifier));
        if (m == m_macros.end() || m->second.empty()) //ignore word, continue
            return;
        //replace
        m_dest.insert(m_dest.end(), m_source.begin()+lastWriteOutIndex, m_source.begin()+startOfSpecifier);
        lastWriteOutIndex = m_idx;
        m_dest += m->second;
    }
    SourcePosition getSourcePosition() {
        return SourcePosition(m_lineNumber,1,m_file);
    }
    const SourcePositionFile m_file;
    const std::string& m_source;
    std::string& m_dest;
    std::map<std::string,std::string>& m_macros;
    unsigned int m_idx;
    int m_lineNumber;
};

#endif //SPINCOMPILER_MACROPREPROCESSOR_H

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
