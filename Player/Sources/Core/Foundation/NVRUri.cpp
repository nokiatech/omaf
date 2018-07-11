
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
#include "Foundation/NVRUri.h"
#include "Foundation/NVRMemoryAllocator.h"
OMAF_NS_BEGIN

Uri::StringView::StringView()
{
    mLength = 0;
    mData = OMAF_NULL;
}
Uri::StringView::StringView(const char_t* data, size_t len)
{
    mLength = len;
    mData = data;
}
Uri::StringView::~StringView()
{
}
const char_t Uri::StringView::operator[](size_t offset)const
{
    if (offset < mLength)
        return mData[offset];
    return 0;
}
size_t Uri::StringView::getLength() const
{
    return mLength;
}
const char_t* Uri::StringView::getData() const
{
    return mData;
}
bool Uri::StringView::isEmpty() const
{
    return mLength == 0;
}
void Uri::StringView::clear()
{
    mData = OMAF_NULL;
    mLength = 0;
}

Uri::StringView Uri::StringView::substring(size_t start, size_t length) const
{
    StringView tmp;
    if (start < mLength)
    {
        if (length == Npos) length = mLength;
        if ((start + length) > mLength) length = mLength - start;
        if (length > 0)
        {
            tmp = StringView(mData + start, length);
        }
    }
    return tmp;
}

/*const FixedString<1024> Uri::StringView::toString() const
{
    FixedString<1024> tmp;
    tmp.resize(mLength);
    memcpy(tmp.getData(), mData, mLength);
    return tmp;
}
void Uri::StringView::toString(FixedString<1024>& tmp) const
{
    tmp.resize(mLength);
    memcpy(tmp.getData(), mData, mLength);    
}*/

bool Uri::StringView::operator==(const StringView& other) const
{
    if (mLength != other.mLength) return false;
    if (0 == memcmp(mData, other.mData, mLength)) return true;
    return false;
}

bool Uri::StringView::operator!=(const StringView& other) const
{
    return !(*this == other);
}
size_t Uri::StringView::FindFirst(char_t chr, size_t offset) const
{
    if (offset == Npos) offset = 0;
    if (offset >= mLength) return Npos;
    const char_t* src = mData + offset;
    const char_t* end = mData + mLength;
    while (src < end)
    {
        if (*src == chr)
        {
            return src - mData;
        }
        src++;
    }
    return Npos;

}
size_t Uri::StringView::FindLast(char_t chr, size_t offset) const
{
    if (mLength == 0) return Npos;
    if (offset == Npos) offset = 0;
    if (offset >= mLength) return Npos;
    const char_t* src = mData + mLength - 1;
    const char_t* start = mData + offset;

    while (src >= start)
    {
        if (*src == chr)
        {
            return src - mData;
        }
        src--;
    }
    return Npos;
}


Uri::Uri()
{
}
Uri::Uri(const char_t* aUrl, size_t aLength)
{
    parse(aUrl,aLength);
}
Uri::~Uri()
{
}
Uri::Uri(const Uri& other)
{
    mString = other.mString;
    mScheme = StringView(mString.getData() + (other.mScheme.getData() - other.mString), other.mScheme.getLength());
    mAuthority = StringView(mString.getData() + (other.mAuthority.getData() - other.mString), other.mAuthority.getLength());
    mPath = StringView(mString.getData() + (other.mPath.getData() - other.mString), other.mPath.getLength());
    mQuery = StringView(mString.getData() + (other.mQuery.getData() - other.mString), other.mQuery.getLength());
    mFragment = StringView(mString.getData() + (other.mFragment.getData() - other.mString), other.mFragment.getLength());
}
Uri& Uri::operator=(const Uri& other)
{
    if (this != &other)
    {
        //okay start copying..
        mTempPath.clear();
        mString = other.mString;
        mScheme = StringView(mString.getData() + (other.mScheme.getData() - other.mString), other.mScheme.getLength());
        mAuthority = StringView(mString.getData() + (other.mAuthority.getData() - other.mString), other.mAuthority.getLength());
        mPath = StringView(mString.getData() + (other.mPath.getData() - other.mString), other.mPath.getLength());
        mQuery = StringView(mString.getData() + (other.mQuery.getData() - other.mString), other.mQuery.getLength());
        mFragment = StringView(mString.getData() + (other.mFragment.getData() - other.mString), other.mFragment.getLength());
    }
    return *this;
}

