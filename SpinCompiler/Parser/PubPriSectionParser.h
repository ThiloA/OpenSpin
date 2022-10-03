//////////////////////////////////////////////////////////////
//                                                          //
// Propeller Spin/PASM Compiler                             //
// (c)2012-2016 Parallax Inc. DBA Parallax Semiconductor.   //
// Adapted from Chip Gracey's x86 asm code by Roy Eltham    //
// Rewritten to modern C++ by Thilo Ackermann               //
// See end of file for terms of use.                        //
//                                                          //
////////////////////////////////////////////////////////////// 

#ifndef SPINCOMPILER_PUBPRISECTIONPARSER_H
#define SPINCOMPILER_PUBPRISECTIONPARSER_H

#include "SpinCompiler/Parser/InstructionBlockParser.h"
#include "SpinCompiler/Parser/ParsedObject.h"

class PubPriSectionParser {
public:
private:
    TokenReader &m_reader;
    ParserObjectContext &m_objectContext;
public:
    explicit PubPriSectionParser(TokenReader& reader, ParserObjectContext& objectContext):m_reader(reader),m_objectContext(objectContext) {}
    // This function parses all sub (PUB/PRI) names as symbol and skips the sub body
    void parseSubSymbols(bool forceAtLeastOnePubMethod) {
        parseSubSymbolsOfType(BlockType::BlockPUB);
        if (m_objectContext.currentObject->methods.empty() && forceAtLeastOnePubMethod)
            throw CompilerError(ErrorType::nprf,m_reader.getSourcePosition());
        parseSubSymbolsOfType(BlockType::BlockPRI);
    }

    // This function parses the actual sub (PUB/PRI) body
    void parseSubBlocks() {
        int subCount = 0;
        parseSubBlocksOfType(BlockType::BlockPUB, subCount);
        parseSubBlocksOfType(BlockType::BlockPRI, subCount);
    }
private:
    void skipParameters() {
        if (!m_reader.checkElement(Token::LeftBracket)) // '(' available?
            return; //no parameters given
        while (true) {
            m_reader.skipToken();
            if (!m_reader.getCommaOrRight()) // we got the ')' so fall out of counting parameters
                break;
        }
    }
    void skipResult() {
        if (!m_reader.checkElement(Token::Colon)) //result name specified
            return;
        m_reader.skipToken();
    }
    void skipLocals() {
        if (!m_reader.getPipeOrEnd())
            return;
        while (true) {
            m_reader.skipToken();
            // is it an array?
            if (m_reader.checkElement(Token::LeftIndex)) { // it is, so read the index (size of array)
                ConstantExpressionParser::tryResolveValueNonAsm(m_reader, m_objectContext.childObjectSymbols, true, true);
                m_reader.forceElement(Token::RightIndex); // get passed ]
            }
            if (!m_reader.getCommaOrEnd())
                break;
        }
    }

    void parseSub(const int subCount) {
        SymbolMap localSymbols;
        m_reader.setupLocalSymbolMap(&localSymbols);
        auto methodSymbolId = m_reader.getNextToken().symbolId; //method name

        if (subCount>=int(m_objectContext.currentObject->methods.size()))
            throw CompilerError(ErrorType::internal, m_reader.getSourcePosition());
        auto method = m_objectContext.currentObject->methods[subCount];
        if (method->symbolId != methodSymbolId)
            throw CompilerError(ErrorType::internal, m_reader.getSourcePosition());

        //append locals to local symbol table
        for (auto l:method->allLocals)
            if (l.symbolId.valid()) //do not add RESULT to table
                localSymbols.addSymbol(l.symbolId, SpinAbstractSymbolP(new SpinLocSymbol(l.localVarId)), 0);

        skipParameters();
        skipResult();
        skipLocals();
        method->functionBody = InstructionBlockParser::compileFunction(m_reader, m_objectContext); // instruction block compiler
        m_reader.setupLocalSymbolMap(nullptr); // cancel local symbols
    }

