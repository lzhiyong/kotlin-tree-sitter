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

// define global variables
jclass javaTSCaptureClass = nullptr;
jclass javaTSQueryCaptureClass = nullptr;
jclass javaTSQueryMatchClass = nullptr;

// java TSQueryCapture array
jobjectArray javaQueryCaptures(JNIEnv *env, const TSQueryCapture *captures, const uint32_t count) {
    jmethodID constructor = env->GetMethodID(
        javaTSQueryCaptureClass, 
        "<init>", 
        "(Lio/github/module/treesitter/TSNode;I)V"
    );
    
    jobjectArray captureArray = env->NewObjectArray(count, javaTSQueryCaptureClass, nullptr);
    
    for(int i=0; i < count; ++i) {
        jobject nodeObject = javaNode(env, &captures[i].node);
        jobject captureObject = env->NewObject(
            javaTSQueryCaptureClass, 
            constructor,
            nodeObject,
            captures[i].index
        );
        env->SetObjectArrayElement(captureArray, i, captureObject);
    }
    
    return captureArray;
}

// java TSQueryMatch
jobject javaQueryMatch(JNIEnv *env, const TSQueryMatch *match) {
    jmethodID constructor = env->GetMethodID(
        javaTSQueryMatchClass, 
        "<init>", 
        "(III[Lio/github/module/treesitter/TSQueryCapture;)V"
    );
    
    jobjectArray captureArray = javaQueryCaptures(env, match->captures, match->capture_count);
    
    return env->NewObject(
        javaTSQueryMatchClass, 
        constructor,
        match->id,
        match->pattern_index,
        match->capture_count,
        captureArray
    );
}

/**
 * Create a new cursor for executing a given query.
 *
 * The cursor stores the state that is needed to iteratively search
 * for matches. To use the query cursor, first call `ts_query_cursor_exec`
 * to start running a given query on a given syntax node. Then, there are
 * two options for consuming the results of the query:
 * 1. Repeatedly call `ts_query_cursor_next_match` to iterate over all of the
 *    *matches* in the order that they were found. Each match contains the
 *    index of the pattern that matched, and an array of captures. Because
 *    multiple patterns can match the same set of nodes, one match may contain
 *    captures that appear *before* some of the captures from a previous match.
 * 2. Repeatedly call `ts_query_cursor_next_capture` to iterate over all of the
 *    individual *captures* in the order that they appear. This is useful if
 *    don't care about which pattern matched, and just want a single ordered
 *    sequence of captures.
 *
 * If you don't care about consuming all of the results, you can stop calling
 * `ts_query_cursor_next_match` or `ts_query_cursor_next_capture` at any point.
 *  You can then start executing another query on another node by calling
 *  `ts_query_cursor_exec` again.
 */
JNIEXPORT jlong JNICALL
Java_io_github_module_treesitter_TreeSitter_newQueryCursor(JNIEnv* env, jobject thiz) {
    return reinterpret_cast<jlong>(ts_query_cursor_new());
}

/**
 * Delete a query cursor, freeing all of the memory that it used.
 */
JNIEXPORT void JNICALL
Java_io_github_module_treesitter_TreeSitter_deleteQueryCursor(JNIEnv* env, jobject thiz, jlong cursor) {  
    ts_query_cursor_delete(reinterpret_cast<TSQueryCursor*>(cursor));
}

/**
 * Start running a given query on a given node.
 */
JNIEXPORT void JNICALL
Java_io_github_module_treesitter_TreeSitter_queryCursorExec(JNIEnv* env, jobject thiz, 
                                                            jlong cursor, jlong query, jobject node) {
    ts_query_cursor_exec(
        reinterpret_cast<TSQueryCursor*>(cursor),
        reinterpret_cast<TSQuery*>(query),
        nativeNode(env, node)
    );
}

/**
 * Manage the maximum number of in-progress matches allowed by this query
 * cursor.
 *
 * Query cursors have an optional maximum capacity for storing lists of
 * in-progress captures. If this capacity is exceeded, then the
 * earliest-starting match will silently be dropped to make room for further
 * matches. This maximum capacity is optional — by default, query cursors allow
 * any number of pending matches, dynamically allocating new space for them as
 * needed as the query is executed.
 */
