
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
#include "api/isobmff/commontypes.h"
#include "bitstream.hpp"
#include "dynarrayimpl.hpp"

#include <algorithm>
#include <cassert>
#include <type_traits>

namespace ISOBMFF
{
    // ISOBMFF type instantiations for library use
    template struct DynArray<OneViewpointSwitchingStruct>;
    template struct DynArray<SphereRegionDynamic>;
    template struct DynArray<ViewpointSwitchRegionStruct>;

    NoneType none;  // instantiate for optional.h

    namespace
    {
        template <typename Container>
        auto makeDynArray(const Container& container) -> DynArray<typename Container::value_type>
        {
            DynArray<typename Container::value_type> array(container.size());
            for (typename Container::const_iterator it = container.begin(); it != container.end(); ++it)
            {
                array.elements[it - container.begin()] = *it;
            }
            return array;
        }

        template <typename T, typename = int>
        struct HasDoesExist : std::false_type
        {
        };

        template <typename T>
        struct HasDoesExist<T, decltype((void) T::doesExist, 0)> : std::true_type
        {
        };

        template <bool x, typename T>
        struct CopyDoesExistHelper
        {
        };

        template <typename T>
        struct CopyDoesExistHelper<true, T>
        {
            void operator()(T& aDst, const T& aSrc)
            {
                aDst.doesExist = aSrc.doesExist;
            }
        };

        template <typename T>
        struct CopyDoesExistHelper<false, T>
        {
            void operator()(T&, const T&)
            {
            }
        };

        template <typename T>
        void copyDoesExist(T& aDst, const T& aSrc)
        {
            CopyDoesExistHelper<HasDoesExist<T>::value, T>()(aDst, aSrc);
        }

        bool implies(bool a, bool b)
        {
            return !a || b;
        }
    }  // namespace

    namespace Utils
    {
        DynArray<char> parseDynArrayString(BitStream& bitstr)
        {
            String buffer;
            bitstr.readZeroTerminatedString(buffer);
            if (buffer.size())
            {
                return {&buffer[0], &buffer[0] + buffer.size()};
            }
            else
            {
                return {};
            }
        }

        void writeDynArrayString(BitStream& bitstr, const DynArray<char>& aString)
        {
            bitstr.write8BitsArray(aString);
            bitstr.write8Bits(0);
        }
    }  // namespace Utils

    using namespace Utils;

#define GENERATE_CLASS_FUNCTIONS(CLASS_NAME)                      \
    CLASS_NAME::CLASS_NAME() = default;                           \
                                                                  \
    CLASS_NAME::CLASS_NAME(const CLASS_NAME& aOther)              \
    {                                                             \
        BitStream bitstr;                                         \
        aOther.write(bitstr);                                     \
        copyDoesExist(*this, aOther);                             \
        bitstr.reset();                                           \
        parse(bitstr);                                            \
    }                                                             \
                                                                  \
    CLASS_NAME::CLASS_NAME(CLASS_NAME&& aOther)                   \
        : CLASS_NAME(const_cast<const CLASS_NAME&>(aOther))       \
    {                                                             \
    }                                                             \
                                                                  \
    CLASS_NAME& CLASS_NAME::operator=(const CLASS_NAME& aOther)   \
    {                                                             \
        BitStream bitstr;                                         \
        aOther.write(bitstr);                                     \
        copyDoesExist(*this, aOther);                             \
        bitstr.reset();                                           \
        parse(bitstr);                                            \
        return *this;                                             \
    }                                                             \
                                                                  \
    CLASS_NAME& CLASS_NAME::operator=(CLASS_NAME&& aOther)        \
    {                                                             \
        return ((*this) = const_cast<const CLASS_NAME&>(aOther)); \
    }                                                             \
                                                                  \
    CLASS_NAME::~CLASS_NAME() = default;

    OverlayControlFlagBase::~OverlayControlFlagBase() = default;

    void Rotation::write(BitStream& bitstr) const
    {
        bitstr.write32BitsSigned(yaw);
        bitstr.write32BitsSigned(pitch);
        bitstr.write32BitsSigned(roll);
    }

    void Rotation::parse(BitStream& bitstr)
    {
        yaw   = bitstr.read32BitsSigned();
        pitch = bitstr.read32BitsSigned();
        roll  = bitstr.read32BitsSigned();
    }

    void SphereRegion::write(BitStream& bitstr, bool hasRange) const
    {
        bitstr.write32BitsSigned(centreAzimuth);
        bitstr.write32BitsSigned(centreElevation);
        bitstr.write32BitsSigned(centreTilt);
        if (hasRange)
        {
            SphereRegionRange::writeOpt(bitstr);
        }
        bitstr.write8Bits(interpolate ? 0b10000000 : 0);
    }

    void SphereRegion::parse(BitStream& bitstr, bool hasRange)
    {
        centreAzimuth   = bitstr.read32BitsSigned();
        centreElevation = bitstr.read32BitsSigned();
        centreTilt      = bitstr.read32BitsSigned();
        if (hasRange)
        {
            SphereRegionRange::parseOpt(bitstr);
        }
        interpolate = bitstr.read8Bits() & 0b10000000;
    }

    void SphereRegionRange::writeOpt(BitStream& bitstr) const
    {
        bitstr.write32Bits(azimuthRange);
        bitstr.write32Bits(elevationRange);
    }

    std::uint16_t SphereRegionRange::sizeOpt() const
    {
        return 2 * 4;
    }

    void SphereRegionRange::parseOpt(BitStream& bitstr)
    {
        azimuthRange   = bitstr.read32Bits();
        elevationRange = bitstr.read32Bits();
    }

    void SphereRegionInterpolate::writeOpt(BitStream& bitstr) const
    {
        bitstr.write8Bits(interpolate ? 0b10000000 : 0);
    }

    std::uint16_t SphereRegionInterpolate::sizeOpt() const
    {
        return 1;
    }

    void SphereRegionInterpolate::parseOpt(BitStream& bitstr)
    {
        interpolate = bitstr.read8Bits() & 0b10000000;
    }

    template <bool HasRange, bool HasInterpolate>
    void SphereRegionStatic<HasRange, HasInterpolate>::write(BitStream& bitstr) const
    {
        bitstr.write32BitsSigned(centreAzimuth);
        bitstr.write32BitsSigned(centreElevation);
        bitstr.write32BitsSigned(centreTilt);
        SelectIf<HasRange, SphereRegionRange>::writeOpt(bitstr);
        SelectIf<HasInterpolate, SphereRegionInterpolate>::writeOpt(bitstr);
    }

    template <bool HasRange, bool HasInterpolate>
    uint16_t SphereRegionStatic<HasRange, HasInterpolate>::size() const
    {
        uint16_t currentSize = 0;
        currentSize += 3 * 4;
        currentSize += SelectIf<HasRange, SphereRegionRange>::sizeOpt();
        currentSize += SelectIf<HasInterpolate, SphereRegionInterpolate>::sizeOpt();
        return currentSize;
    }

    template <bool HasRange, bool HasInterpolate>
    void SphereRegionStatic<HasRange, HasInterpolate>::parse(BitStream& bitstr)
    {
        centreAzimuth   = bitstr.read32BitsSigned();
        centreElevation = bitstr.read32BitsSigned();
        centreTilt      = bitstr.read32BitsSigned();
        SelectIf<HasRange, SphereRegionRange>::parseOpt(bitstr);
        SelectIf<HasInterpolate, SphereRegionInterpolate>::parseOpt(bitstr);
    }

    template struct SelectIf<true, SphereRegionRange>;
    template struct SelectIf<false, SphereRegionRange>;
    template struct SelectIf<true, SphereRegionInterpolate>;
    template struct SelectIf<false, SphereRegionInterpolate>;
    template struct SphereRegionStatic<false, false>;
    template struct SphereRegionStatic<false, true>;
    template struct SphereRegionStatic<true, false>;
    template struct SphereRegionStatic<true, true>;

