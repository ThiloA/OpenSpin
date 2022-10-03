//////////////////////////////////////////////////////////////
//                                                          //
// Propeller Spin/PASM Compiler                             //
// (c)2012-2016 Parallax Inc. DBA Parallax Semiconductor.   //
// Adapted from Chip Gracey's x86 asm code by Roy Eltham    //
// Rewritten to modern C++ by Thilo Ackermann               //
// See end of file for terms of use.                        //
//                                                          //
////////////////////////////////////////////////////////////// 

#ifndef SPINCOMPILER_INSTRUCTION_H
#define SPINCOMPILER_INSTRUCTION_H

#include "SpinCompiler/Generator/Expression.h"
#include "SpinCompiler/Types/ConstantExpression.h"

struct InstructionLoopContext {
    enum Type {NoLoop,RepeatCountLoop,NonRepeatCountLoop};
    InstructionLoopContext():type(NoLoop),caseCount(0) {}
    InstructionLoopContext(Type type, int caseCount, SpinByteCodeLabel nextAddress, SpinByteCodeLabel quitAddress):
        type(type),caseCount(caseCount),nextAddress(nextAddress),quitAddress(quitAddress) {}
    const Type type;
    const int caseCount;
    const SpinByteCodeLabel nextAddress;
    const SpinByteCodeLabel quitAddress;

    InstructionLoopContext newLoopBlock(bool isRepeatLoop, SpinByteCodeLabel nextAddressOfThisLoop, SpinByteCodeLabel quitAddressOfThisLoop) const {
        return InstructionLoopContext(isRepeatLoop ? RepeatCountLoop : NonRepeatCountLoop, 0, nextAddressOfThisLoop, quitAddressOfThisLoop);
    }
    InstructionLoopContext newCaseBlock() const {
        if (type == NoLoop)
            return InstructionLoopContext(); //ignore
        return InstructionLoopContext(type, caseCount+1, nextAddress, quitAddress);
    }
};

typedef std::shared_ptr<class AbstractInstruction> AbstractInstructionP;
class AbstractInstruction {
public:
    const SourcePosition sourcePosition;
    explicit AbstractInstruction(const SourcePosition &sourcePosition):sourcePosition(sourcePosition) {}
    virtual ~AbstractInstruction() {}
    virtual void generate(SpinByteCodeWriter &byteCodeWriter, const InstructionLoopContext& parentLoopContext) const=0;
    virtual void iterateChildExpressions(const std::function<void(AbstractExpressionP)>& callback) const=0;
    static void iterateInstruction(AbstractInstructionP instruction, const std::function<void(AbstractInstructionP)>& callback) {
        if (!instruction)
            return;
        callback(instruction);
        instruction->iterateChildInstructions(callback);
    }
    static AbstractInstructionP mapInstruction(AbstractInstructionP instruction, const std::function<AbstractInstructionP(AbstractInstructionP)>& instrCallback, const std::function<AbstractExpressionP(AbstractExpressionP)>& exprCallback, bool *modifiedMarker=nullptr) {
        if (!instruction)
            return AbstractInstructionP();
        auto inner = instruction->mapChildren(instrCallback,exprCallback); //returns nullptr iff no change
        auto res = instrCallback(inner ? inner : instruction);
        if (res != instruction && modifiedMarker)
            *modifiedMarker = true;
        return res;
    }
protected:
    virtual void iterateChildInstructions(const std::function<void(AbstractInstructionP)>& callback) const=0;
    virtual AbstractInstructionP mapChildren(const std::function<AbstractInstructionP(AbstractInstructionP)>& instrCallback, const std::function<AbstractExpressionP(AbstractExpressionP)>& exprCallback)=0;
};

