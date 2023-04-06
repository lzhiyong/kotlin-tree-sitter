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

#ifndef __TS_UTILS_H__
#define __TS_UTILS_H__

#include <jni.h>
#include <tree_sitter/api.h>

#ifdef __cplusplus
extern "C" {
#endif

// native TSNode -> java TSNode
jobject javaNode(JNIEnv*, const TSNode*);

// java TSNode -> native TSNode
TSNode nativeNode(JNIEnv*, const jobject);

// native TSPoint -> java TSPoint
jobject javaPoint(JNIEnv*, const TSPoint*);

// java TSPoint -> native TSPoint
TSPoint nativePoint(JNIEnv*, const jobject);

// get callable object from kotlin lambda
jmethodID getMethod(JNIEnv*, const jobject, const char*);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // __TS_UTILS_H__

