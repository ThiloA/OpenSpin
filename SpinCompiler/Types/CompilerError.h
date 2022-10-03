//////////////////////////////////////////////////////////////
//                                                          //
// Propeller Spin/PASM Compiler                             //
// (c)2012-2016 Parallax Inc. DBA Parallax Semiconductor.   //
// Adapted from Chip Gracey's x86 asm code by Roy Eltham    //
// Rewritten to modern C++ by Thilo Ackermann               //
// See end of file for terms of use.                        //
//                                                          //
////////////////////////////////////////////////////////////// 

#ifndef SPINCOMPILER_COMPILERERROR_H
#define SPINCOMPILER_COMPILERERROR_H

#include "SpinCompiler/Types/SourcePosition.h"
#include "SpinCompiler/Types/Token.h"

enum struct ErrorType {
    ainl,
    aioor,
    bmbpbb,
    bdmbifc,
    bnso,
    ccsronfp,
    ce32b,
    coxmbs,
    csmnexc,
    cxnawrc,
    cxswcm,
    dbz,
    drcex,
    eaaeoeol,
    eaasme,
    eaasmi,
    eaboor,
    eacn,
    eacuool,
    eads,
    eaet,
    eaiov,
    eals,
    eamvaa,
    easn,
    easoon,
    eatq,
    eauon,
    eav,
    eaucnop,
    eaunbwlo,
    eaupn,
    eaurn,
    eausn,
    eauvn,
    ebwol,
    ecoeol,
    ecolon,
    ecomma,
    ecor,
    ecoxmbs,
    edot,
    eeol,
    eelcoeol,
    efrom,
    eitc,
    eleft,
    eleftb,
    epoa,
    epoeol,
    epound,
    erb,
    erbb,
    eright,
    erightb,
    es,
    esoeol,
    eto,
    ftl,
    fpcmbw,
    fpnaiie,
    fpo,
    ibn,
    icms,
    idbn,
    idfnf,
    ifc,
    ifufiq,
    inaifpe,
    internal,
    ionaifpe,
    loxlve,
    loxpe,
    loxspoe,
    micuwn,
    nce,
    nprf,
    ocmbf1tx,
    odo,
    oefl,
    oex,
    oexl,
    oinah,
    omblc,
    pclo,
    rainl,
    raioor,
    rinah,
    rinaiom,
    safms,
    sccx,
    scmr,
    sdcobu,
    sexc,
    siad,
    snah,
    sombl,
    sombs,
    srccex,
    ssaf,
    stif,
    tioawarb,
    tmsc,
    tmscc,
    tmvsid,
    uc,
    urs,
    us,
    vnao,
    fnf,
    maceif,
    macstr,
    macsp,
    macerr,
    circ,
    sztl
};

struct ErrorMessage {
    ErrorMessage():type(ErrorType::internal),typeName(""),message("unknown") {}
    ErrorMessage(ErrorType type, const char* typeName, const char* message):type(type),typeName(typeName),message(message) {}
    ErrorType type;
    const char* typeName;
    const char* message;
};

struct CompilerError {
    explicit CompilerError(ErrorType errType):errType(errType) {}
    CompilerError(ErrorType errType, const Token& tk):errType(errType),sourcePosition(tk.sourcePosition) {}
    CompilerError(ErrorType errType, const SourcePosition& sourcePosition):errType(errType), sourcePosition(sourcePosition) {}
    CompilerError(ErrorType errType, const SourcePosition& sourcePosition, const std::string& extraMessage):errType(errType), sourcePosition(sourcePosition),extraMessage(extraMessage) {}
    ErrorType errType;
    SourcePosition sourcePosition;
    std::string extraMessage;
};

