//////////////////////////////////////////////////////////////
//                                                          //
// Propeller Spin/PASM Compiler                             //
// (c)2012-2016 Parallax Inc. DBA Parallax Semiconductor.   //
// Adapted from Chip Gracey's x86 asm code by Roy Eltham    //
// Rewritten to modern C++ by Thilo Ackermann               //
// See end of file for terms of use.                        //
//                                                          //
//////////////////////////////////////////////////////////////

#ifndef SPINCOMPILER_CONSTANTEXPRESSIONPARSER_H
#define SPINCOMPILER_CONSTANTEXPRESSIONPARSER_H

#include "SpinCompiler/Types/ConstantExpression.h"
#include "SpinCompiler/Tokenizer/TokenReader.h"

class ConstantExpressionParser {
public:
    struct DatSectionContext {
    public:
        DatSectionContext():asmLocal(0) {}
        SymbolMap localSymbols;
        int asmLocal;
    };
private:
    TokenReader &m_reader;
    const SymbolMap &m_childObjectSymbols;
    DatSectionContext *m_datSectionContext;
    bool m_mustResolve;             // the expression must resolve
public:
    enum struct DataType { Any, Integer, Float, Illg };
    struct Result {
        Result(AbstractConstantExpressionP expression, DataType dataType):expression(expression),dataType(dataType) {}
        Result(DataType dataType):dataType(dataType) {}
        AbstractConstantExpressionP expression;
        DataType dataType;
    };

    static Result tryResolveValueNonAsm(TokenReader &reader, const SymbolMap &childObjectSymbols, bool mustResolve, bool isInteger) {
        return ConstantExpressionParser(reader, childObjectSymbols, mustResolve, nullptr).resolveExpression(isInteger ? DataType::Integer : DataType::Any);
    }

    static AbstractConstantExpressionP tryResolveValueAsm(TokenReader &reader, const SymbolMap &childObjectSymbols, bool mustResolve, bool isInteger, DatSectionContext *datSectionContext) {
        return ConstantExpressionParser(reader, childObjectSymbols, mustResolve, datSectionContext).resolveExpression(isInteger ? DataType::Integer : DataType::Any).expression;
    }
private:
    ConstantExpressionParser(TokenReader &reader, const SymbolMap &childObjectSymbols, bool mustResolve, DatSectionContext *datSectionContext):
        m_reader(reader),
        m_childObjectSymbols(childObjectSymbols),
        m_datSectionContext(datSectionContext),
        m_mustResolve(mustResolve)
    {
    }

    Result resolveExpression(const DataType expectedDataType) {
        return resolveSubExpression(10, expectedDataType); //11-1
    }

    Result resolveSubExpression(int precedence, const DataType expectedDataType) {
        Result res = (precedence < 0) ? getTerm(precedence, expectedDataType) : resolveSubExpression(precedence - 1, expectedDataType);
        while (true) {
            auto tk = m_reader.getNextToken();
            if (tk.type != Token::Binary) {
                m_reader.goBack();
                return res;
            }
            auto expDtOp2 = previewOp(tk.sourcePosition, tk.opType,res.dataType);
            if (precedence != tk.value) {
                m_reader.goBack();
                return res;
            }
            const auto right = resolveSubExpression(precedence - 1, expDtOp2);
            res = performBinary(res,static_cast<OperatorType::Type>(tk.opType),right,tk.sourcePosition);
        }
    }

    Result getTerm(int& precedence, const DataType expectedDataType) {
        // skip over any leading +'s
        auto tk = m_reader.getNextToken();
        while (!tk.eof && tk.type == Token::Binary && tk.opType == OperatorType::OpAdd)
            tk = m_reader.getNextToken();
        const auto constRes = checkConstant(tk,expectedDataType);
        if (constRes.dataType != DataType::Illg)
            return constRes;

        if (tk.subToNeg())
            precedence = 0;

        if (tk.type  == Token::Unary) {
            auto expParamDt = previewOp(tk.sourcePosition, tk.opType,expectedDataType);
            precedence = tk.value; // for unary types, value = precedence
            const auto res = resolveSubExpression(precedence - 1, expParamDt);
            return performUnary(res,static_cast<OperatorType::Type>(tk.opType),tk.sourcePosition);
        }
        if (tk.type == Token::LeftBracket) {
            const auto res = resolveExpression(expectedDataType);
            m_reader.forceElement(Token::RightBracket);
            return res;
        }
        if (m_mustResolve)
            throw CompilerError(ErrorType::eacuool, tk);
        return Result(expectedDataType);
    }

