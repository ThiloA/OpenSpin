//////////////////////////////////////////////////////////////
//                                                          //
// Propeller Spin/PASM Compiler                             //
// (c)2012-2016 Parallax Inc. DBA Parallax Semiconductor.   //
// Adapted from Chip Gracey's x86 asm code by Roy Eltham    //
// Rewritten to modern C++ by Thilo Ackermann               //
// See end of file for terms of use.                        //
//                                                          //
////////////////////////////////////////////////////////////// 

#ifndef SPINCOMPILER_COMPILER_H
#define SPINCOMPILER_COMPILER_H

#include "SpinCompiler/Types/AbstractFileHandler.h"
#include "SpinCompiler/Parser/Parser.h"
#include "SpinCompiler/Generator/BinaryGenerator.h"
#include "SpinCompiler/Generator/UnusedMethodElimination.h"
#include "SpinCompiler/Generator/FinalGenerator.h"
#include "SpinCompiler/Generator/AnnotationWriter.h"

struct CompilerResult {
    std::vector<unsigned char> binary;
    CompilerMessages messages;
    std::vector<BinaryAnnotation> annotation;
};

struct Compiler {
    static void runCompiler(CompilerResult &result, AbstractFileHandler *fileHandler, const CompilerSettings& settings, const std::string& rootFileName) noexcept {
        try {
            auto parser = new Parser(fileHandler, settings);
            auto rootObj = parser->compileObject(fileHandler->findFile(rootFileName,AbstractFileHandler::RootSpinFile,FileDescriptorP(),SourcePosition()), nullptr, SourcePosition());
            if (settings.unusedMethodOptimization != CompilerSettings::UnusedMethods::Keep)
                UnusedMethodElimination::eliminateUnused(rootObj, settings.unusedMethodOptimization == CompilerSettings::UnusedMethods::RemovePartial);
            GeneratorGlobalState globalGeneratorState(settings);
            BinaryObjectGenerator binGen(globalGeneratorState, parser->stringMap, rootObj);
            auto bin = binGen.run(false);
            globalGeneratorState.generatedObjects[rootObj.get()] = bin;
            std::vector<unsigned char> tmpRes;
            std::map<BinaryObject*,int> alreadyGenerated;

            bin->distilledToBinary(tmpRes,result.annotation,globalGeneratorState.settings);

            if (settings.annotatedOutput != CompilerSettings::AnnotatedOutput::None) {
                AnnotationWriter awr(tmpRes,result.annotation);
                awr.generateJSON();
                result.binary = awr.result;
                return;
            }

            FinalGenerator finGen(settings,rootObj, bin, parser->stringMap);
            FinalGenerator::TopLevelConstants tlc;
            if (!settings.compileDatOnly)
                tlc = finGen.determineTopLevelConstants();

            // Check to make sure object fits into 32k (or eeprom size if specified as larger than 32k)
            int i = 0x10 + int(tmpRes.size()) + bin->totalVarSize() + (tlc.stackRequirement << 2);
            if (settings.defaultCompileMode && (i > settings.eepromSize))
                throw CompilerError(ErrorType::sztl,SourcePosition(),std::to_string(i - settings.eepromSize)+" bytes");

            result.binary = settings.compileDatOnly ? finGen.composeRAMDatOnly(tmpRes) : finGen.composeRAM(tmpRes, tlc);
        }
        catch(CompilerError& e) {
            result.messages.addError(e);
        }
    }
};

#endif //SPINCOMPILER_COMPILER_H

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
