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
#include "ts_language.h"

#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT jlong JNICALL
Java_io_github_module_treesitter_TreeSitter_getSupportLanguage(JNIEnv* env, jobject thiz, jstring name) {
    
    const char *language = env->GetStringUTFChars(name, nullptr);
    env->ReleaseStringUTFChars(name, language);
    
    if(strcmp(language, "C") == 0)
        return reinterpret_cast<jlong>(tree_sitter_c());
    else
        return 0;
}

#ifdef __cplusplus
}
#endif // __cplusplus