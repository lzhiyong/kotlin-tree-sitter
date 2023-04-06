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

class TSQueryCursor : Pointer(), Closeable {
    
    init {
        // init native TSQueryCursor pointer
        this.pointer = TreeSitter.newQueryCursor()
    }
    
    fun didExceedMatchLimit() = TreeSitter.queryCursorDidExceedMatchLimit(this.pointer)
    
    fun exec(query: TSQuery, node: TSNode) {
        TreeSitter.queryCursorExec(this.pointer, query.pointer, node)
    }
    
    fun setMatchLimit(limit: Int) {
        TreeSitter.queryCursorSetMatchLimit(this.pointer, limit)
    }
    
    // offset[start, end]
    fun setByteRange(startOffset: Int, endOffset: Int) {
        TreeSitter.queryCursorSetByteRange(this.pointer, startOffset, endOffset)
    }
    
    // Point[startPoint, endPoint] 
    fun setPointRange(startPoint: TSPoint, endPoint: TSPoint) {
        TreeSitter.queryCursorSetPointRange(this.pointer, startPoint, endPoint)
    }
    
    // Range[startRow, startColumn, endRow, endColumn]
    fun setRange(startRow: Int, startColumn: Int, endRow: Int, endColumn: Int) {
        TreeSitter.queryCursorSetRange(this.pointer, startRow, startColumn, endRow, endColumn)
    }
    
    fun nextMatch(): TSQueryMatch? {
        return TreeSitter.queryCusorNextMatch(this.pointer)
    }
    
    fun nextCapture(): TSCapture? {
        return TreeSitter.queryCusorNextCapture(this.pointer)
    }
    
    fun removeMatch(id: Int) {
        TreeSitter.queryCursorRemoveMatch(this.pointer, id)
    }
    
    override fun close() {
        TreeSitter.deleteQueryCursor(this.pointer)
    }
}

