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

internal val nullptr: Long = 0L

enum class TSInputEncoding { 
    UTF8, UTF16 
}

enum class TSLogType { 
    PARSE, LEX 
}

enum class TSQueryPredicateStepType {
    DOWN, CAPTURE, STRING
}

enum class TSQuantifier {
    ZERO, // must match the array initialization value
    ZERO_OR_ONE,
    ZERO_OR_MORE,
    ONE,
    ONE_OR_MORE
}

enum class TSQueryError {
  NONE,
  SYNTAX,
  NODE_TYPE,
  FIELD,
  CAPTURE,
  STRUCTURE,
  LANGUAGE
}


data class TSQueryPredicateStep(
    val type: TSQueryPredicateStepType, 
    val id: Int
)

data class TSQueryCapture(
    val node: TSNode,
    val index: Int
)

data class TSQueryMatch(
    val id: Int, 
    val patternIndex: Int,
    val captureCount: Int,
    val captures: Array<TSQueryCapture>
)

data class TSCapture(
    val match: TSQueryMatch,
    val captureIndex: Int
)

data class TSPoint(
    val row: Int = 0,
    val column: Int = 0
)

data class TSInputEdit(
    val startByte: Int,
    val oldEndByte: Int,
    val newEndByte: Int,
    val startPoint: TSPoint,
    val oldEndPoint: TSPoint,
    val newEndPoint: TSPoint
)

data class TSRange(
    val startPoint: TSPoint,
    val endPoint: TSPoint,
    val startByte: Int,
    val endByte: Int
)

// Pointer class
open class Pointer(var pointer: Long = nullptr)


internal object TreeSitter {
    init {
        System.loadLibrary("tree-sitter-jni")
    }
    
    // ================= parser ==================
    // ts_parser_new
    external fun newParser(): Long
    // ts_parser_delete
    external fun deleteParser(parser: Long)
    // ts_parser_reset
    external fun resetParser(parser: Long)
    // ts_parser_set_language
    external fun setParserLanguage(parser: Long, language: Long)
    // ts_parser_set_language
    external fun getParserLanguage(parser: Long): Long
    
    // ts_parser_parse_string
    external fun parseString(
        parser: Long, 
        oldTree: Long, 
        bytes: ByteArray, 
        encoding: TSInputEncoding
    ): Long
    
    // ts_parser_parse
    external fun parserParse(
        parser: Long, 
        oldTree: Long, 
        encoding: TSInputEncoding
    ): Long
   
    // ts_parser_set_timeout_micros
    external fun setParserTimeout(parser: Long, timeout: Long)
    // ts_parser_timeout_micros
    external fun getParserTimeout(parser: Long): Long
    // ts_parser_set_cancellation_flag
    external fun setParserCancellationFlag(parser: Long, flag: Boolean)
    // ts_parser_cancellation_flag
    external fun getParserCancellationFlag(parser: Long): Boolean
    // ts_parser_set_logger
    external fun setParserLogger(parser: Long)
    // ts_parser_set_included_ranges
    external fun setParserIncludedRanges(parser: Long, ranges: Array<IntArray>, length: Int)
    // ts_parser_print_dot_graphs
    external fun parserDotGraphs(parser: Long, file: String)
    
    // ================= tree ==================
    // ts_tree_delete
    external fun deleteTree(tree: Long)
    // ts_tree_root_node
    external fun getRootNode(tree: Long): TSNode
    // ts_tree_language
    external fun getTreeLanguage(tree: Long): Long
    // ts_tree_edit
    external fun editTree(tree: Long, input: TSInputEdit)
    // ts_tree_get_changed_ranges
    external fun getTreeChangedRanges(oldTree: Long, newTree: Long): Array<TSRange>
    // ts_tree_print_dot_graph
    external fun treeDotGraph(tree: Long, file: String)
    
    // ================= node ==================
    // ts_node_string
    external fun nodeString(node: TSNode): String
    // ts_node_start_byte
    external fun nodeStartByte(node: TSNode): Int
    // ts_node_end_byte
    external fun nodeEndByte(node: TSNode): Int
    // ts_node_start_point
    external fun nodeStartPoint(node: TSNode): TSPoint
    // ts_node_end_point
    external fun nodeEndPoint(node: TSNode): TSPoint
    // ts_node_type
    external fun nodeType(node: TSNode): String
    // ts_node_symbol
    external fun nodeSymbol(node: TSNode): Int
    // ts_node_child_count
    external fun nodeChildCount(node: TSNode): Int
    // ts_node_named_child_count
    external fun nodeNamedChildCount(node: TSNode): Int
    // ts_node_child
    external fun nodeChildAt(node: TSNode, index: Int): TSNode
     // ts_node_named_child
    external fun nodeNamedChildAt(node: TSNode, index: Int): TSNode
    // ts_node_next_sibling
    external fun nodePrevSibling(node: TSNode): TSNode
    // ts_node_next_sibling
    external fun nodeNextSibling(node: TSNode): TSNode
    // ts_node_next_named_sibling
    external fun nodePrevNamedSibling(node: TSNode): TSNode
    // ts_node_next_named_sibling
    external fun nodeNextNamedSibling(node: TSNode): TSNode
    // ts_node_child_by_field_name
    external fun nodeChildByFieldName(node: TSNode, name: String, length: Int): TSNode
    // ts_node_is_named
    external fun nodeIsNamed(node: TSNode): Boolean
    // ts_node_is_null
    external fun nodeIsNull(node: TSNode): Boolean
    // ts_node_has_error
    external fun nodeHasError(node: TSNode): Boolean
    // ts_node_eq
    external fun nodeEquals(a: TSNode, b: TSNode): Boolean
    
