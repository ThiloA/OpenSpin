//////////////////////////////////////////////////////////////
//                                                          //
// Propeller Spin/PASM Compiler                             //
// (c)2012-2016 Parallax Inc. DBA Parallax Semiconductor.   //
// Adapted from Chip Gracey's x86 asm code by Roy Eltham    //
// Rewritten to modern C++ by Thilo Ackermann               //
// See end of file for terms of use.                        //
//                                                          //
////////////////////////////////////////////////////////////// 

#ifndef SPINCOMPILER_FINALGENERATOR_H
#define SPINCOMPILER_FINALGENERATOR_H

#include "SpinCompiler/Parser/ParsedObject.h"
#include "SpinCompiler/Generator/BinaryObject.h"
#include "SpinCompiler/Tokenizer/StringMap.h"

class FinalGenerator {
private:
    const CompilerSettings& settings;
    ParsedObjectP parsedRootObject;
    BinaryObjectP binaryRootObject;
    const StringMap& stringMap;
public:
    struct TopLevelConstants {
        TopLevelConstants():clkFreq(0),clkMode(0),debugBaud(0),stackRequirement(0) {}
        int clkFreq;
        int clkMode;
        int debugBaud;
        int stackRequirement;
    };

    FinalGenerator(const CompilerSettings& settings, ParsedObjectP parsedRootObject, BinaryObjectP binaryRootObject, const StringMap& stringMap):settings(settings),parsedRootObject(parsedRootObject),binaryRootObject(binaryRootObject),stringMap(stringMap){}

    std::vector<unsigned char> composeRAM(const std::vector<unsigned char>& binary, TopLevelConstants tlc) {
        const int varsize = binaryRootObject->totalVarSize();                                                // variable size (in bytes)
        const int codsize = binary.size();                                                // code size (in bytes)
        const int pubaddr = readWord(binary,4);                        // address of first public method
        const int publocs = readWord(binary,6);                       // number of stack variables (locals), in bytes, for the first public method
        const int pbase = 0x0010;                                                                  // base of object code
        const int vbase = pbase + codsize;                                                         // variable base = object base + code size
        const int dbase = vbase + varsize + 8;                                                     // data base = variable base + variable size + 8
        const int pcurr = pbase + pubaddr;                                                         // Current program start = object base + public address (first public method)
        const int firstPubParams = !parsedRootObject->methods.empty() ? parsedRootObject->methods.front()->parameterCount : 0;
        const int dcurr = dbase + 4 + (firstPubParams << 2) + publocs;      // current data stack pointer = data base + 4 + FirstParams*4 + publocs

        std::vector<unsigned char> result(settings.binaryMode ? vbase : settings.eepromSize);
        if (!settings.binaryMode) {
            if (vbase + 8 > settings.eepromSize)
               throw CompilerError(ErrorType::sztl,SourcePosition(),std::to_string(vbase + 8 - settings.eepromSize)+" bytes");
            // reset ram
            result[dbase-8] = 0xFF;
            result[dbase-7] = 0xFF;
            result[dbase-6] = 0xF9;
            result[dbase-5] = 0xFF;
            result[dbase-4] = 0xFF;
            result[dbase-3] = 0xFF;
            result[dbase-2] = 0xF9;
            result[dbase-1] = 0xFF;
        }
        // set clock frequency and clock mode
        writeLong(result,0,tlc.clkFreq);
        result[4] = tlc.clkMode;
        // result[5] is checksum (set later)

        // set interpreter parameters
        writeWord(result,6,pbase);
        writeWord(result,8,vbase);
        writeWord(result,10,dbase);
        writeWord(result,12,pcurr);
        writeWord(result,14,dcurr);

        // set code
        for (int i=0; i<codsize; ++i)
            result[pbase+i] = binary[i];

        // install ram checksum byte
        unsigned char sum = 0;
        for (int i = 0; i < vbase; i++)
            sum += result[i];
        result[5] = (unsigned char)((-(sum+2028)) );
        return result;
    }
    std::vector<unsigned char> composeRAMDatOnly(const std::vector<unsigned char>& binary) {
        if (binary.size() > 65535)
            throw CompilerError(ErrorType::sztl);
        const int objSize = readWord(binary,0);
        const int startOfDat = 4+binary[3] * 4;
        const int endOfDat = objSize;
        return std::vector<unsigned char>(binary.begin()+startOfDat,binary.begin()+endOfDat);
    }
    int readWord(const std::vector<unsigned char>& binary, int i) {
        return binary[i] | (binary[i+1]<<8);
    }
    void writeWord(std::vector<unsigned char>& result, int i, int value) {
        result[i] = value & 0xFF;
        result[i+1] = (value>>8)&0xFF;
    }
    void writeLong(std::vector<unsigned char>& result, int i, int value) {
        result[i] = value & 0xFF;
        result[i+1] = (value>>8)&0xFF;
        result[i+2] = (value>>16)&0xFF;
        result[i+3] = (value>>24)&0xFF;
    }


