
/**
 * This file is part of Nokia OMAF implementation
 *
 * Copyright (c) 2018-2021 Nokia Corporation and/or its subsidiary(-ies). All rights reserved.
 *
 * Contact: omaf@nokia.com
 *
 * This software, including documentation, is protected by copyright controlled by Nokia Corporation and/ or its
 * subsidiaries. All rights are reserved.
 *
 * Copying, including reproducing, storing, adapting or translating, any or all of this material requires the prior
 * written consent of Nokia.
 */
#include "Renderer/NVRVideoRenderer.h"

OMAF_NS_BEGIN

VideoRenderer::VideoRenderer()
    : mValid(false)
{
}

VideoRenderer::~VideoRenderer()
{
}

void_t VideoRenderer::create()
{
    createImpl();

    mValid = true;
}

void_t VideoRenderer::destroy()
{
    if (isValid())
    {
        destroyImpl();
    }

    mValid = false;
}

bool_t VideoRenderer::isValid()
{
    return mValid;
}

OMAF_NS_END
