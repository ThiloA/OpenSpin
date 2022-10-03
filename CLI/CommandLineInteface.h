//////////////////////////////////////////////////////////////
//                                                          //
// Propeller Spin/PASM Compiler                             //
// (c)2012-2016 Parallax Inc. DBA Parallax Semiconductor.   //
// Adapted from Chip Gracey's x86 asm code by Roy Eltham    //
// Rewritten to modern C++ by Thilo Ackermann               //
// See end of file for terms of use.                        //
//                                                          //
////////////////////////////////////////////////////////////// 

#ifndef COMMANDLINEINTERFACE_H
#define COMMANDLINEINTERFACE_H

#include <string>
#include <iostream>
#include <vector>
#include <map>
#include "SpinCompiler/Types/CompilerSettings.h"
#include "SpinCompiler/Generator/Compiler.h"
#include "SpinCompiler/Types/DefaultFileHandler.h"
#include "CLI/HtmlFiles.h"


struct CommandLineInterface {
    CommandLineInterface():m_quiet(false) {}
    static void banner() {
        std::cerr<<"Propeller Spin/PASM Compiler \'OpenSpin\' (c)2012-2018 Parallax Inc. DBA Parallax Semiconductor."<<std::endl;
        std::cerr<<"Adapted from Chip Gracey's x86 asm code by Roy Eltham"<<std::endl;
        std::cerr<<"Rewritten to modern C++ by Thilo Ackermann"<<std::endl;
        std::cerr<<"Version 1.01.0 Compiled on "<<__DATE__<<" "<<__TIME__<<std::endl;
    }

    /* Usage - display a usage message and exit */
    static void usage() {
        banner();
        std::cerr << "usage: openspin" <<std::endl;
        std::cerr << "    [ -h ]                                display this help"<<std::endl;
        std::cerr << "    [ -L or -I <path> ]                   add a directory to the include path"<<std::endl;
        std::cerr << "    [ -o <path> ]                         output filename"<<std::endl;
        std::cerr << "    [ -b ]                                output binary file format"<<std::endl;
        std::cerr << "    [ -e ]                                output eeprom file format"<<std::endl;
        std::cerr << "    [ -c ]                                output only DAT sections"<<std::endl;
        //std::cerr << "    [ -d ]                 dump out doc mode"<<std::endl;
        //std::cerr << "    [ -t ]                 output just the object file tree"<<std::endl;
        //std::cerr << "    [ -f ]                 output a list of filenames for use in archiving"<<std::endl;
        std::cerr << "    [ -q ]                                quiet mode (suppress banner and non-error text)"<<std::endl;
        //std::cerr << "    [ -v ]                 verbose output"<<std::endl;
        std::cerr << "    [ -p ]                                disable the preprocessor"<<std::endl;
        //std::cerr << "    [ -a ]                 use alternative preprocessor rules"<<std::endl;
        std::cerr << "    [ -D <define> ]                       add a define"<<std::endl;
        std::cerr << "    [ -M <size> ]                         size of eeprom (up to 16777216 bytes)"<<std::endl;
        //std::cerr << "    [ -s ]                 dump PUB & CON symbol information for top object"<<std::endl;
        std::cerr << "    [ -u ]                                enable unused method elimination"<<std::endl;
        std::cerr << "    [ --annotated-output <json|html> ]    generated annotated json or html output"<<std::endl;
        std::cerr << "    <name.spin>                           spin file to compile"<<std::endl;
        std::cerr<<std::endl;
    }

    static bool endsWith(const std::string& str, const std::string ending) {
        if (str.size()<ending.size())
            return false;
        return str.substr(str.size()-ending.size()) == ending;
    }

    std::string m_inputFileName;
    std::string m_outputFileName;
    std::vector<std::string> m_searchPath;
    CompilerSettings m_settings;
    bool m_quiet;