    void SphereRegionDynamic::write(BitStream& bitstr, const SphereRegionContext& aSampleContext) const
    {
        bitstr.write32BitsSigned(centreAzimuth);
        bitstr.write32BitsSigned(centreElevation);
        bitstr.write32BitsSigned(centreTilt);
        if (aSampleContext.hasRange)
        {
            SphereRegionRange::writeOpt(bitstr);
        }
        if (aSampleContext.hasInterpolate)
        {
            SphereRegionInterpolate::writeOpt(bitstr);
        }
    }

    void SphereRegionDynamic::parse(BitStream& bitstr, const SphereRegionContext& aSampleContext)
    {
        centreAzimuth   = bitstr.read32BitsSigned();
        centreElevation = bitstr.read32BitsSigned();
        centreTilt      = bitstr.read32BitsSigned();
        if (aSampleContext.hasRange)
        {
            SphereRegionRange::parseOpt(bitstr);
        }
        if (aSampleContext.hasInterpolate)
        {
            SphereRegionInterpolate::parseOpt(bitstr);
        }
    }

    std::uint16_t SphereRegionDynamic::size(const SphereRegionContext& aSampleContext) const
    {
        uint16_t currentSize = 0;
        currentSize += 3 * 4;
        currentSize += aSampleContext.hasRange ? SphereRegionRange::sizeOpt() : 0;
        currentSize += aSampleContext.hasInterpolate ? SphereRegionInterpolate::sizeOpt() : 0;
        return currentSize;
    }

    void PackedPictureRegion::write(BitStream& bitstr) const
    {
        bitstr.write16Bits(pictureWidth);
        bitstr.write16Bits(pictureHeight);
        bitstr.write16Bits(regionWidth);
        bitstr.write16Bits(regionHeight);
        bitstr.write16Bits(regionTop);
        bitstr.write16Bits(regionLeft);
    }

    void PackedPictureRegion::parse(BitStream& bitstr)
    {
        pictureWidth  = bitstr.read16Bits();
        pictureHeight = bitstr.read16Bits();
        regionWidth   = bitstr.read16Bits();
        regionHeight  = bitstr.read16Bits();
        regionTop     = bitstr.read16Bits();
        regionLeft    = bitstr.read16Bits();
    }

    void ProjectedPictureRegion::write(BitStream& bitstr) const
    {
        bitstr.write32Bits(pictureWidth);
        bitstr.write32Bits(pictureHeight);
        bitstr.write32Bits(regionWidth);
        bitstr.write32Bits(regionHeight);
        bitstr.write32Bits(regionTop);
        bitstr.write32Bits(regionLeft);
    }

    std::uint16_t ProjectedPictureRegion::size() const
    {
        return 6 * 4;
    }

    void ProjectedPictureRegion::parse(BitStream& bitstr)
    {
        pictureWidth  = bitstr.read32Bits();
        pictureHeight = bitstr.read32Bits();
        regionWidth   = bitstr.read32Bits();
        regionHeight  = bitstr.read32Bits();
        regionTop     = bitstr.read32Bits();
        regionLeft    = bitstr.read32Bits();
    }

    bool OverlayControlFlagBase::write(BitStream& bitstr) const
    {
        if (!doesExist)
        {
            return true;
        }

        if (inheritFromOverlayConfigSampleEntry)
        {
            bitstr.write16Bits(overlayControlEssentialFlag ? 0x8000 : 0);
            return true;
        }

        bitstr.write16Bits((overlayControlEssentialFlag ? 0x8000 : 0) | size());
        return false;
    }

    std::uint16_t OverlayControlFlagBase::parse(BitStream& bitstr)
    {
        if (!doesExist)
        {
            return 0;
        }

        auto firstWord              = bitstr.read16Bits();
        overlayControlEssentialFlag = firstWord & 0x8000;
        std::uint16_t readSize      = firstWord & 0x7fff;

        if (readSize == 0)
        {
            inheritFromOverlayConfigSampleEntry = true;
        }

        return readSize;
    }

    GENERATE_CLASS_FUNCTIONS(ViewportRelativeOverlay);

    std::uint16_t ViewportRelativeOverlay::size() const
    {
        return 4 * 2 + 1 + 2;
    }

    void ViewportRelativeOverlay::write(BitStream& bitstr) const
    {
        if (OverlayControlFlagBase::write(bitstr))
        {
            return;
        }

        bitstr.write16Bits(rectLeftPercent);
        bitstr.write16Bits(rectTopPercent);
        bitstr.write16Bits(rectWidthtPercent);
        bitstr.write16Bits(rectHeightPercent);

        const uint8_t alignmentBits        = static_cast<uint8_t>(mediaAlignment) << 3;
        const uint8_t relativeDisparityBit = relativeDisparityFlag ? 0b00000100 : 0;
        bitstr.write8Bits(alignmentBits | relativeDisparityBit);
        bitstr.write16BitsSigned(disparity);
    }

    void ViewportRelativeOverlay::parse(BitStream& bitstr)
    {
        auto parsedSize = OverlayControlFlagBase::parse(bitstr);
        if (parsedSize == 0)
        {
            return;
        }

        rectLeftPercent   = bitstr.read16Bits();
        rectTopPercent    = bitstr.read16Bits();
        rectWidthtPercent = bitstr.read16Bits();
        rectHeightPercent = bitstr.read16Bits();

        const uint8_t bitFields = bitstr.read8Bits();
        mediaAlignment          = static_cast<MediaAlignmentType>(bitFields >> 3);
        relativeDisparityFlag   = bitFields & 0b00000100;
        disparity               = bitstr.read16BitsSigned();
    }

    GENERATE_CLASS_FUNCTIONS(SphereRelativeOmniOverlay);

    std::uint16_t SphereRelativeOmniOverlay::size() const
    {
        std::uint16_t currentSize = 0u;
        currentSize += 3u;  // indication_type, timeline_change_flag and region_depth_minus1
        switch (region.getKey())
        {
        case RegionIndicationType::SPHERE:
            currentSize += region.at<RegionIndicationType::SPHERE>().size();
            break;
        case RegionIndicationType::PROJECTED_PICTURE:
            currentSize += region.at<RegionIndicationType::PROJECTED_PICTURE>().size();
            break;
        }

        return currentSize;
    }

    void SphereRelativeOmniOverlay::write(BitStream& bitstr) const
    {
        if (OverlayControlFlagBase::write(bitstr))
        {
            return;
        }

        bitstr.writeBits(unsigned(region.getKey()), 1);
        bitstr.writeBits(timelineChangeFlag, 1);
        bitstr.writeBits(0u, 6);  // reserved

        writeOptional(region.getOpt<RegionIndicationType::PROJECTED_PICTURE>(), bitstr);
        writeOptional(region.getOpt<RegionIndicationType::SPHERE>(), bitstr);

        bitstr.write16Bits(regionDepthMinus1);
    }

    void SphereRelativeOmniOverlay::parse(BitStream& bitstr)
    {
        auto parsedSize = OverlayControlFlagBase::parse(bitstr);
        if (parsedSize == 0)
        {
            return;
        }

        auto regionIndication = RegionIndicationType(bitstr.readBits(1));
        timelineChangeFlag    = bitstr.readBits(1);
        bitstr.readBits(6);  // reserved

        switch (regionIndication)
        {
        case RegionIndicationType::PROJECTED_PICTURE:
            region.set<RegionIndicationType::PROJECTED_PICTURE>({}).parse(bitstr);
            break;
        case RegionIndicationType::SPHERE:
            region.set<RegionIndicationType::SPHERE>({}).parse(bitstr);
            break;
        }

        regionDepthMinus1 = bitstr.read16Bits();
    }

    GENERATE_CLASS_FUNCTIONS(SphereRelative2DOverlay);

    std::uint16_t SphereRelative2DOverlay::size() const
    {
        return sphereRegion.size() + 1 + 3 * 4 + 2;
    }

