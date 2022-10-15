//////////////////////////////////////////////////////////////
//                                                          //
// Propeller Spin/PASM Compiler                             //
// (c)2012-2016 Parallax Inc. DBA Parallax Semiconductor.   //
// Adapted from Chip Gracey's x86 asm code by Roy Eltham    //
// Rewritten to modern C++ by Thilo Ackermann               //
// See end of file for terms of use.                        //
//                                                          //
////////////////////////////////////////////////////////////// 

#ifndef SPINCOMPILER_EXPRESSIONPARSER_H
#define SPINCOMPILER_EXPRESSIONPARSER_H

#include "SpinCompiler/Parser/ConstantExpressionParser.h"
#include "SpinCompiler/Generator/Expression.h"
#include "SpinCompiler/Parser/ParserFunctionContext.h"
#include "SpinCompiler/Parser/ParserObjectContext.h"

class ExpressionParser {
private:
    ParserFunctionContext m_context;
    TokenReader &m_reader;
    const bool m_stringEnabled;
public:
    explicit ExpressionParser(const ParserFunctionContext &context, bool stringEnabled):m_context(context),m_reader(context.reader),m_stringEnabled(stringEnabled) {}

    AbstractExpressionP parseExpression() {
        return parseTopExpression();
    }

    // compiles either a value or a range
    AbstractExpressionP parseRangeOrExpression(AbstractExpressionP& optionalSecondExpression) {
        optionalSecondExpression = AbstractExpressionP();
        auto expr1 = parseExpression();
        if (!m_reader.checkElement(Token::DotDot))
            return expr1;
        optionalSecondExpression = parseExpression();
        return expr1;
    }

    std::vector<AbstractExpressionP> parseParameters(const int numParameters) {
        std::vector<AbstractExpressionP> paramExpr(numParameters);
        if (numParameters <= 0)
            return paramExpr;
        m_reader.forceElement(Token::LeftBracket); // (
        for (int i = 0; i < numParameters; i++) {
            paramExpr[i] = parseExpression();
            if (i < numParameters - 1)
                m_reader.forceElement(Token::Comma);
        }
        m_reader.forceElement(Token::RightBracket); // )
        return paramExpr;
    }

    AbstractExpressionP compileVariableSideEffectOperation(int vOperator, AbstractSpinVariableP varInfo) {
        return AbstractExpressionP(new UnaryAssignExpression(varInfo,vOperator));
    }

    AbstractExpressionP compileVariableAssignExpression(OperatorType::Type operation, AbstractSpinVariableP varInfo) {
        auto pos = m_reader.getSourcePosition();
        auto valueExpression = parseExpression();
        return AbstractExpressionP(new AssignExpression(pos, valueExpression, varInfo, operation));
    }

    AbstractExpressionP compileVariablePreSignExtendOrRandom(int vOperator) {
        return compileVariableSideEffectOperation(vOperator, getVariable());
    }

    AbstractExpressionP compileVariableIncOrDec(int vOperator, AbstractSpinVariableP varInfo) {
        auto cogReg = std::dynamic_pointer_cast<CogRegisterSpinVariable>(varInfo);
        if (!cogReg || !cogReg->indexExpression)
            vOperator |= (((varInfo->size + 1) & 3) << 1);
        return AbstractExpressionP(new UnaryAssignExpression(varInfo,vOperator));
    }

    AbstractExpressionP compileVariablePreIncOrDec(int vOperator) {
        return compileVariableIncOrDec(vOperator, getVariable());
    }

    AbstractSpinVariableP getVariable() {
        return getVariable(m_reader.getNextToken(), ErrorType::eav);
    }

