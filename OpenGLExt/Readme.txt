
------ license start marker -----

This file is part of Nokia OMAF implementation

Copyright (c) 2018 Nokia Corporation and/or its subsidiary(-ies). All rights reserved.

Contact: omaf@nokia.com

This software, including documentation, is protected by copyright controlled by Nokia Corporation and/ or its
subsidiaries. All rights are reserved.

Copying, including reproducing, storing, adapting or translating, any or all of this material requires the prior
written consent of Nokia.

------ license end marker -----

When building the player for Windows, please download
https://www.khronos.org/registry/OpenGL/api/GL/glext.h
https://www.khronos.org/registry/OpenGL/api/GL/wglext.h
and place them under the gl-folder and wgl-folder, respectively.
In addition, the recent versions need a common header, so download
https://www.khronos.org/registry/EGL/api/KHR/khrplatform.h
and place it under the KHR-folder.

for example like this:

curl -o gl/glext.h https://www.khronos.org/registry/OpenGL/api/GL/glext.h
curl -o wgl/wglext.h https://www.khronos.org/registry/OpenGL/api/GL/wglext.h
curl -o KHR/khrplatform.h https://www.khronos.org/registry/EGL/api/KHR/khrplatform.h

For more information, see
http://www.opengl.org/registry/