    void SphereRelative2DOverlay::write(BitStream& bitstr) const
    {
        if (OverlayControlFlagBase::write(bitstr))
        {
            return;
        }

        sphereRegion.write(bitstr);
        bitstr.write8Bits(timelineChangeFlag ? 0b10000000 : 0);
        overlayRotation.write(bitstr);
        bitstr.write16Bits(regionDepthMinus1);
    }

    void SphereRelative2DOverlay::parse(BitStream& bitstr)
    {
        auto parsedSize = OverlayControlFlagBase::parse(bitstr);
        if (parsedSize == 0)
        {
            return;
        }

        sphereRegion.parse(bitstr);
        timelineChangeFlag = bitstr.readBits(1);
        bitstr.readBits(7);  // reserved
        overlayRotation.parse(bitstr);

        regionDepthMinus1 = bitstr.read16Bits();
    }

    GENERATE_CLASS_FUNCTIONS(OverlaySourceRegion);

    std::uint16_t OverlaySourceRegion::size() const
    {
        return 13;
    }

    void OverlaySourceRegion::write(BitStream& bitstr) const
    {
        if (OverlayControlFlagBase::write(bitstr))
        {
            return;
        }

        region.write(bitstr);
        bitstr.write8Bits((std::uint8_t) transformType << 5);
    }

    void OverlaySourceRegion::parse(BitStream& bitstr)
    {
        auto parsedSize = OverlayControlFlagBase::parse(bitstr);
        if (parsedSize == 0)
        {
            return;
        }

        region.parse(bitstr);
        transformType = (TransformType)(bitstr.read8Bits() >> 5);
    }

    GENERATE_CLASS_FUNCTIONS(RecommendedViewportOverlay);

    std::uint16_t RecommendedViewportOverlay::size() const
    {
        return 4;
    }

    void RecommendedViewportOverlay::write(BitStream& bitstr) const
    {
        if (OverlayControlFlagBase::write(bitstr))
        {
            return;
        }
    }

    void RecommendedViewportOverlay::parse(BitStream& bitstr)
    {
        OverlayControlFlagBase::parse(bitstr);
    }

    GENERATE_CLASS_FUNCTIONS(OverlayLayeringOrder);

    std::uint16_t OverlayLayeringOrder::size() const
    {
        return 2;
    }

    void OverlayLayeringOrder::write(BitStream& bitstr) const
    {
        if (OverlayControlFlagBase::write(bitstr))
        {
            return;
        }
        bitstr.write16BitsSigned(layeringOrder);
    }

    void OverlayLayeringOrder::parse(BitStream& bitstr)
    {
        auto parsedSize = OverlayControlFlagBase::parse(bitstr);
        if (parsedSize == 0)
        {
            return;
        }
        layeringOrder = bitstr.read16BitsSigned();
    }

    GENERATE_CLASS_FUNCTIONS(OverlayOpacity);

    std::uint16_t OverlayOpacity::size() const
    {
        return 1;
    }

    void OverlayOpacity::write(BitStream& bitstr) const
    {
        if (OverlayControlFlagBase::write(bitstr))
        {
            return;
        }
        bitstr.write8Bits(opacity);
    }

    void OverlayOpacity::parse(BitStream& bitstr)
    {
        auto parsedSize = OverlayControlFlagBase::parse(bitstr);
        if (parsedSize == 0)
        {
            return;
        }
        opacity = bitstr.read8Bits();
    }

    GENERATE_CLASS_FUNCTIONS(OverlayInteraction);

    std::uint16_t OverlayInteraction::size() const
    {
        return 1;
    }

    void OverlayInteraction::write(BitStream& bitstr) const
    {
        if (OverlayControlFlagBase::write(bitstr))
        {
            return;
        }

        bitstr.write8Bits((changePositionFlag ? 0b10000000 : 0) | (changeDepthFlag ? 0b01000000 : 0) |
                          (switchOnOffFlag ? 0b00100000 : 0) | (changeOpacityFlag ? 0b00010000 : 0) |
                          (resizeFlag ? 0b000001000 : 0) | (rotationFlag ? 0b00000100 : 0) |
                          (sourceSwitchingFlag ? 0b00000010 : 0) | (cropFlag ? 0b00000001 : 0));
    }

    void OverlayInteraction::parse(BitStream& bitstr)
    {
        auto parsedSize = OverlayControlFlagBase::parse(bitstr);
        if (parsedSize == 0)
        {
            return;
        }

        auto flagBits = bitstr.read8Bits();

        changePositionFlag  = flagBits & 0b10000000;
        changeDepthFlag     = flagBits & 0b01000000;
        switchOnOffFlag     = flagBits & 0b00100000;
        changeOpacityFlag   = flagBits & 0b00010000;
        resizeFlag          = flagBits & 0b00001000;
        rotationFlag        = flagBits & 0b00000100;
        sourceSwitchingFlag = flagBits & 0b00000010;
        cropFlag            = flagBits & 0b00000001;
    }

    GENERATE_CLASS_FUNCTIONS(OverlayLabel);

    std::uint16_t OverlayLabel::size() const
    {
        return (std::uint16_t) overlayLabel.size() + 1;
    }

    void OverlayLabel::write(BitStream& bitstr) const
    {
        if (OverlayControlFlagBase::write(bitstr))
        {
            return;
        }

        bitstr.writeZeroTerminatedString(overlayLabel.c_str());
    }

    void OverlayLabel::parse(BitStream& bitstr)
    {
        auto parsedSize = OverlayControlFlagBase::parse(bitstr);
        if (parsedSize == 0)
        {
            return;
        }

        String readBuf;
        bitstr.readStringWithLen(readBuf, parsedSize);
        overlayLabel = readBuf.c_str();
    }

    GENERATE_CLASS_FUNCTIONS(OverlayPriority);

    std::uint16_t OverlayPriority::size() const
    {
        return 1;
    }

    void OverlayPriority::write(BitStream& bitstr) const
    {
        if (OverlayControlFlagBase::write(bitstr))
        {
            return;
        }
        bitstr.write8Bits(overlayPriority);
    }

    void OverlayPriority::parse(BitStream& bitstr)
    {
        auto parsedSize = OverlayControlFlagBase::parse(bitstr);
        if (parsedSize == 0)
        {
            return;
        }
        overlayPriority = bitstr.read8Bits();
    }

    GENERATE_CLASS_FUNCTIONS(AssociatedSphereRegion);

    std::uint16_t AssociatedSphereRegion::size() const
    {
        return 1 + sphereRegion.size();
    }

    void AssociatedSphereRegion::write(BitStream& bitstr) const
    {
        if (OverlayControlFlagBase::write(bitstr))
        {
            return;
        }

        bitstr.write8Bits(static_cast<uint8_t>(shapeType));
        sphereRegion.write(bitstr);
    }

    void AssociatedSphereRegion::parse(BitStream& bitstr)
    {
        auto parsedSize = OverlayControlFlagBase::parse(bitstr);
        if (parsedSize == 0)
        {
            return;
        }
        shapeType = static_cast<SphereRegionShapeType>(bitstr.read8Bits());
        sphereRegion.parse(bitstr);
    }

    GENERATE_CLASS_FUNCTIONS(OverlayAlphaCompositing);

    std::uint16_t OverlayAlphaCompositing::size() const
    {
        return 1;
    }

    void OverlayAlphaCompositing::write(BitStream& bitstr) const
    {
        if (OverlayControlFlagBase::write(bitstr))
        {
            return;
        }

        bitstr.write8Bits(static_cast<uint8_t>(alphaBlendingMode));
    }

    void OverlayAlphaCompositing::parse(BitStream& bitstr)
    {
        auto parsedSize = OverlayControlFlagBase::parse(bitstr);
        if (parsedSize == 0)
        {
            return;
        }
        alphaBlendingMode = static_cast<AlphaBlendingModeType>(bitstr.read8Bits());
    }

    // Cannot use GENERATE_CLASS_FUNCTIONS due to special write/parse functions
    SingleOverlayStruct::SingleOverlayStruct() = default;

    SingleOverlayStruct::SingleOverlayStruct(const SingleOverlayStruct& aOther)
    {
        BitStream bitstr;
        aOther.write(bitstr, 2);
        parse(bitstr, 2);
    }

