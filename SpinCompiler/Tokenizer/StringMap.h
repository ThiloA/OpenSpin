//////////////////////////////////////////////////////////////
//                                                          //
// Propeller Spin/PASM Compiler                             //
// (c)2012-2016 Parallax Inc. DBA Parallax Semiconductor.   //
// Adapted from Chip Gracey's x86 asm code by Roy Eltham    //
// Rewritten to modern C++ by Thilo Ackermann               //
// See end of file for terms of use.                        //
//                                                          //
////////////////////////////////////////////////////////////// 

#ifndef SPINCOMPILER_STRINGMAP_H
#define SPINCOMPILER_STRINGMAP_H

#include "SpinCompiler/Types/StrongTypedefInt.h"
#include <string>
#include <map>
#include <vector>

class StringMap {
public:
    SpinSymbolId hasSymbolName(const std::string& str) const {
        auto it = m_map.find(str);
        if (it == m_map.end())
            return SpinSymbolId(-1);
        return it->second;
    }
    SpinSymbolId getOrPutSymbolName(const std::string& str) {
        auto &res = m_map[str];
        if (!res.valid()) {
            res = SpinSymbolId(m_entries.size());
            m_entries.push_back(str);
        }
        return res;
    }
    std::string getNameBySymbolId(SpinSymbolId id) const {
        return id.valid() ? m_entries[id.value()] : std::string();
    }
private:
    std::map<std::string, SpinSymbolId> m_map;
    std::vector<std::string> m_entries;
};

#endif //SPINCOMPILER_STRINGMAP_H

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
