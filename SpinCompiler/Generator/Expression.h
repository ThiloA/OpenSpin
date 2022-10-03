//////////////////////////////////////////////////////////////
//                                                          //
// Propeller Spin/PASM Compiler                             //
// (c)2012-2016 Parallax Inc. DBA Parallax Semiconductor.   //
// Adapted from Chip Gracey's x86 asm code by Roy Eltham    //
// Rewritten to modern C++ by Thilo Ackermann               //
// See end of file for terms of use.                        //
//                                                          //
////////////////////////////////////////////////////////////// 

#ifndef SPINCOMPILER_EXPRESSION_H
#define SPINCOMPILER_EXPRESSION_H

#include "SpinCompiler/Generator/SpinByteCodeWriter.h"
#include <functional>

class AbstractExpression {
public:
    const SourcePosition sourcePosition;

    explicit AbstractExpression(const SourcePosition& sourcePosition):sourcePosition(sourcePosition) {}
    virtual ~AbstractExpression() {}
    virtual void generate(SpinByteCodeWriter &byteCodeWriter) const=0;
    static void iterateExpression(const std::vector<AbstractExpressionP>& list, const std::function<void(AbstractExpressionP)>& callback) {
        for (auto e:list)
            iterateExpression(e, callback);
    }
    static void iterateExpression(AbstractExpressionP expression, const std::function<void(AbstractExpressionP)>& callback) {
        if (!expression)
            return;
        callback(expression);
        expression->iterateChildExpressions(callback);
    }
    static AbstractExpressionP mapExpression(AbstractExpressionP expression, const std::function<AbstractExpressionP(AbstractExpressionP)>& callback, bool *modifiedMarker=nullptr) {
        if (!expression)
            return AbstractExpressionP();
        auto inner = expression->mapChildExpressions(callback); //returns nullptr iff no change
        auto res = callback(inner ? inner : expression);
        if (res != expression && modifiedMarker)
            *modifiedMarker = true;
        return res;
    }
    static std::vector<AbstractExpressionP> mapExpression(const std::vector<AbstractExpressionP>& lst, const std::function<AbstractExpressionP(AbstractExpressionP)>& callback, bool *modifiedMarker=nullptr) {
        std::vector<AbstractExpressionP> lstNew(lst.size());
        for (unsigned int i=0; i<lst.size(); ++i)
            lstNew[i] = mapExpression(lst[i],callback,modifiedMarker);
        return lstNew;
    }
protected:
    virtual void iterateChildExpressions(const std::function<void(AbstractExpressionP)>& callback) const=0;
    virtual AbstractExpressionP mapChildExpressions(const std::function<AbstractExpressionP(AbstractExpressionP)>& callback) const=0;
};
typedef std::shared_ptr<AbstractExpression> AbstractExpressionP;

typedef std::shared_ptr<class AbstractSpinVariable> AbstractSpinVariableP;
class AbstractSpinVariable {
public:
    enum VariableOperation {
        Read=0,
        Write=1,
        Modify=2,
        Reference=3
    };
    const SourcePosition sourcePosition;
    const int size; //TODO enum?, const, constructor set
    AbstractSpinVariable(const SourcePosition& sourcePosition, int size):sourcePosition(sourcePosition),size(size) {}
    virtual ~AbstractSpinVariable() {}
    virtual void generate(SpinByteCodeWriter &byteCodeWriter, VariableOperation operation) const=0;
    virtual bool canTakeReference() const=0;
    virtual void iterateChildExpressions(const std::function<void(AbstractExpressionP)>& callback) const=0;
    virtual AbstractSpinVariableP mapChildExpressions(const std::function<AbstractExpressionP(AbstractExpressionP)>& callback) const=0;
};

struct NamedSpinVariable : public AbstractSpinVariable {
    enum VarType { VarMemoryAccess, DatMemoryAccess, LocMemoryAccess};
    VarType varType;
    AbstractConstantExpressionP fixedAddressExpression;
    AbstractExpressionP indexExpression; //might be nullptr

