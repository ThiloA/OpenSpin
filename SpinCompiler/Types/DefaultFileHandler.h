//////////////////////////////////////////////////////////////
//                                                          //
// Propeller Spin/PASM Compiler                             //
// (c)2012-2016 Parallax Inc. DBA Parallax Semiconductor.   //
// Adapted from Chip Gracey's x86 asm code by Roy Eltham    //
// Rewritten to modern C++ by Thilo Ackermann               //
// See end of file for terms of use.                        //
//                                                          //
////////////////////////////////////////////////////////////// 

#ifndef SPINCOMPILER_DEFAULTFILEHANDLER_H
#define SPINCOMPILER_DEFAULTFILEHANDLER_H

#include "SpinCompiler/Types/AbstractFileHandler.h"
#include "SpinCompiler/Types/CompilerError.h"
#include <map>
#include <fstream>

class DefaultFileHandler : public AbstractFileHandler {
private:
    std::map<std::string, FileDescriptorP> m_files;
    std::vector<std::string> m_searchPath;
public:
    explicit DefaultFileHandler(const std::vector<std::string>& searchPath):m_searchPath(searchPath) {
    }
    virtual ~DefaultFileHandler() {
    }
    virtual FileDescriptorP findFile(const std::string& fileName, FileType fileType, FileDescriptorP parent, const SourcePosition& includedInPosition) {
        std::string modFileName = fileName;
        if (fileType == SpinFile) {
            if (modFileName.size()<5 || modFileName.substr(modFileName.size()-5) != ".spin")
                modFileName += ".spin";
        }
        auto entry = m_files.find(modFileName);
        if (entry != m_files.end())
            return entry->second;

        if (fileType == RootSpinFile) {
            std::ifstream fs(modFileName, std::ios_base::in | std::ios_base::binary);
            if (!fs)
                throw CompilerError(ErrorType::fnf, includedInPosition, modFileName);
            return readFile(fs, modFileName);
        }

        for (auto path:m_searchPath) {
            std::ifstream fs(path+modFileName, std::ios_base::in | std::ios_base::binary);
            if (fs)
                return readFile(fs, modFileName);
        }
        throw CompilerError(ErrorType::fnf, includedInPosition, modFileName);
    }
private:
    FileDescriptorP readFile(std::ifstream& fs, const std::string& fileName) {
        fs.seekg(0,std::ios::end);
        auto length = fs.tellg();
        fs.seekg(0,std::ios::beg);
        //TODO file too large error
        std::vector<unsigned char> buffer(length);
        fs.read(reinterpret_cast<char*>(buffer.data()),length);

        FileDescriptorP newFileDesc(new FileDescriptor(buffer, fileName));
        m_files[fileName] = newFileDesc;
        return newFileDesc;
    }
};

#endif //SPINCOMPILER_DEFAULTFILEHANDLER_H

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
