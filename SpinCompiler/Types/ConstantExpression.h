//////////////////////////////////////////////////////////////
//                                                          //
// Propeller Spin/PASM Compiler                             //
// (c)2012-2016 Parallax Inc. DBA Parallax Semiconductor.   //
// Adapted from Chip Gracey's x86 asm code by Roy Eltham    //
// Rewritten to modern C++ by Thilo Ackermann               //
// See end of file for terms of use.                        //
//                                                          //
////////////////////////////////////////////////////////////// 

#ifndef SPINCOMPILER_CONSTANTEXPRESSION_H
#define SPINCOMPILER_CONSTANTEXPRESSION_H

#include "SpinCompiler/Types/Token.h"
#include "SpinCompiler/Types/CompilerError.h"
#include "SpinCompiler/Types/AbstractBinaryGenerator.h"
#include <math.h>

struct AbstractConstantExpression {
    SourcePosition sourcePosition;
    AbstractConstantExpression(const SourcePosition& sourcePosition):sourcePosition(sourcePosition) {}
    virtual ~AbstractConstantExpression() {}
    virtual int evaluate(AbstractBinaryGenerator* generator) const = 0;
    virtual std::string toILangStr() const = 0;
    virtual bool isConstant(int *) const {
        return false;
    }
};
typedef std::shared_ptr<AbstractConstantExpression> AbstractConstantExpressionP;

struct ConstantValueExpression : AbstractConstantExpression {
    const int value;
    ConstantValueExpression(const SourcePosition& sourcePosition, int value):AbstractConstantExpression(sourcePosition),value(value) {};
    virtual ~ConstantValueExpression() {}
    static std::shared_ptr<ConstantValueExpression> create(const SourcePosition& sourcePosition, int value) {
        return std::shared_ptr<ConstantValueExpression>(new ConstantValueExpression(sourcePosition,value));
    }
    virtual int evaluate(AbstractBinaryGenerator*) const {
        return value;
    }
    virtual std::string toILangStr() const {
        return std::to_string(value);
    };
    virtual bool isConstant(int *resultValue) const {
        if (resultValue)
            *resultValue = value;
        return true;
    }
};

struct UnaryConstantExpression : public AbstractConstantExpression {
    UnaryConstantExpression(const SourcePosition& sourcePosition, AbstractConstantExpressionP param, OperatorType::Type operation, bool floatMode):AbstractConstantExpression(sourcePosition),param(param),operation(operation),floatMode(floatMode) {}
    virtual ~UnaryConstantExpression() {}
    const AbstractConstantExpressionP param;
    const OperatorType::Type operation;
    const bool floatMode;

    virtual int evaluate(AbstractBinaryGenerator* generator) const {
        return performOpUnary(param->evaluate(generator), operation, floatMode, sourcePosition);
    }

    static int performOpUnary(const int value1, const OperatorType::Type operation, const bool isFloatMode, const SourcePosition& sourcePosition) {
        switch(operation) {
            case OperatorType::OpNeg:
                if (isFloatMode) // float neg (using xor)
                    return floatResult(-interpretAsFloat(value1));
                return -value1;
            case OperatorType::OpNot:
                return ~value1;
            case OperatorType::OpAbs:
                if (isFloatMode) // float abs
                    return floatResult((float)fabs(interpretAsFloat(value1)));
                return (value1 < 0) ? -value1 : value1;
            case OperatorType::OpNcd: {
                int result = 32;
                int value1Mod = value1;
                while(!(value1Mod & 0x80000000) && result > 0) {
                    result--;
                    value1Mod <<= 1;
                }
                return result;
            }
            case OperatorType::OpDcd:
                return 1 << (value1 & 0xFF);
            case OperatorType::OpLogNot:
                if (isFloatMode)
                    return floatResult(((!value1) != 0) ? 1.0f : 0.0f);
                return ((!value1) != 0) ? 0xFFFFFFFF : 0;
            case OperatorType::OpSqr: { // sqrt
                if (isFloatMode) {
                    // float sqrt
                    if (interpretAsFloat(value1) < 0.0f)
                        throw CompilerError(ErrorType::ccsronfp, sourcePosition);
                    return floatResult((float)sqrt(interpretAsFloat(value1)));
                }
                int result = 0;
                int value1Mod = value1;
                while (value1Mod >= (2*result)+1)
                    value1Mod -= (2*result++)+1;
                return result;
            }
            case OperatorType::OpFloat:
                return floatResult(float(value1));
            case OperatorType::OpRound:
                return int (interpretAsFloat(value1)+0.5f);
            case OperatorType::OpTrunc:
                return int (interpretAsFloat(value1));
            default:
                return 0;
        }
        return 0;
    }

    static float interpretAsFloat(int value) {
        return *((float*)(&value)); //TODO undef behaviour?
    }

    static int floatResult(float result) {
        return *(int*)(&result); //TODO undef behaviour?
    }

    virtual std::string toILangStr() const {
        return "("+OperatorType::toString(operation)+" "+param->toILangStr()+")";
    };
};

