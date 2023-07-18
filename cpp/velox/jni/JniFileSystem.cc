/*
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "JniFileSystem.h"
#include "jni/JniCommon.h"
#include "jni/JniErrors.h"

namespace {
constexpr std::string_view kFileScheme("jni:");

static JavaVM* vm;

static jclass jniFileSystemClass;
static jclass jniReadFileClass;
static jclass jniWriteFileClass;

static jmethodID jniGetFileSystem;
static jmethodID jniFileSystemOpenFileForRead;
static jmethodID jniFileSystemOpenFileForWrite;
static jmethodID jniFileSystemRemove;
static jmethodID jniFileSystemRename;
static jmethodID jniFileSystemExists;
static jmethodID jniFileSystemList;
static jmethodID jniFileSystemMkdir;
static jmethodID jniFileSystemRmdir;

static jmethodID jniReadFilePread;
static jmethodID jniReadFileShouldCoalesce;
static jmethodID jniReadFileSize;
static jmethodID jniReadFileMemoryUsage;
static jmethodID jniReadFileGetNaturalReadSize;

static jmethodID jniWriteFileAppend;
static jmethodID jniWriteFileFlush;
static jmethodID jniWriteFileClose;
static jmethodID jniWriteFileSize;

jstring createJString(JNIEnv* env, const std::string_view& path) {
  return env->NewStringUTF(std::string(path).c_str());
}

} // namespace

void gluten::initVeloxJniFileSystem(JNIEnv* env) {
  // vm
  if (env->GetJavaVM(&vm) != JNI_OK) {
    throw gluten::GlutenException("Unable to get JavaVM instance");
  }

  // classes
  jniFileSystemClass = createGlobalClassReferenceOrError(env, "Lio/glutenproject/fs/JniFilesystem;");
  jniReadFileClass = createGlobalClassReferenceOrError(env, "Lio/glutenproject/fs/JniFilesystem$ReadFile;");
  jniWriteFileClass = createGlobalClassReferenceOrError(env, "Lio/glutenproject/fs/JniFilesystem$WriteFile;");

  // methods in JniFilesystem
  jniGetFileSystem = getStaticMethodIdOrError(
      env, jniFileSystemClass, "getFileSystem", "(Ljava/lang/String;)Lio/glutenproject/fs/JniFilesystem;");
  jniFileSystemOpenFileForRead = getMethodIdOrError(
      env, jniFileSystemClass, "openFileForRead", "(Ljava/lang/String;)Lio/glutenproject/fs/JniFilesystem$ReadFile;");
  jniFileSystemOpenFileForWrite = getMethodIdOrError(
      env, jniFileSystemClass, "openFileForWrite", "(Ljava/lang/String;)Lio/glutenproject/fs/JniFilesystem$WriteFile;");
  jniFileSystemRemove = getMethodIdOrError(env, jniFileSystemClass, "remove", "(Ljava/lang/String;)V");
  jniFileSystemRename =
      getMethodIdOrError(env, jniFileSystemClass, "rename", "(Ljava/lang/String;Ljava/lang/String;Z)V");
  jniFileSystemExists = getMethodIdOrError(env, jniFileSystemClass, "exists", "(Ljava/lang/String;)Z");
  jniFileSystemList = getMethodIdOrError(env, jniFileSystemClass, "list", "(Ljava/lang/String;)[Ljava/lang/String;");
  jniFileSystemMkdir = getMethodIdOrError(env, jniFileSystemClass, "mkdir", "(Ljava/lang/String;)V");
  jniFileSystemRmdir = getMethodIdOrError(env, jniFileSystemClass, "rmdir", "(Ljava/lang/String;)V");

  // methods in JniFilesystem$ReadFile
  jniReadFilePread = getMethodIdOrError(env, jniReadFileClass, "pread", "(JJJ)V");
  jniReadFileShouldCoalesce = getMethodIdOrError(env, jniReadFileClass, "shouldCoalesce", "()Z");
  jniReadFileSize = getMethodIdOrError(env, jniReadFileClass, "size", "()J");
  jniReadFileMemoryUsage = getMethodIdOrError(env, jniReadFileClass, "memoryUsage", "()J");
  jniReadFileGetNaturalReadSize = getMethodIdOrError(env, jniReadFileClass, "getNaturalReadSize", "()J");

  // methods in JniFilesystem$WriteFile
  jniWriteFileAppend = getMethodIdOrError(env, jniWriteFileClass, "append", "([B)V");
  jniWriteFileFlush = getMethodIdOrError(env, jniWriteFileClass, "flush", "()V");
  jniWriteFileClose = getMethodIdOrError(env, jniWriteFileClass, "close", "()V");
  jniWriteFileSize = getMethodIdOrError(env, jniWriteFileClass, "size", "()J");
}

void gluten::finalizeVeloxJniFileSystem(JNIEnv* env) {
  env->DeleteGlobalRef(jniWriteFileClass);
  env->DeleteGlobalRef(jniReadFileClass);
  env->DeleteGlobalRef(jniFileSystemClass);

  vm = nullptr;
}

std::string_view gluten::JniReadFile::pread(uint64_t offset, uint64_t length, void* buf) const {
  JNIEnv* env;
  attachCurrentThreadAsDaemonOrThrow(vm, &env);
  env->CallVoidMethod(
      obj_, jniReadFilePread, static_cast<jlong>(offset), static_cast<jlong>(length), reinterpret_cast<jlong>(buf));
  checkException(env);
  return std::string_view(reinterpret_cast<const char*>(buf));
}

bool gluten::JniReadFile::shouldCoalesce() const {
  JNIEnv* env;
  attachCurrentThreadAsDaemonOrThrow(vm, &env);
  jboolean out = env->CallBooleanMethod(obj_, jniReadFileShouldCoalesce);
  checkException(env);
  return out;
}

uint64_t gluten::JniReadFile::size() const {
  JNIEnv* env;
  attachCurrentThreadAsDaemonOrThrow(vm, &env);
  jlong out = env->CallLongMethod(obj_, jniReadFileSize);
  checkException(env);
  return static_cast<uint64_t>(out);
}

uint64_t gluten::JniReadFile::memoryUsage() const {
  JNIEnv* env;
  attachCurrentThreadAsDaemonOrThrow(vm, &env);
  jlong out = env->CallLongMethod(obj_, jniReadFileMemoryUsage);
  checkException(env);
  return static_cast<uint64_t>(out);
}

uint64_t gluten::JniReadFile::getNaturalReadSize() const {
  JNIEnv* env;
  attachCurrentThreadAsDaemonOrThrow(vm, &env);
  jlong out = env->CallLongMethod(obj_, jniReadFileGetNaturalReadSize);
  checkException(env);
  return static_cast<uint64_t>(out);
}

std::string gluten::JniReadFile::getName() const {
  return "<JniReadFile>";
}

gluten::JniReadFile::JniReadFile(jobject obj) {
  JNIEnv* env;
  attachCurrentThreadAsDaemonOrThrow(vm, &env);
  obj_ = env->NewGlobalRef(obj);
  checkException(env);
}

gluten::JniReadFile::~JniReadFile() {
  JNIEnv* env;
  attachCurrentThreadAsDaemonOrThrow(vm, &env);
  env->DeleteGlobalRef(obj_);
  checkException(env);
}

void gluten::JniWriteFile::append(std::string_view data) {
  JNIEnv* env;
  attachCurrentThreadAsDaemonOrThrow(vm, &env);
  env->CallVoidMethod(obj_, jniWriteFileAppend);
  checkException(env);
}

void gluten::JniWriteFile::flush() {
  JNIEnv* env;
  attachCurrentThreadAsDaemonOrThrow(vm, &env);
  env->CallVoidMethod(obj_, jniWriteFileFlush);
  checkException(env);
}

void gluten::JniWriteFile::close() {
  JNIEnv* env;
  attachCurrentThreadAsDaemonOrThrow(vm, &env);
  env->CallVoidMethod(obj_, jniWriteFileClose);
  checkException(env);
}

uint64_t gluten::JniWriteFile::size() const {
  JNIEnv* env;
  attachCurrentThreadAsDaemonOrThrow(vm, &env);
  jlong out = env->CallLongMethod(obj_, jniWriteFileSize);
  checkException(env);
  return static_cast<uint64_t>(out);
}

gluten::JniWriteFile::JniWriteFile(jobject obj) {
  JNIEnv* env;
  attachCurrentThreadAsDaemonOrThrow(vm, &env);
  obj_ = env->NewGlobalRef(obj);
  checkException(env);
}

gluten::JniWriteFile::~JniWriteFile() {
  JNIEnv* env;
  attachCurrentThreadAsDaemonOrThrow(vm, &env);
  env->DeleteGlobalRef(obj_);
  checkException(env);
}

std::string gluten::JniFileSystem::name() const {
  return "JNI FS";
}

std::unique_ptr<facebook::velox::ReadFile> gluten::JniFileSystem::openFileForRead(
    std::string_view path,
    const facebook::velox::filesystems::FileOptions& options) {
  GLUTEN_CHECK(
      !options.values.empty(),
      "JniFileSystem::openFileForRead: file options is not empty, this is not currently supported");
  JNIEnv* env;
  attachCurrentThreadAsDaemonOrThrow(vm, &env);
  jobject obj = env->CallObjectMethod(obj_, jniFileSystemOpenFileForRead, createJString(env, path));
  auto out = std::make_unique<JniReadFile>(obj);
  checkException(env);
  return out;
}

std::unique_ptr<facebook::velox::WriteFile> gluten::JniFileSystem::openFileForWrite(
    std::string_view path,
    const facebook::velox::filesystems::FileOptions& options) {
  GLUTEN_CHECK(
      !options.values.empty(),
      "JniFileSystem::openFileForWrite: file options is not empty, this is not currently supported");
  JNIEnv* env;
  attachCurrentThreadAsDaemonOrThrow(vm, &env);
  jobject obj = env->CallObjectMethod(obj_, jniFileSystemOpenFileForWrite, createJString(env, path));
  auto out = std::make_unique<JniWriteFile>(obj);
  checkException(env);
  return out;
}

void gluten::JniFileSystem::remove(std::string_view path) {
  JNIEnv* env;
  attachCurrentThreadAsDaemonOrThrow(vm, &env);
  env->CallVoidMethod(obj_, jniFileSystemRemove, createJString(env, path));
  checkException(env);
}

void gluten::JniFileSystem::rename(std::string_view oldPath, std::string_view newPath, bool overwrite) {
  JNIEnv* env;
  attachCurrentThreadAsDaemonOrThrow(vm, &env);
  env->CallVoidMethod(obj_, jniFileSystemRename, createJString(env, oldPath), createJString(env, newPath), overwrite);
  checkException(env);
}

bool gluten::JniFileSystem::exists(std::string_view path) {
  JNIEnv* env;
  attachCurrentThreadAsDaemonOrThrow(vm, &env);
  bool out = env->CallBooleanMethod(obj_, jniFileSystemExists, createJString(env, path));
  checkException(env);
  return out;
}

std::vector<std::string> gluten::JniFileSystem::list(std::string_view path) {
  JNIEnv* env;
  attachCurrentThreadAsDaemonOrThrow(vm, &env);
  std::vector<std::string> out;
  jobjectArray jarray = (jobjectArray)env->CallObjectMethod(obj_, jniFileSystemList, createJString(env, path));
  jsize length = env->GetArrayLength(jarray);
  for (jsize i = 0; i < length; ++i) {
    jstring element = (jstring)env->GetObjectArrayElement(jarray, i);
    std::string cElement = jStringToCString(env, element);
    out.push_back(cElement);
  }
  checkException(env);
  return out;
}

void gluten::JniFileSystem::mkdir(std::string_view path) {
  JNIEnv* env;
  attachCurrentThreadAsDaemonOrThrow(vm, &env);
  env->CallVoidMethod(obj_, jniFileSystemMkdir, createJString(env, path));
  checkException(env);
}

void gluten::JniFileSystem::rmdir(std::string_view path) {
  JNIEnv* env;
  attachCurrentThreadAsDaemonOrThrow(vm, &env);
  env->CallVoidMethod(obj_, jniFileSystemRmdir, createJString(env, path));
  checkException(env);
}

std::function<bool(std::string_view)> gluten::JniFileSystem::schemeMatcher() {
  return [](std::string_view filePath) { return filePath.find(kFileScheme) == 0; };
}

std::function<std::shared_ptr<
    facebook::velox::filesystems::FileSystem>(std::shared_ptr<const facebook::velox::Config>, std::string_view)>
gluten::JniFileSystem::fileSystemGenerator() {
  return [](std::shared_ptr<const facebook::velox::Config> properties, std::string_view filePath) {
    JNIEnv* env;
    attachCurrentThreadAsDaemonOrThrow(vm, &env);
    jobject obj = env->CallStaticObjectMethod(jniFileSystemClass, jniGetFileSystem, createJString(env, filePath));
    std::shared_ptr<FileSystem> lfs = std::make_shared<JniFileSystem>(obj, properties);
    checkException(env);
    return lfs;
  };
}

void gluten::registerJniFileSystem() {
  facebook::velox::filesystems::registerFileSystem(
      JniFileSystem::schemeMatcher(), JniFileSystem::fileSystemGenerator());
}

gluten::JniFileSystem::JniFileSystem(jobject obj, std::shared_ptr<const facebook::velox::Config> config)
    : FileSystem(config) {
  JNIEnv* env;
  attachCurrentThreadAsDaemonOrThrow(vm, &env);
  obj_ = env->NewGlobalRef(obj);
  checkException(env);
}

gluten::JniFileSystem::~JniFileSystem() {
  JNIEnv* env;
  attachCurrentThreadAsDaemonOrThrow(vm, &env);
  env->DeleteGlobalRef(obj_);
  checkException(env);
}