//////////////////////////////////////////////////////////////
//                                                          //
// Propeller Spin/PASM Compiler                             //
// (c)2012-2016 Parallax Inc. DBA Parallax Semiconductor.   //
// Adapted from Chip Gracey's x86 asm code by Roy Eltham    //
// Rewritten to modern C++ by Thilo Ackermann               //
// See end of file for terms of use.                        //
//                                                          //
////////////////////////////////////////////////////////////// 

#ifndef SPINCOMPILER_STRONGTYPEDEFINT_H
#define SPINCOMPILER_STRONGTYPEDEFINT_H

template<typename T> class StrongTypedefInt {
public:
    StrongTypedefInt():m_value(-1) {}
    explicit StrongTypedefInt(int value):m_value(value) {}
    bool valid() const {
        return m_value>=0;
    }
    bool operator==(StrongTypedefInt<T> other) const {
        return m_value == other.m_value;
    }
    bool operator!=(StrongTypedefInt<T> other) const {
        return m_value != other.m_value;
    }
    bool operator<(StrongTypedefInt<T> other) const {
        return m_value < other.m_value;
    }
    bool operator>(StrongTypedefInt<T> other) const {
        return m_value > other.m_value;
    }
    bool operator<=(StrongTypedefInt<T> other) const {
        return m_value <= other.m_value;
    }
    bool operator>=(StrongTypedefInt<T> other) const {
        return m_value >= other.m_value;
    }
    int value() const {
        return m_value;
    }
private:
    int m_value;
};

typedef StrongTypedefInt<struct SpinSymbolIdT> SpinSymbolId;
typedef StrongTypedefInt<struct TokenIndexT> TokenIndex;
typedef StrongTypedefInt<struct ObjectInstanceIdT> ObjectInstanceId;
typedef StrongTypedefInt<struct ObjectClassIdT> ObjectClassId;
typedef StrongTypedefInt<struct MethodIdT> MethodId;
typedef StrongTypedefInt<struct DatSymbolIdT> DatSymbolId;
typedef StrongTypedefInt<struct LocSymbolIdT> LocSymbolId;
typedef StrongTypedefInt<struct VarSymbolIdT> VarSymbolId;
typedef StrongTypedefInt<struct SpinByteCodeLabelT> SpinByteCodeLabel;

#endif //SPINCOMPILER_STRONGTYPEDEFINT_H

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
