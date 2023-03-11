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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <jni.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <tree_sitter/api.h>

#ifdef __ANDROID__
#include <android/log.h>
#endif

#include <regex>
#include <string>
#include <vector>
#include <codecvt>
#include <locale>

#include "languages.h"
#include "function_pointer.hpp"

#define JNI_VERSION JNI_VERSION_1_6

#define TAG "JNI_LOG_DMESG"

#ifdef __ANDROID__
// android log print
// log.i
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
// log.e
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)
#elif
// c-style log print
#define  LOGI(...) fprintf(stdout, __VA_ARGS__)
#define  LOGE(...) fprintf(stderr, __VA_ARGS__)
#endif // end __ANDROID__

#ifdef __cplusplus
extern "C" {
#endif

static jclass javaNodeClass = nullptr;
static jclass javaPointClass = nullptr;
static jclass javaInputEditClass = nullptr;
static jclass javaQuantifierClass = nullptr;
static jclass javaLogTypeClass = nullptr;

static jclass javaCaptureClass = nullptr;
static jclass javaQueryCaptureClass = nullptr;
static jclass javaQueryMatchClass = nullptr;
static jclass javaQueryPredicateStepClass = nullptr;
static jclass javaQueryPredicateStepTypeClass = nullptr;

// 
static JavaVM *jvm = nullptr;

static pthread_key_t key;

#define loadClass(VARIABLE, CLASS_NAME) \
    do {                                             \
        jclass local = nullptr;                       \
        local = env->FindClass(CLASS_NAME);      \
        VARIABLE = (jclass)env->NewGlobalRef(local); \
        env->DeleteLocalRef(local);                  \
    } while(0)


static JNIEnv* getEnv() {
    JNIEnv *env = static_cast<JNIEnv*>(pthread_getspecific(key));
    // cache the env pointer
    if(env == nullptr) {
        jint status = jvm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION);
        switch(status) {
        case JNI_OK:
            pthread_setspecific(key, env);
            break;
        case JNI_EDETACHED:
            if(jvm->AttachCurrentThread(&env, nullptr) < 0) {
                LOGE("%s\n", "The jvm failed to attach current thread");
            } else {
                pthread_setspecific(key, env);
            }
            break;
        case JNI_EVERSION:
        default:
            LOGE("%s\n", "The jvm failed to get the env pointer");
            break;
        }
    }
    return env;
}

jint JNI_OnLoad(JavaVM *vm, void *reserved) {
    jvm = vm; // init the global jvm
    JNIEnv *env = getEnv();
    if(env == nullptr) {
        LOGE("Failed to init the jvm environment\n");
        return JNI_ERR;
    }
    
    pthread_key_create(&key, [](void*) {
        jvm->DetachCurrentThread();
    });
    
    loadClass(javaNodeClass, "io/github/module/treesitter/TSNode");
    loadClass(javaPointClass, "io/github/module/treesitter/TSPoint");
    loadClass(javaInputEditClass, "io/github/module/treesitter/TSInputEdit");
    loadClass(javaQuantifierClass, "io/github/module/treesitter/TSQuantifier");
    loadClass(javaLogTypeClass, "io/github/module/treesitter/TSLogType");
    
    loadClass(javaCaptureClass, "io/github/module/treesitter/TSCapture");
    loadClass(javaQueryCaptureClass, "io/github/module/treesitter/TSQueryCapture");
    loadClass(javaQueryMatchClass, "io/github/module/treesitter/TSQueryMatch");
    loadClass(javaQueryPredicateStepClass, "io/github/module/treesitter/TSQueryPredicateStep");
    loadClass(javaQueryPredicateStepTypeClass, "io/github/module/treesitter/TSQueryPredicateStepType");
      
    return JNI_VERSION;
}

