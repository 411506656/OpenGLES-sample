#include <math.h>
#include  <sys/time.h>
#include  <X11/Xlib.h>
#include  <X11/Xatom.h>
#include  <X11/Xutil.h>
#include <stdio.h>
#include  <GLES2/gl2.h>
#include  <EGL/egl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>


// 宏定义 
#define VERTEX_ARRAY   0 
  
// 定义Display、config、surface、context 
EGLDisplay          eglDisplay  = 0; 
EGLConfig           eglConfig   = 0; 
EGLSurface          eglSurface  = 0; 
EGLContext          eglContext  = 0; 
EGLNativeWindowType eglWindow   = 0; 
  
// 窗口指针 
Window g_pThis; 
  
bool TestEGLError() 
{ 
    //eglGetError返回上一个egl中的错误，用户在每个egl函数调用结束都需要调用这个函数。 
    EGLint iErr = eglGetError(); 
    if (iErr != EGL_SUCCESS) 
    { 
        return false; 
    } 
  
    return true; 
} 
  
bool CreateEGLContext() 
{ 
    // 第一步：获得或者创建一个可以用于OpenGL ES输出的EGLNativeWindowType 
    eglWindow = (EGLNativeWindowType)g_pThis; 
  
    //第二步：获得默认的Display。通常我们只有一块屏幕，参数传EGL_DEFAULT_DISPLAY就可以了。 
    eglDisplay = eglGetDisplay((EGLNativeDisplayType) EGL_DEFAULT_DISPLAY); 
  
    //第三步：初始化EGL，如果我们不想要版本号，后两个参数也可以传NULL进去。 
    EGLint iMajorVersion, iMinorVersion; 
    if (!eglInitialize(eglDisplay, &iMajorVersion, &iMinorVersion)) 
    { 
        return false; 
    } 
  
    //第四步：指定需要的配置属性，一个EGL的配置描述了Surfaces上像素的格式信息。当前我们要的是Windows的surface，在屏幕上是可见的，以EGL_NONE结尾。 
    const EGLint pi32ConfigAttribs[] = 
    { 
        EGL_LEVEL,              0, 
        EGL_SURFACE_TYPE,       EGL_WINDOW_BIT, 
        EGL_RENDERABLE_TYPE,    EGL_OPENGL_ES2_BIT, 
        EGL_NATIVE_RENDERABLE,  EGL_FALSE, 
        EGL_DEPTH_SIZE,         EGL_DONT_CARE, 
        EGL_NONE 
    }; 
  
    //第五步：寻找一个符合所有要求的配置，我们需要的只是其中一个，所以可以限制config的个数为1。 
    int iConfigs; 
    if (!eglChooseConfig(eglDisplay, pi32ConfigAttribs, &eglConfig, 1, &iConfigs) || (iConfigs != 1)) 
    { 
        return false; 
    } 
  
    //第六步：创建一个可画的surface。这里创建时可见的windows surface。Pixmaps和pbuffers都是不可见的。 
    eglSurface = eglCreateWindowSurface(eglDisplay, eglConfig, eglWindow, NULL); 
  
    if(eglSurface == EGL_NO_SURFACE) 
    { 
        eglGetError(); // Clear error 
        eglSurface = eglCreateWindowSurface(eglDisplay, eglConfig, NULL, NULL); 
    } 
  
    if (!TestEGLError()) 
    { 
        return false; 
    } 
  
    //第七步：创建Context。我们OpenGL ES的资源，比如纹理只有在这个context里是可见的。 
    // 绑定API (可以是OpenGLES或者 OpenVG) 
    eglBindAPI(EGL_OPENGL_ES_API); 
    EGLint ai32ContextAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE }; 
    eglContext = eglCreateContext(eglDisplay, eglConfig, NULL, ai32ContextAttribs); 
  
    if (!TestEGLError()) 
    { 
        return false; 
    } 
  
    //第八步：将创建的context绑定到当前的线程，使用我们创建的surface进行读和写。Context绑定到线程上，你就可以不用担心其他的进程影响你的OpenGL ES应用程序。 
    eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext); 
    if (!TestEGLError()) 
    { 
        return false; 
    } 
    return true; 
} 
 

//--------------------------------------------------------------------------------------------------------------

