//////////////////////////////////////////////////////////////
//                                                          //
// Propeller Spin/PASM Compiler                             //
// (c)2012-2016 Parallax Inc. DBA Parallax Semiconductor.   //
// Adapted from Chip Gracey's x86 asm code by Roy Eltham    //
// Rewritten to modern C++ by Thilo Ackermann               //
// See end of file for terms of use.                        //
//                                                          //
////////////////////////////////////////////////////////////// 

#ifndef SPINCOMPILER_OBJSECTIONPARSER_H
#define SPINCOMPILER_OBJSECTIONPARSER_H

#include "SpinCompiler/Parser/AbstractParser.h"
#include "SpinCompiler/Parser/ParserObjectContext.h"
#include "SpinCompiler/Parser/ParsedObject.h"
#include "SpinCompiler/Parser/ConstantExpressionParser.h"
#include <algorithm>

class ObjSectionParser {
public:
    explicit ObjSectionParser(TokenReader& reader, ParserObjectContext& objectContext):m_reader(reader),m_objectContext(objectContext) {}
private:
    TokenReader &m_reader;
    ParserObjectContext &m_objectContext;
public:
    void parseChildObjectNames(const ObjectHierarchy &hierarchy) {
        m_reader.reset();
        while (m_reader.getNextBlock(BlockType::BlockOBJ)) {
            while (true) {
                auto tk = m_reader.getNextNonBlockOrNewlineToken();
                if (tk.eof)
                    break;
                if (tk.type != Token::Undefined)
                    throw CompilerError(ErrorType::eauon, tk);
                parseSingleChildObjectName(tk.sourcePosition, tk.symbolId, hierarchy);
            }
        }
    }

    void loadChildObjects() {
        std::vector<ObjectClassId> doneObjectClasses;
        for (auto childObj:m_objectContext.currentObject->childObjects) {
            if (std::find(doneObjectClasses.begin(),doneObjectClasses.end(),childObj.objectClass) != doneObjectClasses.end())
                continue;
            doneObjectClasses.push_back(childObj.objectClass);
            if (childObj.isUsed) {
                for (auto m:childObj.object->methods)
                    if (m->isPublic)
                        m_objectContext.childObjectSymbols.addSymbol(m->symbolId, SpinAbstractSymbolP(new SpinSubSymbol(m->methodId, m->parameterCount, true)), childObj.objectClass.value());
            }
            for (int j=0; j<int(childObj.object->constants.size()); ++j) {
                const auto c = childObj.object->constants[j];
                AbstractConstantExpressionP expr = c.constantExpression->isConstant(nullptr) ? c.constantExpression : AbstractConstantExpressionP(new ChildObjConstantExpression(c.sourcePosition, childObj.objectClass, j));
                m_objectContext.childObjectSymbols.addSymbol(c.symbolId, SpinAbstractSymbolP(new SpinConstantSymbol(expr, !c.isFloat)), childObj.objectClass.value());
            }
        }
    }
private:
    void parseSingleChildObjectName(const SourcePosition& sourcePosition, SpinSymbolId symbolId, const ObjectHierarchy &hierarchy) {
        AbstractConstantExpressionP instanceCount;
        if (m_reader.checkElement(Token::LeftIndex)) { // see if there is a count
            instanceCount = ConstantExpressionParser::tryResolveValueNonAsm(m_reader, m_objectContext.childObjectSymbols, true, true).expression;
            m_reader.forceElement(Token::RightIndex); // get the closing bracket
        }
        else
            instanceCount = ConstantValueExpression::create(sourcePosition,1); //if no [] is given, 1 instance per default

        // must have the colon
        m_reader.forceElement(Token::Colon);

        // now get the filename
        auto fileName = m_reader.readFileNameString();
        auto file = m_objectContext.parser->fileHandler->findFile(fileName, AbstractFileHandler::SpinFile, FileDescriptorP(), sourcePosition);
        auto childObj = m_objectContext.parser->compileObject(file, &hierarchy, sourcePosition);
        // enter obj symbol
        auto objectInstanceId = m_objectContext.currentObject->reserveNextObjectInstanceId();
        const auto objectClass = m_objectContext.currentObject->reserveOrGetObjectClassId(childObj);
        m_objectContext.globalSymbols.addSymbol(symbolId, SpinAbstractSymbolP(new SpinObjSymbol(objectClass,objectInstanceId)), 0);
        m_objectContext.currentObject->childObjects.push_back(ParsedObject::ChildObject(sourcePosition, symbolId, instanceCount, objectInstanceId, objectClass, childObj, true));

        m_reader.forceElement(Token::End);
    }
};

#endif //SPINCOMPILER_OBJSECTIONPARSER_H

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