    // ================= tree cursor ==================
    // ts_tree_cursor_new
    external fun newTreeCursor(node: TSNode): Long
    // ts_tree_cursor_delete
    external fun deleteTreeCursor(cursor: Long)
    // ts_tree_cursor_goto_first_child
    external fun cursorGotoFirstChild(cursor: Long): Boolean
    // ts_tree_cursor_goto_next_sibling
    external fun cursorGotoNextSibling(cursor: Long): Boolean
    // ts_tree_cursor_goto_parent
    external fun cursorGotoParent(cursor: Long): Boolean
    // ts_tree_cursor_current_field_name
    external fun cursorCurrentFieldName(cursor: Long): String?
    // ts_tree_cursor_current_node
    external fun cursorCurrentNode(cursor: Long): TSNode
    
    // ================= query ==================
    // ts_query_new
    external fun newQuery(
        language: Long, 
        expression: String,
        onError: ((offset: Int, type: TSQueryError) -> Unit)?
    ): Long
    // ts_query_delete
    external fun deleteQuery(query: Long)
    // ts_query_pattern_count
    external fun queryPatternCount(query: Long): Int
    // ts_query_capture_count
    external fun queryCaptureCount(query: Long): Int
    // ts_query_string_count
    external fun queryStringCount(query: Long): Int
    // ts_query_start_byte_for_pattern
    external fun queryStartByteForPattern(query: Long, start: Int): Int
    // ts_query_predicates_for_pattern
    external fun queryPredicatesForPattern(query: Long, index: Int): Array<TSQueryPredicateStep>
    // ts_query_is_pattern_guaranteed_at_step
    external fun queryIsPatternGuaranteedAtStep(query: Long, offset: Int): Boolean
    // ts_query_capture_name_for_id
    external fun queryCaptureNameForId(query: Long, id: Int): String
    // ts_query_capture_quantifier_for_id
    external fun queryCaptureQuantifierForId(query: Long, patternId: Int, captureId: Int): TSQuantifier
    // ts_query_string_value_for_id
    external fun queryStringValueForId(query: Long, id: Int): String
    // ts_query_disable_capture
    external fun queryDisableCapture(query: Long, name: String?, id: Int)
    // ts_query_disable_pattern
    external fun queryDisablePattern(query: Long, id: Int)
    
    // ================= query cursor ==================
    // ts_query_cursor_new
    external fun newQueryCursor(): Long
    // ts_query_cursor_delete
    external fun deleteQueryCursor(cursor: Long)
    // ts_query_cursor_exec
    external fun queryCursorExec(cursor: Long, query: Long, node: TSNode)
    // ts_query_cursor_did_exceed_match_limit
    external fun queryCursorDidExceedMatchLimit(cursor: Long): Boolean
    // ts_query_cursor_match_limit
    external fun queryCursorMatchLimit(cursor: Long): Int
    // ts_query_cursor_set_match_limit
    external fun queryCursorSetMatchLimit(cursor: Long, limit: Int)
    // ts_query_cursor_get_name
    external fun queryCursorGetName(): String?
    // ts_query_cursor_set_byte_range
    external fun queryCursorSetByteRange(cursor: Long, startOffset: Int, endOffset: Int)
    // ts_query_cursor_set_point_range
    external fun queryCursorSetPointRange(cursor: Long, startPoint: TSPoint, endPoint: TSPoint)
    external fun queryCursorSetRange(cursor: Long, startRow: Int, startColumn: Int, endRow: Int, endColumn: Int)
    // ts_query_cursor_next_match
    external fun queryCusorNextMatch(cursor: Long): TSQueryMatch?
    // ts_query_cursor_remove_match
    external fun queryCursorRemoveMatch(cursor: Long, id: Int)
    // ts_query_cursor_next_capture
    external fun queryCusorNextCapture(cursor: Long): TSCapture?
    
    // ================= others ==================
    // languages
    external fun getSupportLanguage(name: String?): Long
}

