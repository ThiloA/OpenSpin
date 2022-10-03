//////////////////////////////////////////////////////////////
//                                                          //
// Propeller Spin/PASM Compiler                             //
// (c)2012-2016 Parallax Inc. DBA Parallax Semiconductor.   //
// Adapted from Chip Gracey's x86 asm code by Roy Eltham    //
// Rewritten to modern C++ by Thilo Ackermann               //
// See end of file for terms of use.                        //
//                                                          //
////////////////////////////////////////////////////////////// 

#ifndef TESTCASE_H
#define TESTCASE_H

#include "SpinCompiler/Generator/Compiler.h"
#include "SpinCompiler/Types/DefaultFileHandler.h"
#include <fstream>
#include <iostream>

struct TestCase {
    static std::string dbgHexNumber(int n) {
        const char *hexdigits = "0123456789ABCDEF";
        std::string result="    ";
        result[0] = hexdigits[(n>>12)&0xF];
        result[1] = hexdigits[(n>>8)&0xF];
        result[2] = hexdigits[(n>>4)&0xF];
        result[3] = hexdigits[(n)&0xF];
        return result;
    }

    static bool isSame(const std::vector<unsigned char>& resData, const std::vector<unsigned char>& compareData) {
        bool allOk=true;
        int cmpLength = int(resData.size());
        if (resData.size() != compareData.size()) {
            std::cerr<<"Binary size mismatch got "<<int(resData.size())<<" expected "<<compareData.size()<<std::endl;
            allOk=false;
            cmpLength = std::min(resData.size(),compareData.size());
            //return false;
        }
        for (int i=0; i<cmpLength; ++i)
            if (resData[i] != (compareData[i] & 0xFF)) {
                if (i<16)
                    std::cerr<<"compare file mismatch at (header dez) "<<i<<" "<<(int)resData[i]<<" "<<(int)(compareData[i] & 0xFF)<<std::endl;
                else
                    std::cerr<<"compare file mismatch at hex "<<dbgHexNumber(i-16)<<" "<<dbgHexNumber(resData[i]&0xFF)<<" "<<dbgHexNumber(compareData[i] & 0xFF)<<std::endl;
                allOk=false;
            }
        return allOk;
    }

    static bool runTestCompileAndCompare(std::vector<std::string> args) {
        if (args.size()!= 3) {
            std::cerr<<"invalid args"<<std::endl;
            return false;
        }
        std::string basePath(args[0]);
        std::string fname(args[1]);
        std::string mode(args[2]);
        std::vector<std::string> libraries;
        libraries.push_back(basePath+"/liba/");
        libraries.push_back(basePath+"/libb/");

        std::string inFileName = basePath+"/"+fname+".spin";
        std::string compareFileName = basePath+"/"+fname+".bin";

        CompilerSettings settings;
        settings.preDefinedMacros["__SPIN__"]="1";
        settings.preDefinedMacros["__TARGET__"]="P1";
        settings.compileDatOnly = false;
        if (mode == "um") {
            //compilerConfig.bUnusedMethodElimination = true;
            settings.unusedMethodOptimization = CompilerSettings::UnusedMethods::RemovePartial;
            compareFileName += "um";
        }
        else if (mode == "dat") {
            settings.compileDatOnly = true;
            compareFileName += "dat";
        }
        else if (mode != "def") {
            std::cerr<<"illegal mode"<<std::endl;
            return false;
        }

        std::ifstream fcmp(compareFileName, std::ios_base::in | std::ios_base::binary);
        if (!fcmp) {
            std::cerr<<"unable to open compare file "<<compareFileName<<std::endl;
            return false;
        }

        fcmp.seekg(0,std::ios::end);
        std::vector<unsigned char> compareData(fcmp.tellg());
        fcmp.seekg(0,std::ios::beg);
        fcmp.read(reinterpret_cast<char*>(compareData.data()),compareData.size());
        fcmp.close();
        std::string inFileNamePath = inFileName;
        while (!inFileNamePath.empty() && inFileNamePath.back() != '\\' && inFileNamePath.back() != '/')
            inFileNamePath.pop_back();

        std::vector<std::string> searchPath;
        if (!inFileNamePath.empty())
            searchPath.push_back(inFileNamePath);
        for (unsigned int i=0; i<libraries.size(); ++i)
            searchPath.push_back(libraries[i]);

        DefaultFileHandler fileHandler(searchPath);
        CompilerResult result;
        Compiler::runCompiler(result,&fileHandler,settings,inFileName);
        for (auto e:result.messages.errors) {
            auto m = result.messages.messageByType(e.errType);
            if (e.sourcePosition.file.file)
                std::cerr<<"Error at "<<e.sourcePosition.file.file->fileName<<":"<<e.sourcePosition.line<<":"<<e.sourcePosition.column<<" ";
            else
                std::cout<<"Error ";
            std::cerr<<"["<<m.typeName<<"] "<<m.message<<std::endl;
        }
        if (result.messages.hasError())
            return false;
        try {
            if (!isSame(result.binary,compareData)) {
                std::cerr<<"compare failed"<<inFileName<<std::endl;

                result.binary.erase(result.binary.begin(),result.binary.begin()+16);
                AnnotationWriter awr(result.binary,result.annotation);
                awr.generateJSON();

                std::ofstream errFile(compareFileName+".errjson", std::ios::out | std::ios::binary);
                errFile.write(reinterpret_cast<const char*>(awr.result.data()), awr.result.size());
                errFile.close();
            }
        }
        catch(CompilerError&) {
            std::cerr<<"errors at"<<inFileName<<std::endl;
            // compiler put out an error
            return false;
        }
        return true;
    }
};

#endif //TESTCASE_H

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
