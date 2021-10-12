
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
#include "NVRCubeMapRenderer.h"
#include "Graphics/NVRRenderBackend.h"

#include "Foundation/NVRLogger.h"
#include "Foundation/NVRNew.h"

namespace
{
    static const uint8_t MAX_FACE_TILES =
        32;  // maximum number of uniform slots in the shader for one set of target coords
}

OMAF_NS_BEGIN
OMAF_LOG_ZONE(CubeMapRenderer)

CubeMapRenderer::CubeMapRenderer()
    : mDirtyShader(true)
{
}

CubeMapRenderer::~CubeMapRenderer()
{
    destroy();
}

void_t CubeMapRenderer::createImpl()
{
    mGeometry.create(1, 1);
}

void_t CubeMapRenderer::destroyImpl()
{
    mGeometry.destroy();
    mOpaqueShader.destroy();

    OMAF_DELETE_ARRAY_HEAP(mFaceData.mIndices);
    OMAF_DELETE_ARRAY_HEAP(mFaceData.mSourceCoordinates);
    OMAF_DELETE_ARRAY_HEAP(mFaceData.mTargetCoordinates);
    OMAF_DELETE_ARRAY_HEAP(mFaceData.mSourceOrientations);
}
static const char_t* fragment_code = R"(

    //detect face, by looking at the major pointing direction
    //http://www.nvidia.com/object/cube_map_ogl_tutorial.html (Mapping Texture Coordinates to Cube Map Faces)
    //(order in the mapping function: 0-back 1-left 2-front 3-right 4-top 5-bottom)

    int face = 0;
    float3 absDir = abs(dir);
    float2 uv;    //normalized texture coordinates ("inside a face")

    //left or right face
    if(absDir.x>absDir.y && absDir.x>absDir.z)
    {
        //left
        if(0.0>dir.x)
        {
            face = 1;
            uv = float2( -dir.z, -dir.y);
        }
        //right
        else
        {
            face = 3;
            uv = float2( dir.z, -dir.y);
        }
        uv /= absDir.x;
    }
    //top or bottom face
    else if(absDir.y>absDir.z)
    {
        //bottom
        if(0.0>dir.y)
        {
            face = 5;
            uv = float2( dir.x, dir.z);
        }
        //top
        else
        {
            face = 4;
            uv = float2( dir.x, -dir.z);
        }
        uv /= absDir.y;
    }
    //forward/backward
    else
    {
        //forward
        if(0.0>dir.z)
        {
            face = 0;
            uv = float2( dir.x, -dir.y);
        }
        //backward
        else
        {
            face = 2;
            uv = float2( -dir.x, -dir.y);
        }
        uv /= absDir.z;
    }

    //get the final texture coordinate for the texture;
    uv = (uv + float2(1.0,1.0)) * 0.5; //to 0-1 range

	// these tables were passed somehow wrong with directX 

	int2 cubeFaceIndices[6];
	int cubeFaceSourceOrientation[6];

	cubeFaceIndices[0].x = 0;
	cubeFaceIndices[0].y = 1;
	cubeFaceSourceOrientation[0] = 0;

	cubeFaceIndices[1].x = 1;
	cubeFaceIndices[1].y = 1;
	cubeFaceSourceOrientation[1] = 0;

	cubeFaceIndices[2].x = 2;
	cubeFaceIndices[2].y = 1;
	cubeFaceSourceOrientation[2] = 3;

	cubeFaceIndices[3].x = 3;
	cubeFaceIndices[3].y = 1;
	cubeFaceSourceOrientation[3] = 0;

	cubeFaceIndices[4].x = 4;
	cubeFaceIndices[4].y = 1;
	cubeFaceSourceOrientation[4] = 1;

	cubeFaceIndices[5].x = 5;
	cubeFaceIndices[5].y = 1;
	cubeFaceSourceOrientation[5] = 1;

    // DEBUG make face 0 to flicker if uCubeFaceIndices array is passed badly to shader 
    // if (cubeFaceIndices[4].x != uCubeFaceIndices[4].x && face == 0) face = uCounter%6;

    int2 ind=uCubeFaceIndices[face];
    ind=cubeFaceIndices[face];

    for (int tile=ind.x;tile<ind.x+ind.y;tile++)
    {
        if( all( greaterThanEqual(uv.xy, uCubeFaceTargets[tile].xy) ) && all( lessThanEqual(uv.xy, uCubeFaceTargets[tile].xy + uCubeFaceTargets[tile].zw) ))
        {
            int ori = uCubeFaceSourceOrientation[tile];
            ori = cubeFaceSourceOrientation[tile];

            if (ori==0) uv= uv;
            else if (ori==1) uv = float2(0.0,1.0) - float2(-uv.y,  uv.x);
            else if (ori==2) uv = float2(0.0,1.0) + float2( uv.x, -uv.y);
            else if (ori==3) uv = float2(1.0,0.0) - float2( uv.y, -uv.x);
            else if (ori==4) uv = float2(1.0,0.0) + float2(-uv.x,  uv.y);
            else if (ori==5) uv = float2(1.0,1.0) - float2( uv.y,  uv.x);
            else if (ori==6) uv = float2(1.0,1.0) + float2(-uv.x, -uv.y);
            else uv = float2(0.0,0.0) - float2(-uv.y, -uv.x);

            textureCoord = 
				(uCubeFaceSources[tile].zw - (2.0*uCubemapFacePadding)) * 
				((uv-uCubeFaceTargets[tile].xy)/uCubeFaceTargets[tile].zw) + 
				(uCubeFaceSources[tile].xy + uCubemapFacePadding);
        }
    }
)";