    SingleOverlayStruct::SingleOverlayStruct(SingleOverlayStruct&& aOther)
        : SingleOverlayStruct(const_cast<const SingleOverlayStruct&>(aOther))
    {
        // nothing
    }

    SingleOverlayStruct& SingleOverlayStruct::operator=(const SingleOverlayStruct& aOther)
    {
        BitStream bitstr;
        aOther.write(bitstr, 2);
        parse(bitstr, 2);
        return *this;
    }

    SingleOverlayStruct& SingleOverlayStruct::operator=(SingleOverlayStruct&& aOther)
    {
        return ((*this) = const_cast<const SingleOverlayStruct&>(aOther));
    }

    SingleOverlayStruct::~SingleOverlayStruct() = default;

    void SingleOverlayStruct::write(BitStream& bitstr, std::uint8_t flagByteCount) const
    {
        bitstr.write16Bits(overlayId);

        // write two first bytes according existing control flags and
        // fill rest with zeroes

        // Bit fields 2020-02-24
        //
        // bit 0 - viewport relative ovly
        // bit 1 - sphere relative omni overlay
        // bit 2 - sphere relative 2d overlay
        // bit 3 - 3d mesh (TDB)
        // bit 4 - zero size control only signals that full source is used
        // bit 5 - source region
        // bit 6 - recommended viewport track
        // bit 7 - layering order
        // bit 8 - overlay opacity
        // bit 9 - user interaction
        // bit 10 - overlay label
        // bit 11 - overlay priority
        // bit 12 - associated sphere region
        // bit 13 - alpha composition

        // exactly one between [0,3] range must be set
        // exactly one between [4,6] range must be set

        const uint8_t bit00 = viewportRelativeOverlay.doesExist ? 0b10000000 : 0;
        const uint8_t bit01 = sphereRelativeOmniOverlay.doesExist ? 0b01000000 : 0;
        const uint8_t bit02 = sphereRelative2DOverlay.doesExist ? 0b00100000 : 0;
        const uint8_t bit03 = false ? 0b00010000 : 0;  // NOTE: 3d mesh not implemented
        const uint8_t bit04 = !overlaySourceRegion.doesExist && !recommendedViewportOverlay.doesExist ? 0b00001000 : 0;
        const uint8_t bit05 = overlaySourceRegion.doesExist ? 0b00000100 : 0;
        const uint8_t bit06 = recommendedViewportOverlay.doesExist ? 0b00000010 : 0;
        const uint8_t bit07 = overlayLayeringOrder.doesExist ? 0b00000001 : 0;

        const uint8_t bit08 = overlayOpacity.doesExist ? 0b10000000 : 0;
        const uint8_t bit09 = overlayInteraction.doesExist ? 0b01000000 : 0;
        const uint8_t bit10 = overlayLabel.doesExist ? 0b00100000 : 0;
        const uint8_t bit11 = overlayPriority.doesExist ? 0b00010000 : 0;
        const uint8_t bit12 = associatedSphereRegion.doesExist ? 0b00001000 : 0;
        const uint8_t bit13 = overlayAlphaCompositing.doesExist ? 0b00000100 : 0;

        for (std::int32_t byteNum = 0; byteNum < flagByteCount; byteNum++)
        {
            switch (byteNum)
            {
            case 0:
                bitstr.write8Bits(bit00 | bit01 | bit02 | bit03 | bit04 | bit05 | bit06 | bit07);
                continue;
            case 1:
                bitstr.write8Bits(bit08 | bit09 | bit10 | bit11 | bit12 | bit13);
                continue;
            default:
                bitstr.write8Bits(0);
                continue;
            }
        }

        viewportRelativeOverlay.write(bitstr);
        sphereRelativeOmniOverlay.write(bitstr);
        sphereRelative2DOverlay.write(bitstr);
        if (bit04)
        {
            bitstr.write16Bits(0x8000); // essential, byte_count = 0
        }
        overlaySourceRegion.write(bitstr);
        recommendedViewportOverlay.write(bitstr);
        overlayLayeringOrder.write(bitstr);
        overlayOpacity.write(bitstr);
        overlayInteraction.write(bitstr);
        overlayLabel.write(bitstr);
        overlayPriority.write(bitstr);
        associatedSphereRegion.write(bitstr);
        overlayAlphaCompositing.write(bitstr);
    }

    void SingleOverlayStruct::parse(BitStream& bitstr, std::uint8_t flagByteCount)
    {
        overlayId = bitstr.read16Bits();
        bool bit04 = false;

        for (std::int32_t byteNum = 0; byteNum < flagByteCount; byteNum++)
        {
            auto byte = bitstr.read8Bits();
            switch (byteNum)
            {
            case 0:
                viewportRelativeOverlay.doesExist    = byte & 0b10000000;
                sphereRelativeOmniOverlay.doesExist  = byte & 0b01000000;
                sphereRelative2DOverlay.doesExist    = byte & 0b00100000;
                bit04                                = byte & 0b00001000;
                overlaySourceRegion.doesExist        = byte & 0b00000100;
                recommendedViewportOverlay.doesExist = byte & 0b00000010;
                overlayLayeringOrder.doesExist       = byte & 0b00000001;
                continue;
            case 1:
                overlayOpacity.doesExist          = byte & 0b10000000;
                overlayInteraction.doesExist      = byte & 0b01000000;
                overlayLabel.doesExist            = byte & 0b00100000;
                overlayPriority.doesExist         = byte & 0b00010000;
                associatedSphereRegion.doesExist  = byte & 0b00001000;
                overlayAlphaCompositing.doesExist = byte & 0b00000100;
                continue;
            default:
                continue;
            }
        }

        viewportRelativeOverlay.parse(bitstr);
        sphereRelativeOmniOverlay.parse(bitstr);
        sphereRelative2DOverlay.parse(bitstr);
        if (bit04)
        {
            // expected result: 0x8000
            // essential flag set, byte_count = 0
            size_t numBytes = bitstr.read16Bits() & ~0x8000;
            bitstr.skipBytes(numBytes);
        }
        overlaySourceRegion.parse(bitstr);
        recommendedViewportOverlay.parse(bitstr);
        overlayLayeringOrder.parse(bitstr);
        overlayOpacity.parse(bitstr);
        overlayInteraction.parse(bitstr);
        overlayLabel.parse(bitstr);
        overlayPriority.parse(bitstr);
        associatedSphereRegion.parse(bitstr);
        overlayAlphaCompositing.parse(bitstr);
    }

    GENERATE_CLASS_FUNCTIONS(OverlayStruct);

    void OverlayStruct::write(BitStream& bitstr) const
    {
        bitstr.write16Bits((std::uint16_t) overlays.size());
        bitstr.write8Bits(numFlagBytes);
        for (auto& overlay : overlays)
        {
            overlay.write(bitstr, numFlagBytes);
        }
    }

    void OverlayStruct::parse(BitStream& bitstr)
    {
        std::uint16_t overlayCount = bitstr.read16Bits();
        numFlagBytes               = bitstr.read8Bits();
        for (std::int32_t i = 0; i < overlayCount; i++)
        {
            SingleOverlayStruct overlay;
            overlay.parse(bitstr, numFlagBytes);
            overlays.push_back(overlay);
        }
    }

    GENERATE_CLASS_FUNCTIONS(ViewpointPosStruct);
    void ViewpointPosStruct::write(BitStream& bitstr) const
    {
        bitstr.write32BitsSigned(viewpointPosX);
        bitstr.write32BitsSigned(viewpointPosY);
        bitstr.write32BitsSigned(viewpointPosZ);
    }

    void ViewpointPosStruct::parse(BitStream& bitstr)
    {
        viewpointPosX = bitstr.read32BitsSigned();
        viewpointPosY = bitstr.read32BitsSigned();
        viewpointPosZ = bitstr.read32BitsSigned();
    }