    explicit NamedSpinVariable(const SourcePosition& sourcePosition, int size, VarType varType, AbstractConstantExpressionP fixedAddressExpression, AbstractExpressionP indexExpression):AbstractSpinVariable(sourcePosition,size),varType(varType),fixedAddressExpression(fixedAddressExpression),indexExpression(indexExpression) {}
    virtual ~NamedSpinVariable() {}
    virtual void generate(SpinByteCodeWriter &byteCodeWriter, VariableOperation operation) const {
        if (indexExpression)
            indexExpression->generate(byteCodeWriter);
        SpinFunctionByteCodeEntry::Type ftype = SpinFunctionByteCodeEntry::VariableDat;
        switch(varType) {
            case DatMemoryAccess: ftype = SpinFunctionByteCodeEntry::VariableDat; break;
            case VarMemoryAccess: ftype = SpinFunctionByteCodeEntry::VariableVar; break;
            case LocMemoryAccess: ftype = SpinFunctionByteCodeEntry::VariableLoc; break;
        }
        byteCodeWriter.appendVariableReference(sourcePosition, ftype,operation,size,indexExpression ? true : false, fixedAddressExpression);
    }
    virtual bool canTakeReference() const {
        return true;
    }
    virtual void iterateChildExpressions(const std::function<void(AbstractExpressionP)>& callback) const {
        AbstractExpression::iterateExpression(indexExpression, callback);
    }
    virtual AbstractSpinVariableP mapChildExpressions(const std::function<AbstractExpressionP(AbstractExpressionP)>& callback) const {
        bool modified=false;
        auto newIndexExpr = AbstractExpression::mapExpression(indexExpression, callback, &modified);
        if (!modified)
            return AbstractSpinVariableP(); //nullptr means no change
        return AbstractSpinVariableP(new NamedSpinVariable(sourcePosition, size, varType, fixedAddressExpression, newIndexExpr));
    }
};

struct DirectMemorySpinVariable : public AbstractSpinVariable {
    const AbstractExpressionP dynamicAddressExpression;
    const AbstractExpressionP indexExpression; //might be nullptr

    explicit DirectMemorySpinVariable(const SourcePosition& sourcePosition, int size, AbstractExpressionP dynamicAddressExpression, AbstractExpressionP indexExpression):AbstractSpinVariable(sourcePosition,size),dynamicAddressExpression(dynamicAddressExpression),indexExpression(indexExpression) {}
    virtual ~DirectMemorySpinVariable() {}
    virtual void generate(SpinByteCodeWriter &byteCodeWriter, VariableOperation operation) const {
        dynamicAddressExpression->generate(byteCodeWriter);
        if (indexExpression)
            indexExpression->generate(byteCodeWriter);
        byteCodeWriter.appendStaticByte(0x80 | (size << 5) | operation | (indexExpression ? 0x10 : 0x00));
    }
    virtual bool canTakeReference() const {
        return true;
    }
    virtual void iterateChildExpressions(const std::function<void(AbstractExpressionP)>& callback) const {
        AbstractExpression::iterateExpression(dynamicAddressExpression, callback);
        AbstractExpression::iterateExpression(indexExpression, callback);
    }
    virtual AbstractSpinVariableP mapChildExpressions(const std::function<AbstractExpressionP(AbstractExpressionP)>& callback) const {
        bool modified=false;
        auto newAddressExpr = AbstractExpression::mapExpression(dynamicAddressExpression, callback, &modified);
        auto newIndexExpr = AbstractExpression::mapExpression(indexExpression, callback, &modified);
        if (!modified)
            return AbstractSpinVariableP(); //nullptr means no change
        return AbstractSpinVariableP(new DirectMemorySpinVariable(sourcePosition, size, newAddressExpr, newIndexExpr));
    }
};

struct SpecialPurposeSpinVariable : public AbstractSpinVariable {
    const AbstractExpressionP dynamicAddressExpression;

    explicit SpecialPurposeSpinVariable(const SourcePosition& sourcePosition, AbstractExpressionP dynamicAddressExpression):AbstractSpinVariable(sourcePosition,2),dynamicAddressExpression(dynamicAddressExpression) {}
    virtual ~SpecialPurposeSpinVariable() {}
    virtual void generate(SpinByteCodeWriter &byteCodeWriter, VariableOperation operation) const {
        dynamicAddressExpression->generate(byteCodeWriter);
        byteCodeWriter.appendStaticByte(0x24 | operation);
    }
    virtual void iterateChildExpressions(const std::function<void(AbstractExpressionP)>& callback) const {
        AbstractExpression::iterateExpression(dynamicAddressExpression, callback);
    }
    virtual bool canTakeReference() const {
        return false;
    }
    virtual AbstractSpinVariableP mapChildExpressions(const std::function<AbstractExpressionP(AbstractExpressionP)>& callback) const {
        bool modified=false;
        auto newAddressExpr = AbstractExpression::mapExpression(dynamicAddressExpression, callback, &modified);
        if (!modified)
            return AbstractSpinVariableP(); //nullptr means no change
        return AbstractSpinVariableP(new SpecialPurposeSpinVariable(sourcePosition, newAddressExpr));
    }
};