    std::string parseArguments(const std::vector<std::string>& arguments) {
        m_settings.preDefinedMacros["__SPIN__"]="1";
        m_settings.preDefinedMacros["__TARGET__"]="P1";
        //TODO parse all options
        for (unsigned i=0; i<arguments.size(); ++i) {
            const std::string& arg = arguments[i];
            if (arg.empty())
                continue;
            if (arg[0] != '-') {
                if (!m_inputFileName.empty())
                    return "multiple input filenames given";
                m_inputFileName = arg;
                if (!endsWith(m_inputFileName,".spin"))
                    return "input file name must end with .spin";
                m_inputFileName.erase(m_inputFileName.end()-5,m_inputFileName.end());
                continue;
            }
            const bool hasMoreArguments = i+1<arguments.size();
            if (arg == "-h") //help
                return "?";
            if (arg == "-L" || arg == "-I") {
                if (!hasMoreArguments)
                    return "expected include path";
                m_searchPath.push_back(arguments[++i]+"/");
            }
            else if (arg == "-o") {
                if (!m_outputFileName.empty())
                    return "multiple output filenames given";
                if (!hasMoreArguments)
                    return "expected output filename";
                m_outputFileName = arguments[++i];
                if (endsWith(m_outputFileName,".spin"))
                    return "output filename must not end with .spin";
            }
            else if (arg == "-D") {
                if (!hasMoreArguments)
                    return "expected define name";
                m_settings.preDefinedMacros[arguments[++i]] = "1";
            }
            else if (arg == "-M") {
                if (!hasMoreArguments)
                    return "eeprom size expected";
                auto numStr = arguments[++i];
                size_t pos = 0;
                try {
                    m_settings.eepromSize = std::stoi(numStr,&pos);
                }
                catch(...) {
                    return "invalid eeprom size";
                }
                if (pos != numStr.size() || m_settings.eepromSize<0)
                    return "invalid eeprom size";
            }
            else if (arg == "-c")
                m_settings.compileDatOnly = true;
            else if (arg == "-u")
                m_settings.unusedMethodOptimization = CompilerSettings::UnusedMethods::RemovePartial;
            else if (arg == "-b")
                m_settings.binaryMode = true;
            else if (arg == "-e")
                m_settings.binaryMode = false;
            else if (arg == "-p")
                m_settings.usePreProcessor = false;
            else if (arg == "-q")
                m_quiet = true;
            else if (arg == "--annotated-output") {
                if (!hasMoreArguments)
                    return "annotated output type expected";
                auto type = arguments[++i];
                if (type == "json")
                    m_settings.annotatedOutput = CompilerSettings::AnnotatedOutput::JSON;
                else if (type == "html")
                    m_settings.annotatedOutput = CompilerSettings::AnnotatedOutput::HTML;
                else
                    return "annotated output type must be json or html";
            }
            else
                return "unknown option '"+arg+"'";
        }
        if (m_inputFileName.empty())
            return "no input file given";
        if (m_outputFileName.empty()) {
            m_outputFileName = m_inputFileName;
            if (m_settings.annotatedOutput == CompilerSettings::AnnotatedOutput::JSON)
                m_outputFileName+=".json";
            else if (m_settings.annotatedOutput == CompilerSettings::AnnotatedOutput::HTML)
                m_outputFileName+=".html";
            else if (m_settings.compileDatOnly)
                m_outputFileName+=".dat";
            else if (m_settings.binaryMode)
                m_outputFileName+=".binary";
            else
                m_outputFileName+=".eeprom";
        }
        std::string rootDir = m_inputFileName;
        m_inputFileName += ".spin";
        while (!rootDir.empty() && rootDir.back() != '/' && rootDir.back() != '\\')
            rootDir.pop_back();
        m_searchPath.insert(m_searchPath.begin(), rootDir);
        return std::string();
    }

    bool run() {
        if (!m_quiet)
            banner();
        DefaultFileHandler fileHandler(m_searchPath);
        CompilerResult result;
        Compiler::runCompiler(result, &fileHandler, m_settings, m_inputFileName);
        for (auto e:result.messages.errors) {
            auto m = result.messages.messageByType(e.errType);
            if (e.sourcePosition.file.file)
                std::cerr<<"Error at "<<e.sourcePosition.file.file->fileName<<":"<<e.sourcePosition.line<<":"<<e.sourcePosition.column<<" ";
            else
                std::cerr<<"Error ";
            std::cerr<<"["<<m.typeName<<"] "<<m.message;
            if (!e.extraMessage.empty())
                std::cerr<<": "<<e.extraMessage;
            std::cerr<<std::endl;
        }
        if (result.messages.hasError()) {
            if (!m_quiet)
                std::cerr<<"Aborted"<<std::endl;
            return false;
        }
        if (m_settings.annotatedOutput == CompilerSettings::AnnotatedOutput::HTML) {
            if (!HtmlFiles::generate(result.binary)) {
                std::cerr<<"Unable to generate HTML annotated output, HTML support might be disabled in this compiler"<<std::endl;
                return false;
            }
        }
        std::ofstream outFile(m_outputFileName, std::ios::out | std::ios::binary);
        outFile.write(reinterpret_cast<const char*>(result.binary.data()), result.binary.size());
        outFile.close();
        if (!m_quiet)
            std::cerr<<"Done"<<std::endl;
        return false;
    }
};

#endif //COMMANDLINEINTERFACE_H

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
