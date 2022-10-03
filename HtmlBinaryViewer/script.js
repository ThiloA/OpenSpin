var initObject = function() {
    var typeObjectHeader=0;
    var typeMethodTable=1;
    var typeObjectTable=2;
    var typeDatSection=3;
    var typeMethod=4;
    var typeStringPool=5;
    var typePadding=6;
    var annotationMap = [
            { "cssclass": "annotation-objectheader", "description": "Object Header" },
            { "cssclass": "annotation-methodtable", "description": "Method Table" },
            { "cssclass": "annotation-objecttable", "description": "Child Object Table" },
            { "cssclass": "annotation-datsection", "description": "Dat Section" },
            { "cssclass": "annotation-method", "description": "Spin Method" },
            { "cssclass": "annotation-stringpool", "description": "String Pool" },
            { "cssclass": "annotation-padding", "description": "Padding" }
        ];
    function setData(dataObj) {
        var hexDigits = "0123456789ABCDEF";
        var binary = dataObj.binary;
        var pdisasm = createPASMDisassembler(binary);
        var spindisasm = createSpinDisassembler(binary);
        
        var mainContent = $("#maincontent");
        mainContent.empty();
        var byteOffset = 0;
        var annotations=[]
        var lastObjOffset = 0;
        var lastSpinMethodTbl = []
        var spinMethodIdx = 0;
        for (var it in dataObj.annotation) {
            var size = dataObj.annotation[it][1];
            if (size==0)
                continue;
            var type = dataObj.annotation[it][0];
            var extra = dataObj.annotation[it].length>2 ? dataObj.annotation[it][2] : [];
            if (type == typeObjectHeader) {
                lastObjOffset = byteOffset;
                lastSpinMethodTbl = [];
                spinMethodIdx = 0;
            }
            else if (type == typeMethodTable) {
                lastSpinMethodTbl = extra;
                spinMethodIdx = 0;
            }
            else if (type == typeMethod) {
                extra = "";
                if (spinMethodIdx<lastSpinMethodTbl.length)
                    extra = lastSpinMethodTbl[spinMethodIdx++];
            }
            
            var section = $("<div class='annotation "+annotationMap[type].cssclass+"'></div>");
            mainContent.append(section);
            
            annotations.push({"type":type,"size":size,"offset":byteOffset,"objectBase":lastObjOffset, "dom":section, "extra":extra});
            byteOffset += size;
        }
        if (byteOffset != binary.length) {
            alert("Annotation error: annotation size was "+byteOffset+", but binary size was "+dataObj.binary.length);
            return;
        }
        function htmlEntities(str) {
            return String(str).replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;').replace(/"/g, '&quot;');
        }
        function hexByte(value) {
            return hexDigits[(value>>4)&0xF]+hexDigits[(value)&0xF];
        }
        function hexWord(value) {
            return hexDigits[(value>>12)&0xF]+hexDigits[(value>>8)&0xF]+hexDigits[(value>>4)&0xF]+hexDigits[(value)&0xF];
        }
        function generateButtons(annotationIdx, firstLine, activebutton) {
            if (!firstLine)
                return $("<div class='collapsebuttons'></div>");
            var col = $("<div class='collapsebuttons'><button class='btnminus'>-</button><button class='btndefault'>0</button><button class='btndetail'>+</button></div>");
            col.find(".btndefault").click(function() {
                createDefaultView(annotationIdx);
            });
            col.find(".btnminus").click(function() {
                createMinimalView(annotationIdx);
            });
            col.find(".btndetail").click(function() {
                createDetailView(annotationIdx);
            });
            col.find(activebutton).addClass("selected")
            return col;
        }
        
        function generateOffsetCol(offset) {
            return $("<div class='offset'><a name='"+hexWord(offset)+"'>0x"+hexWord(offset)+"</a></div>")
        }
        
        function appendLine(annotationIdx, offset, size, firstLine, description, activebutton) {
            var an = annotations[annotationIdx];
            var parent = an.dom;
            var line = $("<div class='line'></div>");
            parent.append(line);
            line.append(generateOffsetCol(offset));
            var hexStr = "";
            var lineSize = size>16 ? 16 : size;
            for (var i=0; i<lineSize; ++i) {
                if (i==4 || i == 8 || i == 12)
                    hexStr += " ";
                hexStr += hexByte(binary[offset+i]);
            }
            line.append($("<div class='hexbytes'></div>").text(hexStr));
            line.append(generateButtons(annotationIdx,firstLine,activebutton));
            line.append($("<div class='description'></div>").html(description));
            line.append($("<div class='clear'></div>"));
        }
        
        function createStringPoolView(offset, size, objectBase) {
            var result=[]
            var relPos = 0
            function nextString() {
                str = "";
                var startPos = relPos;
                while (relPos<size) {
                    var ch = binary[offset+relPos];
                    relPos++;
                    if (ch == 0) {
                        result.push([relPos-startPos,"<pre>String &quot;"+htmlEntities(str)+"&quot;</pre>"]);
                        return;
                    }
                    str += String.fromCharCode(ch); //TODO parallax encoding
                }
                result.push([relPos-startPos,"<pre>String (missing terminating char) &quot;"+htmlEntities(str)+"&quot;</pre>"])
            }
            while (relPos<size)
                nextString();
            return result;
        }
        function createMethodTableView(an) {
            var offset = an.offset;
            var size = an.size;
            var result=[]
            var idx = 0
            while (size>0) {
                if (size<4) {
                    result.push([size,"Invalid Entry"])
                    break;
                }
                var offstr = hexWord(an.objectBase+binary[offset]+binary[offset+1]*256);
                var methodName = idx<an.extra.length ? "&quot;"+an.extra[idx]+"&quot;" : "-unknown-";
                result.push([4,"Method "+methodName+" Entrypoint: <a href='#"+offstr+"'>0x"+offstr+"</a>, Locals: "+(binary[offset+2]+binary[offset+3]*256)]);
                offset+=4;
                size-=4;
                idx += 1;
            }
            return result;
        }
        function createObjectTableView(an) {
            var offset = an.offset;
            var size = an.size;
            var result=[]
            var idx = 0
            while (size>0) {
                if (size<4) {
                    result.push([size,"Invalid Entry"])
                    break;
                }
                var offstr = hexWord(an.objectBase+binary[offset]+binary[offset+1]*256);
                var instanceName = idx<an.extra.length ? "&quot;"+an.extra[idx]+"&quot;" : "-unknown-";
                result.push([4,"Object "+instanceName+" Entrypoint: <a href='#"+offstr+"'>0x"+offstr+"</a>, Var Section Offset: "+hexWord(binary[offset+2]+binary[offset+3]*256)]);
                offset+=4;
                size-=4;
                idx += 1;
            }
            return result;
        }
        function createObjectHeaderView(an) {
            if (an.size != 4)
                return [[an.size,"Invalid Object Header Entry"]];
            return [
                [2,"Object Size: "+(binary[an.offset]+binary[an.offset+1]*256)],
                [1,"Functions: "+(binary[an.offset+2]-1)],
                [1,"Child Objects: "+binary[an.offset+3]]
            ];
        }
        function createDatView(an) {
            return pdisasm.disasm(an.offset,an.extra);
        }
        function createSpinView(an) {
            return spindisasm.disasm(an.offset,an.size,an.extra);
        }
        
        function createDetailView(annotationIdx) {
            var an = annotations[annotationIdx];
            var parts=[]
            if (an.type == typeStringPool)
                parts = createStringPoolView(an.offset, an.size, an.objectBase);
            else if (an.type == typeMethodTable)
                parts = createMethodTableView(an);
            else if (an.type == typeObjectTable)
                parts = createObjectTableView(an);
            else if (an.type == typeObjectHeader)
                parts = createObjectHeaderView(an);
            else if (an.type == typeDatSection)
                parts = createDatView(an);
            else if (an.type == typeMethod)
                parts = createSpinView(an);
            else {
                createDefaultView(annotationIdx);
                return;
            }
            var parent = an.dom;
            parent.empty();
            var offset = an.offset;
            var firstLine = true;
            for (var i in parts) {
                var partSize = parts[i][0]
                var partStr = parts[i][1];
                if (partSize == 0) {
                    appendLine(annotationIdx, offset, 0, firstLine, partStr, ".btndetail");
                    firstLine = false;
                }
                while (partSize>0) {
                    var lineSize = partSize>16 ? 16 : partSize;
                    appendLine(annotationIdx, offset, lineSize, firstLine, partStr, ".btndetail");
                    partStr = "";
                    firstLine = false;
                    offset+=lineSize;
                    partSize-=lineSize;
                }
            }
        }
        function createDefaultView(annotationIdx) {
            var an = annotations[annotationIdx];
            var parent = an.dom;
            parent.empty();
            var size = an.size;
            var offset = an.offset;
            var description = annotationMap[an.type].description;
            if (an.type == typeMethod)
                description+=": "+an.extra;
            var firstLine = true;
            while (size>0) {
                appendLine(annotationIdx, offset, size, firstLine, firstLine ? description : "", ".btndefault");
                firstLine = false;
                size-=16;
                offset+=16;
            }
        }
        function createMinimalView(annotationIdx) {
            var an = annotations[annotationIdx];
            var parent = an.dom;
            parent.empty();
            var line = $("<div class='line'></div>");
            parent.append(line);
            line.append(generateOffsetCol(an.offset));
            line.append($("<div class='hexbytes'></div>").text(an.size+" Bytes"));
            line.append(generateButtons(annotationIdx,true,".btnminus"));
            var txt = annotationMap[an.type].description;
            if (an.type == typeMethod)
                txt+=": "+an.extra;
            line.append($("<div class='description'></div>").text(txt));
            line.append($("<div class='clear'></div>"));
        }
        for (var i in annotations)
            createMinimalView(i);
    }
    return setData;
}();