class CompilerMessages {
public:
    CompilerMessages():m_messageText {
    {ErrorType::ainl,      "ainl",     "Address is not long"},
    {ErrorType::aioor,     "aioor",    "Address is out of range"},
    {ErrorType::bmbpbb,    "bmbpbb",   "\"}\" must be preceeded by \"{\" to form a comment"},
    {ErrorType::bdmbifc,   "bdmbifc",  "Block designator must be in first column"},
    {ErrorType::bnso,      "bnso",     "Blocknest stack overflow"},
    {ErrorType::ccsronfp,  "ccsronfp", "Cannot compute square root of negative floating-point number"},
    {ErrorType::ce32b,     "ce32b",    "Constant exceeds 32 bits"},
    {ErrorType::coxmbs,    "coxmbs",   "_CLKFREQ or _XINFREQ must be specified"},
    {ErrorType::csmnexc,   "csmnexc",  "CALL symbol must not exceed 252 characters"},
    {ErrorType::cxnawrc,   "cxnawrc",  "_CLKFREQ/_XINFREQ not allowed with RCFAST/RCSLOW"},
    {ErrorType::cxswcm,    "cxswcm",   "_CLKFREQ/_XINFREQ specified without _CLKMODE"},
    {ErrorType::dbz,       "dbz",      "Divide by zero"},
    {ErrorType::drcex,     "drcex",    "Destination register cannot exceed $1FF"},
    {ErrorType::eaaeoeol,  "eaaeoeol", "Expected an assembly effect or end of line"},
    {ErrorType::eaasme,    "eaasme",   "Expected an assembly effect"},
    {ErrorType::eaasmi,    "eaasmi",   "Expected an assembly instruction"},
    {ErrorType::eaboor,    "eaboor",   "Expected a binary operator or \")\""},
    {ErrorType::eacn,      "eacn",     "Expected a constant name"},
    {ErrorType::eacuool,   "eacuool",  "Expected a constant, unary oeprator, or \"(\""},
    {ErrorType::eads,      "eads",     "Expected a DAT symbol"},
    {ErrorType::eaet,      "eaet",     "Expected an expression term"},
    {ErrorType::eaiov,     "eaiov",    "Expected an instruction or variable"},
    {ErrorType::eals,      "eals",     "Expected a local symbol"},
    {ErrorType::eamvaa,    "eamvaa",   "Expected a memory variable after \"@\""},
    {ErrorType::easn,      "easn",     "Expected a subroutine name"},
    {ErrorType::easoon,    "easoon",   "Expected a subroutine or object name"},
    {ErrorType::eatq,      "eatq",     "Expected a terminating quote"},
    {ErrorType::eauon,     "eauon",    "Expected a unique object name"},
    {ErrorType::eav,       "eav",      "Expected a variable"},
    {ErrorType::eaucnop,   "eaucnop",  "Expected a unique constant name or \"#\""},
    {ErrorType::eaunbwlo,  "eaunbwlo", "Expected a unique name, BYTE, WORD, LONG, or assembly instruction"},
    {ErrorType::eaupn,     "eaupn",    "Expected a unique parameter name"},
    {ErrorType::eaurn,     "eaurn",    "Expected a unique result name"},
    {ErrorType::eausn,     "eausn",    "Expected a unique subroutine name"},
    {ErrorType::eauvn,     "eauvn",    "Expected a unique variable name"},
    {ErrorType::ebwol,     "ebwol",    "Expected BYTE, WORD, or LONG"},
    {ErrorType::ecoeol,    "ecoeol",   "Expected \"\" or end of line"},
    {ErrorType::ecolon,    "ecolon",   "Expected \":\""},
    {ErrorType::ecomma,    "ecomma",   "Expected \"\""},
    {ErrorType::ecor,      "ecor",     "Expected \"\" or \")\""},
    {ErrorType::ecoxmbs,   "ecoxmbs",  "Either _CLKFREQ or _XINFREQ must be specified, but not both"},
    {ErrorType::edot,      "edot",     "Expected \".\""},
    {ErrorType::eeol,      "eeol",     "Expected end of line"},
    {ErrorType::eelcoeol,  "eelcoeol", "Expected \"=\" \"[\" \"\" or end of line"},
    {ErrorType::efrom,     "efrom",    "Expected FROM"},
    {ErrorType::eitc,      "eitc",     "Expression is too complex"},
    {ErrorType::eleft,     "eleft",    "Expected \"(\""},
    {ErrorType::eleftb,    "eleftb",   "Expected \"[\""},
    {ErrorType::epoa,      "epoa",     "Expected PRECOMPILE or ARCHIVE"},
    {ErrorType::epoeol,    "epoeol",   "Expected \"|\" or end of line"},
    {ErrorType::epound,    "epound",   "Expected \"#\""},
    {ErrorType::erb,       "erb",      "Expected \"}\""},
    {ErrorType::erbb,      "erbb",     "Expected \"}}\""},
    {ErrorType::eright,    "eright",   "Expected \")\""},
    {ErrorType::erightb,   "erightb",  "Expected \"]\""},
    {ErrorType::es,        "es",       "Empty string"},
    {ErrorType::esoeol,    "esoeol",   "Expected STEP or end of line"},
    {ErrorType::eto,       "eto",      "Expected TO"},
    {ErrorType::ftl,       "ftl",      "Filename too long"},
    {ErrorType::fpcmbw,    "fpcmbw",   "Floating-point constant must be within +/- 3.4e+38"},
    {ErrorType::fpnaiie,   "fpnaiie",  "Floating-point not allowed in integer expression"},
    {ErrorType::fpo,       "fpo",      "Floating-point overflow"},
    {ErrorType::ibn,       "ibn",      "Invalid binary number"},
    {ErrorType::icms,      "icms",     "Invalid _CLKMODE specified"},
    {ErrorType::idbn,      "idbn",     "Invalid double-binary number"},
    {ErrorType::idfnf,     "idfnf",    "Internal DAT file not found"},
    {ErrorType::ifc,       "ifc",      "Invalid filename character"},
    {ErrorType::ifufiq,    "ifufiq",   "Invalid filename, use \"FilenameInQuotes\""},
    {ErrorType::inaifpe,   "inaifpe",  "Integer not allowed in floating-point expression"},
    {ErrorType::internal,  "internal", "Internal"},
    {ErrorType::ionaifpe,  "ionaifpe", "Integer operator not allowed in floating-point expression"},
    {ErrorType::loxlve,    "loxlve",   "Limit of 4096 local variables exceeded"},
    {ErrorType::loxpe,     "loxpe",    "Limit of 15 parameters exceeded"},
    {ErrorType::loxspoe,   "loxspoe",  "Limit of 256 subroutines + objects exceeded"},
    {ErrorType::micuwn,    "micuwn",   "Memory instructions cannot use WR/NR"},
    {ErrorType::nce,       "nce",      "No cases encountered"},
    {ErrorType::nprf,      "nprf",     "No PUB routines found"},
    {ErrorType::ocmbf1tx,  "ocmbf1tx", "Object count must be from 1 to 255"},
    {ErrorType::odo,       "odo",      "Object distiller overflow"},
    {ErrorType::oefl,      "oefl",     "Origin exceeds FIT limit"},
    {ErrorType::oex,       "oex",      "Object exceeds 128k (before distilling)"},
    {ErrorType::oexl,      "oexl",     "Origin exceeds $1F0 limit"},
    {ErrorType::oinah,     "oinah",    "\"$\" is not allowed here"},
    {ErrorType::omblc,     "omblc",    "OTHER must be last case"},
    {ErrorType::pclo,      "pclo",     "PUB/CON list overflow"},
    {ErrorType::rainl,     "rainl",    "?_RET address is not long"},
    {ErrorType::raioor,    "raioor",   "?_RET address is out of range"},
    {ErrorType::rinah,     "rinah",    "Register is not allowed here"},
    {ErrorType::rinaiom,   "rinaiom",  "RES is not allowed in ORGX mode"},
    {ErrorType::safms,     "safms",    "_STACK and _FREE must sum to under 8k"},
    {ErrorType::sccx,      "sccx",     "Symbols _CLKMODE, _CLKFREQ, _XINFREQ can only be used as integer constants"},
    {ErrorType::scmr,      "scmr",     "String characters must range from 1 to 255"},
    {ErrorType::sdcobu,    "sdcobu",   "Symbol _DEBUG can only be used as an integer constant"},
    {ErrorType::sexc,      "sexc",     "Symbol exceeds 256 characters"},
    {ErrorType::siad,      "siad",     "Symbol is already defined"},
    {ErrorType::snah,      "snah",     "STRING not allowed here"},
    {ErrorType::sombl,     "sombl",    "Size override must be larger"},
    {ErrorType::sombs,     "sombs",    "Size override must be smaller"},
    {ErrorType::srccex,    "srccex",   "Source register/constant cannot exceed $1FF"},
    {ErrorType::ssaf,      "ssaf",     "Symbols _STACK and _FREE can only be used as integer constants"},
    {ErrorType::stif,      "stif",     "Symbol table is full"},
    {ErrorType::tioawarb,  "tioawarb", "This instruction is only allowed within a REPEAT block"},
    {ErrorType::tmsc,      "tmsc",     "Too many string constants"},
    {ErrorType::tmscc,     "tmscc",    "Too many string constant characters"},
    {ErrorType::tmvsid,    "tmvsid",   "Too much variable space is declared"},
    {ErrorType::uc,        "uc",       "Unrecognized character"},
    {ErrorType::urs,       "urs",      "Undefined ?_RET symbol"},
    {ErrorType::us,        "us",       "Undefined symbol"},
    {ErrorType::vnao,      "vnao",     "Variable needs an operator"},
    {ErrorType::fnf,       "fnf",      "File not found"},
    {ErrorType::maceif,    "maceif",   "Preprocessor missing #endif"},
    {ErrorType::macstr,    "macstr",   "Preprocessor end of string expected"},
    {ErrorType::macsp,     "macsp",    "Preprocessor macro specifier expected"},
    {ErrorType::macerr,    "macerr",   "Preprocessor user message"},
    {ErrorType::circ,      "circ",     "Circular object dependency found"},
    {ErrorType::sztl,      "sztl",     "Size of binary object too large"}
} {}
    void addError(const CompilerError& e) {
        errors.push_back(e);
    }
    bool hasError() const { //todo check warning
        return !errors.empty();
    }
    ErrorMessage messageByType(ErrorType type) const {
        for (auto&m : m_messageText)
            if (m.type == type)
                return m;
        return ErrorMessage();
    }
    std::vector<CompilerError> errors;
private:
    std::vector<ErrorMessage> m_messageText;
};


#endif //SPINCOMPILER_COMPILERERROR_H

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
