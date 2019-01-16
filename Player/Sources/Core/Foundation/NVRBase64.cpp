
/**
 * This file is part of Nokia OMAF implementation
 *
 * Copyright (c) 2018-2019 Nokia Corporation and/or its subsidiary(-ies). All rights reserved.
 *
 * Contact: omaf@nokia.com
 *
 * This software, including documentation, is protected by copyright controlled by Nokia Corporation and/ or its
 * subsidiaries. All rights are reserved.
 *
 * Copying, including reproducing, storing, adapting or translating, any or all of this material requires the prior
 * written consent of Nokia.
 */
#include "Foundation/NVRBase64.h"

#include "Foundation/NVRCompatibility.h"
#include "Foundation/NVRAssert.h"


OMAF_NS_BEGIN
    namespace Base64
    {
        static const char_t BASE64_ENCODING_TABLE[64] =
        {
            'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
            'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
            'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
            'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
            'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
            'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
            'w', 'x', 'y', 'z', '0', '1', '2', '3',
            '4', '5', '6', '7', '8', '9', '+', '/',
        };

        static const char_t BASE64_DECODING_TABLE[256] =
        {
            0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
            0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
            0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  62, 0,  0,  0,  63, 52,
            53, 54, 55, 56, 57, 58, 59, 60, 61, 0,  0,  0,  0,  0,  0,  0,
            0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15,
            16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 0,  0,  0,  0,  0,  0,
            26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41,
            42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 0,  0,  0,  0,  0,  0,
            0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
            0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
            0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
            0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
            0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
            0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
            0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
            0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        };

        static const char_t BASE64_ENCODING_PADDING = '=';
        static const char_t BASE64_NULL_TERMINATOR = '\0';
            
        void_t encode(const String& input, String& output)
        {
            // Calculate output size
            size_t inputSize = input.getSize() - 1;
            uint8_t* inputPtr = (uint8_t*)input.getData();
            
            size_t outputSize = (4 * ((inputSize + 2) / 3)) + 1;
            output.resize(outputSize);

            uint8_t* outputPtr = (uint8_t*)output.getData();
            
            // Encode
            for(size_t i = 0, j = 0; i < inputSize;)
            {
                uint32_t octetA = (i < inputSize) ? inputPtr[i++] : 0;
                uint32_t octetB = (i < inputSize) ? inputPtr[i++] : 0;
                uint32_t octetC = (i < inputSize) ? inputPtr[i++] : 0;
                
                uint32_t indices = (octetA << 16) | (octetB << 8) | (octetC << 0);

                uint8_t indexA = (indices & 0x00FC0000) >> 18;
                uint8_t indexB = (indices & 0x0003F000) >> 12;
                uint8_t indexC = (indices & 0x00000FC0) >> 6;
                uint8_t indexD = (indices & 0x0000003F) >> 0;
                
                outputPtr[j++] = BASE64_ENCODING_TABLE[indexA];
                outputPtr[j++] = BASE64_ENCODING_TABLE[indexB];
                outputPtr[j++] = BASE64_ENCODING_TABLE[indexC];
                outputPtr[j++] = BASE64_ENCODING_TABLE[indexD];
            }
            
            // Add padding
            size_t paddingSize = inputSize % 3;
            
            if(paddingSize != 0)
            {
                paddingSize = 3 - paddingSize;
            }
            
            for(size_t i = 0; i < paddingSize; ++i)
            {
                outputPtr[outputSize - 2 - i] = BASE64_ENCODING_PADDING;
            }
            
            outputPtr[outputSize-1] = BASE64_NULL_TERMINATOR;
        }

        void_t decode(const String& input, DataBuffer<uint8_t>& output)
        {
            // Calculate output size
            size_t inputSize = input.getSize() - 1;
            uint8_t* inputPtr = (uint8_t*)input.getData();


            size_t outputSize = 3 * (inputSize / 4);

            // Ignore padding
            for(size_t i = 0; i < 3; ++i)
            {
                if(inputPtr[inputSize - i] == BASE64_ENCODING_PADDING)
                {
                    outputSize--;
                }
            }

            if (output.getCapacity() < outputSize)
            {
                output.reAllocate(outputSize);
            }

            output.setSize(outputSize);

            uint8_t* outputPtr = (uint8_t*)output.getDataPtr();

            // Decode
            for(size_t i = 0, j = 0; i < inputSize;)
            {
                uint8_t charA = (i < inputSize) ? inputPtr[i++] : 1;
                uint8_t charB = (i < inputSize) ? inputPtr[i++] : 1;
                uint8_t charC = (i < inputSize) ? inputPtr[i++] : 1;
                uint8_t charD = (i < inputSize) ? inputPtr[i++] : 1;
                
                uint32_t indexA = BASE64_DECODING_TABLE[charA - 1];
                uint32_t indexB = BASE64_DECODING_TABLE[charB - 1];
                uint32_t indexC = BASE64_DECODING_TABLE[charC - 1];
                uint32_t indexD = BASE64_DECODING_TABLE[charD - 1];

                uint32_t indices = (indexA << 18) | (indexB << 12) | (indexC << 6) | (indexD << 0);
                
                uint32_t octetA = (indices & 0x00FF0000) >> 16;
                uint32_t octetB = (indices & 0x0000FF00) >> 8;
                uint32_t octetC = (indices & 0x000000FF) >> 0;
                
                if(j < outputSize) 
                {
                    outputPtr[j++] = octetA;
                }

                if(j < outputSize) 
                {
                    outputPtr[j++] = octetB;
                }

                if(j < outputSize) 
                {
                    outputPtr[j++] = octetC;
                }
            }

        }
    }
OMAF_NS_END
