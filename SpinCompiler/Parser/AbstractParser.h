//////////////////////////////////////////////////////////////
//                                                          //
// Propeller Spin/PASM Compiler                             //
// (c)2012-2016 Parallax Inc. DBA Parallax Semiconductor.   //
// Adapted from Chip Gracey's x86 asm code by Roy Eltham    //
// Rewritten to modern C++ by Thilo Ackermann               //
// See end of file for terms of use.                        //
//                                                          //
////////////////////////////////////////////////////////////// 

#ifndef SPINCOMPILER_ABSTRACTPARSER_H
#define SPINCOMPILER_ABSTRACTPARSER_H

#include "SpinCompiler/Types/AbstractFileHandler.h"
#include "SpinCompiler/Tokenizer/StringMap.h"
#include "SpinCompiler/Tokenizer/SpinBuiltInSymbolMap.h"
#include "SpinCompiler/Parser/ObjectHierarchy.h"

class AbstractParser {
public:
    explicit AbstractParser(AbstractFileHandler *fileHandler):fileHandler(fileHandler),builtInSymbols(stringMap) {}
    virtual ~AbstractParser() {}
    AbstractFileHandler *fileHandler;
    StringMap stringMap;
    SpinBuiltInSymbolMap builtInSymbols;
    virtual ParsedObjectP compileObject(FileDescriptorP file, const ObjectHierarchy *hierarchy, const SourcePosition& includePos)=0;
};

#endif //SPINCOMPILER_ABSTRACTPARSER_H

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
