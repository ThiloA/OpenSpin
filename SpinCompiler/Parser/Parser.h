//////////////////////////////////////////////////////////////
//                                                          //
// Propeller Spin/PASM Compiler                             //
// (c)2012-2016 Parallax Inc. DBA Parallax Semiconductor.   //
// Adapted from Chip Gracey's x86 asm code by Roy Eltham    //
// Rewritten to modern C++ by Thilo Ackermann               //
// See end of file for terms of use.                        //
//                                                          //
////////////////////////////////////////////////////////////// 

#ifndef SPINCOMPILER_PARSER_H
#define SPINCOMPILER_PARSER_H

#include "SpinCompiler/Parser/AbstractParser.h"
#include "SpinCompiler/Parser/ConSectionParser.h"
#include "SpinCompiler/Parser/VarSectionParser.h"
#include "SpinCompiler/Parser/DatSectionParser.h"
#include "SpinCompiler/Parser/ObjSectionParser.h"
#include "SpinCompiler/Parser/PubPriSectionParser.h"
#include "SpinCompiler/Tokenizer/CharsetConverter.h"
#include "SpinCompiler/Tokenizer/Tokenizer.h"
#include "SpinCompiler/Tokenizer/MacroPreProcessor.h"
#include "SpinCompiler/Types/CompilerSettings.h"

class Parser : public AbstractParser {
public:
    explicit Parser(AbstractFileHandler *fileHandler, const CompilerSettings& settings):AbstractParser(fileHandler),m_settings(settings) {}
    virtual ~Parser() {}
    virtual ParsedObjectP compileObject(FileDescriptorP file, const ObjectHierarchy *hierarchy, const SourcePosition& includePos) {
        auto found = m_objectMap.find(file.get());
        if (found != m_objectMap.end()) {
            if (hierarchy && hierarchy->hasParent(found->second))
                throw CompilerError(ErrorType::circ,hierarchy->position);
            return found->second;
        }

        ParsedObjectP newObj(new ParsedObject(file->baseName()));
        ObjectHierarchy childHierarchy(newObj, hierarchy, includePos);
        m_objectMap[file.get()] = newObj;
        compile(file, childHierarchy);
        return newObj;
    }
    std::vector<ParsedObjectP> listAllObjects(ParsedObjectP root) const {
        std::vector<ParsedObjectP> result;
        result.reserve(m_objectMap.size());
        result.push_back(root);
        for (auto it:m_objectMap)
            if (it.second != root)
                result.push_back(it.second);
        return result;
    }
private:
    std::map<FileDescriptor*, ParsedObjectP> m_objectMap;
    const CompilerSettings &m_settings;


    void compile(FileDescriptorP file, const ObjectHierarchy &hierarchy) {
        std::string preProcessorIn;
        CharsetConverter charsetConverter(file->content,preProcessorIn);
        charsetConverter.convert();
        std::map<std::string,std::string> macros = m_settings.preDefinedMacros;
        std::string sourceCode;
        SourcePositionFile srcPosFile(file, nullptr);
        if (m_settings.usePreProcessor) {
            MacroPreProcessor preProcessor(preProcessorIn,sourceCode,macros,srcPosFile);
            preProcessor.runFile();
        }
        else
            sourceCode = preProcessorIn;

        ParserObjectContext objContext(this,hierarchy.obj);
        auto tokenList = Tokenizer::readTokenList(builtInSymbols, sourceCode,srcPosFile);
        TokenReader reader(tokenList,objContext.globalSymbols);
        compileStep1(reader,objContext,hierarchy);
        compileStep2(reader,objContext);
    }

    void compileStep1(TokenReader& reader, ParserObjectContext& objContext, const ObjectHierarchy &hierarchy) {
        objContext.currentObject->clear();
        objContext.globalSymbols = SymbolMap(); //delete all Symbols
        ConSectionParser(reader, objContext).parseDevBlocks();
        ConSectionParser(reader, objContext).parseConBlocks(0);
        PubPriSectionParser(reader, objContext).parseSubSymbols(m_settings.defaultCompileMode);
        ObjSectionParser(reader, objContext).parseChildObjectNames(hierarchy);
    }

    void compileStep2(class TokenReader &reader, ParserObjectContext& objContext) {
        ObjSectionParser(reader, objContext).loadChildObjects();
        ConSectionParser(reader, objContext).parseConBlocks(1);
        VarSectionParser(reader, objContext).parseVarBlocks();
        DatSectionParser(reader, objContext).parseDatBlocks();
        if (!m_settings.compileDatOnly)
            PubPriSectionParser(reader, objContext).parseSubBlocks();
    }
};

#endif //SPINCOMPILER_PARSER_H

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
