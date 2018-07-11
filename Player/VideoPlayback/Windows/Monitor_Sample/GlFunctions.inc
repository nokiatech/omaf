
/** 
 * This file is part of Nokia OMAF implementation
 *
 * Copyright (c) 2018 Nokia Corporation and/or its subsidiary(-ies). All rights reserved.
 *
 * Contact: omaf@nokia.com
 *
 * This software, including documentation, is protected by copyright controlled by Nokia Corporation and/ or its
 * subsidiaries. All rights are reserved.
 *
 * Copying, including reproducing, storing, adapting or translating, any or all of this material requires the prior
 * written consent of Nokia.
 */
declare(PFNWGLGETSWAPINTERVALEXTPROC , wglGetSwapIntervalEXT)
declare(PFNWGLSWAPINTERVALEXTPROC , wglSwapIntervalEXT)
declare(PFNWGLCREATECONTEXTATTRIBSARBPROC, wglCreateContextAttribsARB)

declare(PFNGLVERTEXATTRIB2FPROC , glVertexAttrib2f)
declare(PFNGLVERTEXATTRIB4FPROC , glVertexAttrib4f)

//buffers
declare(PFNGLBLITFRAMEBUFFERPROC, glBlitFramebuffer)

declare(PFNGLGENBUFFERSPROC , glGenBuffers)
declare(PFNGLBINDBUFFERPROC , glBindBuffer)
declare(PFNGLBUFFERDATAPROC , glBufferData)
declare(PFNGLUNMAPBUFFERPROC , glUnmapBuffer)
declare(PFNGLMAPBUFFERPROC , glMapBuffer)

declare(PFNGLDELETEBUFFERSPROC , glDeleteBuffers)
declare(PFNGLGENVERTEXARRAYSPROC , glGenVertexArrays)
declare(PFNGLBINDVERTEXARRAYPROC , glBindVertexArray)
declare(PFNGLDELETEVERTEXARRAYSPROC , glDeleteVertexArrays)
declare(PFNGLBINDATTRIBLOCATIONPROC , glBindAttribLocation)

//FBO/Texture related..
declare(PFNGLACTIVETEXTUREPROC , glActiveTexture)
declare(PFNGLBINDFRAMEBUFFERPROC , glBindFramebuffer)
declare(PFNGLDELETEFRAMEBUFFERSPROC , glDeleteFramebuffers)
declare(PFNGLGENFRAMEBUFFERSPROC , glGenFramebuffers)
declare(PFNGLFRAMEBUFFERTEXTURE2DPROC , glFramebufferTexture2D)
declare(PFNGLGENRENDERBUFFERSPROC , glGenRenderbuffers)
declare(PFNGLFRAMEBUFFERRENDERBUFFERPROC , glFramebufferRenderbuffer)
declare(PFNGLDELETERENDERBUFFERSPROC , glDeleteRenderbuffers)
declare(PFNGLBINDRENDERBUFFERPROC , glBindRenderbuffer)
declare(PFNGLRENDERBUFFERSTORAGEPROC , glRenderbufferStorage)
declare(PFNGLCHECKFRAMEBUFFERSTATUSPROC , glCheckFramebufferStatus)
declare(PFNGLINVALIDATEFRAMEBUFFERPROC , glInvalidateFramebuffer)

//draw/read buffers.
declare(PFNGLDRAWBUFFERSPROC , glDrawBuffers)

//Blend funcs
declare(PFNGLBLENDFUNCSEPARATEPROC , glBlendFuncSeparate)
declare(PFNGLBLENDEQUATIONSEPARATEPROC , glBlendEquationSeparate)
declare(PFNGLBLENDCOLORPROC , glBlendColor)

//Stencil...
declare(PFNGLSTENCILOPSEPARATEPROC , glStencilOpSeparate)
declare(PFNGLSTENCILFUNCSEPARATEPROC , glStencilFuncSeparate) 

//Shader stuff
declare(PFNGLCREATEPROGRAMPROC , glCreateProgram)
declare(PFNGLDELETESHADERPROC , glDeleteShader)
declare(PFNGLDELETEPROGRAMPROC , glDeleteProgram)
declare(PFNGLATTACHSHADERPROC , glAttachShader)
declare(PFNGLLINKPROGRAMPROC , glLinkProgram)
declare(PFNGLUSEPROGRAMPROC , glUseProgram)
declare(PFNGLENABLEVERTEXATTRIBARRAYPROC , glEnableVertexAttribArray)
declare(PFNGLDISABLEVERTEXATTRIBARRAYPROC , glDisableVertexAttribArray)
declare(PFNGLGETUNIFORMLOCATIONPROC , glGetUniformLocation)
declare(PFNGLGETATTRIBLOCATIONPROC , glGetAttribLocation)
declare(PFNGLGETACTIVEATTRIBPROC , glGetActiveAttrib)
declare(PFNGLGETACTIVEUNIFORMPROC , glGetActiveUniform)

declare(PFNGLUNIFORM1IPROC , glUniform1i)

declare(PFNGLUNIFORM1FPROC , glUniform1f)

declare(PFNGLUNIFORM2IVPROC , glUniform2iv)
declare(PFNGLUNIFORM3IVPROC , glUniform3iv)
declare(PFNGLUNIFORM4IVPROC , glUniform4iv)

declare(PFNGLUNIFORM2FVPROC , glUniform2fv)
declare(PFNGLUNIFORM3FVPROC , glUniform3fv)
declare(PFNGLUNIFORM4FVPROC , glUniform4fv)

declare(PFNGLUNIFORMMATRIX2FVPROC , glUniformMatrix2fv)
declare(PFNGLUNIFORMMATRIX3FVPROC , glUniformMatrix3fv)
declare(PFNGLUNIFORMMATRIX4FVPROC , glUniformMatrix4fv)
declare(PFNGLVERTEXATTRIBPOINTERPROC , glVertexAttribPointer)
declare(PFNGLCREATESHADERPROC , glCreateShader)
declare(PFNGLCOMPILESHADERPROC , glCompileShader)
declare(PFNGLSHADERSOURCEPROC , glShaderSource)
declare(PFNGLGETPROGRAMIVPROC , glGetProgramiv)
declare(PFNGLVALIDATEPROGRAMPROC , glValidateProgram)
declare(PFNGLGETPROGRAMINFOLOGPROC , glGetProgramInfoLog)
declare(PFNGLGETSHADERIVPROC , glGetShaderiv)
declare(PFNGLGETSHADERINFOLOGPROC , glGetShaderInfoLog)

//automipmap
declare(PFNGLGENERATEMIPMAPPROC , glGenerateMipmap )

//
declare(PFNGLGETSTRINGIPROC , glGetStringi)

declare(PFNGLCOMPRESSEDTEXIMAGE2DPROC , glCompressedTexImage2D)

declare(PFNGLDEBUGMESSAGECONTROLPROC, glDebugMessageControl)
declare(PFNGLDEBUGMESSAGECALLBACKPROC, glDebugMessageCallback)