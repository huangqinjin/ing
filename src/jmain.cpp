#include <ing/jmain.h>

#include <cstdlib>
#include <cstring>

#define JMAIN_ALIAS(name)                            \
extern "C" JNIEXPORT jint JNICALL                    \
name(JNIEnv* env, jobject thiz, jobjectArray args)   \
{                                                    \
    return jmain(env, thiz, args);                   \
}

JMAIN_ALIAS(Java_JniMain_main)
JMAIN_ALIAS(Java_JniMain_jmain)
JMAIN_ALIAS(Java_com_github_huangqinjin_JniMain_main)
JMAIN_ALIAS(Java_com_github_huangqinjin_JniMain_jmain)


extern "C" JNIEXPORT jint JNICALL jmain(JNIEnv* env, jobject /*thiz*/, jobjectArray args)
{
    struct cmdline
    {
        jsize argc;
        char** argv;
        char** args;

        static char* strdup(const char* s, jsize n) noexcept
        {
            if (char* p = (char*)std::malloc(n + 1))
            {
                std::memcpy(p, s, n);
                p[n] = '\0';
                return p;
            }
            return nullptr;
        }

        cmdline(JNIEnv* env, jobjectArray args)
            : argc(0), args(nullptr), argv(nullptr)
        {
            this->argc = env->GetArrayLength(args);
            this->argv = (char**)std::malloc((this->argc + 1) * sizeof(char*));
            this->args = (char**)std::calloc(this->argc, sizeof(char*));
            if (this->argv && this->args)
            {
                for (jsize i = 0; i < this->argc; ++i)
                {
                    jstring js = (jstring)env->GetObjectArrayElement(args, i);
                    const char* s = env->GetStringUTFChars(js, nullptr);
                    const jsize n = env->GetStringUTFLength(js);
                    this->argv[i] = this->args[i] = strdup(s, n);
                    env->ReleaseStringUTFChars(js, s);
                }
                this->argv[this->argc] = nullptr;
            }
        }

        ~cmdline()
        {
            if (this->args)
            {
                for (jsize i = 0; i < this->argc; ++i)
                {
                    std::free(this->args[i]);
                }
            }
            std::free(this->args);
            std::free(this->argv);
        }
    } const cmdline(env, args);


    extern int main(int argc, char* argv[]);
    return (jint)main((int)cmdline.argc, (char**)cmdline.argv);
}