struct BinaryConstantExpression : public AbstractConstantExpression {
    BinaryConstantExpression(const SourcePosition& sourcePosition, AbstractConstantExpressionP left, OperatorType::Type operation, AbstractConstantExpressionP right, bool floatMode):AbstractConstantExpression(sourcePosition),left(left),right(right),operation(operation),floatMode(floatMode) {}
    virtual ~BinaryConstantExpression() {}
    const AbstractConstantExpressionP left;
    const AbstractConstantExpressionP right;
    const OperatorType::Type operation;
    const bool floatMode;

    virtual int evaluate(AbstractBinaryGenerator* generator) const {
        return performOpBinary(left->evaluate(generator), right->evaluate(generator), operation, floatMode);
    }

    static int performOpBinary(const int value1, const int value2, const OperatorType::Type operation, const bool isFloatMode) {
        switch(operation) {
            case OperatorType::OpRor:
                return ror(value1, (value2 & 0xFF));
            case OperatorType::OpRol:
                return rol(value1, (value2 & 0xFF));
            case OperatorType::OpShr:
                return int(static_cast<unsigned int>(value1) >> (value2 & 0xFF));
            case OperatorType::OpShl:
                return value1 << (value2 & 0xFF);
            case OperatorType::OpMin:  // limit minimum
                if (isFloatMode)
                    return UnaryConstantExpression::floatResult((UnaryConstantExpression::interpretAsFloat(value1) < UnaryConstantExpression::interpretAsFloat(value2)) ? UnaryConstantExpression::interpretAsFloat(value2) : UnaryConstantExpression::interpretAsFloat(value1));
                return (value1 < value2) ? value2 : value1;
            case OperatorType::OpMax:  // limit maximum
                if (isFloatMode)
                    return UnaryConstantExpression::floatResult((UnaryConstantExpression::interpretAsFloat(value1) > UnaryConstantExpression::interpretAsFloat(value2)) ? UnaryConstantExpression::interpretAsFloat(value2) : UnaryConstantExpression::interpretAsFloat(value1));
                return (value1 > value2) ? value2 : value1;
            case OperatorType::OpAnd:
                return value1 & value2;
            case OperatorType::OpOr:
                return value1 | value2;
                break;
            case OperatorType::OpXor:
                return value1 ^ value2;
                break;
            case OperatorType::OpAdd:
                if (isFloatMode) // float add
                    return UnaryConstantExpression::floatResult(UnaryConstantExpression::interpretAsFloat(value1) + UnaryConstantExpression::interpretAsFloat(value2));
                return value1 + value2;
            case OperatorType::OpSub:
                if (isFloatMode) // float sub
                    return UnaryConstantExpression::floatResult(UnaryConstantExpression::interpretAsFloat(value1) - UnaryConstantExpression::interpretAsFloat(value2));
                return value1 - value2;
            case OperatorType::OpSar:
                return value1 >> (value2 & 0xFF);
            case OperatorType::OpRev: {
                const int value2Mask = value2 & 0xFF;
                int value1Shift = value1;
                int result = 0;
                for (int i = 0; i < value2Mask; i++) {
                    result <<= 1;
                    result |= (value1Shift & 0x01);
                    value1Shift >>= 1;
                }
                return result;
            }
            case OperatorType::OpLogAnd: {
                const int result = (value1 != 0 ? 0xFFFFFFFF : 0) & (value2 != 0 ? 0xFFFFFFFF : 0);
                if (isFloatMode)
                    return UnaryConstantExpression::floatResult(result!=0 ? 1.0f : 0.0f);
                return result;
            }
            case OperatorType::OpLogOr: {
                const int result = (value1 != 0 ? 0xFFFFFFFF : 0) | (value2 != 0 ? 0xFFFFFFFF : 0);
                if (isFloatMode)
                    return UnaryConstantExpression::floatResult(result!=0 ? 1.0f : 0.0f);
                return result;
            }
            case OperatorType::OpMul:
                if (isFloatMode) // float mul
                    return UnaryConstantExpression::floatResult(UnaryConstantExpression::interpretAsFloat(value1) * UnaryConstantExpression::interpretAsFloat(value2));
                return value1 * value2;
            case OperatorType::OpScl: {
                    // calculate the upper 32bits of the 64bit result of multiplying two 32bit numbers
                    // I did it this way to avoid using compiler specific stuff.
                    const int a = (value1 >> 16) & 0xffff;
                    const int b = value1 & 0xffff;
                    const int c = (value2 >> 16) & 0xffff;
                    const int d = value2 & 0xffff;
                    const int x = a * d + c * b;
                    const int y = (((b * d) >> 16) & 0xffff) + x;
                    const int result = (y >> 16) & 0xffff;
                    return result + a * c;
                }
            case OperatorType::OpDiv:
                //TODO div by zero?
                if (isFloatMode) // float div
                    return UnaryConstantExpression::floatResult(UnaryConstantExpression::interpretAsFloat(value1) / UnaryConstantExpression::interpretAsFloat(value2));
                return value1 / value2;
            case OperatorType::OpRem: // remainder (mod)
                //TODO div by zero?
                return value1 % value2;
            case OperatorType::OpCmpB:
            case OperatorType::OpCmpA:
            case OperatorType::OpCmpNe:
            case OperatorType::OpCmpE:
            case OperatorType::OpCmpBe:
            case OperatorType::OpCmpAe: {
                if (isFloatMode) {
                    // float cmp
                    const int tmpRes = (UnaryConstantExpression::interpretAsFloat(value1) < UnaryConstantExpression::interpretAsFloat(value2)) ? 1 :
                                       ((UnaryConstantExpression::interpretAsFloat(value1) > UnaryConstantExpression::interpretAsFloat(value2)) ? 2 : 4);
                    return UnaryConstantExpression::floatResult((tmpRes & operation) != 0 ? 1.0f : 0.0f);
                }
                const int tmpRes = (value1 < value2) ? 1 : ((value1 > value2) ? 2 : 4);
                return (tmpRes & operation) != 0 ? 0xFFFFFFFF : 0;
            }
            default:
                return 0;
        }
        return 0;
    }

