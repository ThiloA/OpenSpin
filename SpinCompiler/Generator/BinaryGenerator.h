//////////////////////////////////////////////////////////////
//                                                          //
// Propeller Spin/PASM Compiler                             //
// (c)2012-2016 Parallax Inc. DBA Parallax Semiconductor.   //
// Adapted from Chip Gracey's x86 asm code by Roy Eltham    //
// Rewritten to modern C++ by Thilo Ackermann               //
// See end of file for terms of use.                        //
//                                                          //
////////////////////////////////////////////////////////////// 

#ifndef SPINCOMPILER_BINARYGENERATOR_H
#define SPINCOMPILER_BINARYGENERATOR_H

#include "SpinCompiler/Generator/BinaryObject.h"
#include "SpinCompiler/Generator/SpinByteCodeWriter.h"
#include "SpinCompiler/Generator/Instruction.h"
#include "SpinCompiler/Parser/ParsedObject.h"
#include "SpinCompiler/Tokenizer/StringMap.h"
#include "SpinCompiler/Generator/DatCodeGenerator.h"

class BinaryGenerator {
public:
    static void generateBinaryForMethod(std::vector<unsigned char>& resultCode, std::vector<BinaryAnnotation>& resultAnnotation, AbstractBinaryGenerator *generator, ParsedObjectP currentObject, ParsedObject::MethodP method, int globalAddressStartCode) {

        std::vector<SpinFunctionByteCodeEntry> intermediateCode;
        SpinByteCodeWriter byteCodeWriter(intermediateCode);
        InstructionLoopContext rootLoopContext; //no loop block
        method->functionBody->generate(byteCodeWriter, rootLoopContext);
        auto strings = byteCodeWriter.retrieveAllStrings();

        BinaryGenerator spinBinGen(generator, currentObject, intermediateCode);
        spinBinGen.generateByteCode(globalAddressStartCode);
        spinBinGen.replaceStringPatches(stringConstantsGetOffsets(strings, globalAddressStartCode+spinBinGen.m_resultByteCode.size()));
        resultAnnotation.push_back(BinaryAnnotation(BinaryAnnotation::Method, spinBinGen.m_resultByteCode.size()));
        //std::cout<<generator->getNameBySymbolId(method->symbolId)<<std::endl;
        resultCode.insert(resultCode.end(), spinBinGen.m_resultByteCode.begin(), spinBinGen.m_resultByteCode.end());
        //append strings
        int stringTotalSize=0;
        for (const auto& s: strings) {
            for (auto cexpr:s.characters) {
                const int c = cexpr->evaluate(generator);
                if (c <= 0 || c > 0xFF)
                    throw CompilerError(ErrorType::scmr, s.sourcePosition);
                resultCode.push_back(char(c));
            }
            resultCode.push_back(char(0));
            stringTotalSize += s.characters.size()+1; //including null character
        }
        resultAnnotation.push_back(BinaryAnnotation(BinaryAnnotation::StringPool, stringTotalSize));
    }
private:
    struct StringPatch {
        StringPatch(int byteCodeOffset, int stringNumber):byteCodeOffset(byteCodeOffset),stringNumber(stringNumber) {}
        int byteCodeOffset; //replace at relative byte code offset
        int stringNumber;   //with absolute pointer to this string number
    };
    std::vector<unsigned char> m_resultByteCode; //this bytecode ist almost done, only string patching required
    std::vector<StringPatch> m_stringPatches;
    std::vector<int> m_absoluteAddresses;
    std::map<SpinByteCodeLabel,int> m_addressOfLabel;
    AbstractBinaryGenerator *m_generator;
    ParsedObjectP m_currentObject;
    const std::vector<SpinFunctionByteCodeEntry>& m_methodCode;

    BinaryGenerator(AbstractBinaryGenerator *generator, ParsedObjectP currentObject, const std::vector<SpinFunctionByteCodeEntry>& methodCode):m_generator(generator),m_currentObject(currentObject),m_methodCode(methodCode) {}