    AbstractSpinVariableP getVariable(const Token& tk0, ErrorType errorOnNoVar) {
        if (tk0.type == Token::SPR)
            return AbstractSpinVariableP(new SpecialPurposeSpinVariable(tk0.sourcePosition,readIndexExpression(true)));
        if (tk0.type == Token::AsmReg) {
            AbstractExpressionP indexExpression;
            AbstractExpressionP indexExpressionUpperRange;
            if (m_reader.checkElement(Token::LeftIndex)) {
                indexExpression = parseExpression();
                if (m_reader.checkElement(Token::DotDot))
                    indexExpressionUpperRange = parseExpression();
                m_reader.forceElement(Token::RightIndex);
            }
            return AbstractSpinVariableP(new CogRegisterSpinVariable(tk0.sourcePosition, tk0.value, indexExpression, indexExpressionUpperRange));
        }
        if (tk0.type == Token::Size) {
            auto dynamicAddressExpression = readIndexExpression(true);
            auto indexExpression = readIndexExpression(false);
            return AbstractSpinVariableP(new DirectMemorySpinVariable(tk0.sourcePosition, AbstractSpinVariable::SizeModifier(tk0.value), dynamicAddressExpression, indexExpression));
        }
        AbstractSpinVariable::SizeModifier size=AbstractSpinVariable::Long;
        NamedSpinVariable::VarType varType = NamedSpinVariable::LocMemoryAccess;
        AbstractConstantExpressionP address;
        if (tk0.type == Token::Result)
            address = ConstantValueExpression::create(tk0.sourcePosition, 0); //RESULT is always at stack position 0
        else if (tk0.type == Token::DefinedSymbol) {
            if (auto varSym = std::dynamic_pointer_cast<SpinVarSectionSymbol>(tk0.resolvedSymbol)) {
                varType = NamedSpinVariable::VarMemoryAccess;
                address = AbstractConstantExpressionP(new VarSymbolConstantExpression(tk0.sourcePosition, varSym->id));
                size = AbstractSpinVariable::SizeModifier(varSym->size);
            }
            else if (auto datSym = std::dynamic_pointer_cast<SpinDatSectionSymbol>(tk0.resolvedSymbol)) {
                //TODO if(datSym->isRes) return false; ? mit original program schauen, ob man auch auf einen res bezeichner zugreifen kann
                varType = NamedSpinVariable::DatMemoryAccess;
                address = AbstractConstantExpressionP(new DatSymbolConstantExpression(tk0.sourcePosition, datSym->id, false));
                size = AbstractSpinVariable::SizeModifier(datSym->size);
            }
            else if (auto locSym = std::dynamic_pointer_cast<SpinLocSymbol>(tk0.resolvedSymbol))
                address = AbstractConstantExpressionP(new LocSymbolConstantExpression(tk0.sourcePosition, locSym->id));
            else
                throw CompilerError(errorOnNoVar, tk0);
        }
        else
            throw CompilerError(errorOnNoVar, tk0);
        // if we got here then it's a var/dat/loc type
        auto indexExpression = readIndexExpression(false);
        if (!indexExpression) {
            // check for .byte/word/long{[index]}
            if (m_reader.checkElement(Token::Dot)) {
                auto tk = m_reader.getNextToken();
                if (tk.type != Token::Size)
                    throw CompilerError(ErrorType::ebwol, tk);
                if (size < (tk.value & 0xFF)) // new size must be same or smaller
                    throw CompilerError(ErrorType::sombs, tk);
                size = AbstractSpinVariable::SizeModifier(tk.value & 0xFF); // update size
                indexExpression = readIndexExpression(false); //TODO index required?
            }
        }
        return AbstractSpinVariableP(new NamedSpinVariable(tk0.sourcePosition, size, varType, address, indexExpression));
    }

    // compile obj[].pub
    AbstractExpressionP parseChildObjectMethodCall(SpinObjSymbolP objSymbol, const bool trapCall) {
        auto sourcePosition = m_reader.getSourcePosition();
        auto objectIndex = readIndexExpression(false); // check for [index]
        m_reader.forceElement(Token::Dot);

        // lookup the pub symbol
        //m_reader.getObjSymbol(tk,Token::type_objpub, objClass);
        auto method = m_context.objectContext.getObjMethod(m_reader.getNextToken(), objSymbol->objectClass);

        // compile any parameters the pub has
        auto parameters = parseParameters(method->parameterCount);
        return AbstractExpressionP(new MethodCallExpression(sourcePosition, parameters, objectIndex, objSymbol->objInstanceId, method->methodId, trapCall));
    }

    AbstractExpressionP parseMethodCall(const SourcePosition& sourcePosition, SpinSubSymbolP subSymbol, const bool trapCall) {
        auto parameters = parseParameters(subSymbol->parameterCount);
        return AbstractExpressionP(new MethodCallExpression(sourcePosition, parameters, AbstractExpressionP(), ObjectInstanceId(), subSymbol->methodId, trapCall));
    }

    // compile \sub or \obj
    AbstractExpressionP parseTryCall(const bool trapCall) {
        auto tk = m_reader.getNextToken();
        if (auto subSym = std::dynamic_pointer_cast<SpinSubSymbol>(tk.resolvedSymbol))
            return parseMethodCall(tk.sourcePosition, subSym, trapCall);
        if (auto objSym = std::dynamic_pointer_cast<SpinObjSymbol>(tk.resolvedSymbol))
            return parseChildObjectMethodCall(objSym, trapCall);
        throw CompilerError(ErrorType::easoon, tk);
    }