static const char_t* vsGL = R"(
    in vec3 aPosition;
    out vec3 vObjVertex;

    uniform mat4 uMVP;
    uniform vec3 uCubemapOrientation;

    #define inData(a) v##a
    #define float3x3 mat3
    #define float2 vec2
    #define float3 vec3
    #define int2 ivec2
    #define mul(a,b) (a*b)

    void main()
    {
        vObjVertex = aPosition.xyz;
        gl_Position = uMVP * vec4(aPosition, 1.0);

        float3x3 cm;
        cm[0] = -cross(uCubemapOrientation, float3(0.0,1.0,0.0));
        cm[1] = cross(uCubemapOrientation, cm[0]);
        cm[2] = uCubemapOrientation;
        vObjVertex = mul(cm, normalize(vObjVertex));

    }
)";

static const char_t* psGLprefix = R"(
    #define inData(a) v##a
    #define float3x3 mat3
    #define float2 vec2
    #define float3 vec3
    #define int2 ivec2
    #define mul(a,b) (a*b)

    in vec3 vObjVertex;
    uniform lowp vec2 uTextureSize;
    uniform vec2 uCubemapFacePadding;
    uniform vec4 uCubeFaceSources[32]; //MAX_FACE_TILES //x, y, width, height
    uniform vec4 uCubeFaceTargets[32]; //MAX_FACE_TILES //x, y, width, height
    uniform int uCubeFaceSourceOrientation[32]; // MAX_FACE_TILES // rotation: 0: none, 1: 90cw, 2: 180, 3: 270cw + mirrored versions
    uniform ivec2 uCubeFaceIndices[6]; //x = first index of tiles uCubeFaceSources/Targets/SourceOrientation y = count of tiles for face
    uniform mat4 uVideoTextureMatrix;
    uniform int uCounter;

    void main()
    {
        float3 dir = normalize(vObjVertex);
        float2 textureCoord = float2(0.0, 0.0);
)";

static const char* psGLpostfix = R"(
        textureCoord = (uVideoTextureMatrix * vec4(textureCoord.x, 1.0-textureCoord.y, 0.0, 1.0)).xy;
        fragmentColor = SampleVideoTexture(textureCoord);
    }
)";