bool Render() 
{ 
    //第九步：使用OpenGL ES API画一些东西。到这里，所有的东西都已准备就绪，我们做好了往屏幕上画东西的准备。 
    bool bRet = false; 
  
    //单元矩阵，用于投影和模型变换 
    float pfIdentity[] = 
    { 
        1.0f,0.0f,0.0f,0.0f, 
        0.0f,1.0f,0.0f,0.0f, 
        0.0f,0.0f,1.0f,0.0f, 
        0.0f,0.0f,0.0f,1.0f 
    }; 
  
    // Vertex和Fragment的shader 
    // gl_FragColor指定了最终的像素颜色。 
    // gl_position则指定了最终的点在人眼坐标中的位置。 
    char szFragShaderSrc[] = {"\ 
                              void main (void)\ 
                              {\ 
                              gl_FragColor = vec4(1.0, 1.0, 0.66 ,1.0);\  
                              }"}; 
    char szVertShaderSrc[] = {"\ 
                              attribute highp vec4  myVertex;\ 
                              uniform mediump mat4  myPMVMatrix;\ 
                              void main(void)\ 
                              {\ 
                              gl_Position = myPMVMatrix * myVertex;\ 
                              }"}; 
  
    char * pszFragShader = (char *)szFragShaderSrc; 
    char * pszVertShader = (char *)szVertShaderSrc; 
  
    GLuint uiFragShader = 0; 
    GLuint uiVertShader = 0;     /* 用来存放Vertex和Fragment shader的句柄 */ 
    GLuint uiProgramObject = 0;                  /* 用来存放创建的program的句柄*/ 
  
    GLint bShaderCompiled; 
    GLint bLinked; 
  
    // 我们要画一个三角形，所以，我们先创建一个顶点缓冲区 
    GLuint  ui32Vbo = 0; // 顶点缓冲区对象句柄 
  
    //顶点数据 这9个数据分别为3个点的X、Y、Z坐标 
    GLfloat afVertices[] = { -0.4f,-0.4f,0.0f, // Position 
        0.4f ,-0.4f,0.0f, 
        0.0f ,0.4f ,0.0f}; 
  
    int i32InfoLogLength, i32CharsWritten; 
    char* pszInfoLog = NULL; 
    int i32Location = 0; 
    unsigned int uiSize = 0; 
  
    //创建Fragment着色器对象 
    uiFragShader = glCreateShader(GL_FRAGMENT_SHADER); 
  
    // 将代码加载进来 
    glShaderSource(uiFragShader, 1, (const char**)&pszFragShader, NULL); 
  
    //编译代码 
    glCompileShader(uiFragShader); 
  
    //看编译是否成功进行 
    glGetShaderiv(uiFragShader, GL_COMPILE_STATUS, &bShaderCompiled); 
  
    if (!bShaderCompiled) 
    { 
        ;
	// 错误发生，首先获取错误的长度 
        //glGetShaderiv(uiFragShader, GL_INFO_LOG_LENGTH, &i32InfoLogLength); 
        //开辟足够的空间来存储错误信息 
        //pszInfoLog = new char[i32InfoLogLength]; 
        //glGetShaderInfoLog(uiFragShader, i32InfoLogLength, &i32CharsWritten, pszInfoLog); 
        //delete[] pszInfoLog; 
        //goto cleanup; 
    } 
  
    // 使用同样的方法加载Vertex Shader 
    uiVertShader = glCreateShader(GL_VERTEX_SHADER); 
    glShaderSource(uiVertShader, 1, (const char**)&pszVertShader, NULL); 
    glCompileShader(uiVertShader); 
    glGetShaderiv(uiVertShader, GL_COMPILE_STATUS, &bShaderCompiled); 
    if (!bShaderCompiled) 
    { 
        ;
	//glGetShaderiv(uiVertShader, GL_INFO_LOG_LENGTH, &i32InfoLogLength); 
        //pszInfoLog = new char[i32InfoLogLength]; 
        //glGetShaderInfoLog(uiVertShader, i32InfoLogLength, &i32CharsWritten, pszInfoLog); 
        //delete[] pszInfoLog; 
        //goto cleanup; 
    } 
  
    // 创建着色器程序 
    uiProgramObject = glCreateProgram(); 
  
    // 将Vertex和Fragment Shader绑定进去。 
    glAttachShader(uiProgramObject, uiFragShader); 
    glAttachShader(uiProgramObject, uiVertShader); 
  
    //将用户自定义的顶点属性myVertex绑定到VERTEX_ARRAY。 
    glBindAttribLocation(uiProgramObject, VERTEX_ARRAY, "myVertex"); 
  
    // 链接着色器程序 
    glLinkProgram(uiProgramObject); 
  
    // 判断链接是否成功的操作 
    glGetProgramiv(uiProgramObject, GL_LINK_STATUS, &bLinked); 
    if (!bLinked) 
    { 
        ;
	//glGetProgramiv(uiProgramObject, GL_INFO_LOG_LENGTH, &i32InfoLogLength); 
        //pszInfoLog = new char[i32InfoLogLength]; 
        //glGetProgramInfoLog(uiProgramObject, i32InfoLogLength, &i32CharsWritten, pszInfoLog); 
        //delete[] pszInfoLog; 
        //goto cleanup; 
    } 
  
    // 使用着色器程序 
    glUseProgram(uiProgramObject); 
  
    // 设置清除颜色，以RGBA的模式，每个分量的值从0.0到1.0 
    glClearColor(0.6f, 0.8f, 1.0f, 1.0f); 
  
    //生成一个顶点缓冲区对象 
    glGenBuffers(1, &ui32Vbo); 
  
    //绑定生成的缓冲区对象到GL_ARRAY_BUFFER 
    glBindBuffer(GL_ARRAY_BUFFER, ui32Vbo); 
  
    // 加载顶点数据 
    uiSize = 3 * (sizeof(GLfloat) * 3); // Calc afVertices size (3 vertices * stride (3 GLfloats per vertex)) 
    glBufferData(GL_ARRAY_BUFFER, uiSize, afVertices, GL_STATIC_DRAW); 
  
    // 画三角形 
    { 
        //清除颜色缓冲区。glClear同样也能清除深度缓冲区(GL_DEPTH_BUFFER)和模板缓冲区(GL_STENCIL_BUFFER_BIT) 
        glClear(GL_COLOR_BUFFER_BIT); 
  
  
        //获取myPMVMatrix在shader中的位置 
        i32Location = glGetUniformLocation(uiProgramObject, "myPMVMatrix"); 
  
        //传值给获取到的位置，也就是将pfIdentity传给myPMVMatrix 
        glUniformMatrix4fv( i32Location, 1, GL_FALSE, pfIdentity); 
  
        //将VERTEX_ARRAY置为有效。 
        glEnableVertexAttribArray(VERTEX_ARRAY); 
  
        // 将定点数据传到VERTEX_ARRAY 
        glVertexAttribPointer(VERTEX_ARRAY, 3, GL_FLOAT, GL_FALSE, 0, 0); 
  
        //画一个三角形。 
        glDrawArrays(GL_TRIANGLES, 0, 3); 
  
        //SwapBuffers。就可以将三角形显示出来 
        eglSwapBuffers(eglDisplay, eglSurface); 
    } 
    bRet = true; 
  
cleanup: 
    // 释放资源 
    if (uiProgramObject) 
        glDeleteProgram(uiProgramObject); 
    if (uiFragShader) 
        glDeleteShader(uiFragShader); 
    if (uiVertShader) 
        glDeleteShader(uiVertShader); 
  
    // Delete the VBO as it is no longer needed 
    if (ui32Vbo) 
        glDeleteBuffers(1, &ui32Vbo); 
  
    return bRet; 
} 
//-------------------------------------------------------------------------------------------------------  
bool DestroyEGLContext() 
{ 
    //第十步：结束OpenGL ES并删除创建的Windows（如果存在的话）.eglminate已经负责清除context和surface。所以调用了eglTerminate后就无需再调用eglDestroySurface和eglDestroyContext了。 
    if (eglDisplay != EGL_NO_DISPLAY) 
    { 
        eglMakeCurrent(eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT); 
        eglDestroyContext(eglDisplay, eglContext ); 
        eglDestroySurface(eglDisplay, eglSurface ); 
        eglTerminate(eglDisplay); 
        eglDisplay = EGL_NO_DISPLAY; 
    } 
  
    //第十一步：删除已经创建的eglWindow。这个是跟具体平台有关的。 
    eglWindow = NULL; 
    return true; 
}



int  main()
{
	CreateEGLContext();
	Render(); 
	DestroyEGLContext();
	return 0; 
} 
