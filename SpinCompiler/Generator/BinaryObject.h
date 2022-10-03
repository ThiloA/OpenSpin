//////////////////////////////////////////////////////////////
//                                                          //
// Propeller Spin/PASM Compiler                             //
// (c)2012-2016 Parallax Inc. DBA Parallax Semiconductor.   //
// Adapted from Chip Gracey's x86 asm code by Roy Eltham    //
// Rewritten to modern C++ by Thilo Ackermann               //
// See end of file for terms of use.                        //
//                                                          //
////////////////////////////////////////////////////////////// 

#ifndef SPINCOMPILER_BINARYOBJECT_H
#define SPINCOMPILER_BINARYOBJECT_H

#include "SpinCompiler/Types/BinaryAnnotation.h"
#include "SpinCompiler/Types/CompilerError.h"
#include "SpinCompiler/Types/SpinLimits.h"
#include "SpinCompiler/Types/CompilerSettings.h"
#include <map>

// OBJ structure:
//
//              word    varsize, pgmsize                ;variable and program sizes
//
//          0:  word    objsize                         ;object size (w/o sub-objects)
//              byte    objindex>>2, objcount           ;sub-object start index and count
//          4:  word    PUBn offset, PUBn locals        ;index to PUBs (multiple)
//              word    PRIn offset, PRIn locals        ;index to PRIs (multiple)
//   objindex:  word    OBJn offset, OBJn var offset    ;index to OBJs (multiple)
//              byte    DAT data...                     ;DAT data
//              byte    PUB data...                     ;PUB data
//              byte    PRI data...                     ;PRI data
//    objsize:
//              long    OBJ data...                     ;OBJ data (sub-objects)
//    pgmsize:
//              byte    checksum                        ;checksum reveals language_version
//              byte    'PUBn', parameters              ;PUB names and parameters (0..15)
//              byte    'CONn', 16, values              ;CON names and values
//

typedef std::shared_ptr<struct BinaryObject> BinaryObjectP;
struct BinaryObject {
    BinaryObject():ownVarSize(0),sumChildVarSize(0),programSize(0),objectSize(0),objectStart(0),objectCount(0) {}
    int ownVarSize;
    int sumChildVarSize;
    int programSize;
    int objectSize;
    int objectStart;
    int objectCount;
    std::vector<int> methodTable; //offset / locals
    BinaryAnnotation::TableEntryNamesP annotatedMethodNames;
    std::vector<int> objectInstanceIndices; //points to a childObject
    BinaryAnnotation::TableEntryNamesP annotatedInstanceNames;
    std::vector<unsigned char> ownData; //DAT, PUB, PRI
    std::vector<BinaryAnnotation> ownDataAnnotation;
    std::vector<BinaryObjectP> childObjects; //order of child objects in binary file
    std::vector<int> constants; //indexed by constant index

    int totalVarSize() const {
        return ownVarSize+sumChildVarSize;
    }

    int calculateSizeWithoutChildren(const CompilerSettings &settings) const {
        return ((settings.defaultCompileMode) ? 4 : 0)+methodTable.size()*4+objectInstanceIndices.size()*4+ownData.size();
    }

