//////////////////////////////////////////////////////////////
//                                                          //
// Propeller Spin/PASM Compiler                             //
// (c)2012-2016 Parallax Inc. DBA Parallax Semiconductor.   //
// Adapted from Chip Gracey's x86 asm code by Roy Eltham    //
// Rewritten to modern C++ by Thilo Ackermann               //
// See end of file for terms of use.                        //
//                                                          //
////////////////////////////////////////////////////////////// 

#ifndef SPINCOMPILER_SPINBYTECODEWRITER_H
#define SPINCOMPILER_SPINBYTECODEWRITER_H

#include "SpinCompiler/Types/CompilerError.h"
#include "SpinCompiler/Types/StrongTypedefInt.h"
#include "SpinCompiler/Types/SpinVariableInfo.h"
#include <vector>
#include <map>
#include <iostream> //TODO weg

struct SpinFunctionByteCodeEntry { //TODO wo anders hin

    enum Type { StaticByte,PushIntConstant,PushExprConstant,StringReference,
                CogInitNewSpinSubroutine,SubroutineOwnObject,SubroutineChildObject,
                VariableDat,VariableVar,VariableLoc,
                PlaceLabel,RelativeAddressToLabel,AbsoluteAddressToLabel};
    SpinFunctionByteCodeEntry(const SourcePosition& sourcePosition, Type type, int value, int subroutineId=0, AbstractConstantExpressionP expression=AbstractConstantExpressionP()):sourcePosition(sourcePosition),type(type),value(value),subroutineIdOrVarAddress(subroutineId),expression(expression) {}
    SourcePosition sourcePosition;
    Type type;
    int value; //for type==CogInitNewSpinSubroutine: parameterCount
    int subroutineIdOrVarAddress;
    AbstractConstantExpressionP expression;


    static int packVarInfo(int operation, int varSize, bool hasIndexExpression) { //TODO weg
        return (!hasIndexExpression ? 1 : 0) + (varSize<<8) + (operation<<16);
    }
    static void unpackVarInfo(int packed, bool& indexSourcePtrIsNull, int& size, int& vOperation) { //TODO weg
        indexSourcePtrIsNull = (packed&1) == 1;
        size = (packed>>8)&0xFF;
        vOperation = (packed>>16)&0xFF;
    }
};

struct SpinStringInfo {
    SpinStringInfo() {}
    SpinStringInfo(const SourcePosition& sourcePosition, const std::vector<AbstractConstantExpressionP> &characters):sourcePosition(sourcePosition),characters(characters) {}
    SourcePosition sourcePosition;
    std::vector<AbstractConstantExpressionP> characters;
};

class SpinByteCodeWriter {
public:
private:
    std::vector<SpinFunctionByteCodeEntry> &m_code;
    std::map<int, SpinStringInfo> m_stringMap;
    int m_nextLabel;
    int m_nextExtraString;
public:
    SpinByteCodeWriter(std::vector<SpinFunctionByteCodeEntry> &code):m_code(code),m_nextLabel(1),m_nextExtraString(1) {}
    std::vector<SpinStringInfo> retrieveAllStrings() {
        std::vector<SpinStringInfo> result;
        for (auto it:m_stringMap)
            result.push_back(it.second);
        return result;
    }

    SpinByteCodeLabel reserveLabel() {
        return SpinByteCodeLabel(m_nextLabel++);
    }

    void placeLabelHere(SpinByteCodeLabel label) {
        m_code.push_back(SpinFunctionByteCodeEntry(SourcePosition(), SpinFunctionByteCodeEntry::PlaceLabel,label.value()));
    }

    void appendAbsoluteAddress(SpinByteCodeLabel label) {
        m_code.push_back(SpinFunctionByteCodeEntry(SourcePosition(), SpinFunctionByteCodeEntry::AbsoluteAddressToLabel,label.value()));
    }

    void appendRelativeAddress(SpinByteCodeLabel label) {
        m_code.push_back(SpinFunctionByteCodeEntry(SourcePosition(), SpinFunctionByteCodeEntry::RelativeAddressToLabel,label.value()));
    }
    void appendPopStack() {
        //TODO
        std::cerr<<"WARN appendPopStack not yet tested"<<std::endl;
        appendStaticPushConstant(SourcePosition(), 4); // enter pop count
        appendStaticByte(0x14); // pop
    }
    void appendStaticByte(int byteCode) {
        m_code.push_back(SpinFunctionByteCodeEntry(SourcePosition(), SpinFunctionByteCodeEntry::StaticByte, byteCode));
    }
    void appendStaticPushConstant(const SourcePosition& sourcePosition, int constantValue) {
        m_code.push_back(SpinFunctionByteCodeEntry(sourcePosition, SpinFunctionByteCodeEntry::PushIntConstant, constantValue, 0));
    }
    void appendStaticPushConstant(const SourcePosition& sourcePosition, AbstractConstantExpressionP expression) {
        m_code.push_back(SpinFunctionByteCodeEntry(sourcePosition, SpinFunctionByteCodeEntry::PushExprConstant, 0, 0, expression));
    }

    void appendStringReference(const SourcePosition& sourcePosition, int stringNumber, const std::vector<AbstractConstantExpressionP>& stringData) {
        if (stringNumber<0)
            stringNumber = -m_nextExtraString++; //see PushStringExpression for description of this mechanism
        m_stringMap[stringNumber] = SpinStringInfo(sourcePosition, stringData);
        m_code.push_back(SpinFunctionByteCodeEntry(sourcePosition, SpinFunctionByteCodeEntry::StringReference, stringNumber));
    }

    void appendCogInitNewSpinSubroutine(const SourcePosition& sourcePosition, int parameterCount, MethodId methodId) {
        m_code.push_back(SpinFunctionByteCodeEntry(sourcePosition, SpinFunctionByteCodeEntry::CogInitNewSpinSubroutine,parameterCount,methodId.value()));
    }

    void appendSubroutineOfOwnObject(const SourcePosition& sourcePosition, MethodId methodId) {
        m_code.push_back(SpinFunctionByteCodeEntry(sourcePosition, SpinFunctionByteCodeEntry::SubroutineOwnObject,0,methodId.value()));
    }

    void appendSubroutineOfChildObject(const SourcePosition& sourcePosition, ObjectInstanceId objectIndexId, MethodId methodId) {
        m_code.push_back(SpinFunctionByteCodeEntry(sourcePosition, SpinFunctionByteCodeEntry::SubroutineChildObject,objectIndexId.value(),methodId.value()));
    }

    void appendVariableReference(const SourcePosition& sourcePosition, SpinFunctionByteCodeEntry::Type type, int operation, int varSize, bool hasIndexExpression, AbstractConstantExpressionP fixedAddressExpression) {
        m_code.push_back(SpinFunctionByteCodeEntry(sourcePosition, type,
                                                   SpinFunctionByteCodeEntry::packVarInfo(operation,varSize,hasIndexExpression),
                                                   0,fixedAddressExpression));
    }
};

#endif //SPINCOMPILER_SPINBYTECODEWRITER_H

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