struct CogRegisterSpinVariable : public AbstractSpinVariable {
    const int cogRegister;
    const AbstractExpressionP indexExpression; //might be nullptr
    const AbstractExpressionP indexExpressionUpperRange; //might be nullptr

    explicit CogRegisterSpinVariable(const SourcePosition& sourcePosition, int cogRegister, AbstractExpressionP indexExpression, AbstractExpressionP indexExpressionUpperRange):AbstractSpinVariable(sourcePosition, 2),cogRegister(cogRegister),indexExpression(indexExpression),indexExpressionUpperRange(indexExpressionUpperRange) {}
    virtual ~CogRegisterSpinVariable() {}
    virtual void generate(SpinByteCodeWriter &byteCodeWriter, VariableOperation operation) const {
        if (indexExpression) {
            indexExpression->generate(byteCodeWriter);
            if (indexExpressionUpperRange) {
                indexExpressionUpperRange->generate(byteCodeWriter);
                byteCodeWriter.appendStaticByte(0x3E);
            }
            else
                byteCodeWriter.appendStaticByte(0x3D);
        }
        else
            byteCodeWriter.appendStaticByte(0x3F);
        // byteCode = 1 in high bit, bottom 2 bits of vOperation in next two bits, then bottom 5 bits of address
        byteCodeWriter.appendStaticByte(0x80 | (operation << 5) | (cogRegister & 0x1F));
    }
    virtual bool canTakeReference() const {
        return false;
    }
    virtual void iterateChildExpressions(const std::function<void(AbstractExpressionP)>& callback) const {
        AbstractExpression::iterateExpression(indexExpression, callback);
        AbstractExpression::iterateExpression(indexExpressionUpperRange, callback);
    }
    virtual AbstractSpinVariableP mapChildExpressions(const std::function<AbstractExpressionP(AbstractExpressionP)>& callback) const {
        bool modified=false;
        auto newIndexExpr = AbstractExpression::mapExpression(indexExpression, callback, &modified);
        auto newIndexUpperExpr = AbstractExpression::mapExpression(indexExpressionUpperRange, callback, &modified);
        if (!modified)
            return AbstractSpinVariableP(); //nullptr means no change
        return AbstractSpinVariableP(new CogRegisterSpinVariable(sourcePosition, cogRegister, newIndexExpr, newIndexUpperExpr));
    }
};

class VariableExpression: public AbstractExpression {
public:
    const AbstractSpinVariableP variable;
    const AbstractSpinVariable::VariableOperation operation; //TODO enum
    const int vOperator;

    explicit VariableExpression(AbstractSpinVariableP variable, AbstractSpinVariable::VariableOperation operation, int vOperator):
        AbstractExpression(variable->sourcePosition),variable(variable),operation(operation),vOperator(vOperator) {
    }
    virtual ~VariableExpression() {}

    virtual void generate(SpinByteCodeWriter &byteCodeWriter) const {
        variable->generate(byteCodeWriter, operation);
        if (operation == AbstractSpinVariable::Modify) // if assign
            byteCodeWriter.appendStaticByte(vOperator);
    }
protected:
    virtual void iterateChildExpressions(const std::function<void(AbstractExpressionP)>& callback) const {
        variable->iterateChildExpressions(callback);
    }
    virtual AbstractExpressionP mapChildExpressions(const std::function<AbstractExpressionP(AbstractExpressionP)>& callback) const {
        auto newVarInfo = variable->mapChildExpressions(callback);
        if (!newVarInfo)
            return AbstractExpressionP(); //nullptr means no change
        return AbstractExpressionP(new VariableExpression(newVarInfo, operation, vOperator));
    }
};
typedef std::shared_ptr<VariableExpression> VariableExpressionP;

class AssignExpression : public AbstractExpression {
public:
    const AbstractExpressionP valueExpression;
    const AbstractExpressionP destinationVariable;