    GENERATE_CLASS_FUNCTIONS(ViewpointGpsPositionStruct);
    void ViewpointGpsPositionStruct::write(BitStream& bitstr) const
    {
        bitstr.write32BitsSigned(viewpointGpsposLongitude);
        bitstr.write32BitsSigned(viewpointGpsposLatitude);
        bitstr.write32BitsSigned(viewpointGpsposAltitude);
    }

    void ViewpointGpsPositionStruct::parse(BitStream& bitstr)
    {
        viewpointGpsposLongitude = bitstr.read32BitsSigned();
        viewpointGpsposLatitude  = bitstr.read32BitsSigned();
        viewpointGpsposAltitude  = bitstr.read32BitsSigned();
    }

    GENERATE_CLASS_FUNCTIONS(ViewpointGeomagneticInfoStruct);
    void ViewpointGeomagneticInfoStruct::write(BitStream& bitstr) const
    {
        bitstr.write32BitsSigned(viewpointGeomagneticYaw);
        bitstr.write32BitsSigned(viewpointGeomagneticPitch);
        bitstr.write32BitsSigned(viewpointGeomagneticRoll);
    }

    void ViewpointGeomagneticInfoStruct::parse(BitStream& bitstr)
    {
        viewpointGeomagneticYaw   = bitstr.read32BitsSigned();
        viewpointGeomagneticPitch = bitstr.read32BitsSigned();
        viewpointGeomagneticRoll  = bitstr.read32BitsSigned();
    }

    GENERATE_CLASS_FUNCTIONS(ViewpointGlobalCoordinateSysRotationStruct);
    void ViewpointGlobalCoordinateSysRotationStruct::write(BitStream& bitstr) const
    {
        bitstr.write32BitsSigned(viewpointGcsYaw);
        bitstr.write32BitsSigned(viewpointGcsPitch);
        bitstr.write32BitsSigned(viewpointGcsRoll);
    }

    void ViewpointGlobalCoordinateSysRotationStruct::parse(BitStream& bitstr)
    {
        viewpointGcsYaw   = bitstr.read32BitsSigned();
        viewpointGcsPitch = bitstr.read32BitsSigned();
        viewpointGcsRoll  = bitstr.read32BitsSigned();
    }

    void ViewpointGroupDescription::writeOpt(BitStream& bitstr) const
    {
        bitstr.write8BitsArray(vwptGroupDescription);
        bitstr.write8Bits(0u);
    }

    void ViewpointGroupDescription::parseOpt(BitStream& bitstr)
    {
        vwptGroupDescription = parseDynArrayString(bitstr);
    }

    std::uint16_t ViewpointGroupDescription::sizeOpt() const
    {
        return vwptGroupDescription.numElements + 1;
    }

    //GENERATE_CLASS_FUNCTIONS(ViewpointGroupStruct);
    template <bool GroupDescrIncludedFlag>
    void ViewpointGroupStruct<GroupDescrIncludedFlag>::write(BitStream& bitstr) const
    {
        bitstr.write8Bits(vwptGroupId);
        SelectIf<GroupDescrIncludedFlag, ViewpointGroupDescription>::writeOpt(bitstr);
    }

    template <bool GroupDescrIncludedFlag>
    void ViewpointGroupStruct<GroupDescrIncludedFlag>::parse(BitStream& bitstr)
    {
        vwptGroupId = bitstr.read8Bits();
        SelectIf<GroupDescrIncludedFlag, ViewpointGroupDescription>::parseOpt(bitstr);
    }

    template struct ViewpointGroupStruct<false>;
    template struct ViewpointGroupStruct<true>;

    GENERATE_CLASS_FUNCTIONS(ViewpointTimelineSwitchStruct);
    void ViewpointTimelineSwitchStruct::write(BitStream& bitstr) const
    {
        bitstr.writeBits(unsigned(tOffset.getKey()), 1);
        bitstr.writeBits(bool(minTime), 1);
        bitstr.writeBits(bool(maxTime), 1);

        bitstr.writeBits(0, 5);

        writeOptional(minTime, bitstr, &BitStream::write32BitsSigned);
        writeOptional(maxTime, bitstr, &BitStream::write32BitsSigned);

        // only one of these is possible:
        writeOptional(tOffset.getOpt<OffsetKind::ABSOLUTE_TIME>(), bitstr, &BitStream::write32Bits);
        writeOptional(tOffset.getOpt<OffsetKind::RELATIVE_TIME>().map(
                          [](const std::int64_t x) { return static_cast<std::uint64_t>(x); }),
                      bitstr, &BitStream::write64Bits);
    }

    void ViewpointTimelineSwitchStruct::parse(BitStream& bitstr)
    {
        auto absoluteRelativeTOffsetFlag = OffsetKind(bitstr.readBit());
        bool minTimeFlag                 = bitstr.readBit();
        bool maxTimeFlag                 = bitstr.readBit();

        bitstr.readBits(5);

        parseOptionalIf(minTimeFlag, minTime, bitstr, &BitStream::read32BitsSigned);
        parseOptionalIf(maxTimeFlag, maxTime, bitstr, &BitStream::read32BitsSigned);
        switch (absoluteRelativeTOffsetFlag)
        {
        case OffsetKind::ABSOLUTE_TIME:
            tOffset.set<OffsetKind::ABSOLUTE_TIME>(bitstr.read32Bits());
            break;
        case OffsetKind::RELATIVE_TIME:
            tOffset.set<OffsetKind::RELATIVE_TIME>(static_cast<int32_t>(bitstr.read64Bits()));
            break;
        }
    }

    GENERATE_CLASS_FUNCTIONS(TransitionEffect);
    void TransitionEffect::write(BitStream& bitstr) const
    {
        bitstr.write8Bits(static_cast<uint8_t>(transitionEffect.getKey()));
        switch (transitionEffect.getKey())
        {
        case TransitionEffectType::ZOOM_IN_EFFECT:
            break;
        case TransitionEffectType::WALK_THOUGH_EFFECT:
            break;
        case TransitionEffectType::FADE_TO_BLACK_EFFECT:
            break;
        case TransitionEffectType::MIRROR_EFFECT:
            break;
        case TransitionEffectType::VIDEO_TRANSITION_TRACK_ID:
            bitstr.write32Bits(transitionEffect.at<TransitionEffectType::VIDEO_TRANSITION_TRACK_ID>());
            break;
        case TransitionEffectType::VIDEO_TRANSITION_URL:
            writeDynArrayString(bitstr, transitionEffect.at<TransitionEffectType::VIDEO_TRANSITION_URL>());
            break;
        case TransitionEffectType::RESERVED:
            break;
        }
    }

    void TransitionEffect::parse(BitStream& bitstr)
    {
        auto checkedTransitionEffectType = bitstr.read8Bits();
        checkedTransitionEffectType =
            std::min(checkedTransitionEffectType, static_cast<uint8_t>(TransitionEffectType::RESERVED));
        switch (static_cast<TransitionEffectType>(checkedTransitionEffectType))
        {
        case TransitionEffectType::ZOOM_IN_EFFECT:
            transitionEffect.set<TransitionEffectType::ZOOM_IN_EFFECT>({});
            break;
        case TransitionEffectType::WALK_THOUGH_EFFECT:
            transitionEffect.set<TransitionEffectType::WALK_THOUGH_EFFECT>({});
            break;
        case TransitionEffectType::FADE_TO_BLACK_EFFECT:
            transitionEffect.set<TransitionEffectType::FADE_TO_BLACK_EFFECT>({});
            break;
        case TransitionEffectType::MIRROR_EFFECT:
            transitionEffect.set<TransitionEffectType::MIRROR_EFFECT>({});
            break;
        case TransitionEffectType::VIDEO_TRANSITION_TRACK_ID:
            transitionEffect.set<TransitionEffectType::VIDEO_TRANSITION_TRACK_ID>(bitstr.read32Bits());
            break;
        case TransitionEffectType::VIDEO_TRANSITION_URL:
            transitionEffect.set<TransitionEffectType::VIDEO_TRANSITION_URL>(parseDynArrayString(bitstr));
            break;
        case TransitionEffectType::RESERVED:
            transitionEffect.set<TransitionEffectType::RESERVED>({});
            break;
        }
    }