class ExpressionInstruction : public AbstractInstruction {
public:
    const AbstractExpressionP expression;
    explicit ExpressionInstruction(AbstractExpressionP expression):AbstractInstruction(expression->sourcePosition),expression(expression) {}
    virtual ~ExpressionInstruction() {}
    virtual void generate(SpinByteCodeWriter &byteCodeWriter, const InstructionLoopContext&) const {
        expression->generate(byteCodeWriter);
    }
    virtual void iterateChildExpressions(const std::function<void(AbstractExpressionP)>& callback) const {
        AbstractExpression::iterateExpression(expression, callback);
    }
protected:
    virtual void iterateChildInstructions(const std::function<void(AbstractInstructionP)>&) const {
    }
    virtual AbstractInstructionP mapChildren(const std::function<AbstractInstructionP(AbstractInstructionP)>&, const std::function<AbstractExpressionP(AbstractExpressionP)>& exprCallback) {
        bool modified = false;
        auto expressionNew = AbstractExpression::mapExpression(expression, exprCallback, &modified);
        if (!modified)
            return AbstractInstructionP(); //nullptr means no change
        return AbstractInstructionP(new ExpressionInstruction(expressionNew));
    }
};

class CogInitSpinInstruction : public AbstractInstruction {
public:
    const std::vector<AbstractExpressionP> parameters;
    const AbstractExpressionP cogIdExpression;
    const AbstractExpressionP stackExpression;
    const MethodId methodId;

    explicit CogInitSpinInstruction(const SourcePosition& sourcePosition, const std::vector<AbstractExpressionP> &parameters, AbstractExpressionP cogIdExpression, AbstractExpressionP stackExpression, MethodId methodId):
        AbstractInstruction(sourcePosition),parameters(parameters),cogIdExpression(cogIdExpression),stackExpression(stackExpression),methodId(methodId) {}

    virtual ~CogInitSpinInstruction() {}
    virtual void generate(SpinByteCodeWriter &byteCodeWriter, const InstructionLoopContext&) const {
        for (auto p: parameters)
            p->generate(byteCodeWriter);
        byteCodeWriter.appendCogInitNewSpinSubroutine(sourcePosition, int(parameters.size()), methodId);
        stackExpression->generate(byteCodeWriter);
        byteCodeWriter.appendStaticByte(0x15); // run
        cogIdExpression->generate(byteCodeWriter);
        byteCodeWriter.appendStaticByte(0x3F); // regop
        byteCodeWriter.appendStaticByte(0x8F); // read+dcurr
        byteCodeWriter.appendStaticByte(0x37); // constant mask
        byteCodeWriter.appendStaticByte(0x61); // -4
        byteCodeWriter.appendStaticByte(0xD1); // write long[base][index]
        byteCodeWriter.appendStaticByte(0x2C); // coginit
    }
    virtual void iterateChildExpressions(const std::function<void(AbstractExpressionP)>& callback) const {
        AbstractExpression::iterateExpression(parameters, callback);
        AbstractExpression::iterateExpression(cogIdExpression, callback);
        AbstractExpression::iterateExpression(stackExpression, callback);
    }
protected:
    virtual void iterateChildInstructions(const std::function<void(AbstractInstructionP)>&) const {
    }
    virtual AbstractInstructionP mapChildren(const std::function<AbstractInstructionP(AbstractInstructionP)>&, const std::function<AbstractExpressionP(AbstractExpressionP)>& exprCallback) {
        bool modified = false;
        auto parametersNew = AbstractExpression::mapExpression(parameters, exprCallback, &modified);
        auto cogIdExpressionNew = AbstractExpression::mapExpression(cogIdExpression, exprCallback, &modified);
        auto stackExpressionNew = AbstractExpression::mapExpression(stackExpression, exprCallback, &modified);
        if (!modified)
            return AbstractInstructionP(); //nullptr means no change
        return AbstractInstructionP(new CogInitSpinInstruction(sourcePosition, parametersNew, cogIdExpressionNew, stackExpressionNew, methodId));
    }
};

class CogInitAsmInstruction : public AbstractInstruction {
public:
    const AbstractExpressionP cogId;
    const AbstractExpressionP address;
    const AbstractExpressionP startParameter;