    explicit AssignExpression(const SourcePosition& sourcePosition, AbstractExpressionP valueExpression, AbstractExpressionP destinationVariable):
        AbstractExpression(sourcePosition),valueExpression(valueExpression),destinationVariable(destinationVariable) {}
    virtual ~AssignExpression() {}

    virtual void generate(SpinByteCodeWriter &byteCodeWriter) const {
        valueExpression->generate(byteCodeWriter);
        destinationVariable->generate(byteCodeWriter);
    }
protected:
    virtual void iterateChildExpressions(const std::function<void(AbstractExpressionP)>& callback) const {
        iterateExpression(valueExpression, callback);
        iterateExpression(destinationVariable, callback);
    }
    virtual AbstractExpressionP mapChildExpressions(const std::function<AbstractExpressionP(AbstractExpressionP)>& callback) const {
        bool modified=false;
        auto valueExpressionNew = mapExpression(valueExpression, callback, &modified);
        auto destinationVariableNew = mapExpression(destinationVariable, callback, &modified);
        if (!modified)
            return AbstractExpressionP(); //nullptr means no change
        return AbstractExpressionP(new AssignExpression(sourcePosition, valueExpressionNew, destinationVariableNew));
    }
};

class CogNewSpinExpression : public AbstractExpression {
public:
    const std::vector<AbstractExpressionP> parameters;
    const AbstractExpressionP stackExpression;
    const MethodId methodId;
    const bool popReturnValue;

    explicit CogNewSpinExpression(const SourcePosition& sourcePosition, const std::vector<AbstractExpressionP> &parameters, AbstractExpressionP stackExpression, MethodId methodId, bool popReturnValue):
        AbstractExpression(sourcePosition),parameters(parameters),stackExpression(stackExpression),methodId(methodId),popReturnValue(popReturnValue) {}
    virtual ~CogNewSpinExpression() {}
    virtual void generate(SpinByteCodeWriter &byteCodeWriter) const {
        for (auto p:parameters)
            p->generate(byteCodeWriter);
        byteCodeWriter.appendCogInitNewSpinSubroutine(sourcePosition, int(parameters.size()), methodId);
        stackExpression->generate(byteCodeWriter);
        byteCodeWriter.appendStaticByte(0x15); // run
        byteCodeWriter.appendStaticByte(popReturnValue ? 0x2C : 0x28); // coginit
    }
protected:
    virtual void iterateChildExpressions(const std::function<void(AbstractExpressionP)>& callback) const {
        iterateExpression(parameters, callback);
        iterateExpression(stackExpression, callback);
    }
    virtual AbstractExpressionP mapChildExpressions(const std::function<AbstractExpressionP(AbstractExpressionP)>& callback) const {
        bool modified=false;
        auto parametersNew = mapExpression(parameters, callback, &modified);
        auto stackExpressionNew = mapExpression(stackExpression, callback, &modified);
        if (!modified)
            return AbstractExpressionP(); //nullptr means no change
        return AbstractExpressionP(new CogNewSpinExpression(sourcePosition, parametersNew, stackExpressionNew, methodId, popReturnValue));
    }
};

class CogNewAsmExpression : public AbstractExpression {
public:
    const AbstractExpressionP address;
    const AbstractExpressionP startParameter;
    const bool popReturnValue;

    explicit CogNewAsmExpression(const SourcePosition& sourcePosition, AbstractExpressionP address, AbstractExpressionP startParameter, bool popReturnValue):
        AbstractExpression(sourcePosition),address(address),startParameter(startParameter),popReturnValue(popReturnValue) {}
    virtual ~CogNewAsmExpression() {}
    virtual void generate(SpinByteCodeWriter &byteCodeWriter) const {
        byteCodeWriter.appendStaticByte(0x34); // constant -1
        address->generate(byteCodeWriter);
        startParameter->generate(byteCodeWriter);
        byteCodeWriter.appendStaticByte(popReturnValue ? 0x2C : 0x28);
    }
protected:
    virtual void iterateChildExpressions(const std::function<void(AbstractExpressionP)>& callback) const {
        iterateExpression(address, callback);
        iterateExpression(startParameter, callback);
    }
    virtual AbstractExpressionP mapChildExpressions(const std::function<AbstractExpressionP(AbstractExpressionP)>& callback) const {
        bool modified=false;
        auto addressNew = mapExpression(address, callback, &modified);
        auto startParameterNew = mapExpression(startParameter, callback, &modified);
        if (!modified)
            return AbstractExpressionP(); //nullptr means no change
        return AbstractExpressionP(new CogNewAsmExpression(sourcePosition, addressNew, startParameterNew, popReturnValue));
    }
};

