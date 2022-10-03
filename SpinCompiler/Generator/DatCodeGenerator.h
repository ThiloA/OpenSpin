//////////////////////////////////////////////////////////////
//                                                          //
// Propeller Spin/PASM Compiler                             //
// (c)2012-2016 Parallax Inc. DBA Parallax Semiconductor.   //
// Adapted from Chip Gracey's x86 asm code by Roy Eltham    //
// Rewritten to modern C++ by Thilo Ackermann               //
// See end of file for terms of use.                        //
//                                                          //
////////////////////////////////////////////////////////////// 

#ifndef SPINCOMPILER_DATCODEGENERATOR_H
#define SPINCOMPILER_DATCODEGENERATOR_H

#include "SpinCompiler/Types/BinaryAnnotation.h"
#include "SpinCompiler/Types/AbstractBinaryGenerator.h"
#include "SpinCompiler/Types/DatCodeEntry.h"

class DatCodeGenerator {
public:
    DatCodeGenerator(std::vector<unsigned char>& resultCode, std::vector<BinaryAnnotation>& resultAnnotation, AbstractBinaryGenerator* generator, const std::vector<DatCodeEntry>& code, const int objOffsetPtr, const bool finalRun):
        resultCode(resultCode),resultAnnotation(resultAnnotation),generator(generator),code(code),objOffsetPtr(objOffsetPtr),cogOrg(generator->currentDatCogOrg()),resultSize(0),finalRun(finalRun),orgXMode(false) {
        cogOrg=0;
    }
private:
    std::vector<unsigned char>& resultCode;
    std::vector<BinaryAnnotation>& resultAnnotation;
    std::vector<BinaryAnnotation::DatAnnotation::Entry> annotations; //even entries are raw bytes, odd entries are asm bytes
    AbstractBinaryGenerator* generator;
    const std::vector<DatCodeEntry>& code;
    const int objOffsetPtr;
    int &cogOrg;
    int resultSize;
    const bool finalRun;
    bool orgXMode;

    void append(int value, int size, bool isDataEntry) {
        const int byteCount = 1<<size;
        if (finalRun) {
            resultCode.push_back(value & 0x000000FF);
            if (size>0)
                resultCode.push_back((value >> 8) & 0x000000FF);
            if (size>1) {
                resultCode.push_back((value >> 16) & 0x000000FF);
                resultCode.push_back((value >> 24) & 0x000000FF);
            }
            auto tpe = isDataEntry ? BinaryAnnotation::DatAnnotation::Entry::Data : BinaryAnnotation::DatAnnotation::Entry::Instruction;
            if (annotations.empty() || annotations.back().type != tpe)
                annotations.push_back(BinaryAnnotation::DatAnnotation::Entry(tpe, byteCount));
            else
                annotations.back().value += byteCount;
        }
        resultSize += byteCount;
        if (!orgXMode)
            cogOrg += byteCount;
    }
    void appendAnnotation(BinaryAnnotation::DatAnnotation::Entry::Type type, int value) {
        if (finalRun)
            annotations.push_back(BinaryAnnotation::DatAnnotation::Entry(type, value));
    }
public:
    void generate() {
        for (auto e:code) {
            switch(e.type) {
                case DatCodeEntry::Align: {
                    //pad to match alignment
                    const int testValMask = (1 << e.sizeOrDatIdOrOpcode) - 1;
                    while ((resultSize & testValMask) != 0)
                        append(0,0,true);
                    break;
                }
                case DatCodeEntry::SetDatSymbol:
                    generator->setDatSymbol(DatSymbolId(e.sizeOrDatIdOrOpcode), objOffsetPtr+resultSize, cogOrg);
                    break;
                case DatCodeEntry::AsmInstruction: {
                    const int src = (finalRun && e.srcOrValue) ? e.srcOrValue->evaluate(generator) : 0;
                    const int dst = (finalRun && e.dstOrCount) ? e.dstOrCount->evaluate(generator) : 0;
                    if (src<0 || src>0x1FF)
                        throw CompilerError(ErrorType::srccex, e.srcOrValue ? e.srcOrValue->sourcePosition : e.sourcePosition);
                    if (dst<0 || dst>0x1FF)
                        throw CompilerError(ErrorType::drcex, e.dstOrCount ? e.dstOrCount->sourcePosition : e.sourcePosition);
                    append(e.sizeOrDatIdOrOpcode | src | (dst<<9),2,false);
                    break;
                }
                case DatCodeEntry::RawData: {
                    const int count = e.dstOrCount->evaluate(generator);
                    const int value = finalRun ? e.srcOrValue->evaluate(generator) : 0;
                    if (count>0) { //TODO error if < 0 ?
                        for (int i=0; i<count; ++i)
                            append(value, e.sizeOrDatIdOrOpcode,true);
                    }
                    break;
                }
                case DatCodeEntry::RawFixedByte:
                    append(e.sizeOrDatIdOrOpcode, 0,true);
                    break;
                case DatCodeEntry::DirectiveFit: {
                    const int count = e.dstOrCount->evaluate(generator);
                    const int fit = count<<2;
                    if ((unsigned int)(cogOrg) > (unsigned int)fit)
                        throw CompilerError(ErrorType::oefl, e.dstOrCount->sourcePosition);
                    break;
                }
                case DatCodeEntry::DirectiveRes: {
                    const int count = e.dstOrCount->evaluate(generator);
                    cogOrg += count << 2;
                    if (cogOrg > (0x1F0 * 4))
                        throw CompilerError(ErrorType::oexl, e.dstOrCount->sourcePosition);
                    appendAnnotation(BinaryAnnotation::DatAnnotation::Entry::CogOrg,cogOrg);
                    break;
                }
                case DatCodeEntry::DirectiveOrg: {
                    const int count = e.dstOrCount->evaluate(generator);
                    if (count>0x1F0)
                        throw CompilerError(ErrorType::oexl, e.dstOrCount->sourcePosition);
                    cogOrg = count << 2;
                    if (orgXMode)
                        appendAnnotation(BinaryAnnotation::DatAnnotation::Entry::CogOrgX,0);
                    orgXMode = false;
                    appendAnnotation(BinaryAnnotation::DatAnnotation::Entry::CogOrg,cogOrg);
                    break;
                }
                case DatCodeEntry::DirectiveOrgX: {
                    cogOrg = 0;
                    orgXMode = true;
                    appendAnnotation(BinaryAnnotation::DatAnnotation::Entry::CogOrgX,1);
                    appendAnnotation(BinaryAnnotation::DatAnnotation::Entry::CogOrg,cogOrg);
                    break;
                }
            }
        }
        if (finalRun)
            resultAnnotation.push_back(BinaryAnnotation(BinaryAnnotation::DatSection,resultSize,BinaryAnnotation::AbstractExtraInfoP(new BinaryAnnotation::DatAnnotation(annotations))));
    }
};

#endif //SPINCOMPILER_DATCODEGENERATOR_H

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