void Uri::parse(const char_t* url, size_t aLength)
{
    /*
    foo://example.com:8042/over/there?name=ferret#nose
    \_/   \______________/\_________/ \_________/ \__/
    |           |            |            |        |
    scheme     authority     path        query   fragment
    |   _____________________|__
    / \ /                        \
    urn:example:animal:ferret:nose
    */

    mScheme.clear();
    mPath.clear();
    mAuthority.clear();
    mFragment.clear();
    mQuery.clear();
    if (aLength != Npos)
    {
        mString.append(url, aLength);
    }
    else
    {
        mString = url;
    }

    StringView tmp(mString.getData(), mString.getLength());
    size_t path_start = 0;
    size_t query_start = Npos;
    size_t frag_start = Npos;
    //input is uri-reference
    //so does it have a scheme    
    size_t scheme_end = tmp.FindFirst(':');
    query_start = tmp.FindFirst('?');
    frag_start = tmp.FindFirst('#');
    if (Npos != scheme_end)
    {
        //yes
        mScheme = tmp.substring(0, scheme_end);// .set(tmp.getData(), scheme_end);
        scheme_end += 1;//skip the :        
    }
    else
    {
        scheme_end = 0;
    }

    if (tmp.substring(scheme_end, 2) == StringView("//", 2))
    {
        //has authority.
        size_t auth_start = 2 + scheme_end;
        path_start = tmp.FindFirst('/', auth_start);
        size_t auth_end = tmp.getLength();
        if (path_start != Npos)
        {
            auth_end = path_start;
        }
        else if (query_start != Npos)
        {
            auth_end = query_start;
        }
        else if (frag_start != Npos)
        {
            auth_end = frag_start;
        }
        mAuthority = tmp.substring(auth_start, auth_end - auth_start);// .set(tmp.getData() + auth_start, auth_end - auth_start);
    }
    else
    {
        //no authority, so path starts at end of scheme.
        path_start = scheme_end;
    }

    if (path_start != Npos)
    {
        //we have path..
        size_t path_end = tmp.getLength();
        if (query_start != Npos)
        {
            path_end = query_start;
        }
        else if (frag_start != Npos)
        {
            path_end = frag_start;
        }
        mPath = tmp.substring(path_start, path_end - path_start);// .set(tmp.getData() + path_start, path_end - path_start);
    }

    if (query_start != Npos)
    {
        //we have query
        size_t query_end = tmp.getLength();
        if (frag_start != Npos)
        {
            query_end = frag_start;
        }
        mQuery = tmp.substring(query_start + 1, query_end - query_start - 1);// .set(tmp.getData() + query_start + 1, query_end - query_start - 1);
    }

    if (frag_start != Npos)
    {
        //we have fragment
        size_t frag_end = tmp.getLength();
        mFragment = tmp.substring(frag_start + 1, frag_end - frag_start - 1);// .set(tmp.getData() + frag_start + 1, frag_end - frag_start - 1);
    }
}

