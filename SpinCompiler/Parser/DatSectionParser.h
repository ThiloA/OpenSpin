//////////////////////////////////////////////////////////////
//                                                          //
// Propeller Spin/PASM Compiler                             //
// (c)2012-2016 Parallax Inc. DBA Parallax Semiconductor.   //
// Adapted from Chip Gracey's x86 asm code by Roy Eltham    //
// Rewritten to modern C++ by Thilo Ackermann               //
// See end of file for terms of use.                        //
//                                                          //
////////////////////////////////////////////////////////////// 

#ifndef SPINCOMPILER_DATSECTIONPARSER_H
#define SPINCOMPILER_DATSECTIONPARSER_H

#include "SpinCompiler/Parser/ConstantExpressionParser.h"
#include "SpinCompiler/Types/DatCodeEntry.h"
#include "SpinCompiler/Parser/ParserObjectContext.h"
#include "SpinCompiler/Parser/ParsedObject.h"
#include "SpinCompiler/Parser/AbstractParser.h"

class DatSectionParser {
public:
    DatSectionParser(TokenReader& reader, ParserObjectContext& objectContext):m_reader(reader),m_objectContext(objectContext),m_pass(0),m_orgXMode(false) {}
    void parseDatBlocks() {
        for (m_pass = 0; m_pass < 2; m_pass++) {
            m_objectContext.currentObject->clearDatSection();
            m_datSectionContext.asmLocal = 0;
            m_reader.reset();
            int size = 0;
            while(m_reader.getNextBlock(BlockType::BlockDAT))
                parseSingleDatBlock(size);
        }
    }
private:
    TokenReader &m_reader;
    ParserObjectContext &m_objectContext;
    ConstantExpressionParser::DatSectionContext m_datSectionContext;
    int m_pass;
    bool m_orgXMode;

    struct CurrentSymbolInfo {
        explicit CurrentSymbolInfo(int& size):size(size),bResSymbol(false),bLocal(false) {}
        SpinSymbolId symbolId;
        int &size;
        bool bResSymbol;
        bool bLocal;
    };

    AbstractConstantExpressionP tryResolveValue(bool mustResolve, bool isInteger) {
        return ConstantExpressionParser::tryResolveValueAsm(m_reader,m_objectContext.childObjectSymbols,mustResolve,isInteger,&m_datSectionContext);
    }

    AbstractConstantExpressionP tryResolveInteger(bool mustResolve) {
        return tryResolveValue(mustResolve, true);
    }

    AbstractConstantExpressionP resolveInteger() {
        return tryResolveValue(true, true);
    }

    void appendSymbolIfGiven(const SourcePosition& sourcePosition, const CurrentSymbolInfo& symbol, bool align) {
        if (align)
            m_objectContext.currentObject->datCode.push_back(DatCodeEntry(sourcePosition, DatCodeEntry::Align,symbol.size,AbstractConstantExpressionP(),AbstractConstantExpressionP()));
        if (!symbol.symbolId.valid())
            return;

        auto& symbolTable = symbol.bLocal ? m_datSectionContext.localSymbols : m_objectContext.globalSymbols;
        const int symbolClass = symbol.bLocal ? m_datSectionContext.asmLocal : 0;

        DatSymbolId datSymbolId;
        if (m_pass == 0) {
            datSymbolId = m_objectContext.currentObject->reserveNextDatSymbolId();
            SpinAbstractSymbolP s(new SpinDatSectionSymbol(datSymbolId, symbol.size, symbol.bResSymbol));
            symbolTable.addSymbol(symbol.symbolId, s, symbolClass);
        }
        else {
            auto sym = symbolTable.hasSpecificSymbol<SpinDatSectionSymbol>(symbol.symbolId, symbolClass);
            if (!sym || sym->isRes != symbol.bResSymbol || sym->size != symbol.size)
                throw CompilerError(ErrorType::internal, sourcePosition);
            datSymbolId = sym->id;
        }
        m_objectContext.currentObject->datCode.push_back(DatCodeEntry(sourcePosition, datSymbolId));
    }

    void appendAsmInstruction(const SourcePosition& sourcePosition, int opcode, AbstractConstantExpressionP src, AbstractConstantExpressionP dst) {
        m_objectContext.currentObject->datCode.push_back(DatCodeEntry(sourcePosition, DatCodeEntry::AsmInstruction, opcode, src, dst));
    }

