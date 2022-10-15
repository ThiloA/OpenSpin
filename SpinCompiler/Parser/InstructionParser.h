//////////////////////////////////////////////////////////////
//                                                          //
// Propeller Spin/PASM Compiler                             //
// (c)2012-2016 Parallax Inc. DBA Parallax Semiconductor.   //
// Adapted from Chip Gracey's x86 asm code by Roy Eltham    //
// Rewritten to modern C++ by Thilo Ackermann               //
// See end of file for terms of use.                        //
//                                                          //
////////////////////////////////////////////////////////////// 

#ifndef SPINCOMPILER_INSTRUCTIONPARSER_H
#define SPINCOMPILER_INSTRUCTIONPARSER_H

#include "SpinCompiler/Parser/ExpressionParser.h"
#include "SpinCompiler/Generator/Instruction.h"

class InstructionParser {
private:
    ParserFunctionContext m_context;
    TokenReader &m_reader;
public:
    explicit InstructionParser(const ParserFunctionContext &context):m_context(context),m_reader(context.reader) {}

    AbstractInstructionP parseInstruction(Token& tk0) {
        ExpressionParser exprCompiler(m_context, true);
        switch(tk0.type) {
            case Token::DefinedSymbol: {
                if (auto objSym = std::dynamic_pointer_cast<SpinObjSymbol>(tk0.resolvedSymbol))
                    return AbstractInstructionP(new ExpressionInstruction(exprCompiler.parseChildObjectMethodCall(objSym, false)));
                if (auto subSym = std::dynamic_pointer_cast<SpinSubSymbol>(tk0.resolvedSymbol))
                    return AbstractInstructionP(new ExpressionInstruction(exprCompiler.parseMethodCall(tk0.sourcePosition, subSym, false)));
                break;
            }
            case Token::Backslash:
                return AbstractInstructionP(new ExpressionInstruction(exprCompiler.parseTryCall(true)));
            case Token::NextQuit:
                return parseNextOrQuit((tk0.value & 0xFF) == 0);
            case Token::Abort:
                return parseAbortOrReturn(true);
            case Token::Return:
                return parseAbortOrReturn(false);
            case Token::Reboot:
                return parseReboot();
            case Token::CogNew:
                return AbstractInstructionP(new ExpressionInstruction(exprCompiler.parseCogNew())); // no push;
            case Token::CogInit:
                return parseCogInit(exprCompiler);
            case Token::InstCanReturn: // instruction can-return
                return AbstractInstructionP(new ExpressionInstruction(AbstractExpressionP(new CallBuiltInExpression(tk0.sourcePosition,
                                                                                                             exprCompiler.parseParameters(tk0.unpackBuiltInParameterCount()),
                                                                                                             BuiltInFunction::Type(tk0.unpackBuiltInByteCode())
                                                                                                             ))));
            case Token::InstNeverReturn: // instruction never-return
                return AbstractInstructionP(new CallBuiltInInstruction(tk0.sourcePosition,
                                                                       exprCompiler.parseParameters(tk0.unpackBuiltInParameterCount()),
                                                                       tk0.unpackBuiltInByteCode()));
            case Token::Inc: // assign pre-inc  ++var
                return AbstractInstructionP(new ExpressionInstruction(exprCompiler.compileVariablePreIncOrDec(0x20)));
            case Token::Dec: // assign pre-dec  --var
                return AbstractInstructionP(new ExpressionInstruction(exprCompiler.compileVariablePreIncOrDec(0x30)));
            case Token::Tilde: // assign sign-extern byte  ~var
                return AbstractInstructionP(new ExpressionInstruction(exprCompiler.compileVariablePreSignExtendOrRandom(0x10)));
            case Token::TildeTilde: // assign sign-extern word  ~~var
                return AbstractInstructionP(new ExpressionInstruction(exprCompiler.compileVariablePreSignExtendOrRandom(0x14)));
            case Token::Random: // assign random forward  ?var
                return AbstractInstructionP(new ExpressionInstruction(exprCompiler.compileVariablePreSignExtendOrRandom(0x08)));
            default:
                break;
        }

        tk0.subToNeg();
        if (tk0.type == Token::Unary)
            return parseUnary(exprCompiler,tk0.opType);

        auto varInfo = exprCompiler.getVariable(tk0, ErrorType::eaiov);
        // check for post-var modifier
        const auto tk2 = m_reader.getNextToken();
        switch (tk2.type) {
            case Token::Inc: // assign post-inc
                return AbstractInstructionP(new ExpressionInstruction(exprCompiler.compileVariableIncOrDec(0x28, varInfo)));
            case Token::Dec: // assign post-dec
                return AbstractInstructionP(new ExpressionInstruction(exprCompiler.compileVariableIncOrDec(0x38, varInfo)));
            case Token::Random: // assign random reverse
                return AbstractInstructionP(new ExpressionInstruction(exprCompiler.compileVariableSideEffectOperation(0x0C, varInfo)));
            case Token::Tilde: // assign post-clear
                return AbstractInstructionP(new ExpressionInstruction(exprCompiler.compileVariableSideEffectOperation(0x18, varInfo)));
            case Token::TildeTilde: // assign post-set
                return AbstractInstructionP(new ExpressionInstruction(exprCompiler.compileVariableSideEffectOperation(0x1C, varInfo)));
            case Token::Assign:
                return parseAssign(exprCompiler, varInfo);
            default:
                break;
        }

        // var binaryop?
        if (tk2.type == Token::Binary) {
            // check for '=' after binary op
            const auto tk3 = m_reader.getNextToken();
            if (tk3.type == Token::Equal)
                return AbstractInstructionP(new ExpressionInstruction(exprCompiler.compileVariableAssignExpression(OperatorType::Type(tk2.opType), varInfo))); //TODO operator type cast weg
            m_reader.goBack(); // not '=' so backup
        }
        m_reader.goBack(); // no post-var modifier, so backup
        throw CompilerError(ErrorType::vnao, tk2);
    }
private:
    AbstractInstructionP parseNextOrQuit(bool isNext) {
        auto sourcePosition = m_reader.getSourcePosition();
        if (!m_context.insideLoopBody)
            throw CompilerError(ErrorType::tioawarb, sourcePosition);
        return AbstractInstructionP (new NextOrQuitInstruction(sourcePosition, isNext));
    }

