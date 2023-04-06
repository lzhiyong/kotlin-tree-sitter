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

package io.github.module.treesitter

sealed class TSLanguage(val pointer: Long = nullptr) {
    object Empty: TSLanguage(0L) // invalid language
    object Bash : TSLanguage(TreeSitter.getSupportLanguage(Bash::class.simpleName))
    object C : TSLanguage(TreeSitter.getSupportLanguage(C::class.simpleName))
    object Cpp : TSLanguage(TreeSitter.getSupportLanguage(Cpp::class.simpleName))
    object Java : TSLanguage(TreeSitter.getSupportLanguage(Java::class.simpleName))
    object Kotlin : TSLanguage(TreeSitter.getSupportLanguage(Kotlin::class.simpleName))
    object Python : TSLanguage(TreeSitter.getSupportLanguage(Python::class.simpleName))
    object Rust : TSLanguage(TreeSitter.getSupportLanguage(Rust::class.simpleName))
}