    explicit CogInitAsmInstruction(const SourcePosition& sourcePosition, AbstractExpressionP cogId, AbstractExpressionP address, AbstractExpressionP startParameter):
        AbstractInstruction(sourcePosition),cogId(cogId),address(address),startParameter(startParameter) {}
    virtual ~CogInitAsmInstruction() {}
    virtual void generate(SpinByteCodeWriter &byteCodeWriter, const InstructionLoopContext&) const {
        cogId->generate(byteCodeWriter);
        address->generate(byteCodeWriter);
        startParameter->generate(byteCodeWriter);
        byteCodeWriter.appendStaticByte(0x2C);
    }
    virtual void iterateChildExpressions(const std::function<void(AbstractExpressionP)>& callback) const {
        AbstractExpression::iterateExpression(cogId, callback);
        AbstractExpression::iterateExpression(address, callback);
        AbstractExpression::iterateExpression(startParameter, callback);
    }
protected:
    virtual void iterateChildInstructions(const std::function<void(AbstractInstructionP)>&) const {
    }
    virtual AbstractInstructionP mapChildren(const std::function<AbstractInstructionP(AbstractInstructionP)>&, const std::function<AbstractExpressionP(AbstractExpressionP)>& exprCallback) {
        bool modified = false;
        auto cogIdNew = AbstractExpression::mapExpression(cogId, exprCallback, &modified);
        auto addressNew = AbstractExpression::mapExpression(address, exprCallback, &modified);
        auto startParameterNew = AbstractExpression::mapExpression(startParameter, exprCallback, &modified);
        if (!modified)
            return AbstractInstructionP(); //nullptr means no change
        return AbstractInstructionP(new CogInitAsmInstruction(sourcePosition, cogIdNew, addressNew, startParameterNew));
    }
};

class AbortOrReturnInstruction : public AbstractInstruction {
public:
    const AbstractExpressionP returnValue; //might be nullptr
    const int isAbort;

    explicit AbortOrReturnInstruction(const SourcePosition& sourcePosition, AbstractExpressionP returnValue, bool isAbort):
        AbstractInstruction(sourcePosition),returnValue(returnValue),isAbort(isAbort) {}
    virtual ~AbortOrReturnInstruction() {}
    virtual void generate(SpinByteCodeWriter &byteCodeWriter, const InstructionLoopContext&) const {
        if (returnValue) {
            returnValue->generate(byteCodeWriter);
            byteCodeWriter.appendStaticByte(isAbort ? 0X31 : 0X33);
        }
        else
            byteCodeWriter.appendStaticByte(isAbort ? 0X30 : 0X32);
    }
    virtual void iterateChildExpressions(const std::function<void(AbstractExpressionP)>& callback) const {
        AbstractExpression::iterateExpression(returnValue, callback);
    }
protected:
    virtual void iterateChildInstructions(const std::function<void(AbstractInstructionP)>&) const {
    }
    virtual AbstractInstructionP mapChildren(const std::function<AbstractInstructionP(AbstractInstructionP)>&, const std::function<AbstractExpressionP(AbstractExpressionP)>& exprCallback) {
        bool modified = false;
        auto returnValueNew = AbstractExpression::mapExpression(returnValue, exprCallback, &modified);
        if (!modified)
            return AbstractInstructionP(); //nullptr means no change
        return AbstractInstructionP(new AbortOrReturnInstruction(sourcePosition, returnValueNew, isAbort));
    }
};

class NextOrQuitInstruction : public AbstractInstruction {
public:
    const bool isNext;

