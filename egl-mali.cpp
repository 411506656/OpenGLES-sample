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



bool setupGraphics(int width, int height)
{
    /* Full paths to the shader files */
    string vertexShaderPath = "Triangle_triangle.vert"; 
    string fragmentShaderPath = "Triangle_triangle.frag";
 
    GLuint vertexShaderID = 0;
    GLuint fragmentShaderID = 0;
 
    /* Initialize OpenGL ES. */
    GL_CHECK(glEnable(GL_BLEND));
    /* Should do: src * (src alpha) + dest * (1-src alpha). */
    GL_CHECK(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
 
    /* Process shaders. */
    Shader::processShader(&vertexShaderID, vertexShaderPath.c_str(), GL_VERTEX_SHADER);
    Shader::processShader(&fragmentShaderID, fragmentShaderPath.c_str(), GL_FRAGMENT_SHADER);
 
    programID = GL_CHECK(glCreateProgram());
    if (programID == 0)
    {
        //Could not create Program error
        return false;
    }
 
    GL_CHECK(glAttachShader(programID, vertexShaderID));
    GL_CHECK(glAttachShader(programID, fragmentShaderID));
    GL_CHECK(glLinkProgram(programID));
    GL_CHECK(glUseProgram(programID));
 
    /* Positions. */
    GL_CHECK(iLocPosition = glGetAttribLocation(programID, "a_v4Position"));
    /* Fill colors. */
    GL_CHECK(iLocFillColor = glGetAttribLocation(programID, "a_v4FillColor"));
 
    GL_CHECK(glViewport(0, 0, width, height));
 
    /* Set clear screen color. */
    GL_CHECK(glClearColor(0.0f, 0.0f, 1.0f, 1.0f));
 
    return true;
}




void renderFrame(void)
{
   GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
   GL_CHECK(glUseProgram(programID));
 
    /* Pass the triangle vertex positions to the shader */
    GL_CHECK(glVertexAttribPointer(iLocPosition, 3, GL_FLOAT, GL_FALSE, 0, triangleVertices));
 
    GL_CHECK(glEnableVertexAttribArray(iLocPosition));
 
    if(iLocFillColor != -1)
    {
        /* Pass the vertex colours to the shader */
        GL_CHECK(glVertexAttribPointer(iLocFillColor, 4, GL_FLOAT, GL_FALSE, 0, triangleColors));
        GL_CHECK(glEnableVertexAttribArray(iLocFillColor));
    }
 
    GL_CHECK(glDrawArrays(GL_TRIANGLES, 0, 3));
}







int main(void)
{
    /* Intialize the Platform object for platform specific functions. */
    Platform *platform = Platform::getInstance();
 
    /* Initialize windowing system. */
    platform->createWindow(WINDOW_W, WINDOW_H);
 
    /* Initialize EGL. */
    EGLRuntime::initializeEGL(EGLRuntime::OPENGLES2);
    EGL_CHECK(eglMakeCurrent(EGLRuntime::display, EGLRuntime::surface, EGLRuntime::surface, EGLRuntime::context));
 
    /* Initialize OpenGL ES graphics subsystem. */
    setupGraphics(WINDOW_W, WINDOW_H);
 
    bool end = false;
    /* The rendering loop to draw the scene. */
    while(!end)
    {
        /* If something has happened to the window, end the sample. */
        if(platform->checkWindow() != Platform::WINDOW_IDLE)
        {
            end = true;
        }
        /* Render a single frame */
        renderFrame();
 
        /* 
         * Push the EGL surface color buffer to the native window.
         * Causes the rendered graphics to be displayed on screen.
         */
        eglSwapBuffers(EGLRuntime::display, EGLRuntime::surface);
    }
 
    /* Shut down EGL. */
    EGLRuntime::terminateEGL();
 
    /* Shut down windowing system. */
    platform->destroyWindow();
 
    /* Shut down the Platform object*/
    delete platform;
 
    return 0;
}


