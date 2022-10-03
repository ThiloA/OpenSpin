function createSpinDisassembler(binary) {
    var opNone=0;
    var opSigned=1;
    var opByte=2;
    var opObj=3;
    var opEffect=4;
    var opPush=5;
    var opMem=6;
    var lowerOpcodes = [
        [opNone,    "anchor:ret"],
        [opNone,    "anchor:ret; pop"],
        [opNone,    "anchor:abort"],
        [opNone,    "anchor:abort; pop"],
        [opSigned,  "jmp"],
        [opByte,    "call"],
        [opObj,     "call obj"],
        [opObj,     "call obj[]"],
        [opSigned,  "loopstart"],
        [opSigned,  "loopcontinue"],
        [opSigned,  "jmpne"],
        [opSigned,  "jmpe"],
        [opNone,    "jmp"],
        [opSigned,  "case"],
        [opSigned,  "caserange"],
        [opNone,    "lookabort"],
        [opNone,    "lookup"],
        [opNone,    "lookdown"],
        [opNone,    "lookuprange"],
        [opNone,    "lookdownrange"],
        [opNone,    "quit"],
        [opNone,    "mark"],
        [opNone,    "strsize"],
        [opNone,    "strcomp"],
        [opNone,    "bytefill"],
        [opNone,    "wordfill"],
        [opNone,    "longfill"],
        [opNone,    "waitpeq"],
        [opNone,    "bytemove"],
        [opNone,    "wordmove"],
        [opNone,    "longmove"],
        [opNone,    "waitpne"],
        [opNone,    "clkset"],
        [opNone,    "cogstop"],
        [opNone,    "lockret"],
        [opNone,    "waitcnt"],
        [opNone,    "readidxspr"],
        [opNone,    "writeidxspr"],
        [opEffect,  "effectidxspr"],
        [opNone,    "waitvid"],
        [opNone,    "coginitret"],
        [opNone,    "locknewret"],
        [opNone,    "locksetret"],
        [opNone,    "lockclrret"],
        [opNone,    "coginit"],
        [opNone,    "locknew"],
        [opNone,    "lockset"],
        [opNone,    "lockclr"],
        [opNone,    "abort"],
        [opNone,    "abortvalue"],
        [opNone,    "return"],
        [opNone,    "returnvalue"],
        [opNone,    "push -1"],
        [opNone,    "push 0"],
        [opNone,    "push 1"],
        [opPush],
        [opPush],
        [opPush],
        [opPush],
        [opPush],
        [opNone,    "illg"],
        [opMem,     "memidx"],
        [opMem,     "memrange"],
        [opMem,     "memop"]
    ];
    var mathOps = [
        "ror","rol","shr","shl","min","max","neg","not","bitand","abs","bitor","bitxor","add","sub","sar","rev","logand","enc","logor","dec","mul","mulupper","div","mod","sqrt","less","greater","neq","eq","lesseq","greatereq","lognot"
    ];
    return {
        disasm:function(offset,size,methodname) {
            var relpos = 0;
            var result=[[0,"Spin Method: "+methodname]];
            function readByte() {
                if (relpos>=size)
                    throw 0;
                return binary[offset+relpos++];
            }
            function decodeEffect() {
                return "effect("+readByte()+")";
            }
            function decodeLower(opcode) {
                var lop = lowerOpcodes[opcode];
                if (lop[0] == opNone)
                    return lop[1];
                if (lop[0] == opSigned) {
                    var b1 = readByte();
                    if ((b1 & 0x80) == 0x80) {
                        var b2 = readByte();
                        return lop[1]+" "+((b1 & 0x7F)*256 + b2); //TODO signed
                    }
                    return lop[1]+" "+b1; //TODO signed
                }
                if (lop[0] == opByte)
                    return lop[1]+" "+readByte();
                if (lop[0] == opObj) {
                    var b1 = readByte();
                    return lop[1]+" "+b1+" "+readByte();
                }
                if (lop[0] == opEffect)
                    return lop[1]+" "+decodeEffect();
                if (lop[0] == opPush) {
                    if (opcode == 0x37)
                        return "push mask "+readByte();
                    var sz = 0x3b-opcode;
                    var value = 0;
                    for (var k=0; k<sz; ++k)
                        value = value*256+readByte();
                    return "push "+value;
                }
                if (lop[0] == opMem)
                    return "opmem "+readByte(); //TODO cog reg?
                throw 0;
            }
            function decodeVarOrMemOp(operation, memstr) {
                if (operation == 0)
                    return "rd "+memstr
                if (operation == 1)
                    return "wr "+memstr
                if (operation == 3)
                    return "ref "+memstr
                return "assign "+memstr+" "+decodeEffect();
            }
            function decodeVarOp(opcode) {
                var operation = opcode & 0x3;
                var varno = (opcode >> 2) & 0x7;
                var type = (opcode & 0x20) != 0 ? "stack" : "var";
                return decodeVarOrMemOp(operation, type+"["+varno+"]");
                
            }
            function decodeMemOp(opcode) {
                var operation = opcode & 0x3;
                var base = (opcode>>2) & 0x3;
                var isIdx = (opcode & 0x10) != 0;
                var size = (opcode>>5)&0x3;
                var baseName = "";
                if (base == 0)
                    baseName = "popbase";
                else if (base == 1)
                    baseName = "objbase";
                else if (base == 2)
                    baseName = "varbase";
                else
                    baseName = "stackbase";
                if (base > 0) {
                    var b1 = readByte();
                    if (b1<0x80)
                        baseName += "+"+b1;
                    else
                        baseName += "+"+((b1&0x7F)*256+readByte());
                }
                if (isIdx)
                    baseName += "+popidx";
                if (size == 0)
                    baseName = "byte "+baseName;
                else if (size == 1)
                    baseName = "word "+baseName;
                else
                    baseName = "long "+baseName;
                return decodeVarOrMemOp(operation,baseName);
            }
            function decodeMathOp(opcode) {
                return mathOps[opcode-0xE0];
            }
            try {
                while (relpos<size) {
                    var startPos = relpos;
                    var opcode= binary[offset+relpos++];
                    var opTxt = "";
                    if (opcode<0x40) //lower op
                        opTxt = decodeLower(opcode);
                    else if (opcode<0x80) //var op
                        opTxt = decodeVarOp(opcode);
                    else if (opcode<0xE0)
                        opTxt = decodeMemOp(opcode);
                    else
                        opTxt = decodeMathOp(opcode);
                    
                    result.push([relpos-startPos,opTxt]);
                }
            }
            catch(e) {
            }
            if (relpos<size)
                result.push([size-relpos,"unable to decode"]);
            return result;
        }
    };
}