    explicit NextOrQuitInstruction(const SourcePosition& sourcePosition, bool isNext):
        AbstractInstruction(sourcePosition),isNext(isNext) {}
    virtual ~NextOrQuitInstruction() {}
    virtual void generate(SpinByteCodeWriter &byteCodeWriter, const InstructionLoopContext& parentLoopContext) const {
        if (parentLoopContext.type == InstructionLoopContext::NoLoop)
            throw CompilerError(ErrorType::internal, sourcePosition);
        if (parentLoopContext.caseCount > 0) { // pop 2 longs for each nested 'case'
            byteCodeWriter.appendStaticPushConstant(sourcePosition, ConstantValueExpression::create(sourcePosition, parentLoopContext.caseCount*8)); // enter pop count
            byteCodeWriter.appendStaticByte(0x14); // pop
        }
        byteCodeWriter.appendStaticByte(isNext ? 0x04 : (parentLoopContext.type == InstructionLoopContext::RepeatCountLoop ? 0x0B : 0x04)); // jmp 'next' otherwise jnz/jmp 'quit'
        byteCodeWriter.appendRelativeAddress(isNext ? parentLoopContext.nextAddress : parentLoopContext.quitAddress);
    }
    virtual void iterateChildExpressions(const std::function<void(AbstractExpressionP)>&) const {
    }
protected:
    virtual void iterateChildInstructions(const std::function<void(AbstractInstructionP)>&) const {
    }
    virtual AbstractInstructionP mapChildren(const std::function<AbstractInstructionP(AbstractInstructionP)>&, const std::function<AbstractExpressionP(AbstractExpressionP)>&) {
        return AbstractInstructionP(); //nullptr means no change
    }
};


class CallBuiltInInstruction : public AbstractInstruction {
public:
    const std::vector<AbstractExpressionP> parameters;
    const int opCode; //TODO
    explicit CallBuiltInInstruction(const SourcePosition& sourcePosition, const std::vector<AbstractExpressionP> &parameters, int opCode):AbstractInstruction(sourcePosition),parameters(parameters),opCode(opCode) {}
    virtual ~CallBuiltInInstruction() {}
    virtual void generate(SpinByteCodeWriter &byteCodeWriter, const InstructionLoopContext&) const {
        for (auto p:parameters)
            p->generate(byteCodeWriter);
        byteCodeWriter.appendStaticByte(opCode);
    }
    virtual void iterateChildExpressions(const std::function<void(AbstractExpressionP)>& callback) const {
        AbstractExpression::iterateExpression(parameters, callback);
    }
protected:
    virtual void iterateChildInstructions(const std::function<void(AbstractInstructionP)>&) const {
    }
    virtual AbstractInstructionP mapChildren(const std::function<AbstractInstructionP(AbstractInstructionP)>&, const std::function<AbstractExpressionP(AbstractExpressionP)>& exprCallback) {
        bool modified = false;
        auto parametersNew = AbstractExpression::mapExpression(parameters, exprCallback, &modified);
        if (!modified)
            return AbstractInstructionP(); //nullptr means no change
        return AbstractInstructionP(new CallBuiltInInstruction(sourcePosition, parametersNew, opCode));
    }
};

class BlockInstruction : public AbstractInstruction {
public:
    const std::vector<AbstractInstructionP> children;
    explicit BlockInstruction(const SourcePosition& sourcePosition, const std::vector<AbstractInstructionP> &children):AbstractInstruction(sourcePosition),children(children) {}
    virtual ~BlockInstruction() {}
    virtual void generate(SpinByteCodeWriter &byteCodeWriter, const InstructionLoopContext& parentLoopContext) const {
        for (auto i:children)
            i->generate(byteCodeWriter, parentLoopContext);
    }
    virtual void iterateChildExpressions(const std::function<void(AbstractExpressionP)>&) const {
    }
protected:
    virtual void iterateChildInstructions(const std::function<void(AbstractInstructionP)>& callback) const {
        for (const auto&c:children)
            iterateInstruction(c, callback);
    }
    virtual AbstractInstructionP mapChildren(const std::function<AbstractInstructionP(AbstractInstructionP)>& instrCallback, const std::function<AbstractExpressionP(AbstractExpressionP)>& exprCallback) {
        bool modified = false;
        std::vector<AbstractInstructionP> childrenNew(children.size());
        for (unsigned int i=0; i<children.size(); ++i)
            childrenNew[i] = mapInstruction(children[i], instrCallback, exprCallback, &modified);
        if (!modified)
            return AbstractInstructionP(); //nullptr means no change
        return AbstractInstructionP(new BlockInstruction(sourcePosition, childrenNew));
    }
};
typedef std::shared_ptr<BlockInstruction> BlockInstructionP;