    void replaceStringPatches(const std::vector<int>& stringOffsets) {
        for (auto patch:m_stringPatches) {
            const int stringPtr = (patch.stringNumber>=0 && size_t(patch.stringNumber)<stringOffsets.size()) ? stringOffsets[patch.stringNumber] : 0;
            m_resultByteCode[patch.byteCodeOffset] = (((stringPtr >> 8) & 0xFF) | 0x80);
            m_resultByteCode[patch.byteCodeOffset+1] = stringPtr & 0xFF;
        }
    }

    static std::vector<int> stringConstantsGetOffsets(const std::vector<SpinStringInfo>& strings, const int baseOffset) {
        std::vector<int> result;
        result.reserve(strings.size());
        int strOffset = baseOffset;
        for (const auto& s: strings) {
            result.push_back(strOffset);
            strOffset += s.characters.size()+1; //including null character
        }
        return result;
    }

    void generateByteCode(const int globalStartAddress) {
        m_resultByteCode.reserve(m_methodCode.size()*2);
        m_absoluteAddresses = std::vector<int>(m_methodCode.size(), 0xFFC0); //TODO resize?
        bool absAddrModified = true;
        unsigned int lastSize = 0;
        while (true) {
            m_resultByteCode.clear();
            m_stringPatches.clear();
            for (unsigned int i=0; i<m_methodCode.size(); ++i) {
                const int thisAddr = globalStartAddress+m_resultByteCode.size();
                if (m_absoluteAddresses[i] != thisAddr) {
                    m_absoluteAddresses[i] = thisAddr;
                    absAddrModified = true;
                }
                generateByteCodeForElement(m_methodCode[i], thisAddr, absAddrModified);
            }
            if (m_resultByteCode.size() == lastSize && !absAddrModified)
                return;
            lastSize = m_resultByteCode.size();
            absAddrModified = false;
        }
    }

    int absoluteAddressOfLabel(SpinByteCodeLabel label) const {
        auto it = m_addressOfLabel.find(label);
        if (it == m_addressOfLabel.end())
            return 0xFFC0;
        return it->second;
    }

