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

#include <pthread.h>

#include "jni_helper.h"

// declare external JNI global variables
extern jclass javaTSNodeClass;
extern jclass javaTSPointClass;
extern jclass javaTSParserClass;
extern jclass javaTSRangeClass;
extern jclass javaTSInputEditClass;
extern jclass javaTSLogTypeClass;
// ...
extern jclass javaTSCaptureClass;
extern jclass javaTSQuantifierClass;
extern jclass javaTSQueryCaptureClass;
extern jclass javaTSQueryMatchClass;
extern jclass javaTSQueryPredicateStepClass;
extern jclass javaTSQueryPredicateStepTypeClass;

// global parser parse callback function
extern jmethodID read;
// global parser log callback function
extern jmethodID logger;

// global JVM
static JavaVM *jvm = nullptr;

static pthread_key_t key;

#define loadClass(VARIABLE, CLASS_NAME) \
    do {                                             \
        jclass local = nullptr;                       \
        local = env->FindClass(CLASS_NAME);      \
        VARIABLE = (jclass)env->NewGlobalRef(local); \
        env->DeleteLocalRef(local);                  \
    } while(0)


extern "C" JavaVM* getJavaVM() {
    return jvm;
}

extern "C" JNIEnv* getEnv() {
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

extern "C" jint JNI_OnLoad(JavaVM *vm, void *reserved) {
    jvm = vm; // init the global jvm
    JNIEnv *env = getEnv();
    if(env == nullptr) {
        LOGE("Failed to init the jvm environment\n");
        return JNI_ERR;
    }
    
    pthread_key_create(&key, [](void*) {
        jvm->DetachCurrentThread();
    });
    
    loadClass(javaTSNodeClass, "io/github/module/treesitter/TSNode");
    loadClass(javaTSPointClass, "io/github/module/treesitter/TSPoint");
    loadClass(javaTSParserClass, "io/github/module/treesitter/TSParser");
    loadClass(javaTSRangeClass, "io/github/module/treesitter/TSRange");
    loadClass(javaTSInputEditClass, "io/github/module/treesitter/TSInputEdit");
    loadClass(javaTSLogTypeClass, "io/github/module/treesitter/TSLogType");
    // ...
    loadClass(javaTSCaptureClass, "io/github/module/treesitter/TSCapture");
    loadClass(javaTSQuantifierClass, "io/github/module/treesitter/TSQuantifier");
    loadClass(javaTSQueryCaptureClass, "io/github/module/treesitter/TSQueryCapture");
    loadClass(javaTSQueryMatchClass, "io/github/module/treesitter/TSQueryMatch");
    loadClass(
        javaTSQueryPredicateStepClass, 
        "io/github/module/treesitter/TSQueryPredicateStep"
    );
    loadClass(
        javaTSQueryPredicateStepTypeClass, 
        "io/github/module/treesitter/TSQueryPredicateStepType"
    );
    
    read = env->GetStaticMethodID(
        javaTSParserClass, 
        "read", 
        "(ILio/github/module/treesitter/TSPoint;)[B"
    );
    
    logger = env->GetStaticMethodID(
        javaTSParserClass, 
        "logger", 
        "(Lio/github/module/treesitter/TSLogType;Ljava/lang/String;)V"
    );
    
    return JNI_VERSION;
}

extern "C" void JNI_OnUnload(JavaVM *vm, void *reserved) {
    JNIEnv *env = getEnv();
    
    env->DeleteGlobalRef(javaTSNodeClass);
    env->DeleteGlobalRef(javaTSPointClass);
    env->DeleteGlobalRef(javaTSParserClass);
    env->DeleteGlobalRef(javaTSRangeClass);
    env->DeleteGlobalRef(javaTSInputEditClass);
    env->DeleteGlobalRef(javaTSLogTypeClass);
    // ...
    env->DeleteGlobalRef(javaTSCaptureClass);
    env->DeleteGlobalRef(javaTSQuantifierClass);
    env->DeleteGlobalRef(javaTSQueryCaptureClass);
    env->DeleteGlobalRef(javaTSQueryMatchClass);
    env->DeleteGlobalRef(javaTSQueryPredicateStepClass);
    env->DeleteGlobalRef(javaTSQueryPredicateStepTypeClass);
    
    LOGI("JNI_OnUnload\n");
}