class IfInstruction : public AbstractInstruction {
public:
    struct Branch {
        Branch():conditionInverted(false) {}
        Branch(AbstractExpressionP condition, AbstractInstructionP instruction, bool conditionInverted):condition(condition),instruction(instruction),conditionInverted(conditionInverted) {}
        AbstractExpressionP condition;
        AbstractInstructionP instruction;
        bool conditionInverted;
    };
    const std::vector<Branch> branches;
    AbstractInstructionP elseBranch; //might be nullptr
    explicit IfInstruction(const SourcePosition& sourcePosition, const std::vector<Branch> &branches, AbstractInstructionP elseBranch):AbstractInstruction(sourcePosition),branches(branches),elseBranch(elseBranch) {}
    virtual ~IfInstruction() {}
    virtual void generate(SpinByteCodeWriter &byteCodeWriter, const InstructionLoopContext& parentLoopContext) const {
        //this label will be placed at the end after all if/elseif/else blocks
        const SpinByteCodeLabel endOfBlockLabel = byteCodeWriter.reserveLabel();
        //this label will always point to the next elseif / else or end of all blocks
        //it will be replaced in every if / elseif block to a new label
        SpinByteCodeLabel nextIfBranchLabel = byteCodeWriter.reserveLabel();

        //iterate through all conditional branches
        for (unsigned int i=0; i<branches.size(); ++i) {
            auto b = branches[i];
            bool hasMoreBranches = (i < branches.size()-1) || elseBranch;
            b.condition->generate(byteCodeWriter);
            byteCodeWriter.appendStaticByte(b.conditionInverted ? 0x0B : 0x0A);
            byteCodeWriter.appendRelativeAddress(nextIfBranchLabel); //jump to next branch
            b.instruction->generate(byteCodeWriter, parentLoopContext);
            if (hasMoreBranches) {
                byteCodeWriter.appendStaticByte(0x04); // jmp
                byteCodeWriter.appendRelativeAddress(endOfBlockLabel);
            }
            byteCodeWriter.placeLabelHere(nextIfBranchLabel);
            nextIfBranchLabel = byteCodeWriter.reserveLabel();
        }
        if (elseBranch)
            elseBranch->generate(byteCodeWriter, parentLoopContext);
        byteCodeWriter.placeLabelHere(nextIfBranchLabel);
        byteCodeWriter.placeLabelHere(endOfBlockLabel);
    }
    virtual void iterateChildExpressions(const std::function<void(AbstractExpressionP)>& callback) const {
        for (auto b:branches)
            AbstractExpression::iterateExpression(b.condition, callback);
    }
protected:
    virtual void iterateChildInstructions(const std::function<void(AbstractInstructionP)>& callback) const {
        for (const auto&b:branches)
            iterateInstruction(b.instruction, callback);
        iterateInstruction(elseBranch, callback);
    }
    virtual AbstractInstructionP mapChildren(const std::function<AbstractInstructionP(AbstractInstructionP)>& instrCallback, const std::function<AbstractExpressionP(AbstractExpressionP)>& exprCallback) {
        bool modified = false;
        std::vector<Branch> branchesNew(branches.size());
        for (unsigned int i=0; i<branches.size(); ++i) {
            branchesNew[i].instruction = mapInstruction(branches[i].instruction, instrCallback, exprCallback, &modified);
            branchesNew[i].conditionInverted = branchesNew[i].conditionInverted;
            branchesNew[i].condition = AbstractExpression::mapExpression(branches[i].condition, exprCallback, &modified);
        }

        auto elseBranchNew = mapInstruction(elseBranch, instrCallback, exprCallback, &modified);
        if (!modified)
            return AbstractInstructionP(); //nullptr means no change
        return AbstractInstructionP(new IfInstruction(sourcePosition, branchesNew, elseBranchNew));
    }
};

