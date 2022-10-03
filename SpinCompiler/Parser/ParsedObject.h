//////////////////////////////////////////////////////////////
//                                                          //
// Propeller Spin/PASM Compiler                             //
// (c)2012-2016 Parallax Inc. DBA Parallax Semiconductor.   //
// Adapted from Chip Gracey's x86 asm code by Roy Eltham    //
// Rewritten to modern C++ by Thilo Ackermann               //
// See end of file for terms of use.                        //
//                                                          //
////////////////////////////////////////////////////////////// 

#ifndef SPINCOMPILER_PARSEDOBJECT_H
#define SPINCOMPILER_PARSEDOBJECT_H

#include "SpinCompiler/Types/ConstantExpression.h"
#include "SpinCompiler/Types/DatCodeEntry.h"


typedef std::shared_ptr<class AbstractInstruction> AbstractInstructionP;
typedef std::shared_ptr<class ParsedObject> ParsedObjectP;
class ParsedObject {
public:
    struct Constant {
        Constant():isFloat(false) {}
        Constant(const SourcePosition& sourcePosition, SpinSymbolId symbolId, AbstractConstantExpressionP constantExpression, bool isFloat):sourcePosition(sourcePosition),symbolId(symbolId),constantExpression(constantExpression),isFloat(isFloat) {}
        SourcePosition sourcePosition;
        SpinSymbolId symbolId;
        AbstractConstantExpressionP constantExpression;
        bool isFloat;
    };
    struct LocalVar { //ReturnValue, LocalVariable or Parameter of Method
        LocalVar() {}
        LocalVar(const SourcePosition& sourcePosition, SpinSymbolId symbolId, LocSymbolId localVarId, AbstractConstantExpressionP count):sourcePosition(sourcePosition),symbolId(symbolId),localVarId(localVarId),count(count) {}
        SourcePosition sourcePosition;
        SpinSymbolId symbolId;
        LocSymbolId localVarId;
        AbstractConstantExpressionP count;
    };
    struct Method {
        Method():parameterCount(0),isPublic(false) {}
        Method(const SourcePosition &sourcePosition, SpinSymbolId symbolId, MethodId methodId, int parameterCount, const std::vector<ParsedObject::LocalVar>& allLocals, bool isPublic):sourcePosition(sourcePosition),symbolId(symbolId),methodId(methodId),parameterCount(parameterCount),allLocals(allLocals),isPublic(isPublic) {}
        SourcePosition sourcePosition;
        AbstractInstructionP functionBody;
        SpinSymbolId symbolId;
        MethodId methodId;
        int parameterCount;
        std::vector<ParsedObject::LocalVar> allLocals;
        bool isPublic;
    };
    typedef std::shared_ptr<Method> MethodP;
    struct ChildObject {
        ChildObject():numberOfInstances(0),objectClass(-1),isUsed(false) {}
        ChildObject(const SourcePosition& sourcePosition, SpinSymbolId symbolId, AbstractConstantExpressionP numberOfInstances, ObjectInstanceId objectInstanceId, ObjectClassId objectClass, ParsedObjectP object, bool isUsed):sourcePosition(sourcePosition),object(object),numberOfInstances(numberOfInstances),objectInstanceId(objectInstanceId),symbolId(symbolId),objectClass(objectClass),isUsed(isUsed) {}
        SourcePosition sourcePosition;
        ParsedObjectP object;
        AbstractConstantExpressionP numberOfInstances;
        ObjectInstanceId objectInstanceId;
        SpinSymbolId symbolId;
        ObjectClassId objectClass;
        bool isUsed; //TODO weg
    };

    ObjectInstanceId reserveNextObjectInstanceId() {
        return ObjectInstanceId(m_nextObjectInstanceId++);
    }
    ObjectClassId reserveOrGetObjectClassId(ParsedObjectP obj) {
        for (const auto& c:childObjects)
            if (c.object == obj)
                return c.objectClass;
        return ObjectClassId(m_nextObjectClassId++);
    }
    MethodId reserveNextMethodId() {
        return MethodId(m_nextMethodId++);
    }
    DatSymbolId reserveNextDatSymbolId() {
        return DatSymbolId(m_nextDatSymbolId++);
    }
    VarSymbolId reserveNextVarSymbolId() {
        return VarSymbolId(m_nextVarSymbolId++);
    }
    const ChildObject& childObjectByObjectInstanceId(ObjectInstanceId objectInstanceId) const {
        for (const auto& ch:childObjects)
            if (ch.objectInstanceId == objectInstanceId)
                return ch;
        throw CompilerError(ErrorType::internal);
    }
    int methodIndexById(MethodId methodId) const {
        for (int i=0; i<int(methods.size()); ++i)
            if (methods[i]->methodId == methodId)
                return i;
        throw CompilerError(ErrorType::internal);
    }
    int indexOfConstantIfAvailable(SpinSymbolId symbolId) const {
        for (unsigned i=0; i<constants.size(); ++i)
            if (constants[i].symbolId == symbolId)
                return int(i);
        return -1;
    }

    explicit ParsedObject():m_nextObjectClassId(1),m_nextObjectInstanceId(1),m_nextMethodId(1),m_nextDatSymbolId(1),m_nextVarSymbolId(1) {}
    void clear() { //TODO still required?
        constants.clear();
        methods.clear();
        globalVariables.clear();
        childObjects.clear();
        m_nextObjectClassId = 1;
        m_nextObjectInstanceId = 1;
        m_nextMethodId = 1;
        m_nextDatSymbolId = 1;
        m_nextVarSymbolId = 1;
        clearDatSection();
    }
    void clearDatSection() {
        datCode.clear();
    }
    std::vector<Constant> constants;
    std::vector<MethodP> methods;
    std::vector<ChildObject> childObjects;
    std::vector<DatCodeEntry> datCode;
    std::vector<SpinVarSectionSymbolP> globalVariables;
private:
    int m_nextObjectClassId;
    int m_nextObjectInstanceId;
    int m_nextMethodId;
    int m_nextDatSymbolId;
    int m_nextVarSymbolId;
};

#endif //SPINCOMPILER_PARSEDOBJECT_H

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
