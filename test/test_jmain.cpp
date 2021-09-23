#include <boost/test/unit_test.hpp>

#include <jni.h>

JavaVM* vmInstance = nullptr;

jint JNI_OnLoad(JavaVM* vm, void* /*reserved*/)
{
    vmInstance = vm;
    return JNI_VERSION_1_6;
}

BOOST_AUTO_TEST_CASE(jvm)
{
    JavaVM* vm = nullptr;
    jsize count = 0;
    BOOST_TEST(JNI_OK == JNI_GetCreatedJavaVMs(&vm, 1, &count));
    BOOST_TEST(count == 1);
    BOOST_TEST(vm == vmInstance);
    BOOST_TEST_REQUIRE(vm != nullptr);

    JNIEnv* env = (JNIEnv*)-1;
    BOOST_TEST(vm->GetEnv((void**)&env, -1) == JNI_EVERSION);
    BOOST_TEST(env == nullptr);

    jint r = vm->GetEnv((void**)&env, JNI_VERSION_1_6);
    BOOST_TEST(r != JNI_EDETACHED);
    BOOST_TEST(r != JNI_EVERSION);
    BOOST_TEST(r == JNI_OK);
    BOOST_TEST_REQUIRE(env != nullptr);
    BOOST_TEST(env->GetVersion() >= JNI_VERSION_1_6);
}