static const char_t* psDXprefix = R"(   
    matrix uMVP;
    matrix uVideoTextureMatrix;

    float2 uTextureSize;
    float2 uCubemapFacePadding;
    float3 uCubemapOrientation;

    float4 uCubeFaceSources[32]; // MAX_FACE_TILES // x, y, width, height
    float4 uCubeFaceTargets[32]; // MAX_FACE_TILES // x, y, width, height
    int uCubeFaceSourceOrientation[32]; // MAX_FACE_TILES // rotation: 0: none, 1: 90cw, 2: 180, 3: 270cw  + mirrored versions
    int2 uCubeFaceIndices[6];   //x = first index of tiles uCubeFaceSources/Targets/SourceOrientation y = count of tiles for face
    int uCounter;

    struct VS_INPUT
    {
        float3 aPosition : POSITION;
    };

    struct PS_INPUT
    {
        float4 Position : SV_POSITION;
        float3 ObjVertex : POSITION0;
    };

    PS_INPUT mainVS(VS_INPUT input)
    {
        PS_INPUT output = (PS_INPUT)0;
        output.Position = mul(uMVP, float4(input.aPosition, 1.0));
        
        float3x3 cm;
        cm[0] = -cross(uCubemapOrientation, float3(0.0,1.0,0.0));
        cm[1] = cross(uCubemapOrientation, cm[0]);
        cm[2] = uCubemapOrientation;
        output.ObjVertex = mul(cm, normalize(input.aPosition));
        return output;
    }

    bool greaterThanEqual(float2 value, float2 comparison)
    {
        return (value.x >= comparison.x && value.y >= comparison.y);
    }

    bool lessThanEqual(float2 value, float2 comparison)
    {
        return (value.x <= comparison.x && value.y <= comparison.y);
    }
#define all(v) v
    float4 mainPS(PS_INPUT input) : SV_TARGET
    {
        float3 dir = normalize(input.ObjVertex); 
        float2 textureCoord = float2(0.0, 0.0);
)";

static const char* psDXpostfix = R"(
        textureCoord = mul(uVideoTextureMatrix, float4(textureCoord.x, 1.0-textureCoord.y, 0.0, 1.0)).xy;    
        return SampleVideoTexture(textureCoord);
    }
)";


void_t OMAF::Private::CubeMapRenderer::createCubemapVideoShader(VideoShader& shader,
                                                                VideoPixelFormat::Enum videoTextureFormat,
                                                                const bool_t maskEnabled /*= false*/)
{
    RendererType::Enum backendType = RenderBackend::getRendererType();

    char_t* vs = OMAF_NULL;
    char_t* ps = OMAF_NULL;
    FixedString8192 fragmentShader;

    if (backendType == RendererType::OPENGL || backendType == RendererType::OPENGL_ES)
    {
        vs = (char_t*) vsGL;
        fragmentShader.append(psGLprefix);
        fragmentShader.append(fragment_code);
        fragmentShader.append(psGLpostfix);
        ps = fragmentShader.getData();
    }
    else if (backendType == RendererType::D3D11 || backendType == RendererType::D3D12)
    {
        fragmentShader.append(psDXprefix);
        fragmentShader.append(fragment_code);
        fragmentShader.append(psDXpostfix);
        vs = ps = fragmentShader.getData();
    }

    shader.create(vs, ps, videoTextureFormat);
}

void_t CubeMapRenderer::initializeCubeMapData(CubeMapSource* source)
{
    VRCubeMap& cubeMap = source->cubeMap;
    OMAF_ASSERT(cubeMap.cubeNumFaces <= 6, "Variable face number cubemaps not yet supported (needs to be 6)");
    mFaceData.mNumCoordinateSets = 0;

    for (uint8_t i = 0; i < cubeMap.cubeNumFaces; i++)
    {
        mFaceData.mNumCoordinateSets += cubeMap.cubeFaces[i].numCoordinates;
    }

    OMAF_ASSERT(mFaceData.mNumCoordinateSets <= MAX_FACE_TILES,
                "Maximum number of supported tiles is 32");

    MemoryAllocator& allocator = *MemorySystem::DefaultHeapAllocator();

    mFaceData.mIndices = OMAF_NEW_ARRAY(allocator, uint32_t, mFaceData.mNumCoordinateSets);
    mFaceData.mTargetCoordinates = OMAF_NEW_ARRAY(allocator, Vector4, mFaceData.mNumCoordinateSets);
    mFaceData.mSourceCoordinates = OMAF_NEW_ARRAY(allocator, Vector4, mFaceData.mNumCoordinateSets);
    mFaceData.mSourceOrientations = OMAF_NEW_ARRAY(allocator, uint32_t, mFaceData.mNumCoordinateSets);
}