    AbstractExpressionP parseCogNew() {
        // see if first param is a sub
        auto sourcePosition = m_reader.getSourcePosition();
        m_reader.forceElement(Token::LeftBracket);
        auto tk1 = m_reader.getNextToken();
        if (auto subSym = std::dynamic_pointer_cast<SpinSubSymbol>(tk1.resolvedSymbol)) {
            // it is a sub, so compile as cognew(subname(params),stack)
            auto parameters = parseParameters(subSym->parameterCount);
            m_reader.forceElement(Token::Comma);
            auto stackExpression = parseExpression(); // compile stack expression
            m_reader.forceElement(Token::RightBracket);
            return AbstractExpressionP(new CogNewSpinExpression(sourcePosition, parameters, stackExpression, subSym->methodId));
        }

        // it is not a sub, so backup and compile as cognew(address, parameter)
        m_reader.goBack();
        m_reader.goBack();
        auto params = parseParameters(2);
        return AbstractExpressionP(new CogNewAsmExpression(sourcePosition, params[0], params[1]));
    }
private:
    ConstantExpressionParser::Result tryParseConstantExpression(bool mustResolve, bool isInteger) {
        return ConstantExpressionParser::tryResolveValueNonAsm(m_reader, m_context.objectContext.childObjectSymbols, mustResolve, isInteger);
    }

    AbstractExpressionP parseTopExpression() {
        return parseBinaryExpression(11);
    }

    AbstractExpressionP compileSubExpressionTermX() {
        // get next element ignoring any leading +'s
        auto tk = m_reader.getNextToken();
        while (tk.type == Token::Binary && tk.opType == OperatorType::OpAdd)
            tk = m_reader.getNextToken();

        combineNegativeSignWithConstant(tk);
        tk.subToNeg();

        switch (tk.type) {
            case Token::AtAt:
                return AbstractExpressionP(new AtAtExpression(tk.sourcePosition, parseBinaryExpression(0)));

            case Token::Unary:
                // tk.value = precedence for Token::type_unary
                return AbstractExpressionP(new UnaryExpression(tk.sourcePosition, parseBinaryExpression(tk.value), OperatorType::Type(tk.opType))); //TODO remove operator cast

            case Token::LeftBracket: {
                auto expr = parseTopExpression();
                m_reader.forceElement(Token::RightBracket);
                return expr;
            }

            default:
                return parsePrimary(tk);
        }
    }

    void combineNegativeSignWithConstant(Token& prevToken) {
        if (prevToken.type != Token::Binary || prevToken.opType != OperatorType::OpSub) //nothing to do?
            return;
        auto nextTk = m_reader.getNextToken();
        if (auto conSym = std::dynamic_pointer_cast<SpinConstantSymbol>(nextTk.resolvedSymbol)) {
            prevToken = nextTk;
            prevToken.symbolId = SpinSymbolId();
            prevToken.resolvedSymbol = SpinAbstractSymbolP(new SpinConstantSymbol(AbstractConstantExpressionP(new UnaryConstantExpression(prevToken.sourcePosition, conSym->expression, OperatorType::OpNeg, !conSym->isInteger)),conSym->isInteger));
        }
        else
            m_reader.goBack();
    }

    AbstractExpressionP parseBinaryExpression(int precedence) {
        precedence--;
        AbstractExpressionP resultExpr = (precedence < 0) ? compileSubExpressionTermX() : parseBinaryExpression(precedence);

        while(true) {
            auto tk = m_reader.getNextToken();
            if (tk.type != Token::Binary || tk.value != precedence) {
                m_reader.goBack();
                break;
            }
            resultExpr = AbstractExpressionP(new BinaryExpression(tk.sourcePosition, resultExpr, parseBinaryExpression(precedence), OperatorType::Type(tk.opType))); //TODO remove operator cast
        }
        return resultExpr;
    }

    AbstractExpressionP parsePrimaryConstant() {
        m_reader.forceElement(Token::LeftBracket);
        auto sourcePosition = m_reader.getSourcePosition();
        auto expr = tryParseConstantExpression(true, false).expression;
        m_reader.forceElement(Token::RightBracket);
        return AbstractExpressionP(new PushConstantExpression(sourcePosition, expr));
    }

