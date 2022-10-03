//////////////////////////////////////////////////////////////
//                                                          //
// Propeller Spin/PASM Compiler                             //
// (c)2012-2016 Parallax Inc. DBA Parallax Semiconductor.   //
// Adapted from Chip Gracey's x86 asm code by Roy Eltham    //
// Rewritten to modern C++ by Thilo Ackermann               //
// See end of file for terms of use.                        //
//                                                          //
////////////////////////////////////////////////////////////// 

#ifndef SPINCOMPILER_UNUSEDMETHODELIMINATION_H
#define SPINCOMPILER_UNUSEDMETHODELIMINATION_H

#include "SpinCompiler/Parser/ParsedObject.h"
#include "SpinCompiler/Generator/Instruction.h"

class UnusedMethodElimination {
private:
    std::map<ParsedObject*,std::vector<bool> > m_usedMethods;
public:
    static void eliminateUnused(ParsedObjectP rootObj, bool includeAllCogNewMethods) {
        UnusedMethodElimination ume;
        if (rootObj->methods.empty())
            return;
        ume.markMethodUsedAndFollow(rootObj.get(),rootObj->methods[0]->methodId);
        if (includeAllCogNewMethods)
            ume.markAllCogNewMethods(); //compatible with old openspin ume method (all cognew methods are marked as used)
        ume.removeUnusedObjects(rootObj);
        ume.removeUnusedMethods();
    }
private:
    UnusedMethodElimination() {
    }

    void markAllCogNewMethods() {
        std::vector<ParsedObject*> usedObjects;
        while (usedObjects.size() != m_usedMethods.size()) {
            usedObjects.clear();
            for (auto it = m_usedMethods.begin(); it != m_usedMethods.end(); ++it)
                usedObjects.push_back(it->first);
            for (auto obj:usedObjects) {
                for (auto method:obj->methods) {
                    inspectInstructions(obj, method->functionBody, true);
                }
            }
        }
    }

    void removeUnusedObjects(ParsedObjectP obj) {
        for (auto& ch:obj->childObjects) {
            if (!ch.isUsed)
                continue;
            if (m_usedMethods.find(ch.object.get()) == m_usedMethods.end())
                ch.isUsed = false;
            else
                removeUnusedObjects(ch.object);
        }
    }
    void removeUnusedMethods() {
        for (auto it = m_usedMethods.begin(); it != m_usedMethods.end(); ++it) {
            auto obj = it->first;
            std::vector<ParsedObject::MethodP> usedMethods;
            for (unsigned int i=0; i<obj->methods.size(); ++i)
                if (it->second[i])
                    usedMethods.push_back(obj->methods[i]);
            obj->methods = usedMethods;
        }
    }
    void markMethodUsedAndFollow(ParsedObject* obj, MethodId methodId) {
        const int methodIdx = obj->methodIndexById(methodId);
        if (markMethodUsed(obj, methodIdx))
            return; //was already marked used, do not follow anymore
        inspectInstructions(obj, obj->methods[methodIdx]->functionBody, false);
    }

    void inspectInstructions(ParsedObject* obj, AbstractInstructionP topInstruction, bool onlyCogInitNewRoutines) {
        AbstractInstruction::iterateInstruction(topInstruction,[this, obj, onlyCogInitNewRoutines](AbstractInstructionP instruction) {
            if (auto cogInit = std::dynamic_pointer_cast<CogInitSpinInstruction>(instruction))
                markMethodUsedAndFollow(obj, cogInit->methodId);
            instruction->iterateChildExpressions([this,obj, onlyCogInitNewRoutines](AbstractExpressionP expression) {
                if (auto cogNew = std::dynamic_pointer_cast<CogNewSpinExpression>(expression))
                    markMethodUsedAndFollow(obj, cogNew->methodId);
                if (onlyCogInitNewRoutines)
                    return;
                if (auto methodCall = std::dynamic_pointer_cast<MethodCallExpression>(expression)) {
                    if (methodCall->objectInstanceId.valid()) //method call of child obj
                        markMethodUsedAndFollow(obj->childObjectByObjectInstanceId(methodCall->objectInstanceId).object.get(), methodCall->methodId);
                    else //method call of own obj
                        markMethodUsedAndFollow(obj, methodCall->methodId);
                }
            });
        });
    }

    //returns true, if the method was already marked used before
    bool markMethodUsed(ParsedObject* obj, int methodIdx) {
        auto mapEntry = m_usedMethods.find(obj);
        if (mapEntry != m_usedMethods.end()) {
            if (mapEntry->second[methodIdx])
                return true;
            mapEntry->second[methodIdx] = true;
            return false;
        }
        //object entry missing
        auto& newEntry = m_usedMethods[obj];
        newEntry = std::vector<bool>(obj->methods.size(),false);
        newEntry[methodIdx] = true;
        return false;
    }
};

#endif //SPINCOMPILER_UNUSEDMETHODELIMINATION_H

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