    void checkUndefined() {
        if(m_reader.checkElement(Token::Pound)) {
            Token tk = m_reader.getNextToken();
            if (!tk.isValidWordSymbol())
                throw CompilerError(ErrorType::eacn, tk);
        }
        if (m_mustResolve)
            throw CompilerError(ErrorType::us, m_reader.getSourcePosition());
    }

    std::shared_ptr<SpinDatSectionSymbol> checkDat(Token& tk0) {
        auto result = std::dynamic_pointer_cast<SpinDatSectionSymbol>(tk0.resolvedSymbol);
        if (!m_datSectionContext && result && result->isRes)
            return std::shared_ptr<SpinDatSectionSymbol>();
        return result;
    }

    Result checkConstant(const SourcePosition& sourcePosition, std::shared_ptr<SpinConstantSymbol> conSym, const DataType expectedDataType) {
        if (conSym->isInteger) {
            if (expectedDataType == DataType::Float)
                throw CompilerError(ErrorType::fpnaiie, sourcePosition);
            return Result(conSym->expression, DataType::Integer);
        }
        //float
        if (expectedDataType == DataType::Integer)
            throw CompilerError(ErrorType::inaifpe, sourcePosition);
        return Result(conSym->expression, DataType::Float);
    }

    Result unaryFloatOperation(const SourcePosition& sourcePosition, AbstractConstantExpressionP expression, OperatorType::Type operation, bool floatMode, DataType dataType) {
        int value=0;
        if (expression->isConstant(&value))
            return Result(ConstantValueExpression::create(sourcePosition, UnaryConstantExpression::performOpUnary(value, operation, floatMode, sourcePosition)), dataType);
        return Result(AbstractConstantExpressionP(new UnaryConstantExpression(sourcePosition, expression, operation, floatMode)), dataType);
    }

    Result checkConstant(Token &tk0, const DataType expectedDataType) {
        if (auto conSym = std::dynamic_pointer_cast<SpinConstantSymbol>(tk0.resolvedSymbol))
            return checkConstant(tk0.sourcePosition, conSym, expectedDataType);
        if (tk0.type == Token::Float) {
            if (expectedDataType == DataType::Integer)
                throw CompilerError(ErrorType::inaifpe, tk0);
            m_reader.forceElement(Token::LeftBracket);
            auto res = resolveExpression(DataType::Integer); // integer mode
            m_reader.forceElement(Token::RightBracket);
            if (!res.expression)
                return Result(DataType::Float);
            return unaryFloatOperation(tk0.sourcePosition, res.expression, OperatorType::OpFloat, true, DataType::Float);
        }
        if (tk0.type == Token::Round) {
            if (expectedDataType == DataType::Float)
                throw CompilerError(ErrorType::fpnaiie, tk0);
            m_reader.forceElement(Token::LeftBracket);
            auto res = resolveExpression(DataType::Float); // float mode
            m_reader.forceElement(Token::RightBracket);
            // convert float to rounded integer
            if (!res.expression)
                return Result(DataType::Integer);
            return unaryFloatOperation(tk0.sourcePosition, res.expression, OperatorType::OpRound, false, DataType::Integer);

        }
        if (tk0.type == Token::Trunc) {
            if (expectedDataType == DataType::Float)
                throw CompilerError(ErrorType::fpnaiie, tk0);
            m_reader.forceElement(Token::LeftBracket);
            auto res = resolveExpression(DataType::Float); // float mode
            m_reader.forceElement(Token::RightBracket);
            // convert float to truncated integer
            if (!res.expression)
                return Result(DataType::Integer);
            return unaryFloatOperation(tk0.sourcePosition, res.expression, OperatorType::OpTrunc, true, DataType::Integer);
        }

        if (m_datSectionContext && tk0.type == Token::Colon) {
            tk0 = m_reader.getNextToken();
            if (!tk0.isValidWordSymbol())
                throw CompilerError(ErrorType::eals, tk0);
            tk0.resolvedSymbol = m_datSectionContext->localSymbols.hasSpecificSymbol<SpinDatSectionSymbol>(tk0.symbolId, m_datSectionContext->asmLocal);
            tk0.type = tk0.resolvedSymbol ? Token::DefinedSymbol : Token::Undefined;
        }

        if (tk0.type == Token::Undefined) {
            checkUndefined();
            return Result(expectedDataType);
        }
        if (tk0.type == Token::AsmOrg) {
            if (!m_datSectionContext)
                throw CompilerError(ErrorType::oinah, tk0);
            return Result(AbstractConstantExpressionP(new DatCurrentCogPosConstantExpression(tk0.sourcePosition)), expectedDataType);
        }
        if (tk0.type == Token::AsmReg) {
            if (!m_datSectionContext)
                throw CompilerError(ErrorType::rinah, tk0);
            if (expectedDataType == DataType::Float)
                throw CompilerError(ErrorType::fpnaiie, tk0);
            return Result(ConstantValueExpression::create(tk0.sourcePosition, tk0.value | 0x1E0), DataType::Integer);
        }
        if (auto objSym = std::dynamic_pointer_cast<SpinObjSymbol>(tk0.resolvedSymbol)) {
            m_reader.forceElement(Token::Pound);
            auto tk1 = m_reader.getNextToken();
            auto conSym = m_childObjectSymbols.hasSpecificSymbol<SpinConstantSymbol>(tk1.symbolId, objSym->objectClass.value());
            if (!conSym)
                throw CompilerError(ErrorType::eacn, tk1);
            return checkConstant(tk0.sourcePosition, conSym, expectedDataType);
        }
        if (tk0.type == Token::At) {
            if (expectedDataType == DataType::Float)
                throw CompilerError(ErrorType::fpnaiie, tk0);
            tk0=m_reader.getNextToken();
            if (auto datSym = checkDat(tk0))
                return Result(AbstractConstantExpressionP(new DatSymbolConstantExpression(tk0.sourcePosition, datSym->id, false)), DataType::Integer);
            if (tk0.type != Token::Undefined)
                throw CompilerError(ErrorType::eads, tk0);
            checkUndefined();
            return Result(DataType::Integer);
        }
        if (auto datSym = checkDat(tk0)) {
            if (expectedDataType == DataType::Float)
                throw CompilerError(ErrorType::fpnaiie, tk0);
            if (!m_datSectionContext)
                return Result(AbstractConstantExpressionP(new DatSymbolConstantExpression(tk0.sourcePosition, datSym->id, false)), DataType::Integer);
            return Result(AbstractConstantExpressionP(new DatSymbolConstantExpression(tk0.sourcePosition, datSym->id, true)), DataType::Integer);
        }
        return Result(DataType::Illg);
    }