    void generateByteCodeForElement(SpinFunctionByteCodeEntry e, int currentAbsoluteAddress, bool &labelAddressModified) {
        switch(e.type) {
            case SpinFunctionByteCodeEntry::StaticByte:
                m_resultByteCode.push_back(e.value);
                break;
            case SpinFunctionByteCodeEntry::PushExprConstant:
                generateByteCodeForPushConstant(e.expression->evaluate(m_generator),ConstantEncoding(e.subroutineIdOrVarAddress));
                break;
            case SpinFunctionByteCodeEntry::PushIntConstant:
                generateByteCodeForPushConstant(e.value,ConstantEncoding(e.subroutineIdOrVarAddress));
                break;
            case SpinFunctionByteCodeEntry::StringReference: {
                m_stringPatches.push_back(StringPatch(m_resultByteCode.size(), e.value));
                m_resultByteCode.push_back(0x80);
                m_resultByteCode.push_back(0);
                break;
            }
            case SpinFunctionByteCodeEntry::CogInitNewSpinSubroutine:
                generateByteCodeForPushConstant(Token::packSubroutineParameterCountAndIndexForInterpreter(e.value, 1+m_currentObject->methodIndexById(MethodId(e.subroutineIdOrVarAddress))),ConstantEncoding::AutoDetect);
                break;
            case SpinFunctionByteCodeEntry::SubroutineOwnObject:
                m_resultByteCode.push_back(1+m_currentObject->methodIndexById(MethodId(e.subroutineIdOrVarAddress)));
                break;
            case SpinFunctionByteCodeEntry::SubroutineChildObject: {
                ObjectInstanceId oid(e.value);
                m_resultByteCode.push_back(m_generator->mapObjectInstanceIdToOffset(oid)); //object index
                const int methodIndex = m_currentObject->childObjectByObjectInstanceId(oid).object->methodIndexById(MethodId(e.subroutineIdOrVarAddress));
                m_resultByteCode.push_back(1+methodIndex);
                break;
            }
            case SpinFunctionByteCodeEntry::VariableDat:
            case SpinFunctionByteCodeEntry::VariableVar:
            case SpinFunctionByteCodeEntry::VariableLoc: {
                generateVariableReferenceByteCode(e.type, e.value, e.expression->evaluate(m_generator));
                break;
            }
            case SpinFunctionByteCodeEntry::PlaceLabel: {
                SpinByteCodeLabel lbl(e.value);
                if (absoluteAddressOfLabel(lbl) != currentAbsoluteAddress) {
                    m_addressOfLabel[lbl] = currentAbsoluteAddress;
                    labelAddressModified = true;
                }
                break;
            }
            case SpinFunctionByteCodeEntry::RelativeAddressToLabel: {
                int address = absoluteAddressOfLabel(SpinByteCodeLabel(e.value))-currentAbsoluteAddress; // make relative address
                address--; // compensate for single-byte
                if ((address >= -64 && address < 0) || (address >= 0 && address < 64)) // single byte, enter
                    address &= 0x007F;
                else { // double byte, compensate and enter
                    address--;
                    m_resultByteCode.push_back((unsigned char)((address >> 8) | 0x80));
                    address &= 0x00FF;
                }
                m_resultByteCode.push_back((unsigned char)address);
                break;
            }
            case SpinFunctionByteCodeEntry::AbsoluteAddressToLabel: {
                const int value = absoluteAddressOfLabel(SpinByteCodeLabel(e.value));
                if (value >= 0x100) { // two byte
                    m_resultByteCode.push_back(0x39); // 0x39 = 00111001b
                    m_resultByteCode.push_back((unsigned char)((value >> 8) & 0xFF));
                }
                else // one byte
                    m_resultByteCode.push_back(0x38); // 0x38 = 00111000b
                m_resultByteCode.push_back((unsigned char)(value & 0xFF));
                break;
            }
        }
    }
    void generateByteCodeForPushConstant(int value, ConstantEncoding encodingHint) {
        if (value >= -1 && value <= 1) {
            // constant is -1, 0, or 1, so compiles to a single bytecode
            m_resultByteCode.push_back((value+1) | 0x34);
            return;
        }

        // see if it's a mask
        // masks can be: only one bit on (e.g. 0x00008000),
        //				 all bits on except one (e.g. 0xFFFF7FFF),
        //			     all bits on up to a bit then all zeros (e.g. 0x0000FFFF),
        //				 or all bits off up to a bit then all ones (e.g. 0xFFFF0000)
        if (encodingHint != ConstantEncoding::NoMask) {
            for (unsigned char i = 0; i < 128; i++) {
                int testVal = 2;
                testVal <<= (i & 0x1F); // mask i, so that we only actually shift 0 to 31

                if (i & 0x20) // i in range 32 to 63 or 96 to 127
                    testVal--;
                if (i& 0x40) // i in range 64 to 127
                    testVal = ~testVal;
                if (testVal == value) {
                    m_resultByteCode.push_back(0x37); // (constant mask)
                    m_resultByteCode.push_back(i);
                    return;
                }
            }
        }

        // handle constants with upper 2 or 3 bytes being 0xFFs, using 'not'
        if ((value & 0xFFFFFF00) == 0xFFFFFF00) {
            // one byte constant using 'not'
            m_resultByteCode.push_back(0x38);
            unsigned char byteCode = (unsigned char)(value & 0xFF);
            m_resultByteCode.push_back(~byteCode);
            m_resultByteCode.push_back(0xE7); // (bitwise bot)
            return;
        }
        else if ((value & 0xFFFF0000) == 0xFFFF0000) {
            // two byte constant using 'not'
            m_resultByteCode.push_back(0x39);
            unsigned char byteCode = (unsigned char)((value >> 8) & 0xFF);
            m_resultByteCode.push_back(~byteCode);
            byteCode = (unsigned char)(value & 0xFF);
            m_resultByteCode.push_back(~byteCode);
            m_resultByteCode.push_back(0xE7); // (bitwise bot)
            return;
        }

        // 1 to 4 byte constant
        unsigned char size = 1;
        if (value & 0xFF000000)
            size = 4;
        else if (value & 0x00FF0000)
            size = 3;
        else if (value & 0x0000FF00)
            size = 2;
        unsigned char byteCode = 0x37 + size; // (constant 1..4 bytes)
        m_resultByteCode.push_back(byteCode);
        for (unsigned char i = size; i > 0; i--) {
            byteCode = (unsigned char)((value >> ((i - 1) * 8)) & 0xFF);
            m_resultByteCode.push_back(byteCode);
        }
    }
    void generateVariableReferenceByteCode(SpinFunctionByteCodeEntry::Type varType, const int packedInfo, int address) {
        int varInfoSize=0;
        int vOperation=0;
        bool indexSourcePtrIsNull=false;
        SpinFunctionByteCodeEntry::unpackVarInfo(packedInfo, indexSourcePtrIsNull, varInfoSize, vOperation);
        if (varType == SpinFunctionByteCodeEntry::VariableDat || varInfoSize != 2 || address >= 8*4 || !indexSourcePtrIsNull) {
            // not compact
            unsigned char byteCodeX = 0x80 | (varInfoSize << 5) | vOperation;
            if (!indexSourcePtrIsNull)
                byteCodeX |= 0x10;
            if (varType == SpinFunctionByteCodeEntry::VariableDat)
                byteCodeX += 4;
            else if (varType == SpinFunctionByteCodeEntry::VariableVar)
                byteCodeX += 8;
            else if (varType == SpinFunctionByteCodeEntry::VariableLoc)
                byteCodeX += 12;
            else
                throw CompilerError(ErrorType::internal);
            m_resultByteCode.push_back(byteCodeX);
            if (address > 0x7F) // two byte address
                m_resultByteCode.push_back((unsigned char)(address >> 8) | 0x80);
            m_resultByteCode.push_back((unsigned char)address);
            return;
        }
        // compact
        unsigned char byteCode = ((varType == SpinFunctionByteCodeEntry::VariableVar) ? 0x40 : 0x60) | vOperation;
        byteCode |= (unsigned char)address;
        m_resultByteCode.push_back(byteCode);
    }
};

