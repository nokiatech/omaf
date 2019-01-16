
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
#pragma once

#include <NVRNamespace.h>
#include <OMAFPlayerDataTypes.h>

#include "Foundation/NVRFixedString.h"
#include "Foundation/NVRDataBuffer.h"


OMAF_NS_BEGIN

namespace Security
{
    // Verify a SHA256 hash ECDSA signature. Algorithm: prime256v1 (aka NIST P-256, BCRYPT_ECDSA_P256_ALGORITHM)
    bool_t verifySignature(const uint8_t *aSignature, size_t aSignatureLength, const uint8_t *aData, size_t aDataLength, const uint8_t *aKey, size_t aKeyLength);

    // create SHA256 hash to output data buffer
    bool_t calculateHashFromData(const uint8_t* aData, const size_t aDataLength, DataBuffer<uint8_t> &aOutput);

    bool_t getExeHash(DataBuffer<uint8_t> &aOutput);
};
OMAF_NS_END