class CaseInstruction : public AbstractInstruction {
public:
    struct ExpressionListEntry {
        ExpressionListEntry() {}
        ExpressionListEntry(AbstractExpressionP expr1, AbstractExpressionP expr2):expr1(expr1),expr2(expr2) {}
        AbstractExpressionP expr1;
        AbstractExpressionP expr2; //might be nullptr if not a range
    };
    struct CaseEntry {
        CaseEntry() {}
        CaseEntry(const std::vector<ExpressionListEntry> &expressions, AbstractInstructionP instruction):expressions(expressions),instruction(instruction) {}
        std::vector<ExpressionListEntry> expressions;
        AbstractInstructionP instruction;
    };
    const AbstractExpressionP condition;
    const std::vector<CaseEntry> cases;
    AbstractInstructionP otherInstruction; //might by nullptr

    explicit CaseInstruction(const SourcePosition& sourcePosition, AbstractExpressionP condition, const std::vector<CaseEntry> &cases, AbstractInstructionP otherInstruction):
        AbstractInstruction(sourcePosition),condition(condition),cases(cases),otherInstruction(otherInstruction) {}
    virtual ~CaseInstruction() {}
    virtual void generate(SpinByteCodeWriter &byteCodeWriter, const InstructionLoopContext& parentLoopContext) const {
        auto endLabel = byteCodeWriter.reserveLabel();
        byteCodeWriter.appendAbsoluteAddress(endLabel);
        condition->generate(byteCodeWriter);

        auto childLoopContext = parentLoopContext.newCaseBlock();

        std::vector<SpinByteCodeLabel> caseLabels(cases.size());
        for (unsigned int i=0; i<cases.size(); ++i) {
            auto caseLbl = byteCodeWriter.reserveLabel();
            caseLabels[i]=caseLbl;
            for (auto e:cases[i].expressions) {
                e.expr1->generate(byteCodeWriter);
                if (e.expr2)
                    e.expr2->generate(byteCodeWriter);
                byteCodeWriter.appendStaticByte(e.expr2 ? 0x0E : 0x0D); // enter bytecode for case range or case value into obj
                byteCodeWriter.appendRelativeAddress(caseLbl);
            }
        }

        if (otherInstruction)
            otherInstruction->generate(byteCodeWriter, childLoopContext);
        byteCodeWriter.appendStaticByte(0x0C); // casedone, end of range checks

        for (unsigned int i=0; i<cases.size(); ++i) {
            byteCodeWriter.placeLabelHere(caseLabels[i]);
            cases[i].instruction->generate(byteCodeWriter, childLoopContext);
            byteCodeWriter.appendStaticByte(0x0C); // casedone
        }
        byteCodeWriter.placeLabelHere(endLabel);
    }
    virtual void iterateChildExpressions(const std::function<void(AbstractExpressionP)>& callback) const {
        AbstractExpression::iterateExpression(condition, callback);
        for (const auto& cse:cases) {
            for (const auto& expr: cse.expressions) {
                AbstractExpression::iterateExpression(expr.expr1, callback);
                AbstractExpression::iterateExpression(expr.expr2, callback);
            }
        }
    }
protected:
    virtual void iterateChildInstructions(const std::function<void(AbstractInstructionP)>& callback) const {
        for (const auto&c:cases)
            iterateInstruction(c.instruction, callback);
        iterateInstruction(otherInstruction, callback);
    }
    virtual AbstractInstructionP mapChildren(const std::function<AbstractInstructionP(AbstractInstructionP)>& instrCallback, const std::function<AbstractExpressionP(AbstractExpressionP)>& exprCallback) {
        bool modified = false;

        auto otherInstructionNew = mapInstruction(otherInstruction, instrCallback, exprCallback, &modified);
        auto conditionNew = AbstractExpression::mapExpression(condition, exprCallback, &modified);

        std::vector<CaseEntry> casesNew(cases.size());
        for (unsigned int i=0; i<cases.size(); ++i) {
            casesNew[i].instruction = mapInstruction(cases[i].instruction, instrCallback, exprCallback, &modified);
            casesNew[i].expressions.resize(cases[i].expressions.size());
            for (unsigned int j=0; j<cases.size(); ++j) {
                casesNew[i].expressions[j].expr1 = AbstractExpression::mapExpression(cases[i].expressions[j].expr1, exprCallback, &modified);
                casesNew[i].expressions[j].expr2 = AbstractExpression::mapExpression(cases[i].expressions[j].expr2, exprCallback, &modified);
            }
        }
        if (!modified)
            return AbstractInstructionP(); //nullptr means no change
        return AbstractInstructionP(new CaseInstruction(sourcePosition, conditionNew, casesNew, otherInstructionNew));
    }
};