    static int rol(unsigned int value, int places) {
        return (value << places) | (value >> ((8 * sizeof(value)) - places));
    }

    static int ror(unsigned int value, int places) {
        return (value >> places) | (value << ((8 * sizeof(value)) - places));
    }

    virtual std::string toILangStr() const {
        return "("+OperatorType::toString(operation)+" "+left->toILangStr()+" "+right->toILangStr()+")";
    };
};

struct DatCurrentCogPosConstantExpression : public AbstractConstantExpression {
    explicit DatCurrentCogPosConstantExpression(const SourcePosition& sourcePosition):AbstractConstantExpression(sourcePosition) {}
    virtual ~DatCurrentCogPosConstantExpression() {}
    virtual int evaluate(AbstractBinaryGenerator* generator) const {
        return generator->currentDatCogOrg() >> 2;
    }
    virtual std::string toILangStr() const {
        return "(currentCogPos)";
    }
};

struct DatSymbolConstantExpression : public AbstractConstantExpression {
    DatSymbolConstantExpression(const SourcePosition& sourcePosition, DatSymbolId datSymbolId, bool isCogPos):AbstractConstantExpression(sourcePosition),datSymbolId(datSymbolId),isCogPos(isCogPos) {}
    virtual ~DatSymbolConstantExpression() {}
    const DatSymbolId datSymbolId;
    const bool isCogPos;
    virtual int evaluate(AbstractBinaryGenerator* generator) const {
        return generator->valueOfDatSymbol(datSymbolId, isCogPos);
    }
    virtual std::string toILangStr() const {
        return (isCogPos ? "(cogPos " : "(datPos ")+std::to_string(datSymbolId.value())+")";
    }
};

struct VarSymbolConstantExpression : public AbstractConstantExpression {
    explicit VarSymbolConstantExpression(const SourcePosition& sourcePosition, VarSymbolId varSymbolId):AbstractConstantExpression(sourcePosition),varSymbolId(varSymbolId) {}
    virtual ~VarSymbolConstantExpression() {}
    const VarSymbolId varSymbolId;
    virtual int evaluate(AbstractBinaryGenerator* generator) const {
        return generator->addressOfVarSymbol(varSymbolId);
    }
    virtual std::string toILangStr() const {
        return "(var "+std::to_string(varSymbolId.value())+")";
    }
};

struct LocSymbolConstantExpression : public AbstractConstantExpression {
    explicit LocSymbolConstantExpression(const SourcePosition& sourcePosition, LocSymbolId locSymbolId):AbstractConstantExpression(sourcePosition),locSymbolId(locSymbolId) {}
    virtual ~LocSymbolConstantExpression() {}
    const LocSymbolId locSymbolId;
    virtual int evaluate(AbstractBinaryGenerator* generator) const {
        return generator->addressOfLocSymbol(locSymbolId);
    }
    virtual std::string toILangStr() const {
        return "(loc "+std::to_string(locSymbolId.value())+")";
    }
};

struct ChildObjConstantExpression : public AbstractConstantExpression {
    explicit ChildObjConstantExpression(const SourcePosition& sourcePosition, ObjectClassId objectClass, int constantIndex):AbstractConstantExpression(sourcePosition),objectClass(objectClass),constantIndex(constantIndex) {}
    virtual ~ChildObjConstantExpression() {}
    const ObjectClassId objectClass;
    const int constantIndex;
    virtual int evaluate(AbstractBinaryGenerator* generator) const {
        return generator->valueOfConstantOfObjectClass(objectClass, constantIndex);
    }
    virtual std::string toILangStr() const {
        return "(objconst "+std::to_string(objectClass.value())+" "+std::to_string(constantIndex)+")";
    }
};

#endif //SPINCOMPILER_CONSTANTEXPRESSION_H

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
