//////////////////////////////////////////////////////////////
//                                                          //
// Propeller Spin/PASM Compiler                             //
// (c)2012-2016 Parallax Inc. DBA Parallax Semiconductor.   //
// Adapted from Chip Gracey's x86 asm code by Roy Eltham    //
// Rewritten to modern C++ by Thilo Ackermann               //
// See end of file for terms of use.                        //
//                                                          //
////////////////////////////////////////////////////////////// 

#ifndef SPINCOMPILER_SYMBOLMAP_H
#define SPINCOMPILER_SYMBOLMAP_H

#include "SpinCompiler/Types/StrongTypedefInt.h"
#include "SpinCompiler/Types/Symbols.h"
#include <string>
#include <map>
#include <vector>

class SymbolMap {
private:
    struct Key {
        Key(SpinSymbolId symbolId, int classId):symbolId(symbolId),classId(classId) {}
        SpinSymbolId symbolId;
        int classId;
        bool operator<(Key other) const {
            if (symbolId<other.symbolId)
                return true;
            if (symbolId>other.symbolId)
                return false;
            return classId < other.classId;
        }
    };
    std::map<Key, SpinAbstractSymbolP> m_map;
public:
    SpinAbstractSymbolP hasSymbol(SpinSymbolId symbolId, int classId) const {
        auto it = m_map.find(Key(symbolId, classId));
        if (it == m_map.end())
            return SpinAbstractSymbolP();
        return it->second;
    }
    template<typename T> std::shared_ptr<T> hasSpecificSymbol(SpinSymbolId symbolId, int classId) const {
        return std::dynamic_pointer_cast<T>(hasSymbol(symbolId, classId));
    }
    void addSymbol(SpinSymbolId symbolId, SpinAbstractSymbolP symbol, int classId) {
        m_map[Key(symbolId,classId)] = symbol;
    }
};

#endif //SPINCOMPILER_SYMBOLMAP_H

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
