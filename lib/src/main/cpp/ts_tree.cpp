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


#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <tree_sitter/api.h>

#include "jni_helper.h"
#include "ts_utils.h"

#ifdef __cplusplus
extern "C" {
#endif

// define global variables
jclass javaTSRangeClass = nullptr;
jclass javaTSInputEditClass = nullptr;

/**
 * Delete the syntax tree, freeing all of the memory that it used.
 */
JNIEXPORT void JNICALL
Java_io_github_module_treesitter_TreeSitter_deleteTree(JNIEnv* env, jobject thiz, jlong tree) {
    ts_tree_delete(reinterpret_cast<TSTree*>(tree));
}

/**
 * Get the root node of the syntax tree.
 */
JNIEXPORT jobject JNICALL
Java_io_github_module_treesitter_TreeSitter_getRootNode(JNIEnv* env, jobject thiz, jlong tree) {
    TSNode tree_node = ts_tree_root_node(reinterpret_cast<TSTree*>(tree));
    return javaNode(env, &tree_node);
}

/**
 * Get the language that was used to parse the syntax tree.
 */
JNIEXPORT jlong JNICALL
Java_io_github_module_treesitter_TreeSitter_getTreeLanguage(JNIEnv* env, jobject thiz, jlong tree) {
    return reinterpret_cast<jlong>(ts_tree_language(reinterpret_cast<TSTree*>(tree)));
}

/**
 * Edit the syntax tree to keep it in sync with source code that has been
 * edited.
 *
 * You must describe the edit both in terms of byte offsets and in terms of
 * (row, column) coordinates.
 */
JNIEXPORT void JNICALL
Java_io_github_module_treesitter_TreeSitter_editTree(JNIEnv* env, jobject thiz, jlong tree, jobject inputEdit) {
    
    jfieldID startByte = env->GetFieldID(javaTSInputEditClass, "startByte", "I");
    jfieldID oldEndByte = env->GetFieldID(javaTSInputEditClass, "oldEndByte", "I");
    jfieldID newEndByte = env->GetFieldID(javaTSInputEditClass, "newEndByte", "I");
   
    jfieldID startPoint = env->GetFieldID(javaTSInputEditClass, "startPoint", "Lio/github/module/treesitter/TSPoint;");
    jfieldID oldEndPoint = env->GetFieldID(javaTSInputEditClass, "oldEndPoint", "Lio/github/module/treesitter/TSPoint;");
    jfieldID newEndPoint = env->GetFieldID(javaTSInputEditClass, "newEndPoint", "Lio/github/module/treesitter/TSPoint;");
   
    TSInputEdit tsInput {
        static_cast<uint32_t>(env->GetIntField(inputEdit, startByte)),
        static_cast<uint32_t>(env->GetIntField(inputEdit, oldEndByte)),
        static_cast<uint32_t>(env->GetIntField(inputEdit, newEndByte)),
        nativePoint(env, env->GetObjectField(inputEdit, startPoint)),
        nativePoint(env, env->GetObjectField(inputEdit, oldEndPoint)),
        nativePoint(env, env->GetObjectField(inputEdit, newEndPoint))
    };
    
    ts_tree_edit(reinterpret_cast<TSTree*>(tree), &tsInput);
}

/**
 * Compare an old edited syntax tree to a new syntax tree representing the same
 * document, returning an array of ranges whose syntactic structure has changed.
 *
 * For this to work correctly, the old syntax tree must have been edited such
 * that its ranges match up to the new tree. Generally, you'll want to call
 * this function right after calling one of the `ts_parser_parse` functions.
 * You need to pass the old tree that was passed to parse, as well as the new
 * tree that was returned from that function.
 *
 * The returned array is allocated using `malloc` and the caller is responsible
 * for freeing it using `free`. The length of the array will be written to the
 * given `length` pointer.
 */
JNIEXPORT jobjectArray JNICALL
Java_io_github_module_treesitter_TreeSitter_getTreeChangedRanges(JNIEnv* env, jobject thiz, 
                                                                 jlong oldTree, jlong newTree) {
    uint32_t length = 0;
    TSRange *ranges = ts_tree_get_changed_ranges(
        reinterpret_cast<TSTree*>(oldTree),
        reinterpret_cast<TSTree*>(newTree), 
        reinterpret_cast<uint32_t*>(&length)
    );
    
    jmethodID constructor = env->GetMethodID(
        javaTSRangeClass, 
        "<init>", 
        "(Lio/github/module/treesitter/TSPoint;Lio/github/module/treesitter/TSPoint;II)V"
    );
    
    jobjectArray rangeArray = env->NewObjectArray(length, javaTSRangeClass, nullptr);
    
    for(int i=0; i < length; ++i) {
        jobject rangeObject = env->NewObject(
            javaTSRangeClass,
            constructor,
            javaPoint(env, &ranges[i].start_point),
            javaPoint(env, &ranges[i].end_point),
            ranges[i].start_byte,
            ranges[i].end_byte
        );
        
        env->SetObjectArrayElement(rangeArray, i, rangeObject);
    }
    
    return rangeArray;
}

/**
 * Write a DOT graph describing the syntax tree to the given file.
 */
JNIEXPORT void JNICALL
Java_io_github_module_treesitter_TreeSitter_treeDotGraph(JNIEnv* env, jobject thiz, 
                                                         jlong tree, jstring pathname) {
    const char *path =env->GetStringUTFChars(pathname, nullptr); 
    int fp = open(path, O_CREAT|O_RDWR, 0666);
    if(fp < 0) 
        LOGE("Error: %s\n", strerror(errno));
    else 
        ts_tree_print_dot_graph(reinterpret_cast<TSTree*>(tree), fp);
    
    env->ReleaseStringUTFChars(pathname, path);
}

#ifdef __cplusplus
}
#endif // __cplusplus

