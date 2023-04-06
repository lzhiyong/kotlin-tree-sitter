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

class TSQuery(
    language: TSLanguage, 
    expression: String,
    onError: ((offset: Int, type: TSQueryError) -> Unit)? = null
) : Pointer(), Closeable {
    
    init {
        // init native TSQuery pointer
        this.pointer = TreeSitter.newQuery(language.pointer, expression, onError)
    }
    
    val patternCount: Int
        get() = TreeSitter.queryPatternCount(this.pointer)
    
    val captureCount: Int
        get() = TreeSitter.queryCaptureCount(this.pointer)
    
    val stringCount: Int
        get() = TreeSitter.queryStringCount(this.pointer)
    
    fun startByteForPattern(start: Int): Int {
        return TreeSitter.queryStartByteForPattern(this.pointer, start)
    }
    
    // pattern index
    fun predicatesForPattern(index: Int): Array<TSQueryPredicateStep> {
        return TreeSitter.queryPredicatesForPattern(this.pointer, index)
    }
    
    fun isPatternGuaranteedAtStep(offset: Int): Boolean {
        return TreeSitter.queryIsPatternGuaranteedAtStep(this.pointer, offset)
    }
    
    fun captureNameForId(id: Int): String {
        return TreeSitter.queryCaptureNameForId(this.pointer, id)
    }
    
    fun captureQuantifierForId(patternId: Int, captureId: Int): TSQuantifier {
        return TreeSitter.queryCaptureQuantifierForId(this.pointer, patternId, captureId);
    }
    
    fun stringValueForId(id: Int): String {
        return TreeSitter.queryStringValueForId(this.pointer, id)
    }
    
    fun disableCapture(name: String?, id: Int) {
        TreeSitter.queryDisableCapture(this.pointer, name, id)
    }
    
    fun disablePattern(id: Int) {
        TreeSitter.queryDisablePattern(this.pointer, id)
    }
    
    override fun close() {
        TreeSitter.deleteQuery(this.pointer)
    }
}

