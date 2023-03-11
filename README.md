This library provides kotlin bindings for the [tree-sitter](https://github.com/tree-sitter/tree-sitter), execute the `gradle test` comnand to run the test suite, or compile the [lib/src/test/resources/src/c/test.c](https://github.com/Lzhiyong/kotlin-tree-sitter/blob/main/lib/src/test/resources/src/c/test.c) to run the native test.

This project is under development, there may be some bugs, if you know how to solve or have a better solution, please submit a pull request.

#### Usage

**1. use the tree-sitter to parse some string**
```kotlin

val lines = mutableListOf<String>(
    "#include <stdio.h>\n",
    "\n",
    "int main() {\n",
    "\tprintf(\"tree-sitter\\n\");\n",
    "\treturn 0;\n",
    "}\n"
 )

// the parse text
val source = lines.joinToString("")

// create parser
val parser = TSParser()
// language c
val language = TSLanguage.C
parser.setLanguage(language)

// start parse
val tree = parser.parse(source)

println(tree.rootNode)

tree.close()
parser.close()

```

**2. use the tree-sitter to parse text from a custom data struct, like `repo` or `piece-table` etc**
```kotlin
val lines = mutableListOf<String>(
    "#include <stdio.h>\n",
    "\n",
    "int main() {\n",
    "\tprintf(\"tree-sitter\\n\");\n",
    "\treturn 0;\n",
    "}\n"
 )

// create parser
val parser = TSParser()
// language c
val language = TSLanguage.C
parser.setLanguage(language)

/*
parser.setLogger{ type, message ->
    println("$type: -> $message")
}
*/

// start parse
val tree = parser.parse(callback = { byteIndex, point ->
    if (point.row >= lines.size) {
        return@parse String().toByteArray(Charsets.UTF_16LE)
    } else {
        return@parse lines[point.row].substring(point.column / 2).toByteArray(Charsets.UTF_16LE)
    }
})

println(tree.rootNode)

tree.close()
parser.close()

```

**3. edit the syntax tree**
```kotlin
// change before
var before = mutableListOf<String>(
    "#include <stdio.h>\n",
    "\n",
    "int main() {\n",
    "\tprintf(\"tree-sitter\\n\");\n",
    "\treturn 0;\n",
    "}\n"
)

// change after        
var after = mutableListOf<String>(
    "#include <stdio.h>\n",
    "int xxx = 100;\n",
    "int main() {\n",
    "\tprintf(\"tree-sitter\\n\");\n",
    "\treturn 0;\n",
    "}\n"
)

// create parser
val parser = TSParser()
val language = TSLanguage.C
parser.setLanguage(language)
// old tree
var tree = parser.parse(before.joinToString(""))
        
tree.edit(TSInputEdit(
    startByte = 19,
    oldEndByte = 20,
    newEndByte = 34,
    startPoint = TSPoint(1, 0),
    oldEndPoint = TSPoint(1, 1),
    newEndPoint = TSPoint(1, 15)
))
        
// new tree
tree = parser.parse(after.joinToString(""), tree)

println(tree.rootNode)

tree.close()
parser.close()

```

**4. query the syntax tree**
```kotlin
// some code
// ...

val query = TSQuery(language, expression)
val cursor = TSQueryCursor()
cursor.exec(query, tree.rootNode)

var match = cursor.nextMatch()

while (match != null) {
    val node = match.captures[0].node
    print("${source.substring(node.startByte / 2, node.endByte / 2)} [${node.startByte / 2}, ${node.endByte / 2}]  -> ")
    println(query.captureNameForId(match.captures[0].index))

    query.predicatesForPattern(match.patternIndex).forEach {
        when (it.type) {
            TSQueryPredicateStepType.CAPTURE -> println("capture: ${query.captureNameForId(it.id)}")
            TSQueryPredicateStepType.STRING -> println("value: ${query.stringValueForId(it.id)}")
            TSQueryPredicateStepType.DOWN -> println()
        }
    }

    match = cursor.nextMatch()
}


```

****

#### parse output
```txt
translation_unit [0, 72] 
  preproc_include [0, 20]                                
    path: system_lib_string [9, 18]                    
  function_definition [20, 71]                           
    type: primitive_type [20, 23]                        
    declarator: function_declarator [24, 30]               
      declarator: identifier [24, 28]                      
      parameters: parameter_list [28, 30]                
    body: compound_statement [31, 71]                      
      expression_statement [34, 58]                          
        call_expression [34, 57]                               
          function: identifier [34, 40]                        
          arguments: argument_list [40, 57]                      
            string_literal [41, 56]                                
              escape_sequence [53, 55]                     
      return_statement [60, 69]                              
        number_literal [67, 68]

```

#### query output
```txt
#include [0, 8]  -> include                          
<stdio.h> [9, 18]  -> string                         
int [20, 23]  -> type                                
main [24, 28]  -> variable                           
main [24, 28]  -> constant                           
value: lua-match?                                    
capture: constant                                    
value: ^[A-Z][A-Z0-9_]+$                                                                                  
main [24, 28]  -> function                           
( [28, 29]  -> punctuation.bracket                   
) [29, 30]  -> punctuation.bracket                   
{ [31, 32]  -> punctuation.bracket                   
printf [34, 40]  -> variable                         
printf [34, 40]  -> constant                         
value: lua-match?                                    
capture: constant                                    
value: ^[A-Z][A-Z0-9_]+$                                                                                  
printf [34, 40]  -> function                         
( [40, 41]  -> punctuation.bracket                   
"tree-sitter\n" [41, 56]  -> string                  
\n [53, 55]  -> string.escape                        
) [56, 57]  -> punctuation.bracket                   
; [57, 58]  -> punctuation.delimiter                 
return [60, 66]  -> keyword.return                   
0 [67, 68]  -> number                                
; [68, 69]  -> punctuation.delimiter                 
} [70, 71]  -> punctuation.bracket

```

****