class MethodCallExpression : public AbstractExpression {
public:
    const std::vector<AbstractExpressionP> parameters;
    const AbstractExpressionP objectIndex; //might be nullptr
    const ObjectInstanceId objectInstanceId; //valid if object call, otherwise invalid for own methods
    const MethodId methodId;
    const int anchor; //TODO enum


    explicit MethodCallExpression(const SourcePosition& sourcePosition, const std::vector<AbstractExpressionP> &parameters, AbstractExpressionP objectIndex, ObjectInstanceId objectInstanceId, MethodId methodId, int anchor):
        AbstractExpression(sourcePosition),parameters(parameters),objectIndex(objectIndex),objectInstanceId(objectInstanceId),methodId(methodId),anchor(anchor) {}
    virtual ~MethodCallExpression() {}
    virtual void generate(SpinByteCodeWriter &byteCodeWriter) const {
        byteCodeWriter.appendStaticByte(anchor); // drop anchor (0..3 return/exception value handling)
        for (auto p:parameters)
            p->generate(byteCodeWriter);
        if (objectInstanceId.valid()) {
            if (objectIndex) {
                objectIndex->generate(byteCodeWriter);
                byteCodeWriter.appendStaticByte(0x07); // call obj[].pub
            }
            else
                byteCodeWriter.appendStaticByte(0x06); // call obj.pub
            byteCodeWriter.appendSubroutineOfChildObject(sourcePosition, objectInstanceId, methodId);
        }
        else {
            byteCodeWriter.appendStaticByte(0x05); // call sub
            byteCodeWriter.appendSubroutineOfOwnObject(sourcePosition, methodId);
        }
    }
protected:
    virtual void iterateChildExpressions(const std::function<void(AbstractExpressionP)>& callback) const {
        iterateExpression(parameters, callback);
        iterateExpression(objectIndex, callback);
    }
    virtual AbstractExpressionP mapChildExpressions(const std::function<AbstractExpressionP(AbstractExpressionP)>& callback) const {
        bool modified=false;
        auto parametersNew = mapExpression(parameters, callback, &modified);
        auto objectIndexNew = mapExpression(objectIndex, callback, &modified);
        if (!modified)
            return AbstractExpressionP(); //nullptr means no change
        return AbstractExpressionP(new MethodCallExpression(sourcePosition, parametersNew, objectIndexNew, objectInstanceId, methodId, anchor));
    }
};

class LookExpression : public AbstractExpression {
public:
    struct ExpressionListEntry {
        ExpressionListEntry() {}
        ExpressionListEntry(AbstractExpressionP expr1, AbstractExpressionP expr2):expr1(expr1),expr2(expr2) {}
        AbstractExpressionP expr1;
        AbstractExpressionP expr2; //might be nullptr if not a range
    };
    const AbstractExpressionP condition;
    const std::vector<ExpressionListEntry> expressionList;
    const int origTokenByteCode; //TODO

    explicit LookExpression(const SourcePosition& sourcePosition, AbstractExpressionP condition, const std::vector<ExpressionListEntry> &expressionList, int origTokenByteCode):
        AbstractExpression(sourcePosition),condition(condition),expressionList(expressionList),origTokenByteCode(origTokenByteCode) {}
    virtual ~LookExpression() {}
    virtual void generate(SpinByteCodeWriter &byteCodeWriter) const {
        SpinByteCodeLabel label = byteCodeWriter.reserveLabel();
        byteCodeWriter.appendStaticByte(origTokenByteCode < 0x80 ? 0x36 : 0x35); //constant 1 or 0
        byteCodeWriter.appendAbsoluteAddress(label);
        condition->generate(byteCodeWriter);
        for (auto e:expressionList) {
            e.expr1->generate(byteCodeWriter);
            if (e.expr2) {
                e.expr2->generate(byteCodeWriter);
                byteCodeWriter.appendStaticByte((origTokenByteCode & 0x7F) | 2);
            }
            else
                byteCodeWriter.appendStaticByte(origTokenByteCode & 0x7F);
        }
        byteCodeWriter.appendStaticByte(0x0F); // lookdone
        byteCodeWriter.placeLabelHere(label);
    }
protected:
    virtual void iterateChildExpressions(const std::function<void(AbstractExpressionP)>& callback) const {
        iterateExpression(condition, callback);
        for (const auto&e: expressionList) {
            iterateExpression(e.expr1, callback);
            iterateExpression(e.expr2, callback);
        }
    }
    virtual AbstractExpressionP mapChildExpressions(const std::function<AbstractExpressionP(AbstractExpressionP)>& callback) const {
        bool modified=false;
        auto conditionNew = mapExpression(condition, callback, &modified);
        std::vector<ExpressionListEntry>expressionListNew(expressionList.size());
        for (unsigned int i=0; i<expressionList.size(); ++i) {
            expressionListNew[i].expr1 = mapExpression(expressionList[i].expr1, callback, &modified);
            expressionListNew[i].expr2 = mapExpression(expressionList[i].expr2, callback, &modified);
        }
        if (!modified)
            return AbstractExpressionP(); //nullptr means no change
        return AbstractExpressionP(new LookExpression(sourcePosition, conditionNew, expressionListNew, origTokenByteCode));
    }
};

