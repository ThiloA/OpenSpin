//////////////////////////////////////////////////////////////
//                                                          //
// Propeller Spin/PASM Compiler                             //
// (c)2012-2016 Parallax Inc. DBA Parallax Semiconductor.   //
// Adapted from Chip Gracey's x86 asm code by Roy Eltham    //
// Rewritten to modern C++ by Thilo Ackermann               //
// See end of file for terms of use.                        //
//                                                          //
////////////////////////////////////////////////////////////// 

#ifndef SPINCOMPILER_ABSTRACTBINARYGENERATOR_H
#define SPINCOMPILER_ABSTRACTBINARYGENERATOR_H

#include "SpinCompiler/Types/StrongTypedefInt.h"
#include <string>

class AbstractBinaryGenerator {
public:
    virtual ~AbstractBinaryGenerator() {}
    virtual int mapObjectInstanceIdToOffset(ObjectInstanceId objectIndexId) const=0;
    virtual int valueOfConstantOfObjectClass(ObjectClassId objectClass, int constantIndex) const=0;
    virtual void setDatSymbol(DatSymbolId datSymbolId, int objPtr, int cogOrg)=0;
    virtual int& currentDatCogOrg()=0;
    virtual int valueOfDatSymbol(DatSymbolId datSymbolId, bool cogPosition) const=0;
    virtual int addressOfVarSymbol(VarSymbolId varSymbolId) const=0;
    virtual std::string getNameBySymbolId(SpinSymbolId symbol) const=0;
    virtual int addressOfLocSymbol(LocSymbolId locSymbolId) const=0; //for current method
};

#endif //SPINCOMPILER_ABSTRACTBINARYGENERATOR_H

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