struct GeneratorGlobalState {
    GeneratorGlobalState(const CompilerSettings &settings):settings(settings) {}
    std::map<ParsedObject*,BinaryObjectP> generatedObjects;
    std::map<ParsedObject*,BinaryObjectP> generatedConstantOnlyObjects;
    const CompilerSettings &settings;
};

class BinaryObjectGenerator : public AbstractBinaryGenerator {
public:
    struct DatSymbolOffset {
        DatSymbolOffset():objPtr(0),cogOrg(0) {}
        DatSymbolOffset(int objPtr, int cogOrg):objPtr(objPtr),cogOrg(cogOrg) {}
        int objPtr;
        int cogOrg;
    };

private:
    enum BuildState { Init=0, MethodTable=1, ObjectTable=2, DatSectionDone=3, MethodsDone=4, Done=5 };
    GeneratorGlobalState& m_globalState;
    const StringMap& m_stringMap;
    ParsedObjectP m_parsedObject;
    std::map<ObjectInstanceId, int> m_objectInstanceIdToObjectOffset;
    std::map<ObjectClassId,std::vector<int>> m_objectConstantsByObjectClass;
    std::map<DatSymbolId,DatSymbolOffset> m_datSymbols;
    std::map<VarSymbolId,int> m_varSymbols;
    std::map<LocSymbolId,int> m_locSymbols;
    BuildState m_state;
    BinaryObjectP m_result;
    int m_virtualAdditionalBinarySize;
    int m_cogOrg;
public:
    virtual ~BinaryObjectGenerator() {}
    explicit BinaryObjectGenerator(GeneratorGlobalState& globalState, const StringMap& nameMap,  ParsedObjectP& parsedObject):m_globalState(globalState),m_stringMap(nameMap),m_parsedObject(parsedObject),m_state(Init),m_result(BinaryObjectP(new BinaryObject())),m_virtualAdditionalBinarySize(0),m_cogOrg(0) {}
    static BinaryObjectP generateBinary(GeneratorGlobalState& globalState, const StringMap& nameMap, ParsedObjectP& parsedObject, const bool onlyConstants) {
        auto previousBuilt = globalState.generatedObjects.find(parsedObject.get());
        if (previousBuilt != globalState.generatedObjects.end())
            return previousBuilt->second;
        if (onlyConstants) {
            previousBuilt = globalState.generatedConstantOnlyObjects.find(parsedObject.get());
            if (previousBuilt != globalState.generatedConstantOnlyObjects.end())
                return previousBuilt->second;
        }

        auto res = BinaryObjectGenerator(globalState, nameMap, parsedObject).run(onlyConstants);
        if (!onlyConstants)
            globalState.generatedObjects[parsedObject.get()] = res;
        else
            globalState.generatedConstantOnlyObjects[parsedObject.get()] = res;
        return res;
    }

