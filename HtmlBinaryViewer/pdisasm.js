function createPASMDisassembler(binary) {
    var typeData=0;
    var typeInstruction=1;
    var typeCogOrg=2;
    var typeCogOrgX=3;
    var condition = [
        "if_never",
        "if_a",
        "if_z_and_nc",
        "if_ae",
        "if_nz_and_c",
        "if_ne",
        "if_z_ne_c",
        "if_nz_or_nc",
        "if_z_and_c",
        "if_z_eq_c",
        "if_e",
        "if_z_or_nc",
        "if_b",
        "if_nz_or_c",
        "if_be",
        ""
    ];
    var typeSeparateWrNr = 0;
    var typeDefaultWr = 1;
    var typeDefaultNr = 2;
    var typeJmp = 3;
    var typeLoop = 4;
    var typeOther = 5;
    var cogRegs = ["par","cnt","ina","inb","outa","outb","dira","dirb","ctra","ctrb","frqa","frqb","phsa","phsb","vcfg","vscl"];
    var instCodes = [
        [typeSeparateWrNr,  "wrbyte", "rdbyte"],
        [typeSeparateWrNr,  "wrword", "rdword"],
        [typeSeparateWrNr,  "wrlong", "rdlong"],
        [typeOther,         "hubop"],
        [typeOther,         "IllgMul"],
        [typeOther,         "IllgMuls"],
        [typeOther,         "IllgEnc"],
        [typeOther,         "IllgOnes"],
        [typeDefaultWr,     "ror"],
        [typeDefaultWr,     "rol"],
        [typeDefaultWr,     "shr"],
        [typeDefaultWr,     "shl"],
        [typeDefaultWr,     "rcr"],
        [typeDefaultWr,     "rcl"],
        [typeDefaultWr,     "sar"],
        [typeDefaultWr,     "rev"],
        [typeDefaultWr,     "mins"],
        [typeDefaultWr,     "maxs"],
        [typeDefaultWr,     "min"],
        [typeDefaultWr,     "max"],
        [typeDefaultWr,     "movs"],
        [typeDefaultWr,     "movd"],
        [typeDefaultWr,     "movi"],
        [typeJmp],
        [typeDefaultWr,     "and"],
        [typeDefaultWr,     "andn"],
        [typeDefaultWr,     "or"],
        [typeDefaultWr,     "xor"],
        [typeDefaultWr,     "muxc"],
        [typeDefaultWr,     "muxnc"],
        [typeDefaultWr,     "muxz"],
        [typeDefaultWr,     "muxnz"],
        [typeDefaultWr,     "add"],
        [typeSeparateWrNr,  "cmp", "sub"],
        [typeDefaultWr,     "addabs"],
        [typeDefaultWr,     "subabs"],
        [typeDefaultWr,     "sumc"],
        [typeDefaultWr,     "sumnc"],
        [typeDefaultWr,     "sumz"],
        [typeDefaultWr,     "sumnz"],
        [typeDefaultWr,     "mov"],
        [typeDefaultWr,     "neg"],
        [typeDefaultWr,     "abs"],
        [typeDefaultWr,     "absNeg"],
        [typeDefaultWr,     "negc"],
        [typeDefaultWr,     "negnc"],
        [typeDefaultWr,     "negz"],
        [typeDefaultWr,     "negnz"],
        [typeDefaultNr,     "cmps"],
        [typeDefaultNr,     "cmpsx"],
        [typeDefaultWr,     "addx"],
        [typeDefaultWr,     "subx"],
        [typeDefaultWr,     "adds"],
        [typeDefaultWr,     "subs"],
        [typeDefaultWr,     "addsx"],
        [typeDefaultWr,     "subsx"],
        [typeDefaultWr,     "cmpsub"],
        [typeLoop,          "djnz", true],
        [typeLoop,          "tjnz", false],
        [typeLoop,          "tjz", false],
        [typeDefaultNr,     "waitpeq"],
        [typeDefaultNr,     "waitpne"],
        [typeDefaultWr,     "waitcnt"],
        [typeDefaultNr,     "waitvid"]
    ];
    
    function hexOp(i) {
        var str = Number(i).toString(16);
        while (str.length<3)
            str = "0"+str;
        return str;
    }
    
    function fixedWidth(str,len) {
        while(str.length<len)
            str+=" ";
        return str;
    }
    function analyseLabels(offset,annotation) {
        var labels=[];
        for (var i=0; i<512; ++i)
            labels.push({jmp:false,ret:false,dat:false,name:"",showLblName:""});
        var cogOrg = 0;
        var cogOrgX = false;
        for (var i in annotation) {
            var an = annotation[i];
            if (an[0] == typeData) {
                offset += an[1];
                if (!cogOrgX)
                    cogOrg += an[1];
            }
            else if (an[0] == typeInstruction) {
                var size = an[1];
                for (var j=0; j<size; j+=4,offset += 4) {
                    if (size-j<4) {
                        offset += size-j;
                        if (!cogOrgX)
                            cogOrg += size-j;
                        break;
                    }
                    var fullOpCode = binary[offset]+(binary[offset+1]*256)+(binary[offset+2]*65536)+(binary[offset+3]*16777216);
                    var mainOperation = instCodes[(fullOpCode>>26)&0x3F];
                    var dst = (fullOpCode>>9) & 0x1FF;
                    var srcImm = ((fullOpCode>>22)&1 == 1);
                    var src = fullOpCode & 0x1FF;
                    var isWr = ((fullOpCode>>23)&1 == 1);
                    if (mainOperation[0] == typeJmp) {
                        if (isWr)
                            labels[dst].ret = true;
                        if (srcImm)
                            labels[src].jmp = true;
                    }
                    else if (mainOperation[0] == typeLoop) {
                        if (isWr)
                            labels[dst].dat = true;
                        if (srcImm)
                            labels[src].jmp = true;
                    }
                    else {
                        labels[dst].dat = true;
                        if (!srcImm)
                            labels[src].dat = true;
                    }
                    if (!cogOrgX)
                        cogOrg += 4;  
                }
            }
            else if (an[0] == typeCogOrg)
                cogOrg = an[1];
            else if (an[1] == typeCogOrgX)
                cogOrgX = an[1] == 1;
        }
        return labels;
    }

    function disasm(offset, annotation, labels) {
        var result=[]
        var cogOrg = 0;
        var cogOrgX = false;
        for (var i in labels) {
            if (labels[i].jmp) {
                labels[i].name = "lbl"+hexOp(i);
                labels[i].showLblName = labels[i].name;
            }
            else if (labels[i].dat) {
                labels[i].name = "var"+hexOp(i);
                labels[i].showLblName = labels[i].name;
            }
            else
                labels[i].name = "$"+hexOp(i);
        }
        for (var i=0; i<16; ++i)
            labels[i+0x1F0].name = cogRegs[i];
        for (var i in annotation) {
            var an = annotation[i];
            if (an[0] == typeData) {
                result.push([an[1],"Data"]);
                offset += an[1];
                if (!cogOrgX)
                    cogOrg += an[1];
            }
            else if (an[0] == typeInstruction) {
                var size = an[1];
                for (var j=0; j<size; j+=4,offset += 4) {
                    if (size-j<4) {
                        result.push([size-j,"Illegal Code"]);
                        offset += size-j;
                        if (!cogOrgX)
                            cogOrg += size-j;
                        break;
                    }
                    var fullOpCode = binary[offset]+(binary[offset+1]*256)+(binary[offset+2]*65536)+(binary[offset+3]*16777216);
                    var mainOperation = instCodes[(fullOpCode>>26)&0x3F];
                    var condCode = fixedWidth(condition[(fullOpCode>>18)&0xF],12);
                    var dst = labels[(fullOpCode>>9) & 0x1FF].name;
                    var srcImm = ((fullOpCode>>22)&1 == 1);
                    var src = (srcImm ? ("#$"+hexOp(fullOpCode&0x1FF)) : " "+labels[fullOpCode&0x1FF].name);
                    var isWr = ((fullOpCode>>23)&1 == 1);
                    var wflg = (((fullOpCode>>24)&1 == 1) ? " wc" : "")+(((fullOpCode>>25)&1 == 1) ? " wz" : "");
                    var asmStr = "";
                    var asmWidth = 10;
                    if(mainOperation[0] == typeDefaultWr)
                        asmStr = fixedWidth(mainOperation[1],asmWidth)+" "+dst+", "+src+(isWr ? "" : " nr");
                    else if(mainOperation[0] == typeDefaultNr)
                        asmStr = fixedWidth(mainOperation[1],asmWidth)+" "+dst+", "+src+(isWr ? " wr" : "");
                    else if(mainOperation[0] == typeSeparateWrNr)
                        asmStr = fixedWidth(isWr ? mainOperation[2] : mainOperation[1],10)+" "+dst+", "+src;
                    else if(mainOperation[0] == typeLoop) {
                        if (srcImm)
                            src = "#"+labels[fullOpCode&0x1FF].name
                        asmStr = fixedWidth(mainOperation[1],asmWidth)+" "+dst+", "+src+(isWr ? " wr" : " nr");
                    }
                    else if (mainOperation[0] == typeJmp) {
                        if (srcImm)
                            src = "#"+labels[fullOpCode&0x1FF].name
                        if (isWr)
                            asmStr = fixedWidth("call",asmWidth)+" "+src;
                        else if (labels[cogOrg>>2].ret)
                            asmStr = fixedWidth("ret",asmWidth);
                        else
                            asmStr = fixedWidth("jmp",asmWidth)+" "+src;
                    }
                    else
                        asmStr = fixedWidth(mainOperation[1],asmWidth)+" "+dst+", "+src+(isWr ? " wr" : " nr");
                    result.push([4,"<pre>"+hexOp(cogOrg>>2)+"   "+fixedWidth(""+(labels[cogOrg>>2].showLblName),8)+"  "+asmStr+wflg+"</pre>"]);
                    if (!cogOrgX)
                        cogOrg += 4;
                }
            }
            else if (an[0] == typeCogOrg)
                cogOrg = an[1];
            else if (an[1] == typeCogOrgX)
                cogOrg = an[1] == 1;
        }
        return result;
    }
    
    return {
        disasm:function(offset,annotation) {
            return disasm(offset,annotation,analyseLabels(offset,annotation));
        }
    };
};
