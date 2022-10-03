//////////////////////////////////////////////////////////////
//                                                          //
// Propeller Spin/PASM Compiler                             //
// (c)2012-2016 Parallax Inc. DBA Parallax Semiconductor.   //
// Adapted from Chip Gracey's x86 asm code by Roy Eltham    //
// Rewritten to modern C++ by Thilo Ackermann               //
// See end of file for terms of use.                        //
//                                                          //
////////////////////////////////////////////////////////////// 

#ifndef SPINCOMPILER_SYMBOLS_H
#define SPINCOMPILER_SYMBOLS_H

#include <memory>
#include "SpinCompiler/Types/SourcePosition.h"
#include "SpinCompiler/Types/StrongTypedefInt.h"

typedef std::shared_ptr<struct AbstractConstantExpression> AbstractConstantExpressionP;

class SpinAbstractSymbol {
public:
    virtual ~SpinAbstractSymbol() {}
};
typedef std::shared_ptr<SpinAbstractSymbol> SpinAbstractSymbolP;

class SpinVarSectionSymbol : public SpinAbstractSymbol {
public:
    SpinVarSectionSymbol(const SourcePosition& sourcePosition, AbstractConstantExpressionP count, VarSymbolId id, int size):sourcePosition(sourcePosition),count(count),id(id),size(size) {}
    virtual ~SpinVarSectionSymbol() {}
    SourcePosition sourcePosition;
    AbstractConstantExpressionP count;
    VarSymbolId id;
    int size;
};
typedef std::shared_ptr<SpinVarSectionSymbol> SpinVarSectionSymbolP;

class SpinDatSectionSymbol : public SpinAbstractSymbol {
public:
    SpinDatSectionSymbol(DatSymbolId id, int size, bool isRes):id(id),size(size),isRes(isRes) {}
    virtual ~SpinDatSectionSymbol() {}
    DatSymbolId id;
    int size;
    bool isRes;
};

class SpinLocSymbol : public SpinAbstractSymbol {
public:
    explicit SpinLocSymbol(LocSymbolId id):id(id) {}
    virtual ~SpinLocSymbol() {}
    LocSymbolId id;
};

class SpinObjSymbol : public SpinAbstractSymbol {
public:
    SpinObjSymbol(ObjectClassId objectClass, ObjectInstanceId objInstanceId):objectClass(objectClass),objInstanceId(objInstanceId) {}
    virtual ~SpinObjSymbol() {}
    ObjectClassId objectClass; //one objectClass per source file
    ObjectInstanceId objInstanceId; //child object index, obj[n] counts n times
};
typedef std::shared_ptr<SpinObjSymbol> SpinObjSymbolP;

class SpinSubSymbol : public SpinAbstractSymbol {
public:
    SpinSubSymbol(MethodId methodId, int parameterCount, bool isPub):methodId(methodId),parameterCount(parameterCount),isPub(isPub) {}
    virtual ~SpinSubSymbol() {}
    MethodId methodId;
    int parameterCount;
    bool isPub;
};
typedef std::shared_ptr<SpinSubSymbol> SpinSubSymbolP;

class SpinConstantSymbol : public SpinAbstractSymbol {
public:
    SpinConstantSymbol(AbstractConstantExpressionP expression, bool isInteger):expression(expression),isInteger(isInteger) {}
    virtual ~SpinConstantSymbol() {}
    AbstractConstantExpressionP expression;
    bool isInteger;
};


#endif //SPINCOMPILER_SYMBOLS_H

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