void_t CubeMapRenderer::updateCubeMapData(CubeMapSource* source)
{
    VRCubeMap& cubeMap = source->cubeMap;
    int32_t index = 0;

    for (uint8_t i = 0; i < cubeMap.cubeNumFaces; ++i)
    {
        for (uint8_t j = 0; j < cubeMap.cubeFaces[i].numCoordinates; ++j)
        {
            mFaceData.mIndices[index] = cubeMap.cubeFaces[i].faceIndex;

            const VRCubeMapFaceSection& facePart = cubeMap.cubeFaces[i].sections[j];
            mFaceData.mSourceCoordinates[index] =
                makeVector4(facePart.sourceX, facePart.sourceY, facePart.sourceWidth, facePart.sourceHeight);
            mFaceData.mTargetCoordinates[index] =
                makeVector4(facePart.originX, facePart.originY, facePart.originWidth, facePart.originHeight);

            mFaceData.mSourceOrientations[index] = facePart.sourceOrientation;
            index++;
        }
    }

    Quaternion rotation = makeQuaternion(toRadians(cubeMap.cubePitch), toRadians(cubeMap.cubeYaw),
                                         toRadians(cubeMap.cubeRoll), EulerAxisOrder::YXZ);
    Matrix44 rotationMatrix = makeMatrix44(rotation);

    mFaceData.mOrientation = makeVector3(rotationMatrix.c[0][2], rotationMatrix.c[1][2], rotationMatrix.c[2][2]);
}

void_t CubeMapRenderer::render(const HeadTransform& headTransform,
                               const RenderSurface& renderSurface,
                               const CoreProviderSources& sources,
                               const TextureID& renderMask,
                               const RenderingParameters& renderingParameters)
{
    OMAF_ASSERT(sources.getSize() == 1, "Supports only one input source");
    OMAF_ASSERT(sources.at(0)->type == SourceType::CUBEMAP, "Source is not a cube map");

    RenderBackend::pushDebugMarker("CubeMapRenderer::render");

    CubeMapSource* source = (CubeMapSource*) sources.at(0);
    bool_t useMask = renderMask != TextureID::Invalid;

    if (mDirtyShader)
    {
        mDirtyShader = false;
        initializeCubeMapData(source);
        createCubemapVideoShader(mOpaqueShader, source->videoFrame.format, false);
    }
    updateCubeMapData(source);
    // Bind shader and constants
    VideoShader& selectedShader = mOpaqueShader;

    selectedShader.bind();
    selectedShader.bindVideoTexture(source->videoFrame);

    Matrix44 viewMatrix;
    if (source->forcedOrientation.valid)
    {
        viewMatrix = makeTranspose(makeMatrix44(source->forcedOrientation.orientation));
    }
    else
    {
        viewMatrix = makeTranspose(makeMatrix44(headTransform.orientation));
    }


    if (renderingParameters.dofStyle >= 2)
    {
        viewMatrix = viewMatrix *
            makeTranslation(-headTransform.position.x, -headTransform.position.y, -headTransform.position.z);
    }

    Matrix44 viewProjectionMatrix = renderSurface.projection * viewMatrix;
    Matrix44 mvp = viewProjectionMatrix * makeMatrix44(source->extrinsicRotation);

    Matrix44 vtm = source->videoFrame.matrix * makeTranslation(source->textureRect.x, source->textureRect.y, 0) *
        makeScale(source->textureRect.w, source->textureRect.h, 1);
    selectedShader.setDefaultVideoShaderConstants(mvp, vtm);

    float32_t pixelPadding = 0.5f;
    mFaceData.mFacePixelPadding = makeVector2(1.f / source->videoFrame.width, 1.f / source->videoFrame.height) *
        pixelPadding;  // half a pixel padding
    setCubemapFaceCoords(selectedShader, mFaceData);
    Vector2 pixelSize = makeVector2((float32_t) source->videoFrame.width, (float32_t) source->videoFrame.height);
    selectedShader.setConstant("uTextureSize", ShaderConstantType::FLOAT2, &pixelSize);

    if (useMask)
    {
        selectedShader.bindMaskTexture(renderMask, renderingParameters.clearColor);
    }

    // Bind geometry
    mGeometry.bind();

    BlendState blendState;
    blendState.blendEnabled = false;

    RasterizerState rasterizerState;
    rasterizerState.scissorTestEnabled = false;
    
	rasterizerState.cullMode = CullMode::BACK;
    rasterizerState.frontFace = FrontFace::CCW;

	//rasterizerState.cullMode = CullMode::NONE;
    //rasterizerState.frontFace = FrontFace::CW;

    RenderBackend::setBlendState(blendState);
    RenderBackend::setRasterizerState(rasterizerState);

    // Draw
    RenderBackend::draw(PrimitiveType::TRIANGLE_LIST, 0, (uint32_t) mGeometry.getVertexCount());

    RenderBackend::popDebugMarker();
}

