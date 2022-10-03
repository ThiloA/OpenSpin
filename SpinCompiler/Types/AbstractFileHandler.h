//////////////////////////////////////////////////////////////
//                                                          //
// Propeller Spin/PASM Compiler                             //
// (c)2012-2016 Parallax Inc. DBA Parallax Semiconductor.   //
// Adapted from Chip Gracey's x86 asm code by Roy Eltham    //
// Rewritten to modern C++ by Thilo Ackermann               //
// See end of file for terms of use.                        //
//                                                          //
////////////////////////////////////////////////////////////// 

#ifndef SPINCOMPILER_ABSTRACTFILEHANDLER_H
#define SPINCOMPILER_ABSTRACTFILEHANDLER_H

#include <string>
#include <vector>
#include <memory>

// The AbstractFileHandler manages searching and reading files used by the compiler.

#include "SpinCompiler/Types/SourcePosition.h"

class FileDescriptor {
public:
    FileDescriptor( const std::vector<unsigned char> &content, const std::string& fileName):content(content),fileName(fileName) {}
    const std::vector<unsigned char> content;
    const std::string fileName;
    virtual ~FileDescriptor() {}
};
typedef std::shared_ptr<FileDescriptor> FileDescriptorP;

class AbstractFileHandler {
public:
    enum FileType {
        RootSpinFile,               //the spin file of the root object is requested
        SpinFile,                   //a (non root) spin file is requested
        PreprocessorInclude,        //a file is included by the preprocessor, the parent file is given in parent
        BinaryDatFile               //a binary dat file is requested
    };
    virtual ~AbstractFileHandler() {}

    /*
     * This function searches for the file given by fileName and fileType and returns a descriptor
     * The file descriptor must be unique for each file: iff filetype is requested twice on the same file handler instance
     * with same parameters it must return the same pointer
     *
     * if fileType is PreprocessorInclude, then parent is set to the file containing the include directive, otherwise it is
     * nullptr
     *
     * if the file was not found or available, a CompilerError must be thrown
     */
    virtual FileDescriptorP findFile(const std::string& fileName, FileType fileType, FileDescriptorP parent, const SourcePosition& includedInPosition)=0;
};


#endif //SPINCOMPILER_ABSTRACTFILEHANDLER_H

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
