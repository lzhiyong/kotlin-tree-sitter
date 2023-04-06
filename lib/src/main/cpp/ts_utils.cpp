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

#include <regex>
#include <string>

#include "ts_utils.h"

#ifdef __cplusplus
extern "C" {
#endif

// declare external global variables
extern jclass javaTSNodeClass;
extern jclass javaTSPointClass;

// java TSNode
jobject javaNode(JNIEnv *env, const TSNode *node) {
    jmethodID constructor = env->GetMethodID(javaTSNodeClass, "<init>", "([IJJ)V");
    // size default is 4
    jint size = sizeof(node->context) / sizeof(node->context[0]);
    jintArray javaArray = env->NewIntArray(size);
    env->SetIntArrayRegion(javaArray, 0, size, (jint*)node->context);
    
    return env->NewObject(
        javaTSNodeClass, 
        constructor, 
        javaArray,
        reinterpret_cast<jlong>(node->id),
        reinterpret_cast<jlong>(node->tree)
    );
}

// native TSNode
TSNode nativeNode(JNIEnv *env, const jobject nodeObject) {
    jfieldID context = env->GetFieldID(javaTSNodeClass, "context", "[I");
    jfieldID id = env->GetFieldID(javaTSNodeClass, "id", "J");
    jfieldID tree = env->GetFieldID(javaTSNodeClass, "tree", "J");
    
    jintArray array = static_cast<jintArray>(env->GetObjectField(nodeObject, context));
    // jint size = env->GetArrayLength(array);
    uint32_t node_ctx[4];
    env->GetIntArrayRegion(array, 0, 4, (jint*)node_ctx);
    
    return TSNode {
        {node_ctx[0], node_ctx[1], node_ctx[2], node_ctx[3]},
        reinterpret_cast<const void*>(env->GetLongField(nodeObject, id)),
        reinterpret_cast<const TSTree*>(env->GetLongField(nodeObject, tree))
    };
}

// java TSPoint
jobject javaPoint(JNIEnv *env, const TSPoint *point) {
    jmethodID constructor = env->GetMethodID(javaTSPointClass, "<init>", "(II)V");
    return env->NewObject(
        javaTSPointClass, 
        constructor,
        point->row,
        point->column
    );
}

// native TSPoint
TSPoint nativePoint(JNIEnv *env, const jobject pointObject) {
    jfieldID row = env->GetFieldID(javaTSPointClass, "row", "I");
    jfieldID column = env->GetFieldID(javaTSPointClass, "column", "I");
    return TSPoint {
        static_cast<uint32_t>(env->GetIntField(pointObject, row)),
        static_cast<uint32_t>(env->GetIntField(pointObject, column))
    };
}

// get lambda callable object
jmethodID getMethod(JNIEnv *env, const jobject object, const char *signature) {
    jclass clazz = env->GetObjectClass(object);
    jmethodID getClass = env->GetMethodID(clazz, "getClass", "()Ljava/lang/Class;");
    jobject callable = env->CallObjectMethod(object, getClass);
    
    jclass clazz2 = env->GetObjectClass(callable);
    jmethodID getName = env->GetMethodID(clazz2, "getName", "()Ljava/lang/String;");
    
    jstring jstrName = (jstring)env->CallObjectMethod(callable, getName);
    const char *className = env->GetStringUTFChars(jstrName, nullptr);                
    
    std::regex pattern("\\.");  
    jclass clazz3 = env->FindClass(regex_replace(className, pattern, "/").c_str());
    
    env->ReleaseStringUTFChars(jstrName, className);
    
    return env->GetMethodID(clazz3, "invoke", signature);
}

#ifdef __cplusplus
}
#endif // __cplusplus