void Uri::remove_dot_segments(const StringView& path, FixedString<1024>& result)
{
    //This method could gain from some cleanup.
    
    FixedString<1024> res;
    //additional check to see if there are any dotsegments..
    if (path.FindFirst('.') == Npos)
    {
        //no dots.. just do a COPY?!?!
        res.append(path.getData(), path.getLength());
        result = res;
        return;
    }

    //See RFC3986 5.2.4
    
    StringView tmp = path;
    for (;;)
    {
        if (tmp.isEmpty()) break;
        char a, b;
        a = tmp[0];
        b = '.';
        size_t len = tmp.getLength();
        if (len > 1) b = tmp[1];
        if (((a == '.') || (a == '/')) && ((b == '.') || (b == '/')))
        {
            char s2[3] = { 0 };
            char s3[4] = { 0 };
            char s4[5] = { 0 };
            s2[0] = s3[0] = s4[0] = tmp[0];
            if (len > 1)
            {
                s2[1] = s3[1] = s4[1] = tmp[1];
            }
            if (len > 2)
            {
                s3[2] = s4[2] = tmp[2];
            }
            if (len > 3)
            {
                s4[3] = tmp[3];
            }
            //A
            if (strcmp(s3, "../") == 0)
            {
                tmp = tmp.substring(3);
                continue;
            }
            if (strcmp(s2, "./") == 0)
            {
                tmp = tmp.substring(2);
                continue;
            }
            //B
            if (strcmp(s3, "/./") == 0)
            {
                tmp = tmp.substring(2);
                continue;
            }
            if (strcmp(s2, "/.") == 0)
            {
                if (tmp.getLength() == 2)
                {
                    tmp = tmp.substring(0, 1);
                    continue;
                }
            }
            //C
            if (strcmp(s4, "/../") == 0)
            {
                tmp = tmp.substring(3);
                size_t p = StringView((char_t*)res.getData(), res.getLength()).FindLast('/');
                if (p == Npos)
                {
                    res.clear();
                }
                else
                {
                    res.resize(p);
                }
                continue;
            }
            if (strcmp(s4, "/..") == 0)
            {
                if (tmp.getLength() == 3)
                {
                    tmp = tmp.substring(0, 1);
                    size_t p = StringView((char_t*)res.getData(), res.getLength()).FindLast('/');
                    if (p == Npos)
                    {
                        res.clear();
                    }
                    else
                    {
                        res.resize(p);
                    }
                    continue;
                }
            }
            //D
            if (tmp == StringView(".", 1))
            {
                break;
            }
            if (tmp == StringView("..", 2))
            {
                break;
            }
        }
        //E
        size_t p, op = 0;
        if (tmp[0] == '/')
        {
            op = 1;
        }
        p = tmp.FindFirst('/', op);
        if (p == Npos)
        {
            //res += tmp.substr(0, p);
            res.append(tmp.getData(), tmp.getLength());
        }
        else
        {
            //res += tmp.substr(0, p);
            res.append(tmp.getData(), p);
        }
        tmp = tmp.substring(p);
    }
    result = res;
}

void Uri::merge(const Uri&base, const Uri&ref, FixedString<1024>& res)
{
    //See RFC3986 5.2.3    
    res.clear();
    if (!base.mAuthority.isEmpty() && base.mPath.isEmpty())
    {
        res = "/";
    }
    else
    {
        size_t last = base.mPath.FindLast('/');
        if (last != Npos)
        {
            res.append(base.mPath.getData(), last + 1);
        }
    }
    res.append(ref.mPath.getData(), ref.mPath.getLength());
}

