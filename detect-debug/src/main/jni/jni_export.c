//
// Created by 薛祥清 on 16/9/13.
//
#include <jni.h>
#include <android/log.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/inotify.h>

#define TAG "TEST_NDK"
#define LOGV(...)  __android_log_print(ANDROID_LOG_VERBOSE, TAG, __VA_ARGS__)
#define LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)
#define LOGI(...)  __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGW(...)  __android_log_print(ANDROID_LOG_WARN, TAG, __VA_ARGS__)
#define LOGE(...)  __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

#ifndef NELEM //计算元素个数
# define NELEM(x) ((int) (sizeof(x) / sizeof((x)[0])))
#endif

#define EVENT_SIZE (sizeof (struct inotify_event))
#define EVENT_BUFF_LEN (1024 * EVENT_SIZE)
#define BUFF_LEN 1024 * 4
#define PATH_LEN 1024
#define TCP_PORT "5D8A" //23946

JNIEnv *g_env;
jclass native_class;

int stop = 0;

void *inotify_read() {
    LOGD("start by read");
    int fd;                         //文件描述符
    int wd;                         //监视器标识符
    int event_len;                  //事件长度
    char buffer[EVENT_BUFF_LEN];    //事件buffer
    char map_path[PATH_LEN];        //监控文件路径

    stop = 0;                       //初始化监控
    fd = inotify_init();
    pid_t pid = getpid();
    sprintf(map_path, "/proc/%d/", pid); //获取当前APP maps路径
    if (fd == -1) {
        LOGE("inotify_init [errno:%d, desc:%s]", errno, strerror(errno));
        return NULL;
    }
    wd = inotify_add_watch(fd, map_path, IN_ALL_EVENTS);  //添加监控 所有事件
    LOGD("add watch success path:%s", map_path);
    while (1) {
        if (stop == 1) break;       //停止监控

        event_len = read(fd, buffer, EVENT_BUFF_LEN);   //读取事件
        if (event_len < 0) {
            LOGE("inotify_event read failed [errno:%d, desc:%s]", errno, strerror(errno));
            return NULL;
        }
        int i = 0;
        while (i < event_len) {
            struct inotify_event *event = (struct inotify_event *) &buffer[i];
            if (event->len && !strcmp(event->name, "maps")) {
                if (event->mask & IN_CREATE) {
                    LOGD("create: %s", event->name);
                }
                else if (event->mask & IN_DELETE) {
                    LOGD("delete: %s", event->name);
                }
                else if (event->mask & IN_MODIFY) {
                    LOGD("modified: %s", event->name);
                }
                else if (event->mask & IN_ACCESS) {
                    LOGD("access: %s", event->name);
                }
                else if (event->mask & IN_OPEN) {
                    LOGD("open : %s", event->name);
                }
                else {
                    LOGD("other event [name:%s, mask:%x]", event->name, event->mask);
                }
            }
            i += EVENT_SIZE + event->len;
        }
    }
    inotify_rm_watch(fd, wd);
    LOGD("rm watch");
    close(fd);
}

JNIEXPORT void start_inotify_read(JNIEnv env, jobject thiz) {
    pthread_t thread;
    int pid = pthread_create(&thread, NULL, inotify_read, NULL);
    if (pid != 0) {
        LOGE("read thread create failed [errno:%d, desc:%s]", errno, strerror(errno));
    }
    LOGD("read thread create success");
}