    void appendData(const SourcePosition& sourcePosition, AbstractConstantExpressionP value, AbstractConstantExpressionP count, int size) {
        m_objectContext.currentObject->datCode.push_back(DatCodeEntry(sourcePosition, DatCodeEntry::RawData, size, value, count));
    }

    void appendFixedByte(const SourcePosition& sourcePosition, int value) {
        m_objectContext.currentObject->datCode.push_back(DatCodeEntry(sourcePosition, DatCodeEntry::RawFixedByte, value, AbstractConstantExpressionP(), AbstractConstantExpressionP()));
    }

    void appendDirective(const SourcePosition& sourcePosition, DatCodeEntry::Type type, AbstractConstantExpressionP expr) {
        m_objectContext.currentObject->datCode.push_back(DatCodeEntry(sourcePosition, type, 0, AbstractConstantExpressionP(), expr));
    }

    void parseData(const SourcePosition& sourcePosition, CurrentSymbolInfo& symbol, const int tokenValue) {
        symbol.size = tokenValue & 0x000000FF;
        int overrideSize = symbol.size;

        appendSymbolIfGiven(sourcePosition, symbol, true);

        auto tk = m_reader.getNextToken();
        if (tk.type == Token::End)
            return;

        while (!tk.eof) {
            // do we have a size override?
            if (tk.type == Token::Size) { // yes, get it
                overrideSize = tk.value & 0x000000FF;
                if (overrideSize < symbol.size)
                    throw CompilerError(ErrorType::sombl, tk);
            }
            else // no, backup
                m_reader.goBack();
            // get the value
            const AbstractConstantExpressionP value = tryResolveValue(m_pass == 1, overrideSize != 2);
            // get the count
            AbstractConstantExpressionP count;
            if (m_reader.checkElement(Token::LeftIndex)) {
                count = resolveInteger();
                m_reader.forceElement(Token::RightIndex);
            }
            else
                count = ConstantValueExpression::create(tk.sourcePosition, 1);

            // enter the value count times into the obj
            appendData(sourcePosition, value, count, overrideSize);
            if (!m_reader.getCommaOrEnd())
                break;
            tk = m_reader.getNextToken();
        }
    }

    void parseDatFile(const SourcePosition& sourcePosition, CurrentSymbolInfo& symbol) {
        symbol.size = 0; // force size to byte
        appendSymbolIfGiven(sourcePosition, symbol, false);
        std::string fileName = m_reader.readFileNameString();
        auto f = m_objectContext.parser->fileHandler->findFile(fileName, AbstractFileHandler::BinaryDatFile, FileDescriptorP(), sourcePosition);
        for (unsigned char c:f->content)
            appendFixedByte(sourcePosition, c);
        m_reader.forceElement(Token::End);
    }

    void parseAsmDirective(const SourcePosition& sourcePosition, CurrentSymbolInfo& symbol, int tokenValue) {
        symbol.size = 2; // force to long size

        const int directive = tokenValue & 0x000000FF;
        switch (directive) {
            case AsmDirective::DirNop: {
                appendSymbolIfGiven(sourcePosition, symbol, true);
                m_reader.forceElement(Token::End);
                appendAsmInstruction(sourcePosition, 0, AbstractConstantExpressionP(), AbstractConstantExpressionP());
                return;
            }
            case AsmDirective::DirFit: {
                appendSymbolIfGiven(sourcePosition, symbol, true);
                AbstractConstantExpressionP fit;
                if (!m_reader.checkElement(Token::End)) {
                    fit = resolveInteger();
                    m_reader.forceElement(Token::End);
                }
                else
                    fit = ConstantValueExpression::create(sourcePosition, 0x1F0);
                appendDirective(sourcePosition, DatCodeEntry::DirectiveFit, fit);
                return;
            }
            case AsmDirective::DirRes: {
                if (m_orgXMode)
                    throw CompilerError(ErrorType::rinaiom, sourcePosition);
                symbol.bResSymbol = true;
                appendSymbolIfGiven(sourcePosition, symbol, true);
                AbstractConstantExpressionP resSize;
                if (!m_reader.checkElement(Token::End)) {
                    resSize = resolveInteger();
                    m_reader.forceElement(Token::End);
                }
                else
                    resSize = ConstantValueExpression::create(sourcePosition, 1);
                appendDirective(sourcePosition, DatCodeEntry::DirectiveRes, resSize);
                return;
            }
            case AsmDirective::DirOrg: {
                appendSymbolIfGiven(sourcePosition, symbol, true);
                AbstractConstantExpressionP newOrg;
                if (!m_reader.checkElement(Token::End)) {
                    newOrg = tryResolveInteger(true);
                    m_reader.forceElement(Token::End);
                }
                else
                    newOrg = ConstantValueExpression::create(sourcePosition, 0);
                appendDirective(sourcePosition, DatCodeEntry::DirectiveOrg, newOrg);
                return;
            }
        }

        //orgx, was ist das?
        appendSymbolIfGiven(sourcePosition, symbol, true);
        m_reader.forceElement(Token::End);
        appendDirective(sourcePosition, DatCodeEntry::DirectiveOrgX,AbstractConstantExpressionP());
        m_orgXMode = true;
        return;
    }