class PushConstantExpression : public AbstractExpression {
public:
    const AbstractConstantExpressionP constant;
    explicit PushConstantExpression(const SourcePosition& sourcePosition, AbstractConstantExpressionP constant):AbstractExpression(sourcePosition),constant(constant) {}
    virtual ~PushConstantExpression() {}
    virtual void generate(SpinByteCodeWriter &byteCodeWriter) const {
        byteCodeWriter.appendStaticPushConstant(sourcePosition, constant);
    }
protected:
    virtual void iterateChildExpressions(const std::function<void(AbstractExpressionP)>&) const {
    }
    virtual AbstractExpressionP mapChildExpressions(const std::function<AbstractExpressionP(AbstractExpressionP)>&) const {
        return AbstractExpressionP(); //nullptr means no change
    }
};

class PushStringExpression : public AbstractExpression {
public:
    const std::vector<AbstractConstantExpressionP> stringData;
    //the purpose of this index is to produce the same binaries as in classic openspin compiler
    //the index will be incremented by the parse (reset to 0 for each function)
    //if this index is negative (e.g. -1) ordering of strings in the binary might be arbitrary
    const int stringIndex;
    explicit PushStringExpression(const SourcePosition& sourcePosition, const std::vector<AbstractConstantExpressionP> &stringData, int stringIndex):AbstractExpression(sourcePosition),stringData(stringData),stringIndex(stringIndex) {}
    virtual ~PushStringExpression() {}
    virtual void generate(SpinByteCodeWriter &byteCodeWriter) const {
        byteCodeWriter.appendStaticByte(0x87); // (memcp byte+pbase+address)
        byteCodeWriter.appendStringReference(sourcePosition, stringIndex, stringData);
    }
protected:
    virtual void iterateChildExpressions(const std::function<void(AbstractExpressionP)>&) const {
    }
    virtual AbstractExpressionP mapChildExpressions(const std::function<AbstractExpressionP(AbstractExpressionP)>&) const {
        return AbstractExpressionP(); //nullptr means no change
    }
};

class CallBuiltInExpression : public AbstractExpression {
public:
    const std::vector<AbstractExpressionP> parameters;
    const int opCode; //TODO
    explicit CallBuiltInExpression(const SourcePosition& sourcePosition, const std::vector<AbstractExpressionP> &parameters, int opCode):AbstractExpression(sourcePosition),parameters(parameters),opCode(opCode) {}
    virtual ~CallBuiltInExpression() {}
    virtual void generate(SpinByteCodeWriter &byteCodeWriter) const {
        for (auto p:parameters)
            p->generate(byteCodeWriter);
        byteCodeWriter.appendStaticByte(opCode);
    }
protected:
    virtual void iterateChildExpressions(const std::function<void(AbstractExpressionP)>& callback) const {
        iterateExpression(parameters, callback);
    }
    virtual AbstractExpressionP mapChildExpressions(const std::function<AbstractExpressionP(AbstractExpressionP)>& callback) const {
        bool modified=false;
        auto parametersNew = mapExpression(parameters, callback, &modified);
        if (!modified)
            return AbstractExpressionP(); //nullptr means no change
        return AbstractExpressionP(new CallBuiltInExpression(sourcePosition, parametersNew, opCode));
    }
};