    GENERATE_CLASS_FUNCTIONS(ViewportRelative);
    void ViewportRelative::write(BitStream& bitstr) const
    {
        bitstr.write16Bits(rectLeftPercent);
        bitstr.write16Bits(rectTopPercent);
        bitstr.write16Bits(rectWidthtPercent);
        bitstr.write16Bits(rectHeightPercent);
    }

    void ViewportRelative::parse(BitStream& bitstr)
    {
        rectLeftPercent   = bitstr.read16Bits();
        rectTopPercent    = bitstr.read16Bits();
        rectWidthtPercent = bitstr.read16Bits();
        rectHeightPercent = bitstr.read16Bits();
    }

    GENERATE_CLASS_FUNCTIONS(ViewpointLoopingStruct);
    void ViewpointLoopingStruct::write(BitStream& bitstr) const
    {
        bitstr.writeBits(bool(maxLoops), 1);
        bitstr.writeBits(bool(loopActivationTime), 1);
        bitstr.writeBits(bool(loopStartTime), 1);
        bitstr.writeBits(bool(loopExitStruct), 1);

        bitstr.writeBits(0, 4);

        writeOptional(maxLoops, bitstr, &BitStream::write8BitsSigned);
        writeOptional(loopActivationTime, bitstr, &BitStream::write32BitsSigned);
        writeOptional(loopStartTime, bitstr, &BitStream::write32BitsSigned);
        writeOptional(loopExitStruct, bitstr);
    }

    void ViewpointLoopingStruct::parse(BitStream& bitstr)
    {
        bool maxLoopsFlag = bool(bitstr.readBits(1));
        bool loopActivationTimeFlag = bool(bitstr.readBits(1));
        bool loopStartTimeFlag = bool(bitstr.readBits(1));
        bool loopExitInfoFlag = bool(bitstr.readBits(1));

        bitstr.readBits(4);

        parseOptionalIf(maxLoopsFlag, maxLoops, bitstr, &BitStream::read8BitsSigned);
        parseOptionalIf(loopActivationTimeFlag, loopActivationTime, bitstr, &BitStream::read32BitsSigned);
        parseOptionalIf(loopStartTimeFlag, loopStartTime, bitstr, &BitStream::read32BitsSigned);
        parseOptionalIf(loopExitInfoFlag, loopExitStruct, bitstr);
    }

    GENERATE_CLASS_FUNCTIONS(ViewpointSwitchRegionStruct);
    void ViewpointSwitchRegionStruct::write(BitStream& bitstr) const
    {
        bitstr.writeBits(unsigned(region.getKey()), 2);
        bitstr.writeBits(0, 6);  // reserved
        switch (region.getKey())
        {
        case ViewpointRegionType::VIEWPORT_RELATIVE:
            region.at<ViewpointRegionType::VIEWPORT_RELATIVE>().write(bitstr);
            break;
        case ViewpointRegionType::SPHERE_RELATIVE:
        {
            auto& shape = region.at<ViewpointRegionType::SPHERE_RELATIVE>();
            bitstr.writeBits(static_cast<std::uint8_t>(shape.shapeType), 8);
            shape.sphereRegion.write(bitstr);
            break;
        }
        case ViewpointRegionType::OVERLAY:
            bitstr.write16Bits(region.at<ViewpointRegionType::OVERLAY>());
            break;
        }
    }

    void ViewpointSwitchRegionStruct::parse(BitStream& bitstr)
    {
        // All the functions could return 'false' upon that case, but that's not currently
        // how the interface works.
        auto key = static_cast<ViewpointRegionType>(bitstr.readBits(2));
        bitstr.readBits(6);  // reserved
        switch (key)
        {
        case ViewpointRegionType::VIEWPORT_RELATIVE:
            region.emplace<ViewpointRegionType::VIEWPORT_RELATIVE>().parse(bitstr);
            break;
        case ViewpointRegionType::SPHERE_RELATIVE:
        {
            auto& shape     = region.emplace<ViewpointRegionType::SPHERE_RELATIVE>();
            shape.shapeType = static_cast<SphereRegionShapeType>(bitstr.readBits(8));
            shape.sphereRegion.parse(bitstr);
            break;
        }
        case ViewpointRegionType::OVERLAY:
            region.set<ViewpointRegionType::OVERLAY>(bitstr.read16Bits());
            break;
        }
    }


    GENERATE_CLASS_FUNCTIONS(OneViewpointSwitchingStruct);
    void OneViewpointSwitchingStruct::write(BitStream& bitstr) const
    {
        bitstr.write32Bits(destinationViewpointId);
        bitstr.writeBits(static_cast<unsigned>(viewingOrientation.getKey()), 2);
        bitstr.writeBits(bool(transitionEffect), 1);
        bitstr.writeBits(bool(viewpointTimelineSwitch), 1);
        bitstr.writeBits(bool(viewpointSwitchRegions.numElements), 1);
        bitstr.writeBits(0, 3);
        writeOptional(viewingOrientation.getOpt<ViewingOrientationMode::VIEWPORT>(), bitstr);
        writeOptional(viewpointTimelineSwitch, bitstr);
        writeOptional(transitionEffect, bitstr);
        if (viewpointSwitchRegions.numElements)
        {
            bitstr.writeBits(viewpointSwitchRegions.numElements, 4);
            bitstr.writeBits(0, 4); // reserved
            for (auto& region: viewpointSwitchRegions)
            {
                region.write(bitstr);
            }
        }
    }

    void OneViewpointSwitchingStruct::parse(BitStream& bitstr)
    {
        destinationViewpointId              = bitstr.read32Bits();
        auto viewingOrientationMode         = static_cast<ViewingOrientationMode>(bitstr.readBits(2));
        bool transitionEffectPresent        = bitstr.readBit();
        bool viewpointTimelineSwitchPresent = bitstr.readBit();
        bool viewpointSwitchRegionPresent   = bitstr.readBit();

        bitstr.readBits(3u);  // ignore bits

        switch (viewingOrientationMode)
        {
        case ViewingOrientationMode::DEFAULT:
            viewingOrientation.set<ViewingOrientationMode::DEFAULT>({});
            break;
        case ViewingOrientationMode::VIEWPORT:
            viewingOrientation.set<ViewingOrientationMode::VIEWPORT>({}).parse(bitstr);
            break;
        case ViewingOrientationMode::NO_INFLUENCE:
            viewingOrientation.set<ViewingOrientationMode::NO_INFLUENCE>({});
            break;
        case ViewingOrientationMode::RESERVED:
            viewingOrientation.set<ViewingOrientationMode::RESERVED>({});
            break;
        }
        parseOptionalIf(viewpointTimelineSwitchPresent, viewpointTimelineSwitch, bitstr);
        parseOptionalIf(transitionEffectPresent, transitionEffect, bitstr);
        if (viewpointSwitchRegionPresent)
        {
            size_t size = bitstr.readBits(4);
            bitstr.readBits(4);
            viewpointSwitchRegions = DynArray<ViewpointSwitchRegionStruct>(size);
            for (size_t index = 0; index < size; ++index)
            {
                viewpointSwitchRegions[index].parse(bitstr);
            }
        }
    }

    GENERATE_CLASS_FUNCTIONS(ViewpointSwitchingListStruct);
    void ViewpointSwitchingListStruct::write(BitStream& bitstr) const
    {
        bitstr.write8Bits(viewpointSwitching.numElements);
        for (auto& x : viewpointSwitching)
        {
            x.write(bitstr);
        }
    }

    void ViewpointSwitchingListStruct::parse(BitStream& bitstr)
    {
        size_t numElements = bitstr.read8Bits();
        viewpointSwitching = DynArray<OneViewpointSwitchingStruct>(numElements);
        for (size_t i = 0u; i < numElements; ++i)
        {
            viewpointSwitching[i].parse(bitstr);
        }
    }

