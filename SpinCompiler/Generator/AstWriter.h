//////////////////////////////////////////////////////////////
//                                                          //
// Propeller Spin/PASM Compiler                             //
// (c)2012-2016 Parallax Inc. DBA Parallax Semiconductor.   //
// Adapted from Chip Gracey's x86 asm code by Roy Eltham    //
// Rewritten to modern C++ by Thilo Ackermann               //
// See end of file for terms of use.                        //
//                                                          //
////////////////////////////////////////////////////////////// 

#ifndef SPINCOMPILER_ASTWRITER_H
#define SPINCOMPILER_ASTWRITER_H

#include "SpinCompiler/Parser/ParsedObject.h"
#include "SpinCompiler/Generator/Instruction.h"
#include "SpinCompiler/Generator/Expression.h"
#include "SpinCompiler/Tokenizer/StringMap.h"
#include <vector>

class ASTWriter {
private:
    const StringMap& m_strMap;
    ParsedObjectP m_rootObj;
    std::vector<unsigned char>& m_output;
    int m_indentLevel;
public:
    ASTWriter(const StringMap& strMap, ParsedObjectP rootObj, std::vector<unsigned char>& output, int indentLevel):m_strMap(strMap),m_rootObj(rootObj),m_output(output),m_indentLevel(indentLevel) {
    }

    void generate() {
        addLine("obj "+m_rootObj->shortName+":");
        indent();
        for (auto c:m_rootObj->childObjects)
            if (c.isUsed)
                addLine("childobj "+std::to_string(c.objectInstanceId.value())+" "+c.object->shortName+" "+c.numberOfInstances->toILangStr());
        for (auto v:m_rootObj->globalVariables)
            addLine("var "+std::to_string(v->id.value())+" "+std::to_string(v->size)+" "+v->count->toILangStr());
        for (auto m:m_rootObj->methods) {
            addLine("method "+std::to_string(m->methodId.value())+" "+m_strMap.getNameBySymbolId(m->symbolId)+" "+std::to_string(m->parameterCount)+":");
            indent();
            for (auto l:m->allLocals)
                addLine("local "+std::to_string(l.localVarId.value())+" "+l.count->toILangStr());
            addLine("body:");
            generateInstructionNoExtraBlock(m->functionBody);
            undent();
        }
    }
private:
    void generateInstructionNoExtraBlock(AbstractInstructionP instruction) {
        indent();
        if (auto ei = std::dynamic_pointer_cast<BlockInstruction>(instruction)) {
            for (auto i:ei->children)
                generateInstruction(i);
        }
        else
            generateInstruction(instruction);
        undent();
    }
    void generateInstruction(AbstractInstructionP instruction) {
        if (auto ei = std::dynamic_pointer_cast<ExpressionInstruction>(instruction)) {
            addLine("expr "+ei->expression->toILangStr());
            return;
        }
        if (auto ei = std::dynamic_pointer_cast<CogInitSpinInstruction>(instruction)) {
            addLine("coginitspin");
            return;
        }
        if (auto ei = std::dynamic_pointer_cast<CogInitAsmInstruction>(instruction)) {
            addLine("coginitasm");
            return;
        }
        if (auto ei = std::dynamic_pointer_cast<AbortOrReturnInstruction>(instruction)) {
            addLine((ei->isAbort ? "abort " : "return ")+exprToStr(ei->returnValue));
            return;
        }
        if (auto ei = std::dynamic_pointer_cast<NextOrQuitInstruction>(instruction)) {
            addLine(ei->isNext ? "next" : "quit");
            return;
        }
        if (auto ei = std::dynamic_pointer_cast<CallBuiltInInstruction>(instruction)) {
            addLine("builtin");
            return;
        }
        if (auto ei = std::dynamic_pointer_cast<BlockInstruction>(instruction)) {
            addLine("block:");
            indent();
            for (auto i:ei->children)
                generateInstruction(i);
            undent();
            return;
        }
        if (auto ei = std::dynamic_pointer_cast<IfInstruction>(instruction)) {
            for (unsigned i=0; i<ei->branches.size(); ++i) {
                auto b = ei->branches[i];
                addLine(std::string(i>0 ? "else " : "")+(b.conditionInverted ? "ifn ": "if ")+exprToStr(b.condition)+":");
                generateInstructionNoExtraBlock(b.instruction);
            }
            if (ei->elseBranch) {
                addLine("else:");
                generateInstructionNoExtraBlock(ei->elseBranch);
            }
            return;
        }
        if (auto ei = std::dynamic_pointer_cast<CaseInstruction>(instruction)) {
            addLine("case");
            return;
        }
        if (auto ei = std::dynamic_pointer_cast<LoopConditionInstruction>(instruction)) {
            switch(ei->type) {
                case LoopConditionInstruction::RepeatCount:
                    addLine("repeat count "+ei->condition->toILangStr()+":"); break;
                case LoopConditionInstruction::PreWhile:
                    addLine("repeat prewhile "+ei->condition->toILangStr()+":"); break;
                case LoopConditionInstruction::PreUntil:
                    addLine("repeat preuntil "+ei->condition->toILangStr()+":"); break;
                case LoopConditionInstruction::PostWhile:
                    addLine("repeat postwhile "+ei->condition->toILangStr()+":"); break;
                case LoopConditionInstruction::PostUntil:
                    addLine("repeat postuntil "+ei->condition->toILangStr()+":"); break;
                case LoopConditionInstruction::RepeatEndless:
                    addLine("repeat forever:"); break;
            }
            generateInstructionNoExtraBlock(ei->instruction);
            return;
        }
        if (auto ei = std::dynamic_pointer_cast<LoopVarInstruction>(instruction)) {
            std::string str = "repeat var "+ei->variable->toILangStr()+" "+ei->from->toILangStr()+" "+ei->to->toILangStr();
            if (ei->step)
                str += " step "+ei->step->toILangStr();
            addLine(str+":");
            generateInstructionNoExtraBlock(ei->instruction);
            return;
        }
        addLine("illegal");
    }
    std::string exprToStr(AbstractExpressionP expr) {
        if (!expr)
            return "null";
        return expr->toILangStr();
    }
    void indent() {
        m_indentLevel+=2;
    }
    void undent() {
        m_indentLevel-=2;
    }
    void addLine(const std::string& str) {
        for (int i=0; i<m_indentLevel; ++i)
            m_output.push_back(' ');
        m_output.insert(m_output.end(),str.begin(),str.end());
        m_output.push_back(char(10));
    }
};

#endif //SPINCOMPILER_ASTWRITER_H

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
