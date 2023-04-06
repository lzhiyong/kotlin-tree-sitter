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
#include <tree_sitter/api.h>

#include "jni_helper.h"
#include "ts_utils.h"

#ifdef __cplusplus
extern "C" {
#endif

// define global variables
jclass javaTSQuantifierClass = nullptr;
jclass javaTSQueryPredicateStepClass = nullptr;
jclass javaTSQueryPredicateStepTypeClass = nullptr;

/**
 * Create a new query from a string containing one or more S-expression
 * patterns. The query is associated with a particular language, and can
 * only be run on syntax nodes parsed with that language.
 *
 * If all of the given patterns are valid, this returns a `TSQuery`.
 * If a pattern is invalid, this returns `NULL`, and provides two pieces
 * of information about the problem:
 * 1. The byte offset of the error is written to the `error_offset` parameter.
 * 2. The type of error is written to the `error_type` parameter.
 */
JNIEXPORT jlong JNICALL
Java_io_github_module_treesitter_TreeSitter_newQuery(JNIEnv* env, jobject thiz, 
                                                     jlong language, jstring expression, jobject lambda) {
    uint32_t error_offset;
    TSQueryError error_type;
    
    const char *source = env->GetStringUTFChars(expression, nullptr);
    const TSQuery *query = ts_query_new(
        reinterpret_cast<TSLanguage*>(language),
        source,
        strlen(source),
        &error_offset,
        &error_type
    );
    
    if(lambda != nullptr) {
        jclass javaTSQueryErrorClass = env->FindClass("io/github/module/treesitter/TSQueryError");
        // callback
        jmethodID invoke = getMethod(env, lambda, "(ILio/github/module/treesitter/TSQueryError;)V");
    
        jfieldID field = nullptr;
    
        switch(error_type) {
            case TSQueryErrorNone:
                field = env->GetStaticFieldID(javaTSQueryErrorClass, "NONE", "Lio/github/module/treesitter/TSQueryError;");
                break;
            case TSQueryErrorSyntax:
                field = env->GetStaticFieldID(javaTSQueryErrorClass, "SYNTAX", "Lio/github/module/treesitter/TSQueryError;");
                break;
            case TSQueryErrorNodeType:
                field = env->GetStaticFieldID(javaTSQueryErrorClass, "NODE_TYPE", "Lio/github/module/treesitter/TSQueryError;");
                break;
            case TSQueryErrorField:
                field = env->GetStaticFieldID(javaTSQueryErrorClass, "FIELD", "Lio/github/module/treesitter/TSQueryError;");
                break;
            case TSQueryErrorCapture:
                field = env->GetStaticFieldID(javaTSQueryErrorClass, "CAPTURE", "Lio/github/module/treesitter/TSQueryError;");
                break;
            case TSQueryErrorStructure:
                field = env->GetStaticFieldID(javaTSQueryErrorClass, "STRUCTURE", "Lio/github/module/treesitter/TSQueryError;");
                break;
            case TSQueryErrorLanguage:
                field = env->GetStaticFieldID(javaTSQueryErrorClass, "LANGUAGE", "Lio/github/module/treesitter/TSQueryError;");
                break;
            default:
                LOGE("Error: Unknown field %d of TSQueryError class\n", error_type);
                break;
        }
    
        jobject typeObject = env->GetStaticObjectField(javaTSQueryErrorClass, field);
        // call onError
        env->CallVoidMethod(lambda, invoke, error_offset, typeObject);
        env->DeleteLocalRef(javaTSQueryErrorClass);
    }
    
    env->ReleaseStringUTFChars(expression, source);
    
    return reinterpret_cast<jlong>(query);
}

/**
 * Delete a query, freeing all of the memory that it used.
 */
JNIEXPORT void JNICALL
Java_io_github_module_treesitter_TreeSitter_deleteQuery(JNIEnv* env, jobject thiz, jlong query) {
    ts_query_delete(reinterpret_cast<TSQuery*>(query));
}

/**
 * Get the number of patterns, captures, or string literals in the query.
 */
JNIEXPORT jint JNICALL
Java_io_github_module_treesitter_TreeSitter_queryPatternCount(JNIEnv* env, jobject thiz, jlong query) {
    return ts_query_pattern_count(reinterpret_cast<TSQuery*>(query));
}

JNIEXPORT jint JNICALL
Java_io_github_module_treesitter_TreeSitter_queryCaptureCount(JNIEnv* env, jobject thiz, jlong query) {
    return ts_query_capture_count(reinterpret_cast<TSQuery*>(query));
}

JNIEXPORT jint JNICALL
Java_io_github_module_treesitter_TreeSitter_queryStringCount(JNIEnv* env, jobject thiz, jlong query) {
    return ts_query_string_count(reinterpret_cast<TSQuery*>(query));
}

/**
 * Get the byte offset where the given pattern starts in the query's source.
 *
 * This can be useful when combining queries by concatenating their source
 * code strings.
 */
JNIEXPORT jint JNICALL
Java_io_github_module_treesitter_TreeSitter_queryStartByteForPattern(JNIEnv* env, jobject thiz, 
                                                                     jlong query, jint startByte) {
    return ts_query_start_byte_for_pattern(reinterpret_cast<TSQuery*>(query), startByte);
}

/**
 * Get all of the predicates for the given pattern in the query.
 *
 * The predicates are represented as a single array of steps. There are three
 * types of steps in this array, which correspond to the three legal values for
 * the `type` field:
 * - `TSQueryPredicateStepTypeCapture` - Steps with this type represent names
 *    of captures. Their `value_id` can be used with the
 *   `ts_query_capture_name_for_id` function to obtain the name of the capture.
 * - `TSQueryPredicateStepTypeString` - Steps with this type represent literal
 *    strings. Their `value_id` can be used with the
 *    `ts_query_string_value_for_id` function to obtain their string value.
 * - `TSQueryPredicateStepTypeDone` - Steps with this type are *sentinels*
 *    that represent the end of an individual predicate. If a pattern has two
 *    predicates, then there will be two steps with this `type` in the array.
 */
JNIEXPORT jobjectArray JNICALL
Java_io_github_module_treesitter_TreeSitter_queryPredicatesForPattern(JNIEnv* env, jobject thiz, 
                                                                      jlong query, jint patternIndex) {
    uint32_t length;
    const TSQueryPredicateStep *predicates = ts_query_predicates_for_pattern(
        reinterpret_cast<TSQuery*>(query), 
        patternIndex, 
        &length
    );
    
    jmethodID constructor = env->GetMethodID(
        javaTSQueryPredicateStepClass, 
        "<init>", 
        "(Lio/github/module/treesitter/TSQueryPredicateStepType;I)V"
    );
    
    jobjectArray predicateArray = env->NewObjectArray(length, javaTSQueryPredicateStepClass, nullptr);
    
    // java enum TSQueryPredicateStepType fields
    jfieldID field = nullptr;
    
    for(int i=0; i < length; ++i) {      
        switch(predicates[i].type) {
            case TSQueryPredicateStepTypeDone:
                field = env->GetStaticFieldID(javaTSQueryPredicateStepTypeClass, "DOWN", "Lio/github/module/treesitter/TSQueryPredicateStepType;");
                break;
            case TSQueryPredicateStepTypeCapture:
                field = env->GetStaticFieldID(javaTSQueryPredicateStepTypeClass, "CAPTURE", "Lio/github/module/treesitter/TSQueryPredicateStepType;");
                break;
            case TSQueryPredicateStepTypeString:
                field = env->GetStaticFieldID(javaTSQueryPredicateStepTypeClass, "STRING", "Lio/github/module/treesitter/TSQueryPredicateStepType;");
                break;
            default:
                LOGE("Error: Unknown field %d of TSQueryPredicateStepType class\n", predicates[i].type);
                break;
        }
        
        // java enum TSQueryPredicateStepType object
        jobject predicateStepType = env->GetStaticObjectField(javaTSQueryPredicateStepTypeClass, field);
        
        jobject predicateObject = env->NewObject(
            javaTSQueryPredicateStepClass,
            constructor,
            predicateStepType,
            predicates[i].value_id
        );
        
        env->SetObjectArrayElement(predicateArray, i, predicateObject);
    }
    
    return predicateArray;
}

JNIEXPORT jlong JNICALL
Java_io_github_module_treesitter_TreeSitter_queryIsPatternGuaranteedAtStep(JNIEnv* env, jobject thiz, 
                                                                           jlong query, jint offset) {
    return ts_query_is_pattern_guaranteed_at_step(reinterpret_cast<TSQuery*>(query), offset);
}

/**
 * Get the name and length of one of the query's captures, or one of the
 * query's string literals. Each capture and string is associated with a
 * numeric id based on the order that it appeared in the query's source.
 */
JNIEXPORT jstring JNICALL
Java_io_github_module_treesitter_TreeSitter_queryCaptureNameForId(JNIEnv* env, jobject thiz, 
                                                                  jlong query, jint id) {
    uint32_t length;
    const char *name = ts_query_capture_name_for_id(reinterpret_cast<TSQuery*>(query), id, &length);
    return env->NewStringUTF(name);
}

/**
 * Get the quantifier of the query's captures. Each capture is * associated
 * with a numeric id based on the order that it appeared in the query's source.
 */
JNIEXPORT jobject JNICALL
Java_io_github_module_treesitter_TreeSitter_queryCaptureQuantifierForId(JNIEnv* env, jobject thiz, 
                                                                        jlong query, jint patternId, jint captureId) {
    TSQuantifier quantifier = ts_query_capture_quantifier_for_id(
        reinterpret_cast<TSQuery*>(query), 
        patternId, 
        captureId
    );
    
    jfieldID field = nullptr;
    
    switch(quantifier) {
        case TSQuantifierZero:
            field = env->GetStaticFieldID(javaTSQuantifierClass, "ZERO", "Lio/github/module/treesitter/TSQuantifier;");
            break;
        case TSQuantifierZeroOrOne:
            field = env->GetStaticFieldID(javaTSQuantifierClass, "ZERO_OR_ONE", "Lio/github/module/treesitter/TSQuantifier;");
            break;
        case TSQuantifierZeroOrMore:
            field = env->GetStaticFieldID(javaTSQuantifierClass, "ZERO_OR_MORE", "Lio/github/module/treesitter/TSQuantifier;");
            break;
        case TSQuantifierOne:
            field = env->GetStaticFieldID(javaTSQuantifierClass, "ONE", "Lio/github/module/treesitter/TSQuantifier;");
            break;
        case TSQuantifierOneOrMore:
            field = env->GetStaticFieldID(javaTSQuantifierClass, "ONE_OR_MORE", "Lio/github/module/treesitter/TSQuantifier;");
            break;
        default:
            LOGE("Error: Unknown field %d of TSQuantifier class\n", quantifier);
            break;
    }
    
    return env->GetStaticObjectField(javaTSQuantifierClass, field);
}

JNIEXPORT jstring JNICALL
Java_io_github_module_treesitter_TreeSitter_queryStringValueForId(JNIEnv* env, jobject thiz, 
                                                                  jlong query, jint id) {
    uint32_t length;
    const char *value = ts_query_string_value_for_id(reinterpret_cast<TSQuery*>(query), id, &length);
    return env->NewStringUTF(value);
}

/**
 * Disable a certain capture within a query.
 *
 * This prevents the capture from being returned in matches, and also avoids
 * any resource usage associated with recording the capture. Currently, there
 * is no way to undo this.
 */
JNIEXPORT void JNICALL
Java_io_github_module_treesitter_TreeSitter_queryDisableCapture(JNIEnv* env, jobject thiz, 
                                                                jlong query, jstring name, jint id) {
    const char *pattern_name = env->GetStringUTFChars(name, nullptr);
    ts_query_disable_capture(reinterpret_cast<TSQuery*>(query), pattern_name, id);
    env->ReleaseStringUTFChars(name, pattern_name);
}

/**
 * Disable a certain pattern within a query.
 *
 * This prevents the pattern from matching and removes most of the overhead
 * associated with the pattern. Currently, there is no way to undo this.
 */
JNIEXPORT void JNICALL
Java_io_github_module_treesitter_TreeSitter_queryDisablePattern(JNIEnv* env, jobject thiz, 
                                                                jlong query, jint id) {
    ts_query_disable_pattern(reinterpret_cast<TSQuery*>(query), id);
}


#ifdef __cplusplus
}
#endif // __cplusplus