void JNI_OnUnload(JavaVM *vm, void *reserved) {
    JNIEnv *env = getEnv();
    
    env->DeleteGlobalRef(javaNodeClass);
    env->DeleteGlobalRef(javaPointClass);
    env->DeleteGlobalRef(javaInputEditClass);
    env->DeleteGlobalRef(javaQuantifierClass);
    env->DeleteGlobalRef(javaLogTypeClass);
    
    env->DeleteGlobalRef(javaCaptureClass);
    env->DeleteGlobalRef(javaQueryCaptureClass);
    env->DeleteGlobalRef(javaQueryMatchClass);
    env->DeleteGlobalRef(javaQueryPredicateStepClass);
    env->DeleteGlobalRef(javaQueryPredicateStepTypeClass);
    
    LOGI("JNI_OnUnload\n");
}

// java Node
jobject javaNode(JNIEnv *env, const TSNode *node) {
    jmethodID constructor = env->GetMethodID(javaNodeClass, "<init>", "([IJJ)V");
    // size default is 4
    jint size = sizeof(node->context) / sizeof(node->context[0]);
    jintArray javaArray = env->NewIntArray(size);
    env->SetIntArrayRegion(javaArray, 0, size, (jint*)node->context);
    
    return env->NewObject(
        javaNodeClass, 
        constructor, 
        javaArray,
        reinterpret_cast<jlong>(node->id),
        reinterpret_cast<jlong>(node->tree)
    );
}

// native TSNode
TSNode nativeNode(JNIEnv *env, const jobject nodeObject) {
    jfieldID context = env->GetFieldID(javaNodeClass, "context", "[I");
    jfieldID id = env->GetFieldID(javaNodeClass, "id", "J");
    jfieldID tree = env->GetFieldID(javaNodeClass, "tree", "J");
    
    jintArray array = static_cast<jintArray>(env->GetObjectField(nodeObject, context));
    //jint size = env->GetArrayLength(array);
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
    jmethodID constructor = env->GetMethodID(javaPointClass, "<init>", "(II)V");
    return env->NewObject(
        javaPointClass, 
        constructor,
        point->row,
        point->column
    );
}

// native TSPoint
TSPoint nativePoint(JNIEnv *env, const jobject pointObject) {
    jfieldID row = env->GetFieldID(javaPointClass, "row", "I");
    jfieldID column = env->GetFieldID(javaPointClass, "column", "I");
    return TSPoint {
        static_cast<uint32_t>(env->GetIntField(pointObject, row)),
        static_cast<uint32_t>(env->GetIntField(pointObject, column))
    };
}

// native TSInputEdit
TSInputEdit nativeInputEdit(JNIEnv *env, const jobject inputObject) {
    jfieldID startByte = env->GetFieldID(javaInputEditClass, "startByte", "I");
    jfieldID oldEndByte = env->GetFieldID(javaInputEditClass, "oldEndByte", "I");
    jfieldID newEndByte = env->GetFieldID(javaInputEditClass, "newEndByte", "I");
   
    jfieldID startPoint = env->GetFieldID(javaInputEditClass, "startPoint", "Lio/github/module/treesitter/TSPoint;");
    jfieldID oldEndPoint = env->GetFieldID(javaInputEditClass, "oldEndPoint", "Lio/github/module/treesitter/TSPoint;");
    jfieldID newEndPoint = env->GetFieldID(javaInputEditClass, "newEndPoint", "Lio/github/module/treesitter/TSPoint;");
   
    return TSInputEdit {
        static_cast<uint32_t>(env->GetIntField(inputObject, startByte)),
        static_cast<uint32_t>(env->GetIntField(inputObject, oldEndByte)),
        static_cast<uint32_t>(env->GetIntField(inputObject, newEndByte)),
        nativePoint(env, env->GetObjectField(inputObject, startPoint)),
        nativePoint(env, env->GetObjectField(inputObject, oldEndPoint)),
        nativePoint(env, env->GetObjectField(inputObject, newEndPoint))
    };
}

