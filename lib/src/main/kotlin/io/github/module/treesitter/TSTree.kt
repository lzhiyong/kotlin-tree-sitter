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

class TSTree : Pointer(), Closeable {

    val rootNode: TSNode
        get() = TreeSitter.getRootNode(this.pointer)
    
    fun edit(input: TSInputEdit) {
        TreeSitter.editTree(this.pointer, input)
    }
    
    fun getChangedRanges(oldTree: TSTree?): Array<TSRange> {
        return TreeSitter.getTreeChangedRanges(oldTree?.pointer ?: 0L, this.pointer)
    }
    
    fun getLanguage(): Long {
        return TreeSitter.getTreeLanguage(this.pointer)
    }

    fun printGraph(pathname: String) {
        TreeSitter.treeDotGraph(this.pointer, pathname)
    }
    
    override fun close() {
        TreeSitter.deleteTree(this.pointer)
    }
}