void *inotify_select() {
    LOGD("start by select");
    int fd;                         //文件描述符
    int wd;                         //监视器标识符
    int event_len;                  //事件长度
    char buffer[EVENT_BUFF_LEN];          //事件buffer
    char map_path[PATH_LEN];        //监控文件路径

    fd_set fds;                     //fd_set
    struct timeval time_to_wait;    //超时时间 100毫秒
    time_to_wait.tv_sec = 0;
    time_to_wait.tv_usec = 0;

    stop = 0;               //初始化监控
    fd = inotify_init();
    pid_t pid = getpid();
    sprintf(map_path, "/proc/%d/maps", pid); //获取当前APP maps路径
    if (fd == -1) {
        LOGE("inotify_init [errno:%d, desc:%s]", errno, strerror(errno));
        return NULL;
    }
    wd = inotify_add_watch(fd, map_path, IN_ALL_EVENTS);  //添加监控 所有事件
    LOGD("add watch success path:%s", map_path);
    while (1) {
        if (stop == 2) break;       //停止监控

        FD_ZERO(&fds);
        FD_SET(fd, &fds);

        int rev = select(fd + 1, &fds, NULL, NULL, &time_to_wait);//fd, readfds, writefds, errorfds, timeout:NULL阻塞, {0.0}直接过, timeout
        if (rev < 0) {
            //error
            LOGE("select failed [error:%d, desc:%s]", errno, strerror(errno));
        }
        else if (!rev) {
            //timeout
            LOGD("select timeout");
        }
        else {
            //
            event_len = read(fd, buffer, EVENT_BUFF_LEN);   //读取事件
            if (event_len < 0) {
                LOGE("inotify_event read failed [errno:%d, desc:%s]", errno, strerror(errno));
                return NULL;
            }
            int i = 0;
            while (i < event_len) {
                struct inotify_event *event = (struct inotify_event *) &buffer[i];
                if (event->mask & IN_CREATE) {
                    LOGD("create: %s", event->name);
                }
                else if (event->mask & IN_DELETE) {
                    LOGD("delete: %s", event->name);
                }
                else if (event->mask & IN_MODIFY) {
                    LOGD("modified: %s", event->name);
                }
                else if (event->mask & IN_ACCESS) {
                    LOGD("access: %s", event->name);
                }
                else if (event->mask & IN_OPEN) {
                    LOGD("open : %s", event->name);
                }
                else {
                    LOGD("other event [name:%s, mask:%x]", event->name, event->mask);
                }
                i += EVENT_SIZE + event->len;
            }
        }
    }
    inotify_rm_watch(fd, wd);
    LOGD("rm watch");
    close(fd);
}

JNIEXPORT void start_inotify_select(JNIEnv env, jobject thiz) {
    pthread_t thread;
    int pid = pthread_create(&thread, NULL, inotify_select, NULL);
    if (pid != 0) {
        LOGE("select thread create failed [errno:%d, desc:%s]", errno, strerror(errno));
    }
    LOGE("select thread create success");
}

JNIEXPORT void stop_inotify(JNIEnv env, jobject thiz, jint type) {
    LOGD("stop_inotify");
    stop = type;
}

JNIEXPORT void net_monitor(JNIEnv env, jobject thiz){
    LOGD("net_monitor");
    char buff[BUFF_LEN];

    FILE *fp;
    const char dir[] = "/proc/net/tcp";
    fp = fopen(dir, "r");
    if(fp == NULL){
        LOGE("file failed [errno:%d, desc:%s]", errno, strerror(errno));
        return;
    }
    while(fgets(buff, BUFF_LEN, fp)){
        if(strstr(buff, TCP_PORT) != NULL){
            LOGI("Line:%s", buff);
            fclose(fp);
            return;
        }
    }
}


static JNINativeMethod methods[] = {
        {"startInotifyByRead",   "()V",  start_inotify_read},
        {"startInotifyBySelect", "()V",  start_inotify_select},
        {"stopInotify",          "(I)V", stop_inotify},
        {"netMonitor",           "()V",  net_monitor},
};

/*
 * 库加载时注册native函数
 */
JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *reserved) {
    if (JNI_OK != (*vm)->GetEnv(vm, (void **) &g_env, JNI_VERSION_1_6)) {
        return -1;
    }
    LOGV("JNI_OnLoad()");
    native_class = (*g_env)->FindClass(g_env, "cn/tongdun/android/inotify/MainActivity");
    if (JNI_OK == (*g_env)->RegisterNatives(g_env, native_class, methods, NELEM(methods))) {
        LOGV("RegisterNatives() --> ok");
    } else {
        LOGE("RegisterNatives() --> failed");
        return -1;
    }

    return JNI_VERSION_1_6;
}

void JNI_OnUnLoad(JavaVM *vm, void *reserved) {
    LOGV("JNI_OnUnLoad()");
    (*g_env)->UnregisterNatives(g_env, native_class);
    LOGV("UnregisterNatives()");
}