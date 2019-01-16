
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

#include "Platform/OMAFCompiler.h"
#include "Foundation/NVRDependencies.h"
#include "Platform/OMAFDataTypes.h"
#include "Foundation/NVRFixedString.h"
OMAF_NS_BEGIN
    class Uri
    {
    public:
        Uri();
        Uri(const Uri& aUri);
        Uri(const char_t* aString,size_t aLength=Npos);
        ~Uri();
  
        void parse(const char_t* aString, size_t aLength = Npos);
        Uri resolve(const char_t* other, size_t aLength = Npos);
        Uri resolve(const Uri& other);

        const FixedString<1024>& toString() const;
        const FixedString<1024> scheme() const;
        const FixedString<1024> authority() const;
        const FixedString<1024> path() const;
        const FixedString<1024> query() const;
        const FixedString<1024> fragment() const;

        void toString(FixedString<1024>&) const;
        void scheme(FixedString<1024>&) const;
        void authority(FixedString<1024>&) const;
        void path(FixedString<1024>&) const;
        void query(FixedString<1024>&) const;
        void fragment(FixedString<1024>&) const;

        Uri& operator=(const Uri& other);
        bool operator==(const Uri& other) const;
        bool operator!=(const Uri& other) const;
    protected:
        
        class StringView
        {
        public:
            //Helper class, to reduce copying. does not own the string data.
            StringView();
            StringView(const char_t* aData,size_t aLength );
            ~StringView();
            
            bool isEmpty() const;
            size_t getLength() const;
            const char_t* getData() const;
            size_t FindFirst(char_t chr, size_t offset = Npos) const;
            size_t FindLast(char_t chr, size_t offset = Npos) const;
            StringView substring(size_t start, size_t length = Npos) const;
            void clear();
            const char_t operator[](size_t)const;
            bool operator==(const StringView&) const;
            bool operator!=(const StringView&) const;
            /*const FixedString<1024> toString() const;
            void toString(FixedString<1024>&) const;*/
        private:
            size_t mLength;
            const char_t* mData;
            //StringView(const StringView& aUri);
            //StringView& operator=(const StringView& other);
        };    
        void merge(const Uri&base, const Uri&ref, FixedString<1024>& res);
        void remove_dot_segments(const StringView& path, FixedString<1024>& result);
        FixedString<1024> mString;
        FixedString<1024> mTempPath;//used during resolve..
        StringView mScheme;
        StringView mAuthority;
        StringView mPath;
        StringView mQuery;
        StringView mFragment;
    };
OMAF_NS_END
