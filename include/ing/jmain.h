#ifndef ING_JMAIN_H
#define ING_JMAIN_H

#include <jni.h>

#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT jint JNICALL jmain(JNIEnv* env, jobject thiz, jobjectArray args);

#ifdef __cplusplus
}
#endif

#endif