    AbstractConstantExpressionP validateCallSymbol(const SourcePosition& sourcePosition, bool bIsRet, SpinSymbolId symbolId) {
        auto datSymbol = m_objectContext.globalSymbols.hasSpecificSymbol<SpinDatSectionSymbol>(symbolId, 0);
        if (!datSymbol)
            throw CompilerError(ErrorType::eads, sourcePosition);
        return AbstractConstantExpressionP(new DatSymbolConstantExpression(sourcePosition, datSymbol->id, true));
    }

    void parseAsmInstruction(const SourcePosition& sourcePosition, CurrentSymbolInfo& symbol, unsigned char condition, const Token& tk0) {
        symbol.size = 2; // force to long size
        appendSymbolIfGiven(sourcePosition, symbol, true);

        int opcode = tk0.value & 0x000000FF;
        // handle dual type entries and also AND and OR (which are the only Token::type_binary that will get here)
        if (tk0.dual || tk0.type == Token::Binary)
            opcode = tk0.asmOp & 0x000000FF;

        unsigned int instruction = opcode << 8;
        if (opcode & 0x80) // sys instruction
            instruction = 0x03 << 8;
        instruction |= condition;
        if (opcode & 0x40) // set WR?
            instruction |= 0x20;
        instruction <<= 18;  // justify the instruction (s & d will go in lower 18 bits)
        AbstractConstantExpressionP srcExpression;
        AbstractConstantExpressionP dstExpression;

        if (opcode & 0x80) { // sys instruction
            instruction |= 0x00400000; // set immediate
            instruction |= (opcode & 0x07); // set s
            dstExpression = tryResolveInteger(m_pass == 1);
        }
        else if (opcode == 0x15) { // call?
            // make 'jmpret label_ret, #label'
            instruction ^= 0x08C00000;
            m_reader.forceElement(Token::Pound);
            Token tk1 = m_reader.getNextToken();
            //if (m_reader.validateAndGetSymbolLength(tk1) <= 0)
            if (!tk1.isValidWordSymbol())
                throw CompilerError(ErrorType::eads, tk1);
            auto symbolId = tk1.symbolId;
            if (m_pass != 0)
                srcExpression = validateCallSymbol(tk1.sourcePosition, false, symbolId); // set #label

            auto retSymbolName = m_objectContext.parser->stringMap.getNameBySymbolId(symbolId)+"_RET";
            auto retSymbolId = m_objectContext.parser->stringMap.getOrPutSymbolName(retSymbolName);
            if (m_pass != 0)
                dstExpression = validateCallSymbol(tk1.sourcePosition, true, retSymbolId); // set label_ret
        }
        else if (opcode == 0x16) // ret?
            instruction ^= 0x04400000; // make 'jmp #0'
        else if (opcode == 0x17) { // jmp?
            // for jmp, we only get s, there is no d
            // see if it's an immediate value for s
            auto tk1 = m_reader.getNextToken();
            if (tk1.type == Token::Pound)
                instruction |= 0x00400000;
            else
                m_reader.goBack();
            srcExpression = tryResolveInteger(m_pass == 1); // set s on instruction
        }
        else { // regular instruction get both d and s
            dstExpression = tryResolveInteger(m_pass == 1); // set d on instruction
            m_reader.forceElement(Token::Comma);
            // see if it's an immediate value for s
            if (m_reader.checkElement(Token::Pound))
                instruction |= 0x00400000;
            srcExpression = tryResolveInteger(m_pass == 1); // set s on instruction
        }

        // check for effects
        bool bAfterComma = false;
        while (true) {
            auto tk2 = m_reader.getNextToken();
            if (tk2.eof)
                break;
            if (tk2.type == Token::AsmEffect) {
                const int effectValue = tk2.value;

                // don't allow wr/nr for r/w instructions
                if ((effectValue & 0x09) && (instruction >> 26) <= 2)
                    throw CompilerError(ErrorType::micuwn, tk2);

                // apply effect to instruction
                int temp = (effectValue & 0x38) << 20;
                instruction |= temp;
                instruction ^= temp;
                instruction |= ((effectValue & 0x07) << 23);


                if (!m_reader.getCommaOrEnd()) // got end, done with effects
                    break;

                // got a comma, expecting another effect
                bAfterComma = true;
            }
            else if (bAfterComma) // expected another effect after the comma
                throw CompilerError(ErrorType::eaasme, tk2);
            else if (tk2.type != Token::End) // if it wasn't an effect the first time in then it should be an end
                throw CompilerError(ErrorType::eaaeoeol, tk2);
            else // we get here if we got no effect and got the proper end
                break;
        }
        // enter instruction as 1 long
        appendAsmInstruction(sourcePosition, instruction, srcExpression, dstExpression);
    }

