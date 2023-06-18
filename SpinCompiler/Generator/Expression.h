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
#include "SpinCompiler/Types/ConstantExpression.h"
#include <functional>

class AbstractExpression {
public:
    const SourcePosition sourcePosition;

    explicit AbstractExpression(const SourcePosition& sourcePosition):sourcePosition(sourcePosition) {}
    virtual ~AbstractExpression() {}
    virtual void generate(SpinByteCodeWriter &byteCodeWriter, bool removeResultFromStack) const=0;
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
    virtual std::string toILangStr() const = 0;
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
    enum SizeModifier {
        Byte=0,
        Word=1,
        Long=2
    };

    const SourcePosition sourcePosition;
    const SizeModifier size;
    AbstractSpinVariable(const SourcePosition& sourcePosition, SizeModifier size):sourcePosition(sourcePosition),size(size) {}
    virtual ~AbstractSpinVariable() {}
    virtual void generate(SpinByteCodeWriter &byteCodeWriter, VariableOperation operation) const=0;
    virtual bool canTakeReference() const=0;
    virtual void iterateChildExpressions(const std::function<void(AbstractExpressionP)>& callback) const=0;
    virtual AbstractSpinVariableP mapChildExpressions(const std::function<AbstractExpressionP(AbstractExpressionP)>& callback) const=0;
    virtual std::string toILangStr() const=0;
    std::string sizeToILangStr() const {
        switch(size) {
            case Byte: return "byte";
            case Word: return "word";
            case Long: return "long";
        }
        return "illegal";
    }
};

struct NamedSpinVariable : public AbstractSpinVariable {
    enum VarType { VarMemoryAccess, DatMemoryAccess, LocMemoryAccess};
    const VarType varType;
    const AbstractExpressionP indexExpression; //might be nullptr
    const int symbolId;

    explicit NamedSpinVariable(const SourcePosition& sourcePosition, SizeModifier size, VarType varType, AbstractExpressionP indexExpression, int symbolId):AbstractSpinVariable(sourcePosition,size),varType(varType),indexExpression(indexExpression),symbolId(symbolId) {}
    virtual ~NamedSpinVariable() {}
    virtual void generate(SpinByteCodeWriter &byteCodeWriter, VariableOperation operation) const {
        if (indexExpression)
            indexExpression->generate(byteCodeWriter, false);
        SpinFunctionByteCodeEntry::Type ftype = SpinFunctionByteCodeEntry::VariableDat;
        switch(varType) {
            case DatMemoryAccess: ftype = SpinFunctionByteCodeEntry::VariableDat; break;
            case VarMemoryAccess: ftype = SpinFunctionByteCodeEntry::VariableVar; break;
            case LocMemoryAccess: ftype = SpinFunctionByteCodeEntry::VariableLoc; break;
        }
        byteCodeWriter.appendVariableReference(sourcePosition, ftype,operation,size,indexExpression ? true : false, symbolId);
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
        return AbstractSpinVariableP(new NamedSpinVariable(sourcePosition, size, varType, newIndexExpr, symbolId));
    }
    virtual std::string toILangStr() const {
        std::string result;
        switch(varType) {
            case DatMemoryAccess: result = "dat "; break;
            case VarMemoryAccess: result = "var "; break;
            case LocMemoryAccess: result = "loc "; break;
        }
        result += std::to_string(symbolId);
        if (indexExpression)
            result += " idx "+indexExpression->toILangStr();
        return result;
    }
};

struct DirectMemorySpinVariable : public AbstractSpinVariable {
    const AbstractExpressionP dynamicAddressExpression;
    const AbstractExpressionP indexExpression; //might be nullptr

    explicit DirectMemorySpinVariable(const SourcePosition& sourcePosition, SizeModifier size, AbstractExpressionP dynamicAddressExpression, AbstractExpressionP indexExpression):AbstractSpinVariable(sourcePosition,size),dynamicAddressExpression(dynamicAddressExpression),indexExpression(indexExpression) {}
    virtual ~DirectMemorySpinVariable() {}
    virtual void generate(SpinByteCodeWriter &byteCodeWriter, VariableOperation operation) const {
        dynamicAddressExpression->generate(byteCodeWriter, false);
        if (indexExpression)
            indexExpression->generate(byteCodeWriter, false);
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

    virtual std::string toILangStr() const {
        std::string result = "mem "+sizeToILangStr()+" "+dynamicAddressExpression->toILangStr();
        if (indexExpression)
            result += " idx "+indexExpression->toILangStr();
        return result;
    }
};

struct SpecialPurposeSpinVariable : public AbstractSpinVariable {
    const AbstractExpressionP dynamicAddressExpression;

