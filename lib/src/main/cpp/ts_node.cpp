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

#include <tree_sitter/api.h>

#include "ts_utils.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Get an S-expression representing the node as a string.
 *
 * This string is allocated with `malloc` and the caller is responsible for
 * freeing it using `free`.
 */
JNIEXPORT jstring JNICALL
Java_io_github_module_treesitter_TreeSitter_nodeString(JNIEnv* env, jobject thiz, jobject node) {
    char *token = ts_node_string(nativeNode(env, node));
    jstring text = env->NewStringUTF(token);
    free(token);
    return text;
}

/**
 * Get the node's start byte.
 */
JNIEXPORT jint JNICALL
Java_io_github_module_treesitter_TreeSitter_nodeStartByte(JNIEnv* env, jobject thiz, jobject node) {
    return ts_node_start_byte(nativeNode(env, node));
}

/**
 * Get the node's end byte.
 */
JNIEXPORT jint JNICALL
Java_io_github_module_treesitter_TreeSitter_nodeEndByte(JNIEnv* env, jobject thiz, jobject node) {
    return ts_node_end_byte(nativeNode(env, node));
}

/**
 * Get the node's start position in terms of rows and columns.
 */
JNIEXPORT jobject JNICALL
Java_io_github_module_treesitter_TreeSitter_nodeStartPoint(JNIEnv* env, jobject thiz, jobject node) {
    TSPoint point = ts_node_start_point(nativeNode(env, node));
    return javaPoint(env, &point);
}

/**
 * Get the node's end position in terms of rows and columns.
 */
JNIEXPORT jobject JNICALL
Java_io_github_module_treesitter_TreeSitter_nodeEndPoint(JNIEnv* env, jobject thiz, jobject node) {
    TSPoint point = ts_node_end_point(nativeNode(env, node));
    return javaPoint(env, &point);
}

/**
 * Get the node's type as a null-terminated string.
 */
JNIEXPORT jstring JNICALL
Java_io_github_module_treesitter_TreeSitter_nodeType(JNIEnv* env, jobject thiz, jobject node) {
    const char* type = ts_node_type(nativeNode(env, node));
    return env->NewStringUTF(type);
}

/**
 * Get the node's type as a numerical id.
 */
JNIEXPORT jint JNICALL
Java_io_github_module_treesitter_TreeSitter_nodeSymbol(JNIEnv* env, jobject thiz, jobject node) {
    return ts_node_symbol(nativeNode(env, node));
}

/**
 * Get the node's number of children.
 */
JNIEXPORT jint JNICALL
Java_io_github_module_treesitter_TreeSitter_nodeChildCount(JNIEnv* env, jobject thiz, jobject node) {
    return ts_node_child_count(nativeNode(env, node));
}

/**
 * Get the node's *named* child at the given index.
 *
 * See also `ts_node_is_named`.
 */
JNIEXPORT jint JNICALL
Java_io_github_module_treesitter_TreeSitter_nodeNamedChildCount(JNIEnv* env, jobject thiz, jobject node) {
    return ts_node_named_child_count(nativeNode(env, node));
}

/**
 * Get the node's child at the given index, where zero represents the first
 * child.
 */
JNIEXPORT jobject JNICALL
Java_io_github_module_treesitter_TreeSitter_nodeChildAt(JNIEnv* env, jobject thiz, 
                                                        jobject node, jint index) {
    TSNode tree_node = ts_node_child(nativeNode(env, node), index);
    return javaNode(env, &tree_node);
}

/**
 * Get the node's *named* child at the given index.
 *
 * See also `ts_node_is_named`.
 */
JNIEXPORT jobject JNICALL
Java_io_github_module_treesitter_TreeSitter_nodeNamedChildAt(JNIEnv* env, jobject thiz, 
                                                             jobject node, jint index) {
    TSNode tree_node = ts_node_named_child(nativeNode(env, node), index);
    return javaNode(env, &tree_node);
}

/**
 * Get the node's next / previous sibling.
 */
JNIEXPORT jobject JNICALL
Java_io_github_module_treesitter_TreeSitter_nodePrevSibling(JNIEnv* env, jobject thiz, jobject node) {
    TSNode tree_node = ts_node_prev_sibling(nativeNode(env, node));
    return javaNode(env, &tree_node);
}

JNIEXPORT jobject JNICALL
Java_io_github_module_treesitter_TreeSitter_nodeNextSibling(JNIEnv* env, jobject thiz, jobject node) {
    TSNode tree_node = ts_node_next_sibling(nativeNode(env, node));
    return javaNode(env, &tree_node);
}

/**
 * Get the node's next / previous *named* sibling.
 */
JNIEXPORT jobject JNICALL
Java_io_github_module_treesitter_TreeSitter_nodePrevNamedSibling(JNIEnv* env, jobject thiz, jobject node) {
    TSNode tree_node = ts_node_prev_named_sibling(nativeNode(env, node));
    return javaNode(env, &tree_node);
}

JNIEXPORT jobject JNICALL
Java_io_github_module_treesitter_TreeSitter_nodeNextNamedSibling(JNIEnv* env, jobject thiz, jobject node) {
    TSNode tree_node = ts_node_next_named_sibling(nativeNode(env, node));
    return javaNode(env, &tree_node);
}

/**
 * Get the node's child with the given field name.
 */
JNIEXPORT jobject JNICALL
Java_io_github_module_treesitter_TreeSitter_nodeChildByFieldName(JNIEnv* env, jobject thiz, 
                                                                 jobject node, jstring name, jint length) {
    const char *field_name = env->GetStringUTFChars(name, nullptr);
    TSNode tree_node = ts_node_child_by_field_name(nativeNode(env, node), field_name, length);
    env->ReleaseStringUTFChars(name, field_name);
    return javaNode(env, &tree_node);
}

/**
 * Check if the node is *named*. Named nodes correspond to named rules in the
 * grammar, whereas *anonymous* nodes correspond to string literals in the
 * grammar.
 */
JNIEXPORT jboolean JNICALL
Java_io_github_module_treesitter_TreeSitter_nodeIsNamed(JNIEnv* env, jobject thiz, jobject node) {
    return ts_node_is_named(nativeNode(env, node));
}

/**
 * Check if the node is null. Functions like `ts_node_child` and
 * `ts_node_next_sibling` will return a null node to indicate that no such node
 * was found.
 */
JNIEXPORT jboolean JNICALL
Java_io_github_module_treesitter_TreeSitter_nodeIsNull(JNIEnv* env, jobject thiz, jobject node) {
    return ts_node_is_null(nativeNode(env, node));
}

JNIEXPORT jboolean JNICALL
Java_io_github_module_treesitter_TreeSitter_nodeHasError(JNIEnv* env, jobject thiz, jobject node) {
    return ts_node_has_error(nativeNode(env, node));
}

JNIEXPORT jboolean JNICALL
Java_io_github_module_treesitter_TreeSitter_nodeEquals(JNIEnv* env, jobject thiz, jobject a, jobject b) {
    return ts_node_eq(
        nativeNode(env, a), 
        nativeNode(env, b)
    );
}

#ifdef __cplusplus
}
#endif // __cplusplus