JNIEXPORT jboolean JNICALL
Java_io_github_module_treesitter_TreeSitter_queryCursorDidExceedMatchLimit(JNIEnv* env, jobject thiz, jlong cursor) {
    return ts_query_cursor_did_exceed_match_limit(reinterpret_cast<TSQueryCursor*>(cursor));
}

JNIEXPORT jint JNICALL
Java_io_github_module_treesitter_TreeSitter_queryCursorMatchLimit(JNIEnv* env, jobject thiz, jlong cursor) {
    return ts_query_cursor_match_limit(reinterpret_cast<TSQueryCursor*>(cursor));
}

JNIEXPORT void JNICALL
Java_io_github_module_treesitter_TreeSitter_queryCursorSetMatchLimit(JNIEnv* env, jobject thiz, 
                                                                     jlong cursor, jint limit) {
    ts_query_cursor_set_match_limit(reinterpret_cast<TSQueryCursor*>(cursor), limit);
}

JNIEXPORT void JNICALL
Java_io_github_module_treesitter_TreeSitter_queryCursorSetByteRange(JNIEnv* env, jobject thiz, jlong cursor, 
                                                                    jint startOffset, jint endOffset) {
    ts_query_cursor_set_byte_range(reinterpret_cast<TSQueryCursor*>(cursor), startOffset, endOffset);
}

JNIEXPORT void JNICALL
Java_io_github_module_treesitter_TreeSitter_queryCursorSetPointRange(JNIEnv* env, jobject thiz, jlong cursor, 
                                                                     jobject startPoint, jobject endPoint) {
    ts_query_cursor_set_point_range(
        reinterpret_cast<TSQueryCursor*>(cursor), 
        nativePoint(env, startPoint),
        nativePoint(env, endPoint)
    );
}

JNIEXPORT void JNICALL
Java_io_github_module_treesitter_TreeSitter_queryCursorSetRange(JNIEnv* env, jobject thiz, jlong cursor,
                                                                jint startRow, jint startColumn,
                                                                jint endRow, jint endColumn) {
    ts_query_cursor_set_point_range(
        reinterpret_cast<TSQueryCursor*>(cursor), 
        {
            static_cast<uint32_t>(startRow), 
            static_cast<uint32_t>(startColumn)
        },
        {
            static_cast<uint32_t>(endRow), 
            static_cast<uint32_t>(endColumn)
        }
    );
}

/**
 * Advance to the next capture of the currently running query.
 *
 * If there is a capture, write its match to `*match` and its index within
 * the matche's capture list to `*capture_index`. Otherwise, return `false`.
 */
JNIEXPORT jobject JNICALL
Java_io_github_module_treesitter_TreeSitter_queryCusorNextMatch(JNIEnv* env, jobject thiz, jlong cursor) {
    TSQueryMatch query_match;
    if(ts_query_cursor_next_match(reinterpret_cast<TSQueryCursor*>(cursor), &query_match))
        return javaQueryMatch(env, &query_match);
    else 
        return nullptr;
}

JNIEXPORT void JNICALL
Java_io_github_module_treesitter_TreeSitter_queryCursorRemoveMatch(JNIEnv* env, jobject thiz, jlong cursor, jint id) {
    ts_query_cursor_remove_match(reinterpret_cast<TSQueryCursor*>(cursor), id);
}

JNIEXPORT jobject JNICALL
Java_io_github_module_treesitter_TreeSitter_queryCusorNextCapture(JNIEnv* env, jobject thiz, jlong cursor) {
    TSQueryMatch query_match;
    uint32_t capture_index;
    
    if(ts_query_cursor_next_capture(reinterpret_cast<TSQueryCursor*>(cursor), &query_match, &capture_index)) {
        jobject match = javaQueryMatch(env, &query_match);
        
        jmethodID constructor = env->GetMethodID(
            javaTSCaptureClass, 
            "<init>", 
            "(Lio/github/module/treesitter/TSQueryMatch;I)V"
        );
        
        return env->NewObject(
            javaTSCaptureClass, 
            constructor,
            query_match,
            capture_index
        );
    }
    
    return nullptr;
}


#ifdef __cplusplus
}
#endif // __cplusplus