    explicit SpecialPurposeSpinVariable(const SourcePosition& sourcePosition, AbstractExpressionP dynamicAddressExpression):AbstractSpinVariable(sourcePosition,SizeModifier::Long),dynamicAddressExpression(dynamicAddressExpression) {}
    virtual ~SpecialPurposeSpinVariable() {}
    virtual void generate(SpinByteCodeWriter &byteCodeWriter, VariableOperation operation) const {
        dynamicAddressExpression->generate(byteCodeWriter, false);
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
    virtual std::string toILangStr() const {
        return "spr "+dynamicAddressExpression->toILangStr();
    }
};

struct CogRegisterSpinVariable : public AbstractSpinVariable {
    const int cogRegister;
    const AbstractExpressionP indexExpression; //might be nullptr
    const AbstractExpressionP indexExpressionUpperRange; //might be nullptr

    explicit CogRegisterSpinVariable(const SourcePosition& sourcePosition, int cogRegister, AbstractExpressionP indexExpression, AbstractExpressionP indexExpressionUpperRange):AbstractSpinVariable(sourcePosition, SizeModifier::Long),cogRegister(cogRegister),indexExpression(indexExpression),indexExpressionUpperRange(indexExpressionUpperRange) {}
    virtual ~CogRegisterSpinVariable() {}
    virtual void generate(SpinByteCodeWriter &byteCodeWriter, VariableOperation operation) const {
        if (indexExpression) {
            indexExpression->generate(byteCodeWriter, false);
            if (indexExpressionUpperRange) {
                indexExpressionUpperRange->generate(byteCodeWriter, false);
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
    virtual std::string toILangStr() const {
        std::string result = "cogreg "+std::to_string(cogRegister);
        if (indexExpression)
            result += " bit "+indexExpression->toILangStr();
        if (indexExpressionUpperRange)
            result += " to "+indexExpressionUpperRange->toILangStr();
        return result;
    }
};

class VariableExpression: public AbstractExpression {
public:
    const AbstractSpinVariableP variable;
    const bool isReadReference;

    explicit VariableExpression(AbstractSpinVariableP variable, bool isReadReference):
        AbstractExpression(variable->sourcePosition),variable(variable),isReadReference(isReadReference) {
    }
    virtual ~VariableExpression() {}

    virtual void generate(SpinByteCodeWriter &byteCodeWriter, bool removeResultFromStack) const {
        variable->generate(byteCodeWriter, isReadReference ? AbstractSpinVariable::Reference : AbstractSpinVariable::Read);
        if (removeResultFromStack)
            byteCodeWriter.appendPopStack();
    }
    virtual std::string toILangStr() const {
        if (isReadReference)
            return "(refvar "+variable->toILangStr()+")";
        return "(rdvar "+variable->toILangStr()+")";
    }
protected:
    virtual void iterateChildExpressions(const std::function<void(AbstractExpressionP)>& callback) const {
        variable->iterateChildExpressions(callback);
    }
    virtual AbstractExpressionP mapChildExpressions(const std::function<AbstractExpressionP(AbstractExpressionP)>& callback) const {
        auto newVarInfo = variable->mapChildExpressions(callback);
        if (!newVarInfo)
            return AbstractExpressionP(); //nullptr means no change
        return AbstractExpressionP(new VariableExpression(newVarInfo, isReadReference));
    }
};

class UnaryAssignExpression: public AbstractExpression {
public:
    const AbstractSpinVariableP variable;
    const int vOperator;

    explicit UnaryAssignExpression(AbstractSpinVariableP variable, int vOperator):
        AbstractExpression(variable->sourcePosition),variable(variable),vOperator(vOperator) {
    }
    virtual ~UnaryAssignExpression() {}

    virtual void generate(SpinByteCodeWriter &byteCodeWriter, bool removeResultFromStack) const {
        variable->generate(byteCodeWriter, AbstractSpinVariable::Modify);
        byteCodeWriter.appendStaticByte(vOperator | (removeResultFromStack ? 0x00 : 0x80));
    }
    virtual std::string toILangStr() const {
        return "(modvar "+std::to_string(vOperator)+" "+variable->toILangStr()+")";
    }
protected:
    virtual void iterateChildExpressions(const std::function<void(AbstractExpressionP)>& callback) const {
        variable->iterateChildExpressions(callback);
    }
    virtual AbstractExpressionP mapChildExpressions(const std::function<AbstractExpressionP(AbstractExpressionP)>& callback) const {
        auto newVarInfo = variable->mapChildExpressions(callback);
        if (!newVarInfo)
            return AbstractExpressionP(); //nullptr means no change
        return AbstractExpressionP(new UnaryAssignExpression(newVarInfo, vOperator));
    }
};

class AssignExpression : public AbstractExpression {
public:
    const AbstractExpressionP valueExpression;
    const AbstractSpinVariableP variable;
    const OperatorType::Type operation; //None if just assign

    explicit AssignExpression(const SourcePosition& sourcePosition, AbstractExpressionP valueExpression, AbstractSpinVariableP variable, OperatorType::Type operation):
        AbstractExpression(sourcePosition),valueExpression(valueExpression),variable(variable),operation(operation) {}
    virtual ~AssignExpression() {}

    virtual void generate(SpinByteCodeWriter &byteCodeWriter, bool removeResultFromStack) const {
        valueExpression->generate(byteCodeWriter, false);
        if (operation != OperatorType::None) {
            variable->generate(byteCodeWriter, AbstractSpinVariable::Modify);
            byteCodeWriter.appendStaticByte(operation | (removeResultFromStack ? 0x40 : 0xC0));
        }
        else if (!removeResultFromStack) {
            variable->generate(byteCodeWriter, AbstractSpinVariable::Modify);
            byteCodeWriter.appendStaticByte(0x80);
        }
        else
            variable->generate(byteCodeWriter, AbstractSpinVariable::Write);
    }
    virtual std::string toILangStr() const {
        std::string result = operation != OperatorType::None ? "(assignop "+OperatorType::toString(operation)+" " : "(assign ";
        return result + variable->toILangStr() + " " + valueExpression->toILangStr()+")";
    }
protected:
    virtual void iterateChildExpressions(const std::function<void(AbstractExpressionP)>& callback) const {
        iterateExpression(valueExpression, callback);
        variable->iterateChildExpressions(callback);
    }
    virtual AbstractExpressionP mapChildExpressions(const std::function<AbstractExpressionP(AbstractExpressionP)>& callback) const {
        bool modified=false;
        auto valueExpressionNew = mapExpression(valueExpression, callback, &modified);
        auto newVarInfo = variable->mapChildExpressions(callback);
        if (!newVarInfo && !modified)
            return AbstractExpressionP(); //nullptr means no change
        return AbstractExpressionP(new AssignExpression(sourcePosition, valueExpressionNew, newVarInfo ? newVarInfo : variable, operation));
    }
};

class CogNewSpinExpression : public AbstractExpression {
public:
    const std::vector<AbstractExpressionP> parameters;
    const AbstractExpressionP stackExpression;
    const MethodId methodId;

    explicit CogNewSpinExpression(const SourcePosition& sourcePosition, const std::vector<AbstractExpressionP> &parameters, AbstractExpressionP stackExpression, MethodId methodId):
        AbstractExpression(sourcePosition),parameters(parameters),stackExpression(stackExpression),methodId(methodId) {}
    virtual ~CogNewSpinExpression() {}
    virtual void generate(SpinByteCodeWriter &byteCodeWriter, bool removeResultFromStack) const {
        for (auto p:parameters)
            p->generate(byteCodeWriter, false);
        byteCodeWriter.appendCogInitNewSpinSubroutine(sourcePosition, int(parameters.size()), methodId);
        stackExpression->generate(byteCodeWriter, false);
        byteCodeWriter.appendStaticByte(0x15); // run
        byteCodeWriter.appendStaticByte(removeResultFromStack ? 0x2C : 0x28); // coginit
    }
    virtual std::string toILangStr() const {
        std::string result = "(cogNewSpin "+std::to_string(methodId.value())+" "+stackExpression->toILangStr();
        for (auto p:parameters)
            result += " "+p->toILangStr();
        result += ")";
        return result;
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
        return AbstractExpressionP(new CogNewSpinExpression(sourcePosition, parametersNew, stackExpressionNew, methodId));
    }
};

class CogNewAsmExpression : public AbstractExpression {
public:
    const AbstractExpressionP address;
    const AbstractExpressionP startParameter;

    explicit CogNewAsmExpression(const SourcePosition& sourcePosition, AbstractExpressionP address, AbstractExpressionP startParameter):
        AbstractExpression(sourcePosition),address(address),startParameter(startParameter) {}
    virtual ~CogNewAsmExpression() {}
    virtual void generate(SpinByteCodeWriter &byteCodeWriter, bool removeResultFromStack) const {
        byteCodeWriter.appendStaticByte(0x34); // constant -1
        address->generate(byteCodeWriter, false);
        startParameter->generate(byteCodeWriter, false);
        byteCodeWriter.appendStaticByte(removeResultFromStack ? 0x2C : 0x28);
    }
    virtual std::string toILangStr() const {
        return "(cogNewAsm "+address->toILangStr()+" "+startParameter->toILangStr()+")";
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
        return AbstractExpressionP(new CogNewAsmExpression(sourcePosition, addressNew, startParameterNew));
    }
};

class MethodCallExpression : public AbstractExpression {
public:
    const std::vector<AbstractExpressionP> parameters;
    const AbstractExpressionP objectIndex; //might be nullptr
    const ObjectInstanceId objectInstanceId; //valid if object call, otherwise invalid for own methods
    const MethodId methodId;
    const bool trapCall;


    explicit MethodCallExpression(const SourcePosition& sourcePosition, const std::vector<AbstractExpressionP> &parameters, AbstractExpressionP objectIndex, ObjectInstanceId objectInstanceId, MethodId methodId, bool trapCall):
        AbstractExpression(sourcePosition),parameters(parameters),objectIndex(objectIndex),objectInstanceId(objectInstanceId),methodId(methodId),trapCall(trapCall) {}
    virtual ~MethodCallExpression() {}
    virtual void generate(SpinByteCodeWriter &byteCodeWriter, bool removeResultFromStack) const {
        byteCodeWriter.appendStaticByte((trapCall ? 2 : 0) | (removeResultFromStack ? 1 : 0)); // drop anchor (0..3 return/exception value handling)
        for (auto p:parameters)
            p->generate(byteCodeWriter, false);
        if (objectInstanceId.valid()) {
            if (objectIndex) {
                objectIndex->generate(byteCodeWriter, false);
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
    virtual std::string toILangStr() const {
        std::string paramstr = " "+std::to_string(methodId.value());
        for (auto p:parameters)
            paramstr += " "+p->toILangStr();
        std::string callType = trapCall ? "(call" : "(trapcall";
        if (objectInstanceId.valid()) {
            if (objectIndex)
                return callType+" obj "+std::to_string(objectInstanceId.value())+" idx "+objectIndex->toILangStr()+paramstr+")";
            return callType+" obj "+std::to_string(objectInstanceId.value())+paramstr+")";
        }
        return callType+paramstr+")";
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
        return AbstractExpressionP(new MethodCallExpression(sourcePosition, parametersNew, objectIndexNew, objectInstanceId, methodId, trapCall));
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
    virtual void generate(SpinByteCodeWriter &byteCodeWriter, bool removeResultFromStack) const {
        SpinByteCodeLabel label = byteCodeWriter.reserveLabel();
        byteCodeWriter.appendStaticByte(origTokenByteCode < 0x80 ? 0x36 : 0x35); //constant 1 or 0
        byteCodeWriter.appendAbsoluteAddress(label);
        condition->generate(byteCodeWriter, false);
        for (auto e:expressionList) {
            e.expr1->generate(byteCodeWriter, false);
            if (e.expr2) {
                e.expr2->generate(byteCodeWriter, false);
                byteCodeWriter.appendStaticByte((origTokenByteCode & 0x7F) | 2);
            }
            else
                byteCodeWriter.appendStaticByte(origTokenByteCode & 0x7F);
        }
        byteCodeWriter.appendStaticByte(0x0F); // lookdone
        byteCodeWriter.placeLabelHere(label);
        if (removeResultFromStack)
            byteCodeWriter.appendPopStack();
    }
    virtual std::string toILangStr() const {
        return "(look todo)"; //TODO
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
    const ConstantEncoding encoding;
    explicit PushConstantExpression(const SourcePosition& sourcePosition, AbstractConstantExpressionP constant, ConstantEncoding encoding):AbstractExpression(sourcePosition),constant(constant),encoding(encoding) {}
    explicit PushConstantExpression(const SourcePosition& sourcePosition, int value, ConstantEncoding encoding):AbstractExpression(sourcePosition),constant(ConstantValueExpression::create(sourcePosition,value)),encoding(encoding) {}
    virtual ~PushConstantExpression() {}
    virtual void generate(SpinByteCodeWriter &byteCodeWriter, bool removeResultFromStack) const {
        byteCodeWriter.appendStaticPushConstant(sourcePosition, constant, encoding);
        if (removeResultFromStack)
            byteCodeWriter.appendPopStack();
    }
    virtual std::string toILangStr() const {
        int c=0;
        if (constant->isConstant(&c))
            return std::to_string(c);
        return "(con "+constant->toILangStr()+")";
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
    virtual void generate(SpinByteCodeWriter &byteCodeWriter, bool removeResultFromStack) const {
        byteCodeWriter.appendStaticByte(0x87); // (memcp byte+pbase+address)
        byteCodeWriter.appendStringReference(sourcePosition, stringIndex, stringData);
        if (removeResultFromStack)
            byteCodeWriter.appendPopStack();
    }
    virtual std::string toILangStr() const {
        return "(pushstr todo )"; //TODO
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
    const BuiltInFunction::Type builtInFunction;
    explicit CallBuiltInExpression(const SourcePosition& sourcePosition, const std::vector<AbstractExpressionP> &parameters, BuiltInFunction::Type builtInFunction):AbstractExpression(sourcePosition),parameters(parameters),builtInFunction(builtInFunction) {}
    virtual ~CallBuiltInExpression() {}
    virtual void generate(SpinByteCodeWriter &byteCodeWriter, bool removeResultFromStack) const {
        for (auto p:parameters)
            p->generate(byteCodeWriter, false);
        if (!removeResultFromStack) {
            byteCodeWriter.appendStaticByte(builtInFunction);
            return;
        }
        if (BuiltInFunction::hasPopReturnValue(builtInFunction))
            byteCodeWriter.appendStaticByte(builtInFunction | 0x04);
        else {
            byteCodeWriter.appendStaticByte(builtInFunction);
            byteCodeWriter.appendPopStack();
        }
    }
    virtual std::string toILangStr() const {
        std::string result = "(callbuiltin "+BuiltInFunction::toString(builtInFunction);
        for (auto p:parameters)
            result += " "+p->toILangStr();
        return result;
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
        return AbstractExpressionP(new CallBuiltInExpression(sourcePosition, parametersNew, builtInFunction));
    }
};

class UnaryExpression : public AbstractExpression {
public:
    const AbstractExpressionP expression;
    const OperatorType::Type operation;
    explicit UnaryExpression(const SourcePosition& sourcePosition, AbstractExpressionP expression, OperatorType::Type operation):AbstractExpression(sourcePosition),expression(expression),operation(operation) {}
    virtual ~UnaryExpression() {}
    virtual void generate(SpinByteCodeWriter &byteCodeWriter, bool removeResultFromStack) const {
        expression->generate(byteCodeWriter, false);
        byteCodeWriter.appendStaticByte(operation | 0xE0); // math
        if (removeResultFromStack)
            byteCodeWriter.appendPopStack();
    }
    virtual std::string toILangStr() const {
        return "("+OperatorType::toString(operation)+" "+expression->toILangStr()+")";
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
        return AbstractExpressionP(new UnaryExpression(sourcePosition, expressionNew, operation));
    }
};

class BinaryExpression : public AbstractExpression {
public:
    const AbstractExpressionP left;
    const AbstractExpressionP right;
    const OperatorType::Type operation;
    explicit BinaryExpression(const SourcePosition& sourcePosition, AbstractExpressionP left, AbstractExpressionP right, OperatorType::Type operation):AbstractExpression(sourcePosition),left(left),right(right),operation(operation) {}
    virtual ~BinaryExpression() {}
    virtual void generate(SpinByteCodeWriter &byteCodeWriter, bool removeResultFromStack) const {
        left->generate(byteCodeWriter, false);
        right->generate(byteCodeWriter, false);
        byteCodeWriter.appendStaticByte(operation | 0xE0);
        if (removeResultFromStack)
            byteCodeWriter.appendPopStack();
    }
    virtual std::string toILangStr() const {
        return "("+OperatorType::toString(operation)+" "+left->toILangStr()+" "+right->toILangStr()+")";
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
        return AbstractExpressionP(new BinaryExpression(sourcePosition, leftNew, rightNew, operation));
    }
};

class AtAtExpression : public AbstractExpression {
public:
    const AbstractExpressionP expression;
    explicit AtAtExpression(const SourcePosition& sourcePosition, AbstractExpressionP expression):AbstractExpression(sourcePosition),expression(expression) {}
    virtual ~AtAtExpression() {}
    virtual void generate(SpinByteCodeWriter &byteCodeWriter, bool removeResultFromStack) const {
        expression->generate(byteCodeWriter, false);
        byteCodeWriter.appendStaticByte(0x97); // memop byte+index+pbase+address
        byteCodeWriter.appendStaticByte(0); // address 0
        if (removeResultFromStack)
            byteCodeWriter.appendPopStack();
    }
    virtual std::string toILangStr() const {
        return "(atat "+expression->toILangStr()+")";
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