// java TSQueryPredicateStep array
jobjectArray javaTSQueryPredicateSteps(JNIEnv *env, const TSQueryPredicateStep *predicates, const int size) {
    jmethodID constructor = env->GetMethodID(javaQueryPredicateStepClass, "<init>", "(Lio/github/module/treesitter/TSQueryPredicateStepType;I)V");
    jobjectArray predicateArray = env->NewObjectArray(size, javaQueryPredicateStepClass, nullptr);
    
    // java enum TSQueryPredicateStepType fields
    jfieldID field = nullptr;
    
    for(int i=0; i < size; ++i) {      
        switch(predicates[i].type) {
            case TSQueryPredicateStepTypeDone:
                field = env->GetStaticFieldID(javaQueryPredicateStepTypeClass, "DOWN", "Lio/github/module/treesitter/TSQueryPredicateStepType;");
                break;
            case TSQueryPredicateStepTypeCapture:
                field = env->GetStaticFieldID(javaQueryPredicateStepTypeClass, "CAPTURE", "Lio/github/module/treesitter/TSQueryPredicateStepType;");
                break;
            case TSQueryPredicateStepTypeString:
                field = env->GetStaticFieldID(javaQueryPredicateStepTypeClass, "STRING", "Lio/github/module/treesitter/TSQueryPredicateStepType;");
                break;
            default:
                LOGE("Error: Unknown field %d of TSQueryPredicateStepType class\n", predicates[i].type);
                break;
        }
        
        // java enum TSQueryPredicateStepType object
        jobject predicateStepType = env->GetStaticObjectField(javaQueryPredicateStepTypeClass, field);
        
        jobject predicateObject = env->NewObject(
            javaQueryPredicateStepClass, 
            constructor,
            predicateStepType,
            predicates[i].value_id
        );
        
        env->SetObjectArrayElement(predicateArray, i, predicateObject);
    }
    
    return predicateArray;
}

