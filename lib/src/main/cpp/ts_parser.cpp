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

#ifdef __cplusplus
extern "C" {
#endif

// define global variables
jclass javaTSNodeClass = nullptr;
jclass javaTSParserClass = nullptr;
jclass javaTSPointClass = nullptr;
jclass javaTSLogTypeClass = nullptr;
jclass javaTSInputEncoding = nullptr;

// callbacks
jmethodID read = nullptr;
jmethodID logger = nullptr;

static jbyteArray bytes = nullptr;
static jbyte *chunks = nullptr;

/**
 * Create a new parser.
 */
JNIEXPORT jlong JNICALL
Java_io_github_module_treesitter_TreeSitter_newParser(JNIEnv* env, jobject thiz) {
    return reinterpret_cast<jlong>(ts_parser_new());
}

/**
 * Delete the parser, freeing all of the memory that it used.
 */
JNIEXPORT void JNICALL
Java_io_github_module_treesitter_TreeSitter_deleteParser(JNIEnv* env, jobject thiz, jlong parser) {
    ts_parser_delete(reinterpret_cast<TSParser*>(parser));
}

/**
 * Instruct the parser to start the next parse from the beginning.
 *
 * If the parser previously failed because of a timeout or a cancellation, then
 * by default, it will resume where it left off on the next call to
 * `ts_parser_parse` or other parsing functions. If you don't want to resume,
 * and instead intend to use this parser to parse some other document, you must
 * call `ts_parser_reset` first.
 */
JNIEXPORT void JNICALL
Java_io_github_module_treesitter_TreeSitter_resetParser(JNIEnv* env, jobject thiz, jlong parser) {
    ts_parser_reset(reinterpret_cast<TSParser*>(parser));
}

/**
 * Set the language that the parser should use for parsing.
 *
 * Returns a boolean indicating whether or not the language was successfully
 * assigned. True means assignment succeeded. False means there was a version
 * mismatch: the language was generated with an incompatible version of the
 * Tree-sitter CLI. Check the language's version using `ts_language_version`
 * and compare it to this library's `TREE_SITTER_LANGUAGE_VERSION` and
 * `TREE_SITTER_MIN_COMPATIBLE_LANGUAGE_VERSION` constants.
 */
JNIEXPORT void JNICALL
Java_io_github_module_treesitter_TreeSitter_setParserLanguage(JNIEnv* env, jobject thiz, 
                                                              jlong parser, jlong language) {
    ts_parser_set_language(
        reinterpret_cast<TSParser*>(parser), 
        reinterpret_cast<TSLanguage*>(language)
    );
}

/**
 * Get the parser's current language.
 */
JNIEXPORT jlong JNICALL
Java_io_github_module_treesitter_TreeSitter_getParserLanguage(JNIEnv* env, jobject thiz, jlong parser) {
    
    return reinterpret_cast<jlong>(
        ts_parser_language(reinterpret_cast<TSParser*>(parser))
    );
}

/**
 * Set the logger that a parser should use during parsing.
 *
 * The parser does not take ownership over the logger payload. If a logger was
 * previously assigned, the caller is responsible for releasing any memory
 * owned by the previous logger.
 */
JNIEXPORT void JNICALL
Java_io_github_module_treesitter_TreeSitter_setParserLogger(JNIEnv* env, jobject thiz, jlong parser) {                                                    
    
    // convert lambda to C-Style function pointer
    auto callback = [](void *payload, TSLogType type, const char *message) {
        // local JNIEnv pointer
        JNIEnv *localEnv = static_cast<JNIEnv*>(payload);
        jfieldID field = nullptr;
        switch(type) {
            case TSLogTypeParse:
                field = localEnv->GetStaticFieldID(
                    javaTSLogTypeClass, 
                    "PARSE", 
                    "Lio/github/module/treesitter/TSLogType;"
                );
                break;
            case TSLogTypeLex:
                field = localEnv->GetStaticFieldID(
                    javaTSLogTypeClass, 
                    "LEX", 
                    "Lio/github/module/treesitter/TSLogType;"
                );
                break;
            default:
                LOGE("Error: Unknown field %d of TSLogType class\n", type);
                break;
        }
           
        // java enum TSLogType object
        jobject typeObject = localEnv->GetStaticObjectField(javaTSLogTypeClass, field);
        // call the kotlin lambda expression
        localEnv->CallStaticVoidMethod(
            javaTSParserClass,
            logger,
            typeObject,
            localEnv->NewStringUTF(message)
        );
    };
        
    ts_parser_set_logger(
        reinterpret_cast<TSParser*>(parser), 
        {env, callback}
    );
}


/**
 * Set the maximum duration in microseconds that parsing should be allowed to
 * take before halting.
 *
 * If parsing takes longer than this, it will halt early, returning NULL.
 * See `ts_parser_parse` for more information.
 */
JNIEXPORT void JNICALL
Java_io_github_module_treesitter_TreeSitter_setParserTimeout(JNIEnv* env, jobject thiz, 
                                                             jlong parser, jlong timeout) {
    ts_parser_set_timeout_micros(reinterpret_cast<TSParser*>(parser), timeout);
}


/**
 * Get the duration in microseconds that parsing is allowed to take.
 */
JNIEXPORT jlong JNICALL
Java_io_github_module_treesitter_TreeSitter_getParserTimeout(JNIEnv* env, jobject thiz, jlong parser) {
    return ts_parser_timeout_micros(reinterpret_cast<TSParser*>(parser));
}

/**
 * Set the parser's current cancellation flag pointer.
 *
 * If a non-null pointer is assigned, then the parser will periodically read
 * from this pointer during parsing. If it reads a non-zero value, it will
 * halt early, returning NULL. See `ts_parser_parse` for more information.
 */
JNIEXPORT void JNICALL
Java_io_github_module_treesitter_TreeSitter_setParserCancellationFlag(JNIEnv* env, jobject thiz, 
                                                                     jlong parser, jboolean flag) {
    if(flag == JNI_TRUE) {
        // flag true
        int mark = static_cast<int>(flag);
        ts_parser_set_cancellation_flag(
            reinterpret_cast<TSParser*>(parser), 
            reinterpret_cast<size_t*>(&mark)
        );
    } else {
        // flag false
        ts_parser_set_cancellation_flag(
            reinterpret_cast<TSParser*>(parser), 
            nullptr
        );
    }
}


/**
 * Get the parser's current cancellation flag pointer.
 */
JNIEXPORT jboolean JNICALL
Java_io_github_module_treesitter_TreeSitter_getParserCancellationFlag(JNIEnv* env, jobject thiz, jlong parser) {
    // get cancellation flag
    if(ts_parser_cancellation_flag(reinterpret_cast<TSParser*>(parser)))
        return JNI_TRUE;
    else
        return JNI_FALSE;   
}


JNIEXPORT jlong JNICALL
Java_io_github_module_treesitter_TreeSitter_parserParse(JNIEnv* env, jobject thiz,
                                                        jlong parser, jlong oldTree, jobject charset) {
    // get the text encoding                                       
    jclass javaTSInputEncoding = env->FindClass("io/github/module/treesitter/TSInputEncoding");
    jmethodID ordinal = env->GetMethodID(javaTSInputEncoding, "ordinal", "()I");
    TSInputEncoding encoding = static_cast<TSInputEncoding>(env->CallIntMethod(charset, ordinal));
   
    // reinitialize jbytes
    ::bytes = nullptr; 
    ::chunks = nullptr;
   
    // convert lambda to C-Style function pointer
    auto callback = [](
        void *payload, uint32_t byte_index, TSPoint point, uint32_t *bytes_read
    ) -> const char* {
        // local JNIEnv pointer
        JNIEnv *localEnv = static_cast<JNIEnv*>(payload);
        
        // free the memory of the previous row
        if(bytes != nullptr && chunks != nullptr) {
            localEnv->ReleaseByteArrayElements(bytes, chunks, JNI_ABORT);
        }
            
        // java TSPoint constructor
        jmethodID constructor = localEnv->GetMethodID(javaTSPointClass, "<init>", "(II)V");
        jobject position = localEnv->NewObject(javaTSPointClass, constructor, point.row, point.column);
        
        // jstring to jbyte array
        bytes = reinterpret_cast<jbyteArray>(
            localEnv->CallStaticObjectMethod(javaTSParserClass, read, byte_index, position)
        );
        chunks = localEnv->GetByteArrayElements(bytes, nullptr);
            
        // reset bytes_read
        *bytes_read = localEnv->GetArrayLength(bytes);
            
        return reinterpret_cast<const char*>(chunks);
    };
    
    TSTree *tree = ts_parser_parse(
        reinterpret_cast<TSParser*>(parser),
        reinterpret_cast<TSTree*>(oldTree),
        {env, callback, encoding}
    );
    
    env->DeleteLocalRef(javaTSInputEncoding);
            
    return reinterpret_cast<jlong>(tree);
}

/**
 * Use the parser to parse some source code stored in one contiguous buffer with
 * a given encoding. The first four parameters work the same as in the
 * `ts_parser_parse_string` method above. The final parameter indicates whether
 * the text is encoded as UTF8 or UTF16.
 */
JNIEXPORT jlong JNICALL
Java_io_github_module_treesitter_TreeSitter_parseString(JNIEnv* env, jobject thiz,
                                                        jlong parser, jlong oldTree, 
                                                        jbyteArray bytes, jobject charset) {
    
    jclass javaTSInputEncoding = env->FindClass("io/github/module/treesitter/TSInputEncoding");
    jmethodID ordinal = env->GetMethodID(javaTSInputEncoding, "ordinal", "()I");
    
    TSInputEncoding encoding = static_cast<TSInputEncoding>(env->CallIntMethod(charset, ordinal));
    
    jbyte* source = env->GetByteArrayElements(bytes, NULL);
    size_t length = env->GetArrayLength(bytes);
    
    TSTree *tree = ts_parser_parse_string_encoding(
        reinterpret_cast<TSParser*>(parser),
        reinterpret_cast<TSTree*>(oldTree),
        reinterpret_cast<const char*>(source),
        length,
        encoding
    );
    
    env->ReleaseByteArrayElements(bytes, source, JNI_ABORT);
    env->DeleteLocalRef(javaTSInputEncoding);
    
    return reinterpret_cast<jlong>(tree);
}

/**
 * Set the file descriptor to which the parser should write debugging graphs
 * during parsing. The graphs are formatted in the DOT language. You may want
 * to pipe these graphs directly to a `dot(1)` process in order to generate
 * SVG output. You can turn off this logging by passing a negative number.
 */
JNIEXPORT void JNICALL
Java_io_github_module_treesitter_TreeSitter_parserDotGraphs(JNIEnv* env, jobject thiz,
                                                            jlong parser, jstring pathname) {
    const char *path =env->GetStringUTFChars(pathname, nullptr); 
    int fp = open(path, O_CREAT|O_TRUNC|O_RDWR, 0666);
    if(fp < 0) 
        LOGE("Error: %s\n", strerror(errno));
    else 
        ts_parser_print_dot_graphs(reinterpret_cast<TSParser*>(parser), fp);
    
    env->ReleaseStringUTFChars(pathname, path);
}

#ifdef __cplusplus
}
#endif // __cplusplus

