#include <jni.h>
#include <string>
#include <dlfcn.h>

#include "android/log.h"

#include "CL/cl.h"

// OpenCL.so読込のための型の定義
namespace cl {
    typedef cl_int(CL_API_CALL *PFN_clGetPlatformIDs)(
            cl_uint /* num_entries */, cl_platform_id * /* platforms */,
            cl_uint * /* num_platforms */) CL_API_SUFFIX__VERSION_1_0;
    typedef cl_int(CL_API_CALL *PFN_clGetDeviceIDs)(
            cl_platform_id /* platform */, cl_device_type /* device_type */,
            cl_uint /* num_entries */, cl_device_id * /* devices */,
            cl_uint * /* num_devices */) CL_API_SUFFIX__VERSION_1_0;
    typedef cl_context(CL_API_CALL *PFN_clCreateContext)(
            const cl_context_properties * /* properties */, cl_uint /* num_devices */,
            const cl_device_id * /* devices */,
            void(CL_CALLBACK * /* pfn_notify */)(const char *, const void *, size_t,
                                                 void *),
            void * /* user_data */,
            cl_int * /* errcode_ret */) CL_API_SUFFIX__VERSION_1_0;
    typedef cl_program(CL_API_CALL *PFN_clCreateProgramWithSource)(
            cl_context /* context */, cl_uint /* count */, const char ** /* strings */,
            const size_t * /* lengths */,
            cl_int * /* errcode_ret */) CL_API_SUFFIX__VERSION_1_0;
    typedef cl_int(CL_API_CALL *PFN_clBuildProgram)(
            cl_program /* program */, cl_uint /* num_devices */,
            const cl_device_id * /* device_list */, const char * /* options */,
            void(CL_CALLBACK * /* pfn_notify */)(cl_program /* program */,
                                                 void * /* user_data */),
            void * /* user_data */) CL_API_SUFFIX__VERSION_1_0;

    PFN_clGetPlatformIDs clGetPlatformIDs;
    PFN_clGetDeviceIDs clGetDeviceIDs;
    PFN_clCreateContext clCreateContext;
    PFN_clCreateProgramWithSource clCreateProgramWithSource;
    PFN_clBuildProgram clBuildProgram;
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_android_example_opencl_1keyence_MainActivity_stringFromJNI(
        JNIEnv* env,
        jobject /* this */) {
    // OpenCL.so読込
    void* opencllib = dlopen("libOpenCL.so", RTLD_NOW | RTLD_LOCAL);
    cl::clGetPlatformIDs = reinterpret_cast<cl::PFN_clGetPlatformIDs>(dlsym(opencllib, "clGetPlatformIDs"));
    cl::clGetDeviceIDs = reinterpret_cast<cl::PFN_clGetDeviceIDs>(dlsym(opencllib, "clGetDeviceIDs"));
    cl::clCreateContext = reinterpret_cast<cl::PFN_clCreateContext>(dlsym(opencllib, "clCreateContext"));
    cl::clCreateProgramWithSource = reinterpret_cast<cl::PFN_clCreateProgramWithSource>(dlsym(opencllib, "clCreateProgramWithSource"));
    cl::clBuildProgram = reinterpret_cast<cl::PFN_clBuildProgram>(dlsym(opencllib, "clBuildProgram"));

    int err;

    // OpenCLの初期化
    cl_uint num_platforms;
    cl_platform_id platform;
    err = cl::clGetPlatformIDs(1, &platform, &num_platforms);
    __android_log_print(ANDROID_LOG_DEBUG, "OpenCLKeyence", "clGetPlatformIDs result: %i\n", err);
    cl_device_id device_id;
    err = cl::clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device_id, NULL);
    __android_log_print(ANDROID_LOG_DEBUG, "OpenCLKeyence", "clGetDeviceIDs result: %i\n", err);
    cl_context context = cl::clCreateContext(0, 1, &device_id, NULL, NULL, &err);
    __android_log_print(ANDROID_LOG_DEBUG, "OpenCLKeyence", "clCreateContext result: %i\n", err);

    const char* source =
        "__constant sampler_t smp_none = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_NONE | CLK_FILTER_NEAREST;\n"
        "__constant sampler_t smp_zero = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP | CLK_FILTER_NEAREST;\n"
        "#pragma OPENCL EXTENSION cl_khr_fp16 : enable\n"
        "__kernel void bhwc_to_tensor(__global float* src, __write_only image2d_t tensor_image2d,\n"
        "  int4 shared_int4_0,"
        "  int4 shared_int4_1) {"
        "  int linear_id = get_global_id(0);"
        "  int x = linear_id / shared_int4_0.x;"
        "  int b = linear_id % shared_int4_0.x;"
        "  int y = get_global_id(1);"
        "  int d = get_global_id(2);"
        "  if (x >= shared_int4_1.x || y >= shared_int4_0.z || d >= shared_int4_0.w) return;"
        "  half4 result;"
        "  int c = d * 4;"
        "  int index = ((b * shared_int4_0.z + y) * shared_int4_1.x + x) * shared_int4_0.y + c;"
        "  result.x = src[index];"
        "  result.y = c + 1 < shared_int4_0.y ? src[index + 1] : 1;"
        "  result.z = c + 2 < shared_int4_0.y ? src[index + 2] : 2;"
        "  result.w = c + 3 < shared_int4_0.y ? src[index + 3] : 3;"
        "  write_imageh(tensor_image2d, (int2)(((x) * shared_int4_0.x + (b)), ((y) * shared_int4_0.w + (d))), result);"
        "}";
    cl_program program = cl::clCreateProgramWithSource(context, 1, &source, NULL, &err);
    __android_log_print(ANDROID_LOG_DEBUG, "OpenCLKeyence", "clCreateProgramWithSource result: %i\n", err);

    // 以下の行でエラーが発生する
    err = cl::clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
    __android_log_print(ANDROID_LOG_DEBUG, "OpenCLKeyence", "clBuildProgram result: %i\n", err);

    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}