    // compile string("constantstring")
    AbstractExpressionP parsePrimaryString() {
        auto startSourcePosition = m_reader.getSourcePosition();
        if (!m_stringEnabled)
            throw CompilerError(ErrorType::snah, startSourcePosition);
        m_reader.forceElement(Token::LeftBracket);
        // get the string into the string constant buffer
        std::vector<AbstractConstantExpressionP> tmpStr;
        tmpStr.reserve(128);
        while (true) {
            const auto srcPos2 = m_reader.getSourcePosition();
            const auto res = tryParseConstantExpression(true, false);
            if (res.dataType == ConstantExpressionParser::DataType::Float || !res.expression)
                throw CompilerError(ErrorType::scmr, srcPos2);
            tmpStr.push_back(res.expression);
            if (!m_reader.getCommaOrRight()) // got right ')'
                break;
        }
        //auto strIdx = m_context.appendStringContant(startSourcePosition, startPosMarker, tmpStr);
        auto strIdx = m_context.stringsCounter++;
        return AbstractExpressionP(new PushStringExpression(startSourcePosition, tmpStr, strIdx));
    }

    // compile float(integer)/round(float)/trunc(float)
    AbstractExpressionP parsePrimaryFloatRoundTrunc() {
        m_reader.goBack(); // backup to float/round/trunc
        auto sourcePosition = m_reader.getSourcePosition();
        return AbstractExpressionP(new PushConstantExpression(sourcePosition, tryParseConstantExpression(true, false).expression));
    }

    // compile obj[].pub\obj[]#con
    AbstractExpressionP parseChildObjectAccess(SpinObjSymbolP objSymbol) {
        if (!m_reader.checkElement(Token::Pound)) // check for obj#con
            return parseChildObjectMethodCall(objSymbol, false); // not obj#con, so do obj[].pub
        // lookup the symbol to get the value to compile
        auto sourcePosition = m_reader.getSourcePosition();
        return AbstractExpressionP(new PushConstantExpression(sourcePosition, m_context.objectContext.getObjConstant(m_reader.getNextToken(), objSymbol->objectClass)->expression));
    }

    AbstractExpressionP parseLookExpression(int origTokenByteCode) {
        auto sourcePosition = m_reader.getSourcePosition();
        m_reader.forceElement(Token::LeftBracket);
        auto condition = parseExpression(); // compile primary value
        m_reader.forceElement(Token::Colon);
        std::vector<LookExpression::ExpressionListEntry> listExpr;
        while(true) {
            AbstractExpressionP expr2;
            auto expr1 = parseRangeOrExpression(expr2); // compile (next) value/range
            listExpr.push_back(LookExpression::ExpressionListEntry(expr1, expr2));
            if (!m_reader.getCommaOrRight())
                break;
        }
        return AbstractExpressionP(new LookExpression(sourcePosition, condition, listExpr, origTokenByteCode));
    }

    AbstractExpressionP compileTermClkModeX(const SourcePosition& sourcePosition) {
        return AbstractExpressionP(new FixedByteCodeSequenceExpression(sourcePosition, std::vector<unsigned char>{
            0x38,4,// constant 4
            0x80 //read byte[]
        }));
    }

    AbstractExpressionP compileTermClkFreqX(const SourcePosition& sourcePosition) {
        return AbstractExpressionP(new FixedByteCodeSequenceExpression(sourcePosition, std::vector<unsigned char>{
            0x35, // constant 0
            0xC0 // read long[]
        }));
    }

    AbstractExpressionP compileTermChipVerX(const SourcePosition& sourcePosition) {
        return AbstractExpressionP(new FixedByteCodeSequenceExpression(sourcePosition, std::vector<unsigned char>{
            0x34,// constant -1
            0x80 //read byte[]
        }));
    }

    AbstractExpressionP compileTermCogIdX(const SourcePosition& sourcePosition) {
        return AbstractExpressionP(new FixedByteCodeSequenceExpression(sourcePosition, std::vector<unsigned char>{
            0x3F,// reg op
            0x89 // read id
        }));
    }

    // compile @var
    AbstractExpressionP parseAtExpression() {
        auto varInfo = getVariable();
        if (!varInfo->canTakeReference())
            throw CompilerError(ErrorType::eamvaa, varInfo->sourcePosition);
        return AbstractExpressionP(new VariableExpression(varInfo,true));
    }

    AbstractExpressionP parseBuiltInExpression(const Token& tk) {
        auto parameters = parseParameters(tk.unpackBuiltInParameterCount());
        return AbstractExpressionP(new CallBuiltInExpression(tk.sourcePosition, parameters, BuiltInFunction::Type(tk.unpackBuiltInByteCode())));
    }

