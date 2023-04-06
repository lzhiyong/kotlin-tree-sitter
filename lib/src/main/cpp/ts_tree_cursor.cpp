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
 * Create a new tree cursor starting from the given node.
 *
 * A tree cursor allows you to walk a syntax tree more efficiently than is
 * possible using the `TSNode` functions. It is a mutable object that is always
 * on a certain syntax node, and can be moved imperatively to different nodes.
 */
JNIEXPORT jlong JNICALL
Java_io_github_module_treesitter_TreeSitter_newTreeCursor(JNIEnv* env, jobject thiz, jobject node) {
    return reinterpret_cast<jlong>(
        new TSTreeCursor(ts_tree_cursor_new(nativeNode(env, node)))
    );
}

/**
 * Delete a tree cursor, freeing all of the memory that it used.
 */
JNIEXPORT void JNICALL
Java_io_github_module_treesitter_TreeSitter_deleteTreeCursor(JNIEnv* env, jobject thiz, jlong cursor) {
    ts_tree_cursor_delete(reinterpret_cast<TSTreeCursor*>(cursor));
}

/**
 * Get the tree cursor's current node.
 */
JNIEXPORT jobject JNICALL
Java_io_github_module_treesitter_TreeSitter_cursorCurrentNode(JNIEnv* env, jobject thiz, jlong cursor) {
    TSNode node = ts_tree_cursor_current_node(
        reinterpret_cast<TSTreeCursor*>(cursor)
    );
    return javaNode(env, &node);
}

JNIEXPORT jstring JNICALL
Java_io_github_module_treesitter_TreeSitter_cursorCurrentFieldName(JNIEnv* env, jobject thiz, jlong cursor) {
    const char *name = ts_tree_cursor_current_field_name(
        reinterpret_cast<TSTreeCursor*>(cursor)
    );
    return env->NewStringUTF(name);
}

JNIEXPORT jboolean JNICALL
Java_io_github_module_treesitter_TreeSitter_cursorGotoFirstChild(JNIEnv* env, jobject thiz, jlong cursor) {
    return ts_tree_cursor_goto_first_child(
        reinterpret_cast<TSTreeCursor*>(cursor)
    );
}

JNIEXPORT jboolean JNICALL
Java_io_github_module_treesitter_TreeSitter_cursorGotoNextSibling(JNIEnv* env, jobject thiz, jlong cursor) {
    return ts_tree_cursor_goto_next_sibling(
        reinterpret_cast<TSTreeCursor*>(cursor)
    );
}

JNIEXPORT jboolean JNICALL
Java_io_github_module_treesitter_TreeSitter_cursorGotoParent(JNIEnv* env, jobject thiz, jlong cursor) {
    return ts_tree_cursor_goto_parent(
        reinterpret_cast<TSTreeCursor*>(cursor)
    );
}

#ifdef __cplusplus
}
#endif // __cplusplus