    void DynamicViewpointGroup::write(BitStream& bitstr) const
    {
        bitstr.writeBit(bool(viewpointGroupStruct));
        bitstr.writeBits(0, 7);
        writeOptional(viewpointGroupStruct, bitstr);
    }

    void DynamicViewpointGroup::parse(BitStream& bitstr)
    {
        bool viewpointGroupStructFlag = bitstr.readBit();
        bitstr.readBits(7);
        parseOptionalIf(viewpointGroupStructFlag, viewpointGroupStruct, bitstr);
    }

    void DynamicViewpointSample::write(BitStream& bitstr, const DynamicViewpointSample::Context& aSampleEntry) const
    {
        viewpointPosStruct.write(bitstr);
        assert(bool(aSampleEntry.viewpointGpsPositionStruct) != bool(viewpointGpsPositionStruct));
        writeOptional(viewpointGpsPositionStruct, bitstr);
        assert(bool(aSampleEntry.viewpointGeomagneticInfoStruct) != bool(viewpointGeomagneticInfoStruct));
        writeOptional(viewpointGeomagneticInfoStruct, bitstr);
        assert(bool(aSampleEntry.viewpointGlobalCoordinateSysRotationStruct) !=
               bool(viewpointGlobalCoordinateSysRotationStruct));
        writeOptional(viewpointGlobalCoordinateSysRotationStruct, bitstr);
        assert(bool(aSampleEntry.viewpointSwitchingListStruct) != bool(viewpointSwitchingListStruct));
        writeOptional(viewpointSwitchingListStruct, bitstr);
        assert(bool(dynamicViewpointGroup) == aSampleEntry.dynamicVwptGroupFlag);
        assert(implies(dynamicViewpointGroup && bool(dynamicViewpointGroup->viewpointGroupStruct),
                       aSampleEntry.dynamicVwptGroupFlag));
        writeOptional(dynamicViewpointGroup, bitstr);
    }

    void DynamicViewpointSample::parse(BitStream& bitstr, const DynamicViewpointSample::Context& aSampleEntry)
    {
        viewpointPosStruct.parse(bitstr);
        parseOptionalIf(!aSampleEntry.viewpointGpsPositionStruct, viewpointGpsPositionStruct, bitstr);
        parseOptionalIf(!aSampleEntry.viewpointGeomagneticInfoStruct, viewpointGeomagneticInfoStruct, bitstr);
        parseOptionalIf(!aSampleEntry.viewpointGlobalCoordinateSysRotationStruct,
                        viewpointGlobalCoordinateSysRotationStruct, bitstr);
        parseOptionalIf(!aSampleEntry.viewpointSwitchingListStruct, viewpointSwitchingListStruct, bitstr);
        parseOptionalIf(aSampleEntry.dynamicVwptGroupFlag, dynamicViewpointGroup, bitstr);
        assert(implies(dynamicViewpointGroup && bool(dynamicViewpointGroup->viewpointGroupStruct),
                       aSampleEntry.dynamicVwptGroupFlag));
    }

    /* From OMAFv2-WD6-Draft01-SM.pdf */
    /* class DynamicViewpointSampleEntry extends MetaDataSampleEntry('dyvp') */
    /* { */
    /*     unsigned int(1) vwpt_pos_flag; */
    /*     unsigned int(1) dynamic_gcs_rotation_flag; */
    /*     unsigned int(1) dynamic_vwpt_group_flag; */
    /*     unsigned int(1) dynamic_vwpt_gps_flag; */
    /*     unsigned int(1) dynamic_vwpt_geomagnetic_info_flag; */
    /*     unsigned int(1) vwpt_switch_flag; */
    /*     bit(2) reserved = 0; */
    /*     if (vwpt_pos_flag) */
    /*         ViewpointPosStruct(); */
    /*     if (!dynamic_gcs_rotation_flag) */
    /*         ViewpointGlobalCoordinateSysRotationStruct(); */
    /*     if (!dynamic_vwpt_gps_flag) */
    /*         ViewpointGpsPositionStruct(); */
    /*     if (!dynamic_vwpt_geomagnetic_info_flag) */
    /*         ViewpointGeomagneticInfoStruct(); */
    /*     if (vwpt_switch_flag) */
    /*         ViewpointSwitchingListStruct(); */
    /* } */

    void DynamicViewpointSampleEntry::write(BitStream& bitstr) const
    {
        /*     unsigned int(1) vwpt_pos_flag; */
        bitstr.writeBit(bool(viewpointPosStruct));
        /*     unsigned int(1) dynamic_gcs_rotation_flag; */
        bitstr.writeBit(!bool(viewpointGlobalCoordinateSysRotationStruct));
        /*     unsigned int(1) dynamic_vwpt_group_flag; */
        bitstr.writeBit(dynamicVwptGroupFlag);
        /*     unsigned int(1) dynamic_vwpt_gps_flag; */
        bitstr.writeBit(!bool(viewpointGpsPositionStruct));
        /*     unsigned int(1) dynamic_vwpt_geomagnetic_info_flag; */
        bitstr.writeBit(!bool(viewpointGeomagneticInfoStruct));
        /*     unsigned int(1) dynamic_vwpt_switch_flag; */
        bitstr.writeBit(!bool(viewpointSwitchingListStruct));
        //     bit(2) reserved = 0;
        bitstr.writeBits(0, 2);
        //     if (vwpt_pos_flag)
        //         ViewpointPosStruct();
        writeOptional(viewpointPosStruct, bitstr);
        //     if (!dynamic_gcs_rotation_flag)
        //         ViewpointGlobalCoordinateSysRotationStruct();
        writeOptional(viewpointGlobalCoordinateSysRotationStruct, bitstr);
        //     if (!dynamic_vwpt_gps_flag)
        //         ViewpointGpsPositionStruct();
        writeOptional(viewpointGpsPositionStruct, bitstr);
        //     if (!dynamic_vwpt_geomagnetic_info_flag)
        //         ViewpointGeomagneticInfoStruct();
        writeOptional(viewpointGeomagneticInfoStruct, bitstr);
        //     if (vwpt_switch_flag)
        //         ViewpointSwitchingListStruct();
        writeOptional(viewpointSwitchingListStruct, bitstr);
    }

    void DynamicViewpointSampleEntry::parse(BitStream& bitstr)
    {
        /*     unsigned int(1) vwpt_pos_flag; */
        bool viewpointPosStructFlag = bitstr.readBit();
        /*     unsigned int(1) dynamic_gcs_rotation_flag; */
        bool dynamicViewpointGlobalCoordinateSysRotationStructFlag = bitstr.readBit();
        /*     unsigned int(1) dynamic_vwpt_group_flag; */
        dynamicVwptGroupFlag = bitstr.readBit();
        /*     unsigned int(1) dynamic_vwpt_gps_flag; */
        bool dynamicViewpointGpsPositionStructFlag = bitstr.readBit();
        /*     unsigned int(1) dynamic_vwpt_geomagnetic_info_flag; */
        bool dynamicViewpointGeomagneticInfoStructFlag = bitstr.readBit();
        /*     unsigned int(1) vwpt_switch_flag; */
        bool dynamicViewpointSwitchingStructFlag = bitstr.readBit();
        //     bit(2) reserved = 0;
        bitstr.readBits(2);
        //     if (vwpt_pos_flag)
        //         ViewpointPosStruct();
        parseOptionalIf(viewpointPosStructFlag, viewpointPosStruct, bitstr);
        //     if (!dynamic_gcs_rotation_flag)
        //         ViewpointGlobalCoordinateSysRotationStruct();
        parseOptionalIf(!dynamicViewpointGlobalCoordinateSysRotationStructFlag,
                        viewpointGlobalCoordinateSysRotationStruct, bitstr);
        //     if (!dynamic_vwpt_gps_flag)
        //         ViewpointGpsPositionStruct();
        parseOptionalIf(!dynamicViewpointGpsPositionStructFlag, viewpointGpsPositionStruct, bitstr);
        //     if (!dynamic_vwpt_geomagnetic_info_flag)
        //         ViewpointGeomagneticInfoStruct();
        parseOptionalIf(!dynamicViewpointGeomagneticInfoStructFlag, viewpointGeomagneticInfoStruct, bitstr);
        //     if (vwpt_switch_flag)
        //         ViewpointSwitchingListStruct();
        parseOptionalIf(!dynamicViewpointSwitchingStructFlag, viewpointSwitchingListStruct, bitstr);
    }