    DataType previewOp(const SourcePosition& sourcePosition, int opType, const DataType expectedDataType) {
        //TODO remove ugly hack
        int check = 0x00AACD8F; // 00000000 10101010 11001101 10001111
        check >>= opType;
        if (check & 1) {
            if (expectedDataType == DataType::Float) // integer only op while in float mode
                throw CompilerError(ErrorType::ionaifpe, sourcePosition);
            return DataType::Integer; // force integer mode
        }
        return expectedDataType;
    }

    Result performBinary(const Result left, OperatorType::Type operation, const Result right, const SourcePosition& sourcePosition) {
        if (!left.expression || !right.expression) //undefined
            return Result(right.dataType);
        auto leftConst=std::dynamic_pointer_cast<ConstantValueExpression>(left.expression);
        auto rightConst=std::dynamic_pointer_cast<ConstantValueExpression>(right.expression);
        if (leftConst && rightConst) //if both can be evaluated now, then do it
            return Result(ConstantValueExpression::create(sourcePosition, BinaryConstantExpression::performOpBinary(leftConst->value, rightConst->value, static_cast<OperatorType::Type>(operation), right.dataType == DataType::Float)), right.dataType);
        return Result(AbstractConstantExpressionP(new BinaryConstantExpression(sourcePosition, left.expression, operation, right.expression, right.dataType == DataType::Float)), right.dataType);
    }

    Result performUnary(const Result param, OperatorType::Type operation, const SourcePosition& sourcePosition) {
        if (!param.expression)
            return Result(param.dataType);
        if (auto constParam = std::dynamic_pointer_cast<ConstantValueExpression>(param.expression))
            return Result(ConstantValueExpression::create(sourcePosition, UnaryConstantExpression::performOpUnary(constParam->value, static_cast<OperatorType::Type>(operation), param.dataType == DataType::Float, sourcePosition)), param.dataType);
        return Result(AbstractConstantExpressionP(new UnaryConstantExpression(sourcePosition, param.expression, operation, param.dataType == DataType::Float)), param.dataType);
    }
};

#endif //SPINCOMPILER_CONSTANTEXPRESSIONPARSER_H

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