class LoopConditionInstruction : public AbstractInstruction {
public:
    enum Type {RepeatCount,PreWhile,PreUntil,PostWhile,PostUntil,RepeatEndless};
    const Type type;
    const AbstractExpressionP condition; //might be nullptr if type is RepeatEndless
    const AbstractInstructionP instruction;
    explicit LoopConditionInstruction(const SourcePosition& sourcePosition, Type type, AbstractExpressionP condition, AbstractInstructionP instruction):AbstractInstruction(sourcePosition),type(type),condition(condition),instruction(instruction) {}
    virtual ~LoopConditionInstruction() {}
    virtual void generate(SpinByteCodeWriter &byteCodeWriter, const InstructionLoopContext& parentLoopContext) const {
        auto nextLabel = byteCodeWriter.reserveLabel();
        auto quitLabel = byteCodeWriter.reserveLabel();
        auto childLoopContext = parentLoopContext.newLoopBlock(type == RepeatCount,nextLabel,quitLabel);
        if (type == RepeatCount) {
            condition->generate(byteCodeWriter);
            byteCodeWriter.appendStaticByte(0x08); // (tjz)
            byteCodeWriter.appendRelativeAddress(quitLabel);
            auto reverseLabel = byteCodeWriter.reserveLabel();
            byteCodeWriter.placeLabelHere(reverseLabel);
            instruction->generate(byteCodeWriter, childLoopContext);
            byteCodeWriter.placeLabelHere(nextLabel);
            byteCodeWriter.appendStaticByte(0x09); // (djnz)
            byteCodeWriter.appendRelativeAddress(reverseLabel); // compile reverse address
        }
        else if (type == PreWhile || type == PreUntil) {
            byteCodeWriter.placeLabelHere(nextLabel);
            condition->generate(byteCodeWriter); //condition
            byteCodeWriter.appendStaticByte(type == PreWhile ? 0x0A : 0x0B); // enter the passed in bytecode (jz or jnz)
            byteCodeWriter.appendRelativeAddress(quitLabel);
            instruction->generate(byteCodeWriter, childLoopContext);
            byteCodeWriter.appendStaticByte(0x04); // (jmp)
            byteCodeWriter.appendRelativeAddress(nextLabel);
        }
        else if (type == PostWhile || type == PostUntil) {
            auto reverseLabel = byteCodeWriter.reserveLabel();
            byteCodeWriter.placeLabelHere(reverseLabel);
            instruction->generate(byteCodeWriter, childLoopContext);
            byteCodeWriter.placeLabelHere(nextLabel);
            condition->generate(byteCodeWriter);
            byteCodeWriter.appendStaticByte(type == PostWhile ? 0x0B : 0x0A);
            byteCodeWriter.appendRelativeAddress(reverseLabel);
        }
        else { //RepeatEndless
            auto reverseLabel = byteCodeWriter.reserveLabel();
            byteCodeWriter.placeLabelHere(reverseLabel);
            byteCodeWriter.placeLabelHere(nextLabel);
            instruction->generate(byteCodeWriter, childLoopContext);
            byteCodeWriter.appendStaticByte(0x04);
            byteCodeWriter.appendRelativeAddress(reverseLabel);
        }
        byteCodeWriter.placeLabelHere(quitLabel);
    }
    virtual void iterateChildExpressions(const std::function<void(AbstractExpressionP)>& callback) const {
        AbstractExpression::iterateExpression(condition, callback);
    }
protected:
    virtual void iterateChildInstructions(const std::function<void(AbstractInstructionP)>& callback) const {
        iterateInstruction(instruction, callback);
    }
    virtual AbstractInstructionP mapChildren(const std::function<AbstractInstructionP(AbstractInstructionP)>& instrCallback, const std::function<AbstractExpressionP(AbstractExpressionP)>& exprCallback) {
        bool modified = false;

        auto instructionNew = mapInstruction(instruction, instrCallback, exprCallback, &modified);
        auto conditionNew = AbstractExpression::mapExpression(condition, exprCallback, &modified);
        if (!modified)
            return AbstractInstructionP(); //nullptr means no change
        return AbstractInstructionP(new LoopConditionInstruction(sourcePosition, type, conditionNew, instructionNew));
    }
};

