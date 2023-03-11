/*
 * This file was generated by the Gradle 'init' task.
 *
 * This generated file contains a sample Kotlin library project to get you started.
 * For more details take a look at the 'Building Java & JVM projects' chapter in the Gradle
 * User Manual available at https://docs.gradle.org/7.6/userguide/building_java_projects.html
 */

import org.jetbrains.kotlin.gradle.tasks.KotlinCompile
import org.gradle.api.tasks.testing.logging.TestExceptionFormat.*
import org.gradle.api.tasks.testing.logging.TestLogEvent.*


plugins {
    // Apply the org.jetbrains.kotlin.jvm Plugin to add support for Kotlin.
    id("org.jetbrains.kotlin.jvm") version "1.8.10"

    // Apply the java-library plugin for API and implementation separation.
    `java-library`
}

repositories {
    // Use Maven Central for resolving dependencies.
    mavenCentral()
}

dependencies {
    // Use the Kotlin JUnit 5 integration.
    testImplementation("org.jetbrains.kotlin:kotlin-test-junit5")

    // Use the JUnit 5 integration.
    testImplementation("org.junit.jupiter:junit-jupiter-engine:5.9.1")

    // This dependency is exported to consumers, that is to say found on their compile classpath.
    api("org.apache.commons:commons-math3:3.6.1")

    // This dependency is used internally, and not exposed to consumers on their own compile classpath.
    implementation("com.google.guava:guava:31.1-jre")
}

tasks.named<Jar>("jar") {
    val sourcesMain = sourceSets.main.get()
    sourcesMain.allSource.forEach{ println("add source file: ${it.name}") }
    from(sourcesMain.output)
}

tasks.withType<KotlinCompile> {
    kotlinOptions {
        jvmTarget = "11"
        javaParameters = true
    }
}

val buildNative = task<Exec>("buildNative") {
    workingDir("..")
    commandLine("${project.rootDir}/build.py")
}
tasks.findByName("processResources")?.dependsOn(buildNative)

tasks.named<Test>("test") {
    // Use JUnit Platform for unit tests.
    useJUnitPlatform()
    
    systemProperty("java.library.path", "${projectDir}/build/native")
    
    // show standard out and standard error of the test JVM(s) on the console
    testLogging {
        events(PASSED, FAILED, STANDARD_OUT, STANDARD_ERROR, SKIPPED)
        exceptionFormat = FULL
        showExceptions = true
        showCauses = true
        showStackTraces = true
        showStandardStreams = true
    }
}
