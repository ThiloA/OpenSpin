//////////////////////////////////////////////////////////////
//                                                          //
// Propeller Spin/PASM Compiler                             //
// (c)2012-2016 Parallax Inc. DBA Parallax Semiconductor.   //
// Adapted from Chip Gracey's x86 asm code by Roy Eltham    //
// Rewritten to modern C++ by Thilo Ackermann               //
// See end of file for terms of use.                        //
//                                                          //
////////////////////////////////////////////////////////////// 

#ifndef SPINCOMPILER_INSTRUCTIONBLOCKPARSER_H
#define SPINCOMPILER_INSTRUCTIONBLOCKPARSER_H

#include "SpinCompiler/Parser/InstructionParser.h"

class InstructionBlockParser {
public:
    explicit InstructionBlockParser(const ParserFunctionContext &context):m_context(context),m_reader(context.reader) {}
    static AbstractInstructionP compileFunction(TokenReader &reader, ParserObjectContext& objectContext) {
        int stringCounter=0;
        return InstructionBlockParser(ParserFunctionContext(
                                         reader, objectContext, false,stringCounter
                                         )).parseBlock(0,true);
    }
private:
    ParserFunctionContext m_context;
    TokenReader &m_reader;
    BlockInstructionP parseBlock(int column, bool isTopBlock=false) {
        const auto startSourcePosition = m_reader.getSourcePosition();
        std::vector<AbstractInstructionP> children;
        while (true) {
            auto tk = m_reader.getNextNonBlockOrNewlineToken();
            if (tk.eof)
                break;
            const int tkCol = tk.sourcePosition.column;
            if (tkCol <= column)
                break;
            if (tk.type == Token::If)
                children.push_back(parseCompleteIfOrIfNot(tkCol, false));
            else if (tk.type == Token::IfNot)
                children.push_back(parseCompleteIfOrIfNot(tkCol, true));
            else if (tk.type == Token::Case)
                children.push_back(parseCase(tkCol));
            else if (tk.type == Token::Repeat)
                children.push_back(parseRepeat(tkCol));
            else {
                children.push_back(InstructionParser(m_context).parseInstruction(tk));
                m_reader.forceElement(Token::End);
            }
        }
        if (isTopBlock)
            children.push_back(AbstractInstructionP(new AbortOrReturnInstruction(m_reader.getSourcePosition(), AbstractExpressionP(), false)));
        m_reader.goBack();
        return BlockInstructionP(new BlockInstruction(startSourcePosition, children));
    }

    IfInstruction::Branch parseIfBranch(int column, bool inverted) {
        auto condition = ExpressionParser(m_context,true).parseExpression();
        m_reader.forceElement(Token::End);
        return IfInstruction::Branch(condition,parseBlock(column),inverted);
    }

    AbstractInstructionP parseCompleteIfOrIfNot(int column, bool inverted) {
        const auto sourcePosition = m_reader.getSourcePosition();
        std::vector<IfInstruction::Branch> branches;
        AbstractInstructionP elseBranch;
        branches.push_back(parseIfBranch(column, inverted));
        while (true) {
            auto tk = m_reader.getNextToken();
            if (tk.eof)
                break;
            const int tkCol = tk.sourcePosition.column;
            if (tkCol < column) {
                m_reader.goBack();
                break;
            }
            if (tk.type == Token::ElseIf)
                branches.push_back(parseIfBranch(column, false));
            else if (tk.type == Token::ElseIfNot)
                branches.push_back(parseIfBranch(column, true));
            else if (tk.type == Token::Else) {
                m_reader.forceElement(Token::End);
                elseBranch = parseBlock(column);
                break;
            }
            else {
                m_reader.goBack();
                break;
            }
        }
        return AbstractInstructionP(new IfInstruction(sourcePosition, branches, elseBranch));
    }

    AbstractInstructionP parseCase(int column) {
        const auto sourcePosition = m_reader.getSourcePosition();
        ExpressionParser exprCompiler(m_context,true);
        const AbstractExpressionP condition = exprCompiler.parseExpression();
        m_reader.forceElement(Token::End);
        std::vector<CaseInstruction::CaseEntry> cases;
        AbstractInstructionP otherInstruction; //might by nullptr

        while (true) {
            auto tk1 = m_reader.getNextNonNewlineToken();
            if (tk1.eof)
                break;
            m_reader.goBack();
            if (tk1.sourcePosition.column <= column)
                break;

            if (otherInstruction) // if we have OTHER: it should have been the last case, so we shouldn't get here again
                throw CompilerError(ErrorType::omblc);

            if (tk1.type == Token::Other) {
                m_reader.skipToken(); // skip 'other'
                m_reader.forceElement(Token::Colon);
                otherInstruction = parseBlock(tk1.sourcePosition.column);
                continue;
            }
            //a normal cases consists of (multiple) expressions, followed by : followed by instructions
            std::vector<CaseInstruction::ExpressionListEntry> expressions;
            while (true) {
                AbstractExpressionP expr2;
                AbstractExpressionP expr1=exprCompiler.parseRangeOrExpression(expr2);
                expressions.push_back(CaseInstruction::ExpressionListEntry(expr1, expr2));
                if (!m_reader.checkElement(Token::Comma))
                    break;
            }
            m_reader.forceElement(Token::Colon);
            cases.push_back(CaseInstruction::CaseEntry(expressions, parseBlock(tk1.sourcePosition.column)));
        }

        if (cases.empty())
            throw CompilerError(ErrorType::nce);
        return AbstractInstructionP(new CaseInstruction(sourcePosition, condition, cases, otherInstruction));
    }