    virtual int mapObjectInstanceIdToOffset(ObjectInstanceId objectIndexId) const {
        if (m_state<ObjectTable)
            throw CompilerError(ErrorType::internal);
        auto res = m_objectInstanceIdToObjectOffset.find(objectIndexId);
        if (res == m_objectInstanceIdToObjectOffset.end())
            throw CompilerError(ErrorType::internal);
        return res->second;
    }

    virtual int valueOfConstantOfObjectClass(ObjectClassId objectClass, int constantIndex) const {
        if (m_state<MethodTable)
            throw CompilerError(ErrorType::internal);
        auto res = m_objectConstantsByObjectClass.find(objectClass);
        if (res == m_objectConstantsByObjectClass.end())
            throw CompilerError(ErrorType::internal);
        if (constantIndex<0 || constantIndex>=int(res->second.size()))
            throw CompilerError(ErrorType::internal);
        return res->second[constantIndex];
    }

    virtual void setDatSymbol(DatSymbolId datSymbolId, int objPtr, int cogOrg) {
        if (m_state != ObjectTable)
            throw CompilerError(ErrorType::internal);
        m_datSymbols[datSymbolId] = DatSymbolOffset(objPtr, cogOrg);
    }

    virtual int& currentDatCogOrg() {
        if (m_state<ObjectTable)
            throw CompilerError(ErrorType::internal);
        return m_cogOrg;
    }
    virtual int valueOfDatSymbol(DatSymbolId datSymbolId, bool cogPosition) const {
        if (m_state<ObjectTable)
            throw CompilerError(ErrorType::internal);
        auto res = m_datSymbols.find(datSymbolId);
        if (res == m_datSymbols.end())
            throw CompilerError(ErrorType::internal);
        if (!cogPosition)
            return res->second.objPtr;
        // the offset to the label symbol is in second symbol value
        int cogOrg = res->second.cogOrg;
        // make sure it's long aligned
        if (cogOrg & 0x03)
            throw CompilerError(ErrorType::ainl);
        // make sure is in range
        cogOrg >>= 2;
        if (cogOrg >= 0x1F0)
            throw CompilerError(ErrorType::aioor);
        return cogOrg;
    }

    virtual int addressOfVarSymbol(VarSymbolId varSymbolId) const {
        if (m_state<ObjectTable)
            throw CompilerError(ErrorType::internal);
        auto res = m_varSymbols.find(varSymbolId);
        if (res == m_varSymbols.end())
            throw CompilerError(ErrorType::internal);
        return res->second;
    }

    virtual int addressOfLocSymbol(LocSymbolId locSymbolId) const {
        if (m_state<ObjectTable)
            throw CompilerError(ErrorType::internal);
        auto res = m_locSymbols.find(locSymbolId);
        if (res == m_locSymbols.end())
            throw CompilerError(ErrorType::internal);
        return res->second;
    }

