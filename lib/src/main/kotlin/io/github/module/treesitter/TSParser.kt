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

import java.io.Closeable

import kotlin.text.Charsets

class TSParser : Pointer(), Closeable {
    
    internal companion object {
        // callbacks
        private lateinit var function1: (Int, TSPoint) -> ByteArray
        private lateinit var function2: (TSLogType, String) -> Unit
        
        // see treesitter parser parse function
        // this method call by JNI
        @JvmStatic
        fun read(byteIndex: Int, point: TSPoint): ByteArray {
            return function1(byteIndex, point)
        }
        
        // this method call by JNI
        @JvmStatic
        fun logger(type: TSLogType, message: String) {
            return function2(type, message)
        }
    }
    
    init {
        // init native TSParser pointer
        this.pointer = TreeSitter.newParser()
    }
    
    fun setLanguage(language: TSLanguage) {
        TreeSitter.setParserLanguage(this.pointer, language.pointer)
    }
    
    fun getLanguage(): Long {
        return TreeSitter.getParserLanguage(this.pointer)
    }
    
    fun setTimeout(timeout: Long) {
        TreeSitter.setParserTimeout(this.pointer, timeout)
    }
    
    fun getTimeout(): Long {
        return TreeSitter.getParserTimeout(this.pointer)
    }
    
    fun setLogger(callback: (TSLogType, String) -> Unit) {
        // set callback
        TSParser.function2 = callback
        TreeSitter.setParserLogger(this.pointer)
    }
    
    fun cancel(flag: Boolean) {
        TreeSitter.setParserCancellationFlag(this.pointer, flag)
    }
    
    fun isCancelled(): Boolean {
        return TreeSitter.getParserCancellationFlag(this.pointer)
    }
    
    // parse string
    fun parse(
        text: String, 
        oldTree: TSTree? = null,
        encoding: TSInputEncoding = TSInputEncoding.UTF16
    ): TSTree {
        // specify the encoding of bytes
        val bytes = when(encoding) {
            TSInputEncoding.UTF8 -> text.toByteArray()
            else -> text.toByteArray(Charsets.UTF_16LE)
        }
    
        return when(oldTree) {
            null -> {
                TSTree().also{
                    it.pointer = TreeSitter.parseString(this.pointer, nullptr, bytes, encoding)
                }
            }
            else -> {
                oldTree.pointer = TreeSitter.parseString(this.pointer, oldTree.pointer, bytes, encoding)
                oldTree
            }
        }
    }
    
    // parser parse
    fun parse(
        callback: (byteIndex: Int, position: TSPoint) -> ByteArray, 
        oldTree: TSTree? = null,
        encoding: TSInputEncoding = TSInputEncoding.UTF16
    ): TSTree {
        // set callback
        TSParser.function1 = callback
        return when(oldTree) {
            null -> {
                TSTree().also {
                    it.pointer = TreeSitter.parserParse(this.pointer, nullptr, encoding)
                }
            }
            else -> {
                oldTree.pointer = TreeSitter.parserParse(this.pointer, oldTree.pointer, encoding)
                oldTree
            }
        }
    }
    
    fun reset() {
        TreeSitter.resetParser(this.pointer)
    }
    
    fun printGraph(pathname: String) {
        TreeSitter.parserDotGraphs(this.pointer, pathname)
    }
    
    override fun close() {
        TreeSitter.deleteParser(this.pointer)
    }
}