    void ViewpointInformationStruct::write(BitStream& bitstr) const
    {
        bitstr.writeBit(bool(viewpointGpsPositionStruct));
        bitstr.writeBit(bool(viewpointGeomagneticInfoStruct));
        bitstr.writeBit(bool(viewpointSwitchingListStruct));
        bitstr.writeBit(bool(viewpointLoopingStruct));
        bitstr.writeBits(0, 4);

        viewpointPosStruct.write(bitstr);
        viewpointGroupStruct.write(bitstr);
        viewpointGlobalCoordinateSysRotationStruct.write(bitstr);

        writeOptional(viewpointGpsPositionStruct, bitstr);
        writeOptional(viewpointGeomagneticInfoStruct, bitstr);
        writeOptional(viewpointSwitchingListStruct, bitstr);
        writeOptional(viewpointLoopingStruct, bitstr);
    }

    void ViewpointInformationStruct::parse(BitStream& bitstr)
    {
        bool viewpointGpsPositionStructFlag     = bitstr.readBit();
        bool viewpointGeomagneticInfoStructFlag = bitstr.readBit();
        bool viewpointSwitchingInfoStructFlag   = bitstr.readBit();
        bool viewpointLoopingStructFlag         = bitstr.readBit();
        bitstr.readBits(4);

        viewpointPosStruct.parse(bitstr);
        viewpointGroupStruct.parse(bitstr);
        viewpointGlobalCoordinateSysRotationStruct.parse(bitstr);

        parseOptionalIf(viewpointGpsPositionStructFlag, viewpointGpsPositionStruct, bitstr);
        parseOptionalIf(viewpointGeomagneticInfoStructFlag, viewpointGeomagneticInfoStruct, bitstr);
        parseOptionalIf(viewpointSwitchingInfoStructFlag, viewpointSwitchingListStruct, bitstr);
        parseOptionalIf(viewpointLoopingStructFlag, viewpointLoopingStruct, bitstr);
    }

    void InitialViewpointSampleEntry::parse(BitStream& bitstr)
    {
        idOfInitialViewpoint = bitstr.read32Bits();
    }

    void InitialViewpointSampleEntry::write(BitStream& bitstr) const
    {
        bitstr.write32Bits(idOfInitialViewpoint);
    }

    void InitialViewpointSample::parse(BitStream& bitstr)
    {
        idOfInitialViewpoint = bitstr.read32Bits();
    }

    void InitialViewpointSample::write(BitStream& bitstr) const
    {
        bitstr.write32Bits(idOfInitialViewpoint);
    }

    SphereRegionContext::SphereRegionContext()
        : hasInterpolate(true)
    {
        // nothing
    }

    SphereRegionContext::SphereRegionContext(const SphereRegionConfigStruct& aConfig)
        : hasInterpolate(true)
        , hasRange(!aConfig.staticRange)
    {
        // nothing
    }

    void SphereRegionSample::write(BitStream& bitstr, const SphereRegionSample::Context& aSampleContext) const
    {
        // OMAF 7.7.2.3 says that there is always exactly 1 region, but for future this allows already multiple
        for (auto& region: regions)
        {
            region.write(bitstr, aSampleContext);
        }
    }

    void SphereRegionSample::parse(BitStream& bitstr, const SphereRegionSample::Context& aSampleContext)
    {
        Vector<typename decltype(regions)::value_type> collect;
        // OMAF 7.7.2.3 says that there is always exactly 1 region, but for future this allows already multiple
        while (bitstr.numBytesLeft())
        {
            collect.push_back({});
            collect.back().parse(bitstr, aSampleContext);
        }
        regions = makeDynArray(collect);
    }

    void SubPictureRegionData::write(BitStream& bitstr) const
    {
        bitstr.write16Bits(objectX);
        bitstr.write16Bits(objectY);
        bitstr.write16Bits(objectWidth);
        bitstr.write16Bits(objectHeight);
        bitstr.writeBits(0, 14);  // reserved
        bitstr.writeBits(trackNotAloneFlag, 1);
        bitstr.writeBits(trackNotMergeableFlag, 1);
    }

    void SubPictureRegionData::parse(BitStream& bitstr)
    {
        objectX      = bitstr.read16Bits();
        objectY      = bitstr.read16Bits();
        objectWidth  = bitstr.read16Bits();
        objectHeight = bitstr.read16Bits();
        bitstr.readBits(14);  // reserved
        trackNotAloneFlag     = bitstr.readBits(1);
        trackNotMergeableFlag = bitstr.readBits(1);
    }

    void SpatialRelationship2DSourceData::write(BitStream& bitstr) const
    {
        bitstr.write32Bits(totalWidth);
        bitstr.write32Bits(totalHeight);
        bitstr.write32Bits(sourceId);
    }

    void SpatialRelationship2DSourceData::parse(BitStream& bitstr)
    {
        totalWidth  = bitstr.read32Bits();
        totalHeight = bitstr.read32Bits();
        sourceId    = bitstr.read32Bits();
    }

    template <typename T>
    T isobmffFromBytes(const uint8_t* aFrameData, uint32_t aFrameLen)
    {
        T object;
        BitStream bs({aFrameData, aFrameData + aFrameLen});
        object.parse(bs);
        return object;
    }

    template <typename T>
    T isobmffFromBytes(const uint8_t* aFrameData, uint32_t aFrameLen, const typename T::Context& aContext)
    {
        T object;
        BitStream bs({aFrameData, aFrameData + aFrameLen});
        object.parse(bs, aContext);
        return object;
    }

    template <typename T>
    DynArray<uint8_t> isobmffToBytes(const T& aObject)
    {
        BitStream bs;
        aObject.write(bs);
        auto& storage = bs.getStorage();
        if (storage.size())
        {
            return {&storage[0], &storage[0] + storage.size()};
        }
        else
        {
            return {};
        }
    }

    template <typename T>
    DynArray<uint8_t> isobmffToBytes(const T& aObject, const typename T::Context& aContext)
    {
        BitStream bs;
        aObject.write(bs, aContext);
        auto& storage = bs.getStorage();
        if (storage.size())
        {
            return {&storage[0], &storage[0] + storage.size()};
        }
        else
        {
            return {};
        }
    }

#define INSTANTIATE_ISOBMFF_CONVERSION_FUNCTIONS(TYPE)      \
    template DynArray<uint8_t> isobmffToBytes(const TYPE&); \
    template TYPE isobmffFromBytes(const uint8_t* aFrameData, uint32_t aFrameLen);

#define INSTANTIATE_ISOBMFF_CONVERSION_FUNCTIONS_W_CONTEXT(TYPE)                                   \
    template TYPE isobmffFromBytes(const uint8_t* aFrameData, uint32_t aFrameLen, const typename TYPE::Context& aContext); \
    template DynArray<uint8_t> isobmffToBytes(const TYPE&, const TYPE::Context& aContext)

    INSTANTIATE_ISOBMFF_CONVERSION_FUNCTIONS_W_CONTEXT(ISOBMFF::DynamicViewpointSample);
    INSTANTIATE_ISOBMFF_CONVERSION_FUNCTIONS(ISOBMFF::DynamicViewpointSampleEntry);

    INSTANTIATE_ISOBMFF_CONVERSION_FUNCTIONS(ISOBMFF::InitialViewpointSampleEntry);
    INSTANTIATE_ISOBMFF_CONVERSION_FUNCTIONS(ISOBMFF::InitialViewpointSample);

    INSTANTIATE_ISOBMFF_CONVERSION_FUNCTIONS_W_CONTEXT(ISOBMFF::SphereRegionSample);
}  // namespace ISOBMFF
