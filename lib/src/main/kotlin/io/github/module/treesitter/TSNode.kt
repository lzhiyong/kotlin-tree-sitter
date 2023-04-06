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

data class TSNode(
     @JvmField val context: IntArray?,
     @JvmField val id: Long,
     @JvmField val tree: Long
) {
    
    val startByte: Int
        get() = TreeSitter.nodeStartByte(this)
        
    val endByte: Int
        get() = TreeSitter.nodeEndByte(this)
    
    val startPoint: TSPoint
        get() = TreeSitter.nodeStartPoint(this)
        
    val endPoint: TSPoint
        get() = TreeSitter.nodeEndPoint(this)
        
    val type: String
        get() = TreeSitter.nodeType(this)
    
    val symbol: Int
        get() = TreeSitter.nodeSymbol(this)
        
    fun isNamed() = TreeSitter.nodeIsNamed(this)
        
    fun isNull() = TreeSitter.nodeIsNull(this)
    
    fun hasError() = TreeSitter.nodeHasError(this)
    
    fun getChildCount(): Int {
        return TreeSitter.nodeChildCount(this)
    }
    
    fun getNamedChildCount(): Int {
        return TreeSitter.nodeNamedChildCount(this)
    }
    
    fun getPrevSibling(): TSNode {
        return TreeSitter.nodePrevSibling(this)
    }
    
    fun getNextSibling(): TSNode {
        return TreeSitter.nodeNextSibling(this)
    }
    
    fun getPrevNamedSibling(): TSNode {
        return TreeSitter.nodePrevNamedSibling(this)
    }
    
    fun getNextNamedSibling(): TSNode {
        return TreeSitter.nodeNextNamedSibling(this)
    }
    
    fun walk(): TSTreeCursor {
        val treeCursor = TSTreeCursor()
        treeCursor.pointer = TreeSitter.newTreeCursor(this)
        return treeCursor
    }

    fun childAt(index: Int): TSNode {
        return TreeSitter.nodeChildAt(this, index)
    }
    
    fun namedChildAt(index: Int): TSNode {
        return TreeSitter.nodeNamedChildAt(this, index)
    }
    
    fun childByFieldName(name: String): TSNode {
        return TreeSitter.nodeChildByFieldName(this, name, name.length)
    }
    
    override operator fun equals(other: Any?): Boolean = when {
        other === this -> true
        other !is TSNode -> false
        else -> other.id == id && other.tree == tree
    }
    
    override fun toString(): String {
        return TreeSitter.nodeString(this)
    }
}