    bool checkInstruction(const Token& tk) {
        if (tk.type == Token::AsmInst || tk.dual)
            return true;
        return tk.type == Token::Binary && (tk.opType == OperatorType::OpLogAnd || tk.opType == OperatorType::OpLogOr);
    }

    void parseAsmCondition(const SourcePosition& sourcePosition, CurrentSymbolInfo& symbol, const int tokenValue) {
        auto tk = m_reader.getNextToken();
        if (!checkInstruction(tk))
            throw CompilerError(ErrorType::eaasmi, tk);
        parseAsmInstruction(sourcePosition, symbol, (unsigned char)(tokenValue & 0x000000FF),tk);
    }

    void parseSingleDatBlock(int &symSize) {
        while (true) {
            auto tk = m_reader.getNextNonBlockOrNewlineToken();
            if (tk.eof)
                break;
            // clear symbol flags
            CurrentSymbolInfo symbol(symSize);
            symbol.bLocal = (tk.type == Token::Colon); //local symbol?
            if (symbol.bLocal) {
                tk = m_reader.getNextToken();
                if (!tk.isValidWordSymbol())
                    throw CompilerError(ErrorType::eals, tk);
                symbol.symbolId = tk.symbolId;
                tk.resolvedSymbol = m_datSectionContext.localSymbols.hasSpecificSymbol<SpinDatSectionSymbol>(tk.symbolId, m_datSectionContext.asmLocal);
                tk.type = tk.resolvedSymbol ? Token::DefinedSymbol : Token::Undefined;
            }

            if (tk.type == Token::Undefined) { // undefined here means it's a symbol
                if (!symbol.bLocal) {
                    symbol.symbolId = tk.symbolId;
                    m_datSectionContext.asmLocal++;
                }
                tk = m_reader.getNextToken();
                if (tk.type == Token::End) {
                    appendSymbolIfGiven(tk.sourcePosition, symbol, false);
                    continue;
                }
            }
            else if (std::dynamic_pointer_cast<SpinDatSectionSymbol>(tk.resolvedSymbol)) {
                symbol.symbolId = tk.symbolId;
                if (!symbol.bLocal)
                    m_datSectionContext.asmLocal++;
                if (m_pass == 0)
                    throw CompilerError(ErrorType::siad, tk);
                tk = m_reader.getNextToken();
                if (tk.type == Token::End) {
                    appendSymbolIfGiven(tk.sourcePosition, symbol, false);
                    continue;
                }
            }

            if (tk.type == Token::Size)
                parseData(tk.sourcePosition, symbol, tk.value);
            else if (tk.type == Token::File)
                parseDatFile(tk.sourcePosition, symbol);
            else if (tk.type == Token::AsmDir)
                parseAsmDirective(tk.sourcePosition, symbol, tk.value);
            else if (tk.type == Token::AsmCond)
                parseAsmCondition(tk.sourcePosition, symbol, tk.value);
            else if (checkInstruction(tk))
                parseAsmInstruction(tk.sourcePosition, symbol, AsmConditionType::IfAlways, tk);
            else
                throw CompilerError(ErrorType::eaunbwlo, tk);
        }
    }
};

#endif //SPINCOMPILER_DATSECTIONPARSER_H

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