class LoopVarInstruction : public AbstractInstruction {
public:
    const AbstractSpinVariableP variable;
    const AbstractExpressionP from;
    const AbstractExpressionP to;
    const AbstractExpressionP step; //might be nullptr
    const AbstractInstructionP instruction;
    explicit LoopVarInstruction(const SourcePosition& sourcePosition, AbstractSpinVariableP variable, AbstractExpressionP from, AbstractExpressionP to, AbstractExpressionP step, AbstractInstructionP instruction):AbstractInstruction(sourcePosition),variable(variable),from(from),to(to),step(step),instruction(instruction) {}
    virtual ~LoopVarInstruction() {}
    virtual void generate(SpinByteCodeWriter &byteCodeWriter, const InstructionLoopContext& parentLoopContext) const {
        auto nextLabel = byteCodeWriter.reserveLabel();
        auto quitLabel = byteCodeWriter.reserveLabel();
        auto childLoopContext = parentLoopContext.newLoopBlock(false,nextLabel,quitLabel);
        from->generate(byteCodeWriter);
        variable->generate(byteCodeWriter,AbstractSpinVariable::Write);
        auto reverseLabel = byteCodeWriter.reserveLabel();
        byteCodeWriter.placeLabelHere(reverseLabel);
        instruction->generate(byteCodeWriter, childLoopContext);
        byteCodeWriter.placeLabelHere(nextLabel); // set 'next' address
        if (step)
            step->generate(byteCodeWriter);
        from->generate(byteCodeWriter);
        to->generate(byteCodeWriter);
        variable->generate(byteCodeWriter, AbstractSpinVariable::Modify);
        byteCodeWriter.appendStaticByte(step ? 0x06 : 0x02);
        byteCodeWriter.appendRelativeAddress(reverseLabel); // compile reverse address
        byteCodeWriter.placeLabelHere(quitLabel); // set 'quit'/forward address
    }
    virtual void iterateChildExpressions(const std::function<void(AbstractExpressionP)>& callback) const {
        variable->iterateChildExpressions(callback);
        AbstractExpression::iterateExpression(from, callback);
        AbstractExpression::iterateExpression(to, callback);
        AbstractExpression::iterateExpression(step, callback);
    }
protected:
    virtual void iterateChildInstructions(const std::function<void(AbstractInstructionP)>& callback) const {
        iterateInstruction(instruction, callback);
    }
    virtual AbstractInstructionP mapChildren(const std::function<AbstractInstructionP(AbstractInstructionP)>& instrCallback, const std::function<AbstractExpressionP(AbstractExpressionP)>& exprCallback) {
        bool modified = false;

        auto instructionNew = mapInstruction(instruction, instrCallback, exprCallback, &modified);
        auto variableNew = variable->mapChildExpressions(exprCallback);
        if (variableNew)
            modified = true;
        else
            variableNew = variable;
        auto fromNew = AbstractExpression::mapExpression(from, exprCallback, &modified);
        auto toNew = AbstractExpression::mapExpression(to, exprCallback, &modified);
        auto stepNew = AbstractExpression::mapExpression(step, exprCallback, &modified);
        if (!modified)
            return AbstractInstructionP(); //nullptr means no change
        return AbstractInstructionP(new LoopVarInstruction(sourcePosition, variableNew, fromNew, toNew, stepNew, instructionNew));
    }
};

#endif //SPINCOMPILER_INSTRUCTION_H

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
