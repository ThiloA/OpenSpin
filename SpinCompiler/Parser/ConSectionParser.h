//////////////////////////////////////////////////////////////
//                                                          //
// Propeller Spin/PASM Compiler                             //
// (c)2012-2016 Parallax Inc. DBA Parallax Semiconductor.   //
// Adapted from Chip Gracey's x86 asm code by Roy Eltham    //
// Rewritten to modern C++ by Thilo Ackermann               //
// See end of file for terms of use.                        //
//                                                          //
////////////////////////////////////////////////////////////// 

#ifndef SPINCOMPILER_CONSECTIONPARSER_H
#define SPINCOMPILER_CONSECTIONPARSER_H

#include "SpinCompiler/Parser/ParserObjectContext.h"
#include "SpinCompiler/Parser/ConstantExpressionParser.h"
#include "SpinCompiler/Parser/AbstractParser.h"
#include "SpinCompiler/Parser/ParsedObject.h"
#include <functional>

class ConSectionParser {
public:
    explicit ConSectionParser(TokenReader& reader, ParserObjectContext& objectContext):m_reader(reader),m_objectContext(objectContext),m_enumValid(false) {}
    void parseConBlocks(int pass) {
        m_reader.reset();
        do {
            m_enumValid = true;
            m_enumValue = ConstantValueExpression::create(m_reader.getSourcePosition(),0);
            while (true) {
                auto tk = m_reader.getNextNonBlockOrNewlineToken();
                if (tk.eof)
                    break;

                while(true) {
                    if (auto conSym = std::dynamic_pointer_cast<SpinConstantSymbol>(tk.resolvedSymbol)) { // constant
                        if (pass == 0)
                            throw CompilerError(ErrorType::eaucnop, tk);
                        handleConSymbol(pass, tk.symbolId, std::bind(&ConSectionParser::conAssignVerify,this,!conSym->isInteger,conSym->expression,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3,std::placeholders::_4));
                    }
                    else if (tk.type == Token::Undefined)
                        handleConSymbol(pass, tk.symbolId, std::bind(&ConSectionParser::conAssignAddSymbol,this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3,std::placeholders::_4));
                    else if (tk.type == Token::Pound) {
                        const auto res = ConstantExpressionParser::tryResolveValueNonAsm(m_reader,m_objectContext.childObjectSymbols,pass == 1, true);
                        if (res.expression) {
                            m_enumValid = true;
                            m_enumValue = res.expression;
                        }
                        else
                            m_enumValid = false;
                    }
                    else // we got an element that isn't valid in a con block
                        throw CompilerError(ErrorType::eaucnop, tk);
                    if (!m_reader.getCommaOrEnd())
                        break;
                    tk = m_reader.getNextToken();
                    if (tk.eof)
                        break;
                }
            }
        }
        while(m_reader.getNextBlock(BlockType::BlockCON));
    }
    void parseDevBlocks() {
        m_reader.reset();
        while (m_reader.getNextBlock(BlockType::BlockDEV)) {
            while (true) {
                auto tk = m_reader.getNextNonBlockOrNewlineToken();
                if (tk.eof)
                    break;
                if (tk.type == Token::PreCompile) {
                    m_reader.readFileNameString();
                    m_reader.forceElement(Token::End);
                }
                else if (tk.type == Token::Archive) {
                    m_reader.readFileNameString();
                    m_reader.forceElement(Token::End);
                }
                else // we got an element that wasn't a precompile or archive or the next block
                    throw CompilerError(ErrorType::epoa, tk);
            }
        }
    }
private:
    TokenReader &m_reader;
    ParserObjectContext& m_objectContext;
    AbstractConstantExpressionP m_enumValue;
    bool m_enumValid;

    void handleConSymbol(int pass, SpinSymbolId symbolId, const std::function<void(bool,AbstractConstantExpressionP,SpinSymbolId,const SourcePosition&)>& assignFunction) {
        auto tk = m_reader.getNextToken();
        if (tk.type == Token::Equal) {
            const auto res = ConstantExpressionParser::tryResolveValueNonAsm(m_reader, m_objectContext.childObjectSymbols, pass == 1, false);
            if (res.expression)
                assignFunction(res.dataType == ConstantExpressionParser::DataType::Float, res.expression, symbolId, tk.sourcePosition);
        }
        else if (tk.type == Token::LeftIndex) { // enumx
            const auto res = ConstantExpressionParser::tryResolveValueNonAsm(m_reader, m_objectContext.childObjectSymbols, pass == 1, true);
            m_reader.forceElement(Token::RightIndex);
            if (!res.expression)
                m_enumValid = false;
            else if (m_enumValid) {
                auto old = m_enumValue;
                m_enumValue = addExpression(tk.sourcePosition,old,res.expression);
                assignFunction(false, old, symbolId, tk.sourcePosition);
            }
        }
        else if (tk.type == Token::Comma || tk.type == Token::End) { // enuma
            m_reader.goBack();
            if (m_enumValid) {
                auto old = m_enumValue;
                m_enumValue = addExpression(tk.sourcePosition,old,1);
                assignFunction(false, old, symbolId, tk.sourcePosition);
            }
        }
        else
            throw CompilerError(ErrorType::eelcoeol, tk);
    }

    void conAssignVerify(bool assignFloat, AbstractConstantExpressionP assignValue, bool bFloat, AbstractConstantExpressionP expression, SpinSymbolId, const SourcePosition& sourcePosition) {
        // verify
        if (assignFloat != bFloat /*|| assignValue != expression*/) //TODO was besagt dieser check?
            throw CompilerError(ErrorType::siad, sourcePosition);
    }

    void conAssignAddSymbol(bool bFloat, AbstractConstantExpressionP expression, SpinSymbolId symbolId, const SourcePosition& sourcePosition) {
        m_objectContext.currentObject->constants.push_back(ParsedObject::Constant(sourcePosition,symbolId,expression,bFloat));
        m_objectContext.globalSymbols.addSymbol(symbolId, SpinAbstractSymbolP(new SpinConstantSymbol(expression, !bFloat)), 0);
    }

    AbstractConstantExpressionP addExpression(const SourcePosition& position, AbstractConstantExpressionP left, AbstractConstantExpressionP right) {
        int leftValue=0, rightValue=0;
        if (left->isConstant(&leftValue) && right->isConstant(&rightValue))
            return AbstractConstantExpressionP(ConstantValueExpression::create(position, leftValue+rightValue));
        return AbstractConstantExpressionP(new BinaryConstantExpression(position,left,OperatorType::OpAdd,right,false));
    }
    AbstractConstantExpressionP addExpression(const SourcePosition& position, AbstractConstantExpressionP left, int rightValue) {
        int leftValue=0;
        if (left->isConstant(&leftValue))
            return AbstractConstantExpressionP(ConstantValueExpression::create(position, leftValue+rightValue));
        return AbstractConstantExpressionP(new BinaryConstantExpression(position,left,OperatorType::OpAdd,ConstantValueExpression::create(position, rightValue),false));
    }
};

#endif //SPINCOMPILER_CONSECTIONPARSER_H

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
