//////////////////////////////////////////////////////////////
//                                                          //
// Propeller Spin/PASM Compiler                             //
// (c)2012-2016 Parallax Inc. DBA Parallax Semiconductor.   //
// Adapted from Chip Gracey's x86 asm code by Roy Eltham    //
// Rewritten to modern C++ by Thilo Ackermann               //
// See end of file for terms of use.                        //
//                                                          //
////////////////////////////////////////////////////////////// 

#ifndef SPINCOMPILER_BINARYANNOTATION_H
#define SPINCOMPILER_BINARYANNOTATION_H

#include <memory>
#include <vector>

struct BinaryAnnotation {
    enum Type { ObjectHeader=0,MethodTable=1,ObjectTable=2,DatSection=3,Method=4,StringPool=5,Padding=6 };
    struct AbstractExtraInfo {
        virtual ~AbstractExtraInfo() {}
    };
    typedef std::shared_ptr<AbstractExtraInfo> AbstractExtraInfoP;

    struct TableEntryNames : public AbstractExtraInfo {
        virtual ~TableEntryNames() {}
        std::vector<std::string> names;
    };
    struct DatAnnotation : public AbstractExtraInfo {
        struct Entry {
            enum Type {Data=0,Instruction=1,CogOrg=2,CogOrgX=3};
            Entry(Type type, int value):type(type),value(value) {}
            Type type;
            int value;
        };

        virtual ~DatAnnotation() {}
        explicit DatAnnotation(const std::vector<Entry> &entries):entries(entries) {}


        std::vector<Entry> entries;
    };
    typedef std::shared_ptr<TableEntryNames> TableEntryNamesP;

    BinaryAnnotation():type(ObjectHeader),size(0) {}
    BinaryAnnotation(Type type, int size):type(type),size(size) {}
    BinaryAnnotation(Type type, int size, AbstractExtraInfoP extraInfo):extraInfo(extraInfo),type(type),size(size) {}

    AbstractExtraInfoP extraInfo;
    Type type;
    int size;
};

#endif //SPINCOMPILER_BINARYANNOTATION_H

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