Uri Uri::resolve(const char_t* other, size_t aLength)
{
    return resolve(Uri(other,aLength));
}
Uri Uri::resolve(const Uri& other)
{    
    const int strict = 0;//non strict!
                         //See RFC3986 5.2.2
    Uri T;//result.
    const Uri& R = other;

    /*--The URI reference is parsed into the five URI components*/
    Uri& Base = *this;

    T.mTempPath.clear();
    /*A non - strict parser may ignore a scheme in the reference
    if it is identical to the base URI's scheme.
    */
    bool schemeEmpty = R.mScheme.isEmpty();
    if ((!strict) && (R.mScheme == Base.mScheme))
    {
        schemeEmpty = true;
    }

    if (!schemeEmpty)
    {
        T.mScheme = R.mScheme;
        T.mAuthority = R.mAuthority;
        remove_dot_segments(R.mPath, T.mTempPath);
        T.mQuery = R.mQuery;
    }
    else
    {
        if (!R.mAuthority.isEmpty())
        {
            T.mAuthority = R.mAuthority;
            remove_dot_segments(R.mPath, T.mTempPath);
            T.mQuery = R.mQuery;
        }
        else
        {
            if (R.mPath.isEmpty())
            {
                T.mTempPath.append(Base.mPath.getData(), Base.mPath.getLength());
                if (!R.mQuery.isEmpty())
                {
                    T.mQuery = R.mQuery;
                }
                else
                {
                    T.mQuery = Base.mQuery;
                }
            }
            else
            {
                if (R.mPath[0] == '/')
                {
                    remove_dot_segments(R.mPath, T.mTempPath);
                }
                else
                {
                    merge(Base, R, T.mTempPath);
                    StringView tmp(T.mTempPath.getData(), T.mTempPath.getLength());
                    remove_dot_segments(tmp, T.mTempPath);
                }
                T.mQuery = R.mQuery;
            }
            T.mAuthority = Base.mAuthority;
        }
        T.mScheme = Base.mScheme;
    }
    T.mFragment = R.mFragment;


    //construct new mString.
    size_t tp = 0;
    T.mString.clear();
    if (!T.mScheme.isEmpty())
    {
        T.mString.append(T.mScheme.getData(), T.mScheme.getLength());
        T.mScheme=StringView(T.mString.getData() + tp, T.mScheme.getLength());
        tp += T.mScheme.getLength();
        T.mString.append(":");
        tp++;
    }
    if (!T.mAuthority.isEmpty())
    {
        T.mString.append("//");
        tp += 2;
        T.mString.append(T.mAuthority.getData(), T.mAuthority.getLength());
        T.mAuthority=StringView(T.mString.getData() + tp, T.mAuthority.getLength());
        tp += T.mAuthority.getLength();
    }

    //T.mTempPath contains the new path (merge and remove_dot_segments create a new path string)
    T.mString.append(T.mTempPath);
    T.mPath= StringView(T.mString.getData() + tp, T.mTempPath.getLength());

    if (!T.mQuery.isEmpty())
    {
        T.mString.append("?");
        tp++;
        T.mString.append(T.mQuery.getData(), T.mQuery.getLength());
        T.mQuery= StringView(T.mString.getData() + tp, T.mQuery.getLength());
        tp += T.mQuery.getLength();
    }
    if (!T.mFragment.isEmpty())
    {
        T.mString.append("#");
        tp++;
        T.mString.append(T.mFragment.getData(), T.mFragment.getLength());
        T.mFragment= StringView(T.mString.getData() + tp, T.mFragment.getLength());
        tp += T.mFragment.getLength();
    }

    return T;
}

const FixedString<1024>& Uri::toString() const
{
    return mString;
}
void Uri::toString(FixedString<1024>& string) const
{
    string = mString;
}

void Uri::scheme(FixedString<1024>& r) const
{
    r.clear();
    r.append(mScheme.getData(), mScheme.getLength());
}
void Uri::authority(FixedString<1024>& r) const
{
    r.clear();
    r.append(mAuthority.getData(), mAuthority.getLength());
}
void Uri::path(FixedString<1024>& r) const
{
    r.clear();
    r.append(mPath.getData(), mPath.getLength());
}
void Uri::query(FixedString<1024>& r) const
{
    r.clear();
    r.append(mQuery.getData(), mQuery.getLength());
}
void Uri::fragment(FixedString<1024>& r) const
{
    r.clear();
    r.append(mFragment.getData(), mFragment.getLength());
}

const FixedString<1024> Uri::scheme() const
{
    FixedString<1024> tmp;
    scheme(tmp);
    return tmp;
}
const FixedString<1024> Uri::authority() const
{
    FixedString<1024> tmp;
    authority(tmp);
    return tmp;
}
const FixedString<1024> Uri::path() const
{
    FixedString<1024> tmp;
    path(tmp);
    return tmp;
}
const FixedString<1024> Uri::query() const
{
    FixedString<1024> tmp;
    query(tmp);
    return tmp;
}
const FixedString<1024> Uri::fragment() const
{
    FixedString<1024> tmp;
    fragment(tmp);
    return tmp;
}

bool Uri::operator==(const Uri& other) const
{
    //just check that each component is identical
    if (mScheme != other.mScheme) return false;
    if (mAuthority != other.mAuthority) return false;
    if (mPath != other.mPath) return false;
    if (mQuery != other.mQuery) return false;
    if (mFragment != other.mFragment) return false;
    return true;
}

bool Uri::operator!=(const Uri& other) const
{
    //if any component is different ...
    if (mScheme != other.mScheme) return true;
    if (mAuthority != other.mAuthority) return true;
    if (mPath != other.mPath) return true;
    if (mQuery != other.mQuery) return true;
    if (mFragment != other.mFragment) return true;
    return false;
}
OMAF_NS_END