    AbstractInstructionP parseAbortOrReturn(bool isAbort) {
        // preview next element
        auto tk = m_reader.getNextToken();
        m_reader.goBack();
        AbstractExpressionP returnValue;
        if (tk.type != Token::End) // there's an expression, compile it
            returnValue = ExpressionParser(m_context, true).parseExpression();
        return AbstractInstructionP(new AbortOrReturnInstruction(tk.sourcePosition, returnValue, isAbort));
    }

    AbstractInstructionP parseReboot() {
        //TODO hack
        return AbstractInstructionP(new ExpressionInstruction(AbstractExpressionP(new FixedByteCodeSequenceExpression(m_reader.getSourcePosition(),std::vector<unsigned char>{
            0x37,0x06, // constant 0x80
            0x35, // constant 0
            0x20 //clkset
        }))));
    }

    AbstractInstructionP parseCogInit(ExpressionParser& exprCompiler) {
        m_reader.forceElement(Token::LeftBracket);
        auto cogIdExpression = exprCompiler.parseExpression();
        m_reader.forceElement(Token::Comma);

        // check for subroutine
        const auto tk1 = m_reader.getNextToken();
        auto subSym = std::dynamic_pointer_cast<SpinSubSymbol>(tk1.resolvedSymbol);
        if (!subSym) {
            m_reader.goBack();
            auto param2 = exprCompiler.parseExpression();
            m_reader.forceElement(Token::Comma);
            auto param3 = exprCompiler.parseExpression();
            m_reader.forceElement(Token::RightBracket);
            return AbstractInstructionP(new CogInitAsmInstruction(tk1.sourcePosition,cogIdExpression,param2,param3));
        }

        // compile subroutine 'cognew' (push params+index)
        auto params = exprCompiler.parseParameters(subSym->parameterCount);
        m_reader.forceElement(Token::Comma);
        auto stackExpression = exprCompiler.parseExpression(); // compile stack expression
        m_reader.forceElement(Token::RightBracket);
        return AbstractInstructionP(new CogInitSpinInstruction(tk1.sourcePosition, params, cogIdExpression, stackExpression, subSym->methodId));
    }

    AbstractInstructionP parseUnary(ExpressionParser& exprCompiler, int value) {
        return AbstractInstructionP(new ExpressionInstruction(exprCompiler.compileVariablePreSignExtendOrRandom(0x40 | value)));
    }

    AbstractInstructionP parseAssign(ExpressionParser& exprCompiler, AbstractSpinVariableP varInfo) {
        auto pos = m_reader.getSourcePosition();
        auto valueExpression = exprCompiler.parseExpression();
        return AbstractInstructionP(new ExpressionInstruction(AbstractExpressionP(new AssignExpression(pos, valueExpression, varInfo, OperatorType::None))));
    }
};

#endif //SPINCOMPILER_INSTRUCTIONPARSER_H

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
