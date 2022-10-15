//////////////////////////////////////////////////////////////
//                                                          //
// Propeller Spin/PASM Compiler                             //
// (c)2012-2016 Parallax Inc. DBA Parallax Semiconductor.   //
// Adapted from Chip Gracey's x86 asm code by Roy Eltham    //
// Rewritten to modern C++ by Thilo Ackermann               //
// See end of file for terms of use.                        //
//                                                          //
//////////////////////////////////////////////////////////////


#include <string>
#include <vector>
#include <iostream>
#include "CLI/CommandLineInteface.h"
#include "CLI/TestCase.h"

int main(int argc, char *argv[]) {
    std::vector<std::string> args(argc-1);
    for (int i=1; i<argc; ++i)
        args[i-1] = std::string(argv[i]);
#if 0
    return TestCase::runTestCompileAndCompare(args) ? 0 : 1;
#else
    CommandLineInterface cli;
    if (args.empty()) {
        cli.usage();
        return 1;
    }
    auto cliErr = cli.parseArguments(args);
    if (!cliErr.empty()) {
        cli.usage();
        if (cliErr == "?")
            return 0;
        std::cerr<<"Invalid arguments: "<<cliErr<<std::endl;
        return 1;
    }
    return cli.run() ? 0 : 1;
#endif
}

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