    void parseSubBlocksOfType(BlockType::Type blockType, int &subCount) {
        m_reader.reset();
        while (m_reader.getNextBlock(blockType))
            parseSub(subCount++);
    }

    void parseParameters(std::vector<ParsedObject::LocalVar>& result) {
        if (!m_reader.checkElement(Token::LeftBracket)) // '(' available?
            return; //no parameters given
        auto one = ConstantValueExpression::create(m_reader.getSourcePosition(),1);
        while (true) {
            auto sourcePosition = m_reader.getSourcePosition();
            auto symbolId = m_reader.forceUnusedSymbol(ErrorType::eaupn).symbolId;
            result.push_back(ParsedObject::LocalVar(sourcePosition, symbolId, LocSymbolId(result.size()), one));
            if (!m_reader.getCommaOrRight()) // we got the ')' so fall out of counting parameters
                break;
        }
    }
    void parseResult(ParsedObject::LocalVar& result) {
        if (!m_reader.checkElement(Token::Colon)) //result name specified
            return;
        // yes, so read the name
        auto tk = m_reader.getNextToken();
        if (tk.type != Token::Undefined && tk.type != Token::Result) // this allows for 'RESULT'
            throw CompilerError(ErrorType::eaurn, tk); // result name was not unique

        // if we result symbol then add it to temp symbols
        // we don't increment locals, because result local is already accounted for
        if (tk.type != Token::Result) {
            result.sourcePosition = tk.sourcePosition;
            result.symbolId = tk.symbolId;
        }
    }
    void parseLocals(std::vector<ParsedObject::LocalVar>& result) {
        if (!m_reader.getPipeOrEnd())
            return;
        // count locals (handling arrays)
        auto one = ConstantValueExpression::create(m_reader.getSourcePosition(),1);
        while (true) {
            auto sourcePosition = m_reader.getSourcePosition();
            auto symbolId = m_reader.forceUnusedSymbol(ErrorType::eauvn).symbolId;
            AbstractConstantExpressionP count;
            // is it an array?
            if (m_reader.checkElement(Token::LeftIndex)) { // it is, so read the index (size of array)
                count = ConstantExpressionParser::tryResolveValueNonAsm(m_reader, m_objectContext.childObjectSymbols, true, true).expression;
                m_reader.forceElement(Token::RightIndex); // get passed ]
            }
            else
                count = one;
            result.push_back(ParsedObject::LocalVar(sourcePosition, symbolId, LocSymbolId(result.size()), count));
            if (!m_reader.getCommaOrEnd())
                break;
        }
    }

    void parseSubSymbolsOfType(const BlockType::Type blockType) {
        m_reader.reset();
        while (m_reader.getNextBlock(blockType)) {
            auto sourcePosition = m_reader.getSourcePosition();
            const MethodId methodId = m_objectContext.currentObject->reserveNextMethodId();
            std::vector<ParsedObject::LocalVar> allLocals;
            //reserve on stack space for result value (always present)
            allLocals.push_back(ParsedObject::LocalVar(sourcePosition, SpinSymbolId(), LocSymbolId(0), ConstantValueExpression::create(sourcePosition,1)));
            auto symbolId = m_reader.forceUnusedSymbol(ErrorType::eausn).symbolId; //name of method
            parseParameters(allLocals);
            const int parameterCount = int(allLocals.size())-1; //1 result value subtracted to get parameter count
            parseResult(allLocals[0]);
            parseLocals(allLocals);


            m_objectContext.globalSymbols.addSymbol(symbolId, SpinAbstractSymbolP(new SpinSubSymbol(methodId, parameterCount, blockType == BlockType::BlockPUB)), 0);
            m_objectContext.currentObject->methods.push_back(ParsedObject::MethodP(new ParsedObject::Method(sourcePosition, symbolId, methodId, parameterCount, allLocals, blockType == BlockType::BlockPUB)));
        }
    }
};

#endif //SPINCOMPILER_PUBPRISECTIONPARSER_H

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