    AbstractExpressionP parsePrimary(const Token& tk0) {
        switch(tk0.type) {
            case Token::ConExpr:
                return parsePrimaryConstant();
            case Token::ConStr:
                return parsePrimaryString();
            case Token::Float:
            case Token::Round:
            case Token::Trunc:
                return parsePrimaryFloatRoundTrunc();
            case Token::Backslash:
                return parseTryCall(true);
            case Token::DefinedSymbol: {
                if (auto conSym = std::dynamic_pointer_cast<SpinConstantSymbol>(tk0.resolvedSymbol))
                    return AbstractExpressionP(new PushConstantExpression(tk0.sourcePosition, conSym->expression));
                if (auto objSym = std::dynamic_pointer_cast<SpinObjSymbol>(tk0.resolvedSymbol))
                    return parseChildObjectAccess(objSym);
                if (auto subSym = std::dynamic_pointer_cast<SpinSubSymbol>(tk0.resolvedSymbol))
                    return parseMethodCall(tk0.sourcePosition, subSym, false);
                break;
            }
            case Token::Look:
                return parseLookExpression(tk0.value & 0xFF);
            case Token::ClkMode:
                return compileTermClkModeX(tk0.sourcePosition);
            case Token::ClkFreq:
                return compileTermClkFreqX(tk0.sourcePosition);
            case Token::ChipVer:
                return compileTermChipVerX(tk0.sourcePosition);
            case Token::CogId:
                return compileTermCogIdX(tk0.sourcePosition);
            case Token::CogNew:
                return parseCogNew();
            case Token::InstAlwaysReturn: // instruction always-returns
            case Token::InstCanReturn: // instruction can-return
                return parseBuiltInExpression(tk0);
            case Token::At: // @var
                return parseAtExpression();
            case Token::Inc: // assign pre-inc w/push  ++var
                return compileVariablePreIncOrDec(0x20);
            case Token::Dec: // assign pre-dec w/push  --var
                return compileVariablePreIncOrDec(0x30);
            case Token::Tilde: // assign sign-extern byte w/push  ~var
                return compileVariablePreSignExtendOrRandom(0x10);
            case Token::TildeTilde: // assign sign-extern word w/push  ~~var
                return compileVariablePreSignExtendOrRandom(0x14);
            case Token::Random: // assign random forward w/push  ?var
                return compileVariablePreSignExtendOrRandom(0x08);
            default:
                break;
        }

        auto varInfo = getVariable(tk0, ErrorType::eaet);
        // check for post-var modifier
        const auto tk2 = m_reader.getNextToken();
        switch (tk2.type) {
            case Token::Inc: // assign post-inc w/push  var++
                return compileVariableIncOrDec(0x28, varInfo);
            case Token::Dec: // assign post-dec w/push  var--
                return compileVariableIncOrDec(0x38, varInfo);
            case Token::Random: // assign random reverse w/push  var?
                return compileVariableSideEffectOperation(0x0C, varInfo);
            case Token::Tilde: // assign post-clear w/push  var~
                return compileVariableSideEffectOperation(0x18, varInfo);
            case Token::TildeTilde: // assign post-set w/push  var~~
                return compileVariableSideEffectOperation(0x1C, varInfo);
            case Token::Assign: // assign write w/push  var :=
                return compileVariableAssignExpression(OperatorType::None, varInfo);
            default:
                break;
        }
        // var binaryop?
        if (tk2.type == Token::Binary) {
            // check for '=' after binary op
            auto tk3 = m_reader.getNextToken();
            if (tk3.type == Token::Equal)
                return compileVariableAssignExpression(OperatorType::Type(tk2.opType), varInfo); //TODO operator type cast weg
            m_reader.goBack(); // not '=' so backup
        }
        m_reader.goBack(); // no post-var modifier, so backup
        return AbstractExpressionP(new VariableExpression(varInfo,false));
    }

    AbstractExpressionP readIndexExpression(bool required) {
        if (!m_reader.checkElement(Token::LeftIndex)) {
            if (required)
                throw CompilerError(ErrorType::eleftb, m_reader.getSourcePosition());
            return AbstractExpressionP();
        }
        auto res = parseExpression();
        m_reader.forceElement(Token::RightIndex);
        return res;
    }
};

#endif //SPINCOMPILER_EXPRESSIONPARSER_H

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