    AbstractInstructionP parseEndlessOrPostRepeat(int column){
        auto sourcePosition = m_reader.getSourcePosition();
        auto instruction = parseBlock(column);
        auto tk = m_reader.getNextToken();
        if (!tk.eof) {
            if (tk.sourcePosition.column < column)
                m_reader.goBack();
            else if (tk.type == Token::While || tk.type == Token::Until) {
                auto condition = ExpressionParser(m_context,true).parseExpression(); // compile post-while/until expression
                m_reader.forceElement(Token::End);
                const auto type = (tk.type == Token::While) ? LoopConditionInstruction::PostWhile : LoopConditionInstruction::PostUntil;
                return AbstractInstructionP(new LoopConditionInstruction(sourcePosition, type, condition, instruction));
            }
            else
                m_reader.goBack();
        }
        return AbstractInstructionP(new LoopConditionInstruction(sourcePosition, LoopConditionInstruction::RepeatEndless, AbstractExpressionP(), instruction));
    }

    AbstractInstructionP parsePreRepeat(int column, LoopConditionInstruction::Type type) {
        auto sourcePosition = m_reader.getSourcePosition();
        auto condition = ExpressionParser(m_context,true).parseExpression(); // compile pre-while/until expression
        m_reader.forceElement(Token::End);
        auto instruction = parseBlock(column); // compile repeat-while/until block
        return AbstractInstructionP(new LoopConditionInstruction(sourcePosition, type, condition, instruction));
    }

    AbstractInstructionP parseRepeatCount(int column, AbstractExpressionP countExpression) {
        auto instruction = parseBlock(column); // compile repeat-count block
        return AbstractInstructionP(new LoopConditionInstruction(countExpression->sourcePosition, LoopConditionInstruction::RepeatCount, countExpression, instruction));
    }

    AbstractInstructionP parseRepeatVariable(int column, AbstractSpinVariableP variable) {
        auto sourcePosition = m_reader.getSourcePosition();
        const auto tk1 = m_reader.getNextToken(); // get 'from'
        if (tk1.type != Token::From)
            throw CompilerError(ErrorType::efrom, tk1);
        auto from = ExpressionParser(m_context,false).parseExpression();

        const auto tk2 = m_reader.getNextToken(); // get 'to'
        if (tk2.type != Token::To)
            throw CompilerError(ErrorType::eto, tk2);
        auto to = ExpressionParser(m_context,false).parseExpression();

        const auto tk3 = m_reader.getNextToken(); // check for 'step'
        AbstractExpressionP step;
        if (tk3.type == Token::Step) {
            step = ExpressionParser(m_context,false).parseExpression();
            m_reader.forceElement(Token::End);
        }
        else if (tk3.type != Token::End)
            throw CompilerError(ErrorType::esoeol, tk3);
        return AbstractInstructionP(new LoopVarInstruction(sourcePosition, variable, from, to, step, parseBlock(column)));
    }

    AbstractInstructionP parseRepeat(int column) {
        // determine which type of repeat
        InstructionBlockParser subCompiler(m_context.contextNewLoop());
        auto tk = m_reader.getNextToken();
        if (tk.type == Token::End) // repeat
            return subCompiler.parseEndlessOrPostRepeat(column);
        if (tk.type == Token::While) // repeat while <exp>
            return subCompiler.parsePreRepeat(column, LoopConditionInstruction::PreWhile);
        else if (tk.type == Token::Until) // repeat until <exp>
            return subCompiler.parsePreRepeat(column, LoopConditionInstruction::PreUntil);

        m_reader.goBack();
        auto countExpression = ExpressionParser(m_context,true).parseExpression(); // compile count expression
        if (m_reader.checkElement(Token::End)) // repeat <exp>
            return subCompiler.parseRepeatCount(column, countExpression);
        auto varExpr = std::dynamic_pointer_cast<VariableExpression>(countExpression);
        if (!varExpr || varExpr->operation != AbstractSpinVariable::Read)
            throw CompilerError(ErrorType::eav, countExpression->sourcePosition);
        // repeat var from <exp> to <exp> step <exp>
        return subCompiler.parseRepeatVariable(column, varExpr->variable);
    }
};

#endif //SPINCOMPILER_INSTRUCTIONBLOCKPARSER_H

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