    TopLevelConstants determineTopLevelConstants() {
        TopLevelConstants result;
        result.stackRequirement = 16;

        determineGetSymbolAndSetValue("_STACK", ErrorType::ssaf, result.stackRequirement);
        determineGetSymbolAndSetValue("_FREE", ErrorType::ssaf, result.stackRequirement);
        if (result.stackRequirement > 0x2000)
            throw CompilerError(ErrorType::safms);

        // set to RCFAST for default
        result.clkMode = 0;
        result.clkFreq = 12000000;



        // try to find the values in the symbols

        int mode = 0;
        const bool bHaveClkMode = determineGetSymbolAndSetValue("_CLKMODE", ErrorType::sccx, mode);

        int freq = 0;
        const bool bHaveClkFreq = determineGetSymbolAndSetValue("_CLKFREQ", ErrorType::sccx, freq);

        int xin = 0;
        const bool bHaveXinFreq = determineGetSymbolAndSetValue("_XINFREQ", ErrorType::sccx, xin);

        if (bHaveClkMode == false && bHaveClkFreq == false && bHaveXinFreq == false) // just use default (already set above)
            return result;

        // can't have either freq without clkmode
        if (bHaveClkMode == false && (bHaveClkFreq == true || bHaveXinFreq == true))
            throw CompilerError(ErrorType::cxswcm);

        // can't have both clkfreq and xinfreq
        if (bHaveClkFreq == true && bHaveXinFreq == true)
            throw CompilerError(ErrorType::ecoxmbs);

        // validate the mode
        if (mode == 0 || (mode & 0xFFFFF800) != 0 || (((mode & 0x03) != 0) && ((mode & 0x7FC) != 0)))
            throw CompilerError(ErrorType::icms);

        int freqShift = 0;
        if (mode & 0x03) { // RCFAST or RCSLOW
            // can't have clkfreq or xinfreq in RC mode
            if (bHaveClkFreq == true || bHaveXinFreq == true)
                throw CompilerError(ErrorType::cxnawrc);

            if (mode == 2) { // RCSLOW
                result.clkMode = 1;
                result.clkFreq = 20000;
            }
            // else RCFAST (which is already set as default)
            return result;
        }
        // get xinput/xtal1/xtal2/xtal3
        int bitPos = 0;
        determineGetBitPos((mode >> 2) & 0x0F, bitPos);
        result.clkMode = (unsigned char)((bitPos << 3) | 0x22); // 0x22 = 0100010b

        if (mode & 0x7C0) { // get xmul
            determineGetBitPos(mode >> 6, bitPos);
            freqShift = bitPos;
            result.clkMode += (unsigned char)(bitPos + 0x41); // 0x41 = 1000001b
        }
        // get clkfreq
        // must have xinfreq or clkfreq
        if (bHaveClkFreq == false && bHaveXinFreq == false)
            throw CompilerError(ErrorType::coxmbs);
        result.clkFreq = bHaveClkFreq ? freq : (xin << freqShift);
        determineGetSymbolAndSetValue("_DEBUG", ErrorType::sdcobu,result.debugBaud);
        return result;
    }
private:
    bool determineGetSymbolAndSetValue(const std::string& symbol, ErrorType errorCode, int &destValue) {
        //TODO use global symbol map, and throw error on no constant
        const int constantIndex = parsedRootObject->indexOfConstantIfAvailable(stringMap.hasSymbolName(symbol));
        if (constantIndex<0)
            return false;
        destValue = binaryRootObject->constants[constantIndex];
        return true;
    }
    static void determineGetBitPos(int value, int& bitPos) {
        int bitCount = 0;
        for (int i = 32; i > 0; i--) {
            if (value & 0x01) {
                bitPos = 32 - i;
                bitCount++;
            }
            value >>= 1;
        }
        if (bitCount != 1)
            throw CompilerError(ErrorType::icms);
    }
};


#endif //SPINCOMPILER_FINALGENERATOR_H

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