    virtual std::string getNameBySymbolId(SpinSymbolId symbol) const {
        return m_stringMap.getNameBySymbolId(symbol);
    }
    BinaryObjectP run(const bool onlyConstants) {
        if (m_state != Init)
            throw CompilerError(ErrorType::internal);
        //retrieve all child objects
        auto objectClassToChildObjectIndex = generateChildObjects(onlyConstants);
        //size of following tables
        if (m_globalState.settings.defaultCompileMode)
            m_virtualAdditionalBinarySize+=4;

        //reserve capacity for method table
        if (!m_globalState.settings.compileDatOnly)
            m_virtualAdditionalBinarySize += 4*m_parsedObject->methods.size();
        m_state = MethodTable;

        const int childVarSize = generateChildInstanceTable(objectClassToChildObjectIndex); //table of all child object instances of this object
        generateVarAddresses(); //generate all var symbols
        m_state = ObjectTable;

        if (!onlyConstants) {
            DatCodeGenerator(m_result->ownData, m_result->ownDataAnnotation, this, m_parsedObject->datCode, objectBinarySize(), false).generate();
            DatCodeGenerator(m_result->ownData, m_result->ownDataAnnotation, this, m_parsedObject->datCode, objectBinarySize(), true).generate();
        }
        m_state = DatSectionDone;

        //method table
        if (!onlyConstants && !m_globalState.settings.compileDatOnly)
            generateMethods();
        m_state = MethodsDone;

        //generate constants for external use (in parent objects)
        for (auto c:m_parsedObject->constants)
            m_result->constants.push_back(c.constantExpression->evaluate(this));


        // align obj_ptr to long
        if (!m_globalState.settings.compileDatOnly) {
            int padSize=0;
            while ((m_result->ownData.size() & 3) != 0) {
                m_result->ownData.push_back(0);
                ++padSize;
            }
            if (padSize>0)
                m_result->ownDataAnnotation.push_back(BinaryAnnotation(BinaryAnnotation::Padding,padSize));
        }
        m_result->objectSize = objectBinarySize();
        m_result->sumChildVarSize = childVarSize;

        m_state = Done;
        return m_result;
    }
private:
    std::map<ObjectClassId,int> generateChildObjects(const bool onlyConstants) {
        std::map<ObjectClassId,ParsedObjectP> objectClassToParsedObjectMap;
        //object classes may not be continous, due to unused method elimination
        //this will map object class to a child object index that may be used to index childObjecs of SpinBinaryObject
        std::map<ObjectClassId,int> objectClassToChildObjectIndex;
        for (auto ch:m_parsedObject->childObjects) {
            if (ch.isUsed)
                objectClassToParsedObjectMap[ch.objectClass] = ch.object;
        }
        for (auto it = objectClassToParsedObjectMap.begin(); it != objectClassToParsedObjectMap.end(); ++it) {
            objectClassToChildObjectIndex[it->first] = m_result->childObjects.size();
            m_result->childObjects.push_back(BinaryObjectGenerator::generateBinary(m_globalState,m_stringMap,it->second, onlyConstants));
        }
        for (auto ch:m_parsedObject->childObjects) {
            //search normal object
            auto objIt = objectClassToChildObjectIndex.find(ch.objectClass);
            if (objIt != objectClassToChildObjectIndex.end()) { //found obj
                m_objectConstantsByObjectClass[ch.objectClass] = m_result->childObjects[objIt->second]->constants;
                continue;
            }
            //object was unused, built only for constants
            auto unusedObjConstants = BinaryObjectGenerator::generateBinary(m_globalState,m_stringMap,ch.object, true)->constants;
            m_objectConstantsByObjectClass[ch.objectClass] = unusedObjConstants;
        }
        return objectClassToChildObjectIndex;
    }
    int objectBinarySize() const {
        return m_virtualAdditionalBinarySize + int(m_result->ownData.size());
    }
    int generateLocalsForMethod(ParsedObject::MethodP method) {
        //generate a lookup table to get stack addresses for each local (including parameters and result)
        //returns the size of the stack excluding parameters and result
        int localByteOffset = 0;
        int parameterAndResultSize = 0;
        m_locSymbols.clear();
        for (int j=0; j<int(method->allLocals.size()); ++j) {
            const auto& loc = method->allLocals[j];
            m_locSymbols[loc.localVarId] = localByteOffset;
            localByteOffset += 4*loc.count->evaluate(this);
            if (localByteOffset > SpinLimits::LocLimit)
                throw CompilerError(ErrorType::loxlve, loc.sourcePosition);
            if (j<=method->parameterCount)
                parameterAndResultSize = localByteOffset;
        }
        return localByteOffset-parameterAndResultSize;
    }
    void generateMethods() {
        m_result->annotatedMethodNames = BinaryAnnotation::TableEntryNamesP(new BinaryAnnotation::TableEntryNames());
        for (auto method:m_parsedObject->methods) {
            //calculate stack size for locals (exluding result value and parameters)
            const int sumLocalVarStackSize = generateLocalsForMethod(method);
            m_result->methodTable.push_back((objectBinarySize()&0xFFFF) | (sumLocalVarStackSize << 16));
            m_result->annotatedMethodNames->names.push_back(getNameBySymbolId(method->symbolId));
            BinaryGenerator::generateBinaryForMethod(m_result->ownData, m_result->ownDataAnnotation, this, m_parsedObject, method, objectBinarySize());
            m_locSymbols.clear();
        }
    }
    int generateChildInstanceTable(std::map<ObjectClassId,int> &objectClassToChildObjectIndex) {
        m_result->annotatedInstanceNames = BinaryAnnotation::TableEntryNamesP(new BinaryAnnotation::TableEntryNames());
        m_result->objectStart = objectBinarySize();
        m_result->objectCount = 0;
        int objectOffset = m_parsedObject->methods.size()+1;
        int childVarSize = 0;
        for (auto childObj:m_parsedObject->childObjects) {
            if (!childObj.isUsed)
                continue;
            m_objectInstanceIdToObjectOffset[childObj.objectInstanceId] = objectOffset;
            const int instanceCount = childObj.numberOfInstances->evaluate(this);
            if (instanceCount < 1 || instanceCount > 255)
                throw CompilerError(ErrorType::ocmbf1tx);
            auto instanceName = getNameBySymbolId(childObj.symbolId);
            for (int i=0; i < instanceCount; i++) {
                if (objectBinarySize() >= 256*4) //TODO konstante nicht hardcoden
                    throw CompilerError(ErrorType::loxspoe);
                const int childObjectIndex = objectClassToChildObjectIndex[childObj.objectClass];
                m_result->objectInstanceIndices.push_back(childObjectIndex);
                m_result->annotatedInstanceNames->names.push_back(instanceCount>1 ? instanceName+"["+std::to_string(i)+"]" : instanceName);
                m_virtualAdditionalBinarySize += 4;
                m_result->objectCount++;
                childVarSize += m_result->childObjects[childObjectIndex]->totalVarSize();
            }
            objectOffset += instanceCount;
        }
        return childVarSize;
    }
    void generateVarAddresses() {
        int varByteBytes = 0;
        int varWordBytes = 0;
        int varLongBytes = 0;
        for (auto v:m_parsedObject->globalVariables) {
            const int nCount = v->count->evaluate(this);
            if (nCount > SpinLimits::VarLimit || nCount<0)
                throw CompilerError(ErrorType::tmvsid, v->sourcePosition);
            int varAddr = 0;
            switch(v->size) {
                case 0:
                    varAddr = varByteBytes;
                    varByteBytes += nCount;
                    break;
                case 1:
                    varAddr = varWordBytes;
                    varWordBytes += nCount<<1;
                    break;
                case 2:
                    varAddr = varLongBytes;
                    varLongBytes += nCount<<2;
                    break;
            }
            m_varSymbols[v->id] = varAddr;
        }
        //vars are ordered by size (longs, words, bytes), so address of words and vars must be adjusted
        for (auto v:m_parsedObject->globalVariables) {
            int varAddr = m_varSymbols[v->id];
            switch(v->size) {
                case 0: //bytes follow words
                    varAddr += varLongBytes+varWordBytes;
                    break;
                case 1: //words follow longs
                    varAddr += varLongBytes;
                    break;
                case 2: //longs are at the begin, no adjustment required
                    break;
            }
            if (varAddr > SpinLimits::VarLimit || varAddr<0)
                throw CompilerError(ErrorType::tmvsid, v->sourcePosition);
            m_varSymbols[v->id] = varAddr;
        }

        // calculate var_ptr and align to long
        int varTotalSize = varByteBytes+varWordBytes+varLongBytes;
        if ((varTotalSize & 0x00000003) != 0)
            varTotalSize = (varTotalSize | 0x00000003) + 1;
        if (varTotalSize > SpinLimits::VarLimit)
            throw CompilerError(ErrorType::tmvsid);

        m_result->ownVarSize = varTotalSize;
    }
};

#endif //SPINCOMPILER_BINARYGENERATOR_H

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
