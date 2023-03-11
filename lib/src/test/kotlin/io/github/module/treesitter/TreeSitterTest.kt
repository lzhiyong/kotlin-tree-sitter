/*
 * Copyright © 2023 Github Lzhiyong
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package io.github.module.treesitter

import java.io.BufferedReader
import java.io.File
import java.nio.ByteBuffer

import kotlin.text.Charsets
import kotlin.system.*
import kotlin.test.*

class TreeSitterTest {

    fun indent(count: Int) {
        for (i in 0..count * 2) {
            // space ascii 32
            print(32.toChar())
        }
    }

    fun traverse(node: TSNode) {
        val cursor = node.walk()
        var visited = false
        var count = 0
        while (true) {
            if (visited) {
                if (cursor.gotoNextSibling()) {
                    visited = false
                } else if (cursor.gotoParent()) {
                    visited = true
                    count--
                } else {
                    break
                }
            } else {
                val node = cursor.getCurrNode()
                if (node.isNamed) {
                    indent(count)
                    cursor.getCurrFieldName()?.let { print("$it: ") }
                    println(
                        "${node.type} " +
                        "[${node.startPoint.row}, ${node.startPoint.column / 2}] - " +
                        "[${node.endPoint.row}, ${node.endPoint.column / 2}]"
                    )
                }

                if (cursor.gotoFirstChild()) {
                    visited = false
                    count++
                } else {
                    visited = true
                }
            }
        }

        cursor.close()
    }

    fun queries(
        rootNode: TSNode,
        language: TSLanguage,
        source: String,
        expression: String,
    ) {
        /*
        val query = TSQuery(language, expression) { offset, type ->
            println("$offset -> $type")
        }
        */      
        val query = TSQuery(language, expression)
        val cursor = TSQueryCursor()
        cursor.exec(query, rootNode)
        //cursor.setRange(0, 0, 1, 0)
       
        var match: TSQueryMatch? = null
        var prevNode = rootNode
        
        while ({match = cursor.nextMatch(); match}() != null) {
            val node = match!!.captures[0].node
            if(node != prevNode) {
            prevNode = node
            val scopeName = query.captureNameForId(match!!.captures[0].index)
            val text = source.substring(node.startByte / 2, node.endByte / 2)
            
            println(
                "$text -> $scopeName " +
                "[${node.startPoint.row}, ${node.startPoint.column / 2}] - " + 
                "[${node.endPoint.row}, ${node.endPoint.column / 2}]"
            )
            
            query.predicatesForPattern(match!!.patternIndex).forEach {
                when (it.type) {
                    TSQueryPredicateStepType.CAPTURE -> println("capture: ${query.captureNameForId(it.id)}")
                    TSQueryPredicateStepType.STRING -> println("value: ${query.stringValueForId(it.id)}")
                    TSQueryPredicateStepType.DOWN -> println("Done")
                }
            }
            }
        }
        
        println()
        println("patternCount: ${query.patternCount}")
        println("captureCount: ${query.captureCount}")
        println("stringCount: ${query.stringCount}")
        
        cursor.close()
        query.close()
    }

    @Test fun parserParse() {
        var lines = mutableListOf<String>(
            "#include <stdio.h>\n",
            "\n",
            "int main(\n",
            "    int argc,\n",
            "    char **argv,\n",
            ") {\n",
            "    printf(\"tree-sitter\\n\");\n",
            "    return 0;\n",
            "}\n"
        )
       
        val stream = {}.javaClass.getResource("/queries/c/highlights.scm")?.openStream()
        var expression = stream?.bufferedReader()?.use(BufferedReader::readText) ?: ""

        val parser = TSParser()
        val language = TSLanguage.C
        parser.setLanguage(language)
        
        var tree: TSTree
        val time = measureTimeMillis {
            tree = parser.parse(callback = { byteIndex, point ->
                println("byteIndex: ${byteIndex / 2} -> $point")
                if (point.row >= lines.size) {
                    return@parse String().toByteArray(Charsets.UTF_16LE)
                } else {
                    return@parse lines[point.row].substring(point.column / 2).toByteArray(Charsets.UTF_16LE)
                }
            })
        }

        // traverse the syntax tree
        traverse(tree.rootNode)
        println("child: ${tree.rootNode.getChildCount()}")
        println("parser parse cost time:$time ms")
        
        // query the syntax tree
        queries(tree.rootNode, language, lines.joinToString(""), expression)
        
        tree.close()
        parser.close()
    }

    @Test fun parseString() {
        var source = "#include <stdio.h>\n\nint main() {\n\tprintf(\"tree-sitter\\n\");\n\treturn 0;\n}\n"
       
        val stream = {}.javaClass.getResource("/queries/c/highlights.scm")?.openStream()
        var expression = stream?.bufferedReader()?.use(BufferedReader::readText) ?: ""
        
        val parser = TSParser()
        val language = TSLanguage.C
        parser.setLanguage(language)

        var tree: TSTree 
        val time = measureTimeMillis {
            tree = parser.parse(source)
        }
        
        // traverse the syntax tree
        traverse(tree.rootNode)
        println("child: ${tree.rootNode.getChildCount()}")
        println("parse string cost time:$time ms")

        // query the syntax tree
        queries(tree.rootNode, language, source, expression)
        
        tree.close()
        parser.close()
    }
    
    @Test fun editTree() {
        
        var before = mutableListOf<String>(
            "#include <stdio.h>\n",
            "\n",
            "int main() {\n",
            "\tprintf(\"tree-sitter\\n\");\n",
            "\treturn 0;\n",
            "}\n"
        )
        
        var after = mutableListOf<String>(
            "#include <stdio.h>\n",
            "int xxx = 100;\n",
            "int main() {\n",
            "\tprintf(\"tree-sitter\\n\");\n",
            "\treturn 0;\n",
            "}\n"
        )
        
        val parser = TSParser()
        val language = TSLanguage.C
        parser.setLanguage(language)
        // old tree
        var tree = parser.parse(before.joinToString(""))
        
        val time = measureTimeMillis {
            tree.edit(TSInputEdit(
                startByte = 19,
                oldEndByte = 20,
                newEndByte = 34,
                startPoint = TSPoint(1, 0),
                oldEndPoint = TSPoint(1, 1),
                newEndPoint = TSPoint(1, 15)
            ))
        }
        
        // new tree
        tree = parser.parse(after.joinToString(""), tree)
        // traverse the syntax tree
        traverse(tree.rootNode)
        println("child: ${tree.rootNode.getChildCount()}")
        println("edit tree cost time:$time ms")
        
        parser.setCancellationFlag(true)
        assertEquals(parser.getCancellationFlag(), true)
        
        parser.setCancellationFlag(false)
        assertEquals(parser.getCancellationFlag(), false)
        
        parser.setCancellationFlag(true)
        assertEquals(parser.getCancellationFlag(), true)
        
        tree.close()
        parser.close()
    }
}

