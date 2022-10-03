//////////////////////////////////////////////////////////////
//                                                          //
// Propeller Spin/PASM Compiler                             //
// (c)2012-2016 Parallax Inc. DBA Parallax Semiconductor.   //
// Adapted from Chip Gracey's x86 asm code by Roy Eltham    //
// Rewritten to modern C++ by Thilo Ackermann               //
// See end of file for terms of use.                        //
//                                                          //
////////////////////////////////////////////////////////////// 

#ifndef SPINCOMPILER_COMPILERSETTINGS_H
#define SPINCOMPILER_COMPILERSETTINGS_H

#include <map>
#include <string>

struct CompilerSettings {
    enum struct UnusedMethods {
        Keep, // keeps all methods in binary
        RemovePartial, // removes unused methods, keeps all methods, that might be called by coginit/cognew (openspin ume)
        RemoveFull // removes unused methods
    };
    enum struct AnnotatedOutput {
        None,
        JSON,
        HTML
    };

    CompilerSettings():eepromSize(32768),unusedMethodOptimization(UnusedMethods::Keep),annotatedOutput(AnnotatedOutput::None),defaultCompileMode(true),usePreProcessor(true),compileDatOnly(false),binaryMode(true) {}
    std::map<std::string,std::string> preDefinedMacros;
    int eepromSize;
    UnusedMethods unusedMethodOptimization;
    AnnotatedOutput annotatedOutput;
    bool defaultCompileMode;
    bool usePreProcessor;
    bool compileDatOnly;
    bool binaryMode;
};

#endif //SPINCOMPILER_COMPILERSETTINGS_H

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