void_t CubeMapRenderer::setCubemapFaceCoords(VideoShader& shader, const FaceData& faceData)
{
    shader.setConstant("uCubemapFacePadding", ShaderConstantType::FLOAT2, &faceData.mFacePixelPadding);
    shader.setConstant("uCubemapOrientation", ShaderConstantType::FLOAT3, &faceData.mOrientation);

    // re-order the lists.. (to match our rendering expectations)
    Vector4 sourceCoordinates[32];  // source x,y, width, height
    Vector4 targetCoordinates[32];  // target x,y, width, height
    uint32_t sourceOrientations[32];
    uint32_t sourceStartOffs[6 * 2];
    uint32_t off = 0;

    for (int face = 0; face < 6; face++)
    {
        sourceStartOffs[face * 2] = off;
        for (int i = 0; i < faceData.mNumCoordinateSets; ++i)
        {
            if (faceData.mIndices[i] == face)
            {
                int ori = faceData.mSourceOrientations[i];
                sourceOrientations[off] = (ori == 0) ? 0
                                                     : (ori & 1) ? 1
                                                                 : (ori & 2) ? 2
                                                                             : (ori & 4)
                                ? 3
                                // mirrored
                                : (ori & 8) ? 4 : (ori & 16) ? 5 : (ori & 32) ? 6 : (ori & 64) ? 7 : 0;  // undefined
                sourceCoordinates[off] = faceData.mSourceCoordinates[i];
                targetCoordinates[off] = faceData.mTargetCoordinates[i];
                off++;
            }
        }
        sourceStartOffs[face * 2 + 1] = off - sourceStartOffs[face * 2];
    }

    static uint32_t counter = 0;
    counter++;
    shader.setConstant("uCounter", ShaderConstantType::INTEGER, &counter, 1);

    shader.setConstant("uCubeFaceSources", ShaderConstantType::FLOAT4, sourceCoordinates,
                       faceData.mNumCoordinateSets);  // x, y, width, height
    shader.setConstant("uCubeFaceTargets", ShaderConstantType::FLOAT4, targetCoordinates,
                       faceData.mNumCoordinateSets);  // x, y, width, height

    //      IMPLEMENTATION COPIES INTEGER ARRAYS WRONG TO GPU
    shader.setConstant("uCubeFaceSourceOrientation", 
		ShaderConstantType::INTEGER, sourceOrientations, faceData.mNumCoordinateSets);
    shader.setConstant("uCubeFaceIndices", ShaderConstantType::INTEGER2, sourceStartOffs, 6);
}

OMAF_NS_END