    void distilledToBinary(std::vector<unsigned char> &result, std::vector<BinaryAnnotation>& resultAnnotation, const CompilerSettings &settings) {
        std::vector<SameObjectPair> sameObjects;
        auto distilled = distill(sameObjects);
        std::map<const BinaryObject*,int> objectOffsets;
        int offset = 0;
        for (auto obj:distilled) {
            objectOffsets[obj] = offset;
            offset += obj->calculateSizeWithoutChildren(settings);
        }
        if (!sameObjects.empty()) {
            //object with different pointers have same binary data
            bool modified = false;
            do {
                modified = false;
                for (auto it:sameObjects) {
                    auto foundObj1 = objectOffsets.find(it.obj1);
                    auto foundObj2 = objectOffsets.find(it.obj2);
                    if (foundObj1 == objectOffsets.end() && foundObj2 != objectOffsets.end()) { //first found, second not found
                        modified = true;
                        objectOffsets[it.obj1] = foundObj2->second;
                    }
                    else if (foundObj1 != objectOffsets.end() && foundObj2 == objectOffsets.end()) { //first found, second not found
                        modified = true;
                        objectOffsets[it.obj2] = foundObj1->second;
                    }
                }
            }
            while(modified);
        }
        for (auto obj:distilled)
            obj->singleObjectToBinary(result,resultAnnotation,settings,objectOffsets);
    }
private:
    struct SameObjectPair {
        SameObjectPair():obj1(nullptr),obj2(nullptr) {}
        SameObjectPair(const BinaryObject* obj1, const BinaryObject* obj2):obj1(obj1),obj2(obj2) {}
        const BinaryObject* obj1;
        const BinaryObject* obj2;
    };
    bool isSame(const BinaryObject* other) const {
        if (/*other->ownVarSize != ownVarSize || */other->childObjects.size() != childObjects.size() || other->methodTable.size() != methodTable.size() || other->objectInstanceIndices.size() != objectInstanceIndices.size() || other->ownData.size() != ownData.size())
            return false;
        if (other->ownData != ownData)
            return false;
        if (other->methodTable!=methodTable || other->objectInstanceIndices != objectInstanceIndices || other->childObjects != childObjects)
            return false;
        return true;
    }
    std::vector<const BinaryObject*> distill(std::vector<SameObjectPair>& sameObjects) const {
        std::vector<const BinaryObject*> result;
        result.push_back(this);
        for (auto child:childObjects) {
            auto distilledChild = child->distill(sameObjects);
            result.insert(result.end(), distilledChild.begin(), distilledChild.end());
        }
        //remove duplicates
        for (unsigned int i=0; i<result.size(); ++i) {
            auto obj = result[i];
            unsigned int lastIdx = i;
            bool foundDuplicate = false;
            for (unsigned int j=i+1; j<result.size(); ++j) {
                if (result[j] != obj) {
                    if (!obj->isSame(result[j]))
                        continue;
                    sameObjects.push_back(SameObjectPair(obj,result[j]));
                }
                result.erase(result.begin()+lastIdx);
                --j;
                lastIdx = j;
                foundDuplicate = true;
            }
            if (foundDuplicate)
                --i;
        }
        return result;
    }

    void singleObjectToBinary(std::vector<unsigned char> &result, std::vector<BinaryAnnotation>& resultAnnotation, const CompilerSettings &settings, const std::map<const BinaryObject*,int> &objectOffsets) const {
        const int startOffsetOfThisObject=result.size();
        if (settings.defaultCompileMode) {
            resultAnnotation.push_back(BinaryAnnotation(BinaryAnnotation::ObjectHeader, 4));
            result.push_back(objectSize & 0xFF);
            result.push_back((objectSize>>8) & 0xFF);
            result.push_back((objectStart>>2) & 0xFF);
            result.push_back(objectCount & 0xFF);
        }
        int varOffset=ownVarSize;
        //add method table
        resultAnnotation.push_back(BinaryAnnotation(BinaryAnnotation::MethodTable, 4*methodTable.size(), annotatedMethodNames));
        for (int m:methodTable)
            addLong(result, m);
        resultAnnotation.push_back(BinaryAnnotation(BinaryAnnotation::ObjectTable, 4*objectInstanceIndices.size(), annotatedInstanceNames));
        //add obj table
        for (unsigned int i=0; i<objectInstanceIndices.size(); ++i) {
            const auto obj = childObjects[objectInstanceIndices[i]];
            const auto objOffst = objectOffsets.find(obj.get());
            if (objOffst == objectOffsets.end())
                throw CompilerError(ErrorType::internal);
            addLong(result,((objOffst->second-startOffsetOfThisObject)&0xFFFF)+(varOffset<<16));
            varOffset += obj->totalVarSize();
            if (varOffset > SpinLimits::VarLimit)
                throw CompilerError(ErrorType::tmvsid);
        }
        result.insert(result.end(),ownData.begin(),ownData.end());
        resultAnnotation.insert(resultAnnotation.end(),ownDataAnnotation.begin(),ownDataAnnotation.end());

    }
    static void addLong(std::vector<unsigned char>& result, int value) {
        result.push_back(value&0xFF);
        result.push_back((value>>8)&0xFF);
        result.push_back((value>>16)&0xFF);
        result.push_back((value>>24)&0xFF);
    }
    static void setLong(std::vector<unsigned char>& result, int idx, int value) {
        result[idx] = (value)&0xFF;
        result[idx+1] = (value>>8)&0xFF;
        result[idx+2] = (value>>16)&0xFF;
        result[idx+3] = (value>>24)&0xFF;
    }
};


#endif //SPINCOMPILER_BINARYOBJECT_H

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
