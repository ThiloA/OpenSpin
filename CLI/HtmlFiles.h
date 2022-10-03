//////////////////////////////////////////////////////////////
//                                                          //
// Propeller Spin/PASM Compiler                             //
// (c)2012-2016 Parallax Inc. DBA Parallax Semiconductor.   //
// Adapted from Chip Gracey's x86 asm code by Roy Eltham    //
// Rewritten to modern C++ by Thilo Ackermann               //
// See end of file for terms of use.                        //
//                                                          //
////////////////////////////////////////////////////////////// 

#ifndef HTMLFILES_H
#define HTMLFILES_H

#ifndef SPINCOMPILER_EXCLUDE_HTML_SUPPORT

#include "3rdparty/incbin.h"
#include <iostream>
#include <string.h>
#include <vector>

INCBIN(Files_HtmlBinaryViewer_Main, "HtmlBinaryViewer/binaryviewer.html");
INCBIN(Files_HtmlBinaryViewer_JQuery, "3rdparty/jquery.js");
INCBIN(Files_HtmlBinaryViewer_PDisAsm, "HtmlBinaryViewer/pdisasm.js");
INCBIN(Files_HtmlBinaryViewer_SpinDisAsm, "HtmlBinaryViewer/spindisasm.js");
INCBIN(Files_HtmlBinaryViewer_Script, "HtmlBinaryViewer/script.js");
INCBIN(Files_HtmlBinaryViewer_Style, "HtmlBinaryViewer/style.css");

struct HtmlFiles {
    static bool generate(std::vector<unsigned char>& data) {
        const char *startMarker = "<!-- OpenSpinDynamicInsert:Start -->";
        const char *stopMarker = "<!-- OpenSpinDynamicInsert:Stop -->";
        const char *htmlCode = reinterpret_cast<const char*>(gFiles_HtmlBinaryViewer_MainData);
        const char *startDyn = strstr(htmlCode, startMarker);
        const char *stopDyn = strstr(startDyn, stopMarker);
        if (!startDyn || !stopDyn)
            return false;
        std::vector<unsigned char> result(gFiles_HtmlBinaryViewer_MainData,reinterpret_cast<const unsigned char*>(startDyn));
        append(result,"<style>\n");
        append(result, gFiles_HtmlBinaryViewer_StyleData, gFiles_HtmlBinaryViewer_StyleSize);
        append(result,"</style>\n");
        append(result,"<script>\n");
        append(result, gFiles_HtmlBinaryViewer_JQueryData, gFiles_HtmlBinaryViewer_JQuerySize);
        append(result,"\n");
        append(result, gFiles_HtmlBinaryViewer_PDisAsmData, gFiles_HtmlBinaryViewer_PDisAsmSize);
        append(result,"\n");
        append(result, gFiles_HtmlBinaryViewer_SpinDisAsmData, gFiles_HtmlBinaryViewer_SpinDisAsmSize);
        append(result,"\n");
        append(result, gFiles_HtmlBinaryViewer_ScriptData, gFiles_HtmlBinaryViewer_ScriptSize);
        append(result,"\n");
        append(result,"$(function() {\n    initObject(");
        result.insert(result.end(),data.begin(),data.end());
        append(result,")});\n\n");
        append(result,"</script>\n");
        result.insert(result.end(),reinterpret_cast<const unsigned char*>(stopDyn)+strlen(stopMarker),gFiles_HtmlBinaryViewer_MainData+gFiles_HtmlBinaryViewer_MainSize);
        data = result;
        return true;
    }
private:
    static void append(std::vector<unsigned char>& result, const char* txt) {
        while (*txt)
            result.push_back(*txt++);
    }
    static void append(std::vector<unsigned char>& result, const unsigned char* fileStart, int fileSize) {
        result.insert(result.end(),fileStart,fileStart+fileSize);
    }
};
#else
struct HtmlFiles {
    static bool generate(std::vector<unsigned char>&) {
        return false;
    }
};
#endif

#endif //HTMLFILES_H

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