// java TSQueryCapture array
jobjectArray javaQueryCaptures(JNIEnv *env, const TSQueryCapture *captures, const uint32_t count) {
    jmethodID constructor = env->GetMethodID(javaQueryCaptureClass, "<init>", "(Lio/github/module/treesitter/TSNode;I)V");
    jobjectArray captureArray = env->NewObjectArray(count, javaQueryCaptureClass, nullptr);
    
    for(int i=0; i < count; ++i) {
        jobject nodeObject = javaNode(env, &captures[i].node);
        jobject captureObject = env->NewObject(
            javaQueryCaptureClass, 
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
    jmethodID constructor = env->GetMethodID(javaQueryMatchClass, "<init>", "(III[Lio/github/module/treesitter/TSQueryCapture;)V");
    jobjectArray captureArray = javaQueryCaptures(env, match->captures, match->capture_count);
    
    return env->NewObject(
        javaQueryMatchClass, 
        constructor,
        match->id,
        match->pattern_index,
        match->capture_count,
        captureArray
    );
}

// get lambda callable object
static jmethodID getMethod(JNIEnv *env, const jobject object, const char *signature) {
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


JNIEXPORT jlong JNICALL
Java_io_github_module_treesitter_TreeSitter_getSupportLanguage(JNIEnv* env, jobject thiz, jstring name) {
    
    const char *language = env->GetStringUTFChars(name, nullptr);
    env->ReleaseStringUTFChars(name, language);
    
    if(strcmp(language, "C") == 0)
        return reinterpret_cast<jlong>(tree_sitter_c());
    else
        return 0;
}


// #region parser

/**
 * Create a new parser.
 */
JNIEXPORT jlong JNICALL
Java_io_github_module_treesitter_TreeSitter_newParser(JNIEnv* env, jobject thiz) {
    TSParser *parser = ts_parser_new();
    return reinterpret_cast<jlong>(parser);
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
    const TSLanguage *language = ts_parser_language(reinterpret_cast<TSParser*>(parser));
    return reinterpret_cast<jlong>(language);
}

/**
 * Set the logger that a parser should use during parsing.
 *
 * The parser does not take ownership over the logger payload. If a logger was
 * previously assigned, the caller is responsible for releasing any memory
 * owned by the previous logger.
 */
JNIEXPORT void JNICALL
Java_io_github_module_treesitter_TreeSitter_setParserLogger(JNIEnv* env, jobject thiz, 
                                                            jlong parser, jobject lambda) {
                                                    
    jmethodID invoke = getMethod(env, lambda, "(Lio/github/module/treesitter/TSLogType;Ljava/lang/String;)V");
    
    ts_parser_set_logger(reinterpret_cast<TSParser*>(parser), { nullptr,
        convert([env, lambda, invoke](void *payload, TSLogType type, const char *message) {
            // convert lambda to C-Style function pointer
            jfieldID field = nullptr;
            switch(type) {
                case TSLogTypeParse:
                    field = env->GetStaticFieldID(javaLogTypeClass, "PARSE", "Lio/github/module/treesitter/TSLogType;");
                    break;
                case TSLogTypeLex:
                    field = env->GetStaticFieldID(javaLogTypeClass, "LEX", "Lio/github/module/treesitter/TSLogType;");
                    break;
                default:
                    LOGE("Error: Unknown field %d of TSLogType class\n", type);
                    break;
            }
           
            // java enum TSLogType object
            jobject typeObject = env->GetStaticObjectField(javaLogTypeClass, field);
            // call the kotlin lambda expression
            env->CallVoidMethod(
                lambda,
                invoke,
                typeObject,
                env->NewStringUTF(message)
            );
        })
    });
}


/**
 * Set the maximum duration in microseconds that parsing should be allowed to
 * take before halting.
 *
 * If parsing takes longer than this, it will halt early, returning NULL.
 * See `ts_parser_parse` for more information.
 */
JNIEXPORT void JNICALL
Java_io_github_module_treesitter_TreeSitter_setParserTimeout(JNIEnv* env, jobject thiz, jlong parser, jlong timeout) {
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
Java_io_github_module_treesitter_TreeSitter_setParserCancellationFlag(JNIEnv* env, jobject thiz, jlong parser, jboolean flag) {
    if(flag == JNI_TRUE) {
        // set flag
        int mark = static_cast<int>(flag);
        ts_parser_set_cancellation_flag(
            reinterpret_cast<TSParser*>(parser), reinterpret_cast<size_t*>(&mark)
        );
    } else {
        // cancel flag
        ts_parser_set_cancellation_flag(
            reinterpret_cast<TSParser*>(parser), nullptr
        );
    }
}


/**
 * Get the parser's current cancellation flag pointer.
 */
JNIEXPORT jboolean JNICALL
Java_io_github_module_treesitter_TreeSitter_getParserCancellationFlag(JNIEnv* env, jobject thiz, jlong parser) {
    // set cancel flag
    if(ts_parser_cancellation_flag(reinterpret_cast<TSParser*>(parser)))
        return JNI_TRUE;
    else
        return JNI_FALSE;   
}

JNIEXPORT jlong JNICALL
Java_io_github_module_treesitter_TreeSitter_parserParse(JNIEnv* env, jobject thiz,
                                                        jlong parser, jlong oldTree, 
                                                        jobject charset, jobject lambda) {
    // get the text encoding                                       
    jclass javaTSInputEncoding = env->FindClass("io/github/module/treesitter/TSInputEncoding");
    jmethodID ordinal = env->GetMethodID(javaTSInputEncoding, "ordinal", "()I");
    TSInputEncoding encoding = static_cast<TSInputEncoding>(env->CallIntMethod(charset, ordinal));
   
    jmethodID invoke = getMethod(env, lambda, "(ILio/github/module/treesitter/TSPoint;)[B");
    
    // byte chunk
    jbyteArray bytes = nullptr;
    jbyte *chunks = nullptr;
    
    // convert lambda to C-Style function pointer
    auto callback = convert(
        [env, lambda, invoke, &bytes, &chunks](
            void *payload, uint32_t byte_index, TSPoint point, uint32_t *bytes_read
        ) -> const char* {
            
            // free the memory of the previous row
            if(bytes != nullptr || chunks != nullptr) {
                env->ReleaseByteArrayElements(bytes, chunks, JNI_ABORT);
            }
            
            // java TSPoint constructor
            jmethodID constructor = env->GetMethodID(javaPointClass, "<init>", "(II)V");
            jobject position = env->NewObject(javaPointClass, constructor, point.row, point.column);
    
            // java byte array
            bytes = reinterpret_cast<jbyteArray>(env->CallObjectMethod(lambda, invoke, byte_index, position));
            chunks = env->GetByteArrayElements(bytes, nullptr);
            
            // reset bytes_read
            *bytes_read = env->GetArrayLength(bytes);
           
            return reinterpret_cast<const char*>(chunks);
        }
    );
    
    TSTree *tree = ts_parser_parse(
        reinterpret_cast<TSParser*>(parser),
        reinterpret_cast<TSTree*>(oldTree),
        {nullptr, callback, encoding}
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


// the parseFile function is used to test large text
JNIEXPORT jlong JNICALL
Java_io_github_module_treesitter_TreeSitter_parseFile(JNIEnv* env, jobject thiz,
                                                      jlong parser, jstring path, jobject charset) {
    
    jclass javaTSInputEncoding = env->FindClass("io/github/module/treesitter/TSInputEncoding");
    jmethodID ordinal = env->GetMethodID(javaTSInputEncoding, "ordinal", "()I");
    
    TSInputEncoding encoding = static_cast<TSInputEncoding>(env->CallIntMethod(charset, ordinal));
    
    const char *pathname = env->GetStringUTFChars(path, nullptr);
    
    FILE *fp = fopen(pathname, "r");
    if(!fp) {
        LOGE("%s\n", strerror(errno));
    }
    
    char *line = nullptr;
    size_t size = 0;
    
    // cache each line start index
    std::vector<uint32_t> indexs;
    
    // native read the larger text file with line by line
    auto read = convert(
        [env, fp, &line, &size, &indexs](
            void *payload, uint32_t byte_index, TSPoint point, uint32_t *bytes_read
        ) -> const char* {
            
            // current row of start index
            if(point.row >= indexs.size() && point.column == 0) {
                indexs.push_back(byte_index);
            }
            
            // may fallback to the previous row
            // seek to the start index of current row
            fseek(fp, indexs[point.row], SEEK_SET);
            // line text length
            ssize_t length = getline(&line, &size, fp);
            
            // check the length
            *bytes_read = length > 0 ? length - point.column : 0;
            
            return line + point.column;
        }
    );
    
    TSTree *tree = ts_parser_parse(
        reinterpret_cast<TSParser*>(parser),
        nullptr,
        {nullptr, read, encoding}
    );
    
    env->ReleaseStringUTFChars(path, pathname);
    env->DeleteLocalRef(javaTSInputEncoding);
    
    // release memory
    fclose(fp);
    free((void*)line);
    indexs.clear();
    
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

// #endregion

// #region tree

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
Java_io_github_module_treesitter_TreeSitter_editTree(JNIEnv* env, jobject thiz, jlong tree, jobject input) {
    TSInputEdit input_edit = nativeInputEdit(env, input);
    ts_tree_edit(reinterpret_cast<TSTree*>(tree), &input_edit);
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
JNIEXPORT jlong JNICALL
Java_io_github_module_treesitter_TreeSitter_getTreeChangedRange(JNIEnv* env, jobject thiz, 
                                                                jlong oldTree, jlong newTree, jint length) {
    TSRange *ranges = ts_tree_get_changed_ranges(
        reinterpret_cast<TSTree*>(oldTree),
        reinterpret_cast<TSTree*>(newTree), 
        reinterpret_cast<uint32_t*>(&length)
    );
    
    // not implemented, only return the pointer address
    return reinterpret_cast<jlong>(ranges);
}

/**
 * Write a DOT graph describing the syntax tree to the given file.
 */
JNIEXPORT void JNICALL
Java_io_github_module_treesitter_TreeSitter_treeDotGraph(JNIEnv* env, jobject thiz, jlong tree, jstring pathname) {
    const char *path =env->GetStringUTFChars(pathname, nullptr); 
    int fp = open(path, O_CREAT|O_RDWR, 0666);
    if(fp < 0) 
        LOGE("Error: %s\n", strerror(errno));
    else 
        ts_tree_print_dot_graph(reinterpret_cast<TSTree*>(tree), fp);
    
    env->ReleaseStringUTFChars(pathname, path);
}

// #endregion

// #region node

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
    return ts_node_eq(nativeNode(env, a), nativeNode(env, b));
}

// #endregion

// #region tree cursor

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
    TSNode tree_node = ts_tree_cursor_current_node(reinterpret_cast<TSTreeCursor*>(cursor));
    return javaNode(env, &tree_node);
}

JNIEXPORT jstring JNICALL
Java_io_github_module_treesitter_TreeSitter_cursorCurrentFieldName(JNIEnv* env, jobject thiz, jlong cursor) {
    const char *field_name = ts_tree_cursor_current_field_name(reinterpret_cast<TSTreeCursor*>(cursor));
    return env->NewStringUTF(field_name);
}

JNIEXPORT jboolean JNICALL
Java_io_github_module_treesitter_TreeSitter_cursorGotoFirstChild(JNIEnv* env, jobject thiz, jlong cursor) {
    TSTreeCursor *tree_cursor = reinterpret_cast<TSTreeCursor*>(cursor);
    return ts_tree_cursor_goto_first_child(tree_cursor);
}

JNIEXPORT jboolean JNICALL
Java_io_github_module_treesitter_TreeSitter_cursorGotoNextSibling(JNIEnv* env, jobject thiz, jlong cursor) {
    TSTreeCursor *tree_cursor = reinterpret_cast<TSTreeCursor*>(cursor);
    return ts_tree_cursor_goto_next_sibling(tree_cursor);
}

JNIEXPORT jboolean JNICALL
Java_io_github_module_treesitter_TreeSitter_cursorGotoParent(JNIEnv* env, jobject thiz, jlong cursor) {
    TSTreeCursor *tree_cursor = reinterpret_cast<TSTreeCursor*>(cursor);
    return ts_tree_cursor_goto_parent(tree_cursor);
}

// #endregion

// #region query
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
    const TSQueryPredicateStep *steps = nullptr;
    steps = ts_query_predicates_for_pattern(reinterpret_cast<TSQuery*>(query), patternIndex, &length);
    return javaTSQueryPredicateSteps(env, steps, length);
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
    TSQuantifier quantifier = ts_query_capture_quantifier_for_id(reinterpret_cast<TSQuery*>(query), patternId, captureId);
    
    jfieldID field = nullptr;
    
    switch(quantifier) {
        case TSQuantifierZero:
            field = env->GetStaticFieldID(javaQuantifierClass, "ZERO", "Lio/github/module/treesitter/TSQuantifier;");
            break;
        case TSQuantifierZeroOrOne:
            field = env->GetStaticFieldID(javaQuantifierClass, "ZERO_OR_ONE", "Lio/github/module/treesitter/TSQuantifier;");
            break;
        case TSQuantifierZeroOrMore:
            field = env->GetStaticFieldID(javaQuantifierClass, "ZERO_OR_MORE", "Lio/github/module/treesitter/TSQuantifier;");
            break;
        case TSQuantifierOne:
            field = env->GetStaticFieldID(javaQuantifierClass, "ONE", "Lio/github/module/treesitter/TSQuantifier;");
            break;
        case TSQuantifierOneOrMore:
            field = env->GetStaticFieldID(javaQuantifierClass, "ONE_OR_MORE", "Lio/github/module/treesitter/TSQuantifier;");
            break;
        default:
            LOGE("Error: Unknown field %d of TSQuantifier class\n", quantifier);
            break;
    }
    
    return env->GetStaticObjectField(javaQuantifierClass, field);
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

// #endregion

// #region query cursor

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
        jmethodID constructor = env->GetMethodID(javaCaptureClass, "<init>", "(Lio/github/module/treesitter/TSQueryMatch;I)V");
        
        return env->NewObject(
            javaCaptureClass, 
            constructor,
            query_match,
            capture_index
        );
    }
    return nullptr;
}


// #endregion

#ifdef __cplusplus
}
#endif // __cplusplus