class UnaryExpression : public AbstractExpression {
public:
    const AbstractExpressionP expression;
    const int opCode; //TODO
    explicit UnaryExpression(const SourcePosition& sourcePosition, AbstractExpressionP expression, int opCode):AbstractExpression(sourcePosition),expression(expression),opCode(opCode) {}
    virtual ~UnaryExpression() {}
    virtual void generate(SpinByteCodeWriter &byteCodeWriter) const {
        expression->generate(byteCodeWriter);
        byteCodeWriter.appendStaticByte(opCode); // math
    }
protected:
    virtual void iterateChildExpressions(const std::function<void(AbstractExpressionP)>& callback) const {
        iterateExpression(expression, callback);
    }
    virtual AbstractExpressionP mapChildExpressions(const std::function<AbstractExpressionP(AbstractExpressionP)>& callback) const {
        bool modified=false;
        auto expressionNew = mapExpression(expression, callback, &modified);
        if (!modified)
            return AbstractExpressionP(); //nullptr means no change
        return AbstractExpressionP(new UnaryExpression(sourcePosition, expressionNew, opCode));
    }
};

class BinaryExpression : public AbstractExpression {
public:
    const AbstractExpressionP left;
    const AbstractExpressionP right;
    const int opCode; //TODO
    explicit BinaryExpression(const SourcePosition& sourcePosition, AbstractExpressionP left, AbstractExpressionP right, int opCode):AbstractExpression(sourcePosition),left(left),right(right),opCode(opCode) {}
    virtual ~BinaryExpression() {}
    virtual void generate(SpinByteCodeWriter &byteCodeWriter) const {
        left->generate(byteCodeWriter);
        right->generate(byteCodeWriter);
        byteCodeWriter.appendStaticByte(opCode);
    }
protected:
    virtual void iterateChildExpressions(const std::function<void(AbstractExpressionP)>& callback) const {
        iterateExpression(left, callback);
        iterateExpression(right, callback);
    }
    virtual AbstractExpressionP mapChildExpressions(const std::function<AbstractExpressionP(AbstractExpressionP)>& callback) const {
        bool modified=false;
        auto leftNew = mapExpression(left, callback, &modified);
        auto rightNew = mapExpression(right, callback, &modified);
        if (!modified)
            return AbstractExpressionP(); //nullptr means no change
        return AbstractExpressionP(new BinaryExpression(sourcePosition, leftNew, rightNew, opCode));
    }
};

class AtAtExpression : public AbstractExpression {
public:
    const AbstractExpressionP expression;
    explicit AtAtExpression(const SourcePosition& sourcePosition, AbstractExpressionP expression):AbstractExpression(sourcePosition),expression(expression) {}
    virtual ~AtAtExpression() {}
    virtual void generate(SpinByteCodeWriter &byteCodeWriter) const {
        expression->generate(byteCodeWriter);
        byteCodeWriter.appendStaticByte(0x97); // memop byte+index+pbase+address
        byteCodeWriter.appendStaticByte(0); // address 0
    }
protected:
    virtual void iterateChildExpressions(const std::function<void(AbstractExpressionP)>& callback) const {
        iterateExpression(expression, callback);
    }
    virtual AbstractExpressionP mapChildExpressions(const std::function<AbstractExpressionP(AbstractExpressionP)>& callback) const {
        bool modified=false;
        auto expressionNew = mapExpression(expression, callback, &modified);
        if (!modified)
            return AbstractExpressionP(); //nullptr means no change
        return AbstractExpressionP(new AtAtExpression(sourcePosition, expressionNew));
    }
};

class FixedByteCodeSequenceExpression : public AbstractExpression { //TODO ggf ersetzen
public:
    const std::vector<unsigned char> byteCode;
    explicit FixedByteCodeSequenceExpression(const SourcePosition& sourcePosition, const std::vector<unsigned char> &byteCode):AbstractExpression(sourcePosition),byteCode(byteCode) {}
    virtual ~FixedByteCodeSequenceExpression() {}
    virtual void generate(SpinByteCodeWriter &byteCodeWriter) const {
        for (unsigned char b:byteCode)
            byteCodeWriter.appendStaticByte(b);
    }
protected:
    virtual void iterateChildExpressions(const std::function<void(AbstractExpressionP)>&) const {
    }
    virtual AbstractExpressionP mapChildExpressions(const std::function<AbstractExpressionP(AbstractExpressionP)>&) const {
        return AbstractExpressionP();
    }
};

#endif //SPINCOMPILER_EXPRESSION_H

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
