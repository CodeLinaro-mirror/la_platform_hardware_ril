/*
 * Copyright (c) 2018, 2021 The Linux Foundation. All rights reserved.
 * Not a Contribution.
 */

/*
 * Copyright (C) 2005 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <binder/Parcel2.h>
#include <utils/Errors2.h>
#include <cstring>
#include <locale>
#include <codecvt>

// This macro should never be used at runtime, as a too large value
// of s could cause an integer overflow. Instead, you should always
// use the wrapper function pad_size()
#define PAD_SIZE_UNSAFE(s) (((s)+3)&~3)

static size_t pad_size(size_t s) {
    if (s > (SIZE_T_MAX - 3)) {
        abort();
    }
    return PAD_SIZE_UNSAFE(s);
}

static pthread_mutex_t gParcelGlobalAllocSizeLock = PTHREAD_MUTEX_INITIALIZER;
static size_t gParcelGlobalAllocSize = 0;
static size_t gParcelGlobalAllocCount = 0;

Parcel::Parcel() {
    initState();
}

void Parcel::initState()
{
    mError = NO_ERROR;
    mData = 0;
    mDataSize = 0;
    mDataCapacity = 0;
    mDataPos = 0;
}

Parcel::~Parcel() {
    freeDataNoInit();
}

void Parcel::freeDataNoInit()
{
    if (mData) {
        //LOG_ALLOC("Parcel %p: freeing with %zu capacity", this, mDataCapacity);
        pthread_mutex_lock(&gParcelGlobalAllocSizeLock);
        gParcelGlobalAllocSize -= mDataCapacity;
        gParcelGlobalAllocCount--;
        pthread_mutex_unlock(&gParcelGlobalAllocSizeLock);
        free(mData);
    }
}

const uint8_t* Parcel::data() const {
    return mData;
}

size_t Parcel::dataSize() const {
    return (mDataSize > mDataPos ? mDataSize : mDataPos);
}

size_t Parcel::dataAvail() const {
    return dataSize() - dataPosition();
}

size_t Parcel::dataPosition() const {
    return mDataPos;
}

size_t Parcel::dataCapacity() const {
    return mDataCapacity;
}

status_t Parcel::setDataSize(size_t size) {
    if (size > INT32_MAX) {
        // don't accept size_t values which may have come from an
        // inadvertent conversion from a negative int.
        return BAD_VALUE;
    }

    status_t err;
    err = continueWrite(size);
    if (err == NO_ERROR) {
        mDataSize = size;
    }
    return err;
}

status_t Parcel::continueWrite(size_t desired)
{
    if (desired > INT32_MAX) {
        // don't accept size_t values which may have come from an
        // inadvertent conversion from a negative int.
        return BAD_VALUE;
    }

    if (mData) {
        if (desired > mDataCapacity) {
            uint8_t* data = (uint8_t*)realloc(mData, desired);
            if (data) {
                pthread_mutex_lock(&gParcelGlobalAllocSizeLock);
                gParcelGlobalAllocSize += desired;
                gParcelGlobalAllocSize -= mDataCapacity;
                pthread_mutex_unlock(&gParcelGlobalAllocSizeLock);
                mData = data;
                mDataCapacity = desired;
            } else if (desired > mDataCapacity) {
                mError = NO_MEMORY;
                return NO_MEMORY;
            }
        } else {
            if (mDataSize > desired) {
                mDataSize = desired;
            }
            if (mDataPos > desired) {
                mDataPos = desired;
            }
        }

    } else {
        // This is the first data.  Easy!
        uint8_t* data = (uint8_t*)malloc(desired);
        if (!data) {
            mError = NO_MEMORY;
            return NO_MEMORY;
        }

        pthread_mutex_lock(&gParcelGlobalAllocSizeLock);
        gParcelGlobalAllocSize += desired;
        gParcelGlobalAllocCount++;
        pthread_mutex_unlock(&gParcelGlobalAllocSizeLock);

        mData = data;
        mDataSize = mDataPos = 0;
        mDataCapacity = desired;
    }

    return NO_ERROR;
}

void Parcel::setDataPosition(size_t pos) const {
    if (pos > INT32_MAX) {
        // don't accept size_t values which may have come from an
        // inadvertent conversion from a negative int.
        abort();
    }

    mDataPos = pos;
}
status_t Parcel::setDataCapacity(size_t size) {
    if (size > INT32_MAX) {
        // don't accept size_t values which may have come from an
        // inadvertent conversion from a negative int.
        return BAD_VALUE;
    }

    if (size > mDataCapacity) return continueWrite(size);
    return NO_ERROR;
}

status_t Parcel::setData(const uint8_t* buffer, size_t len) {
    if (len > INT32_MAX) {
        // don't accept size_t values which may have come from an
        // inadvertent conversion from a negative int.
        return BAD_VALUE;
    }

    status_t err = restartWrite(len);
    if (err == NO_ERROR) {
        memcpy(const_cast<uint8_t*>(data()), buffer, len);
        mDataSize = len;
    }
    return err;
}

status_t Parcel::restartWrite(size_t desired)
{
    if (desired > INT32_MAX) {
        // don't accept size_t values which may have come from an
        // inadvertent conversion from a negative int.
        return BAD_VALUE;
    }

    uint8_t* data = (uint8_t*)realloc(mData, desired);
    if (!data && desired > mDataCapacity) {
        mError = NO_MEMORY;
        return NO_MEMORY;
    }

    if (data) {
        pthread_mutex_lock(&gParcelGlobalAllocSizeLock);
        gParcelGlobalAllocSize += desired;
        gParcelGlobalAllocSize -= mDataCapacity;
        pthread_mutex_unlock(&gParcelGlobalAllocSizeLock);
        mData = data;
        mDataCapacity = desired;
    }

    mDataSize = mDataPos = 0;

    return NO_ERROR;
}

status_t Parcel::appendFrom(const Parcel *parcel, size_t offset, size_t len)
{
    status_t err;
    const uint8_t *data = parcel->mData;
    int startPos = mDataPos;
    int firstIndex = -1, lastIndex = -2;

    if (len == 0) {
        return NO_ERROR;
    }

    if (len > INT32_MAX) {
        // don't accept size_t values which may have come from an
        // inadvertent conversion from a negative int.
        return BAD_VALUE;
    }

    // range checks against the source parcel size
    if ((offset > parcel->mDataSize)
        || (len > parcel->mDataSize)
        || (offset + len > parcel->mDataSize)) {
        return BAD_VALUE;
    }

    if ((mDataSize+len) > mDataCapacity) {
        // grow data
        err = growData(len);
        if (err != NO_ERROR) {
            return err;
        }
    }

    // append data
    memcpy(mData + mDataPos, data + offset, len);
    mDataPos += len;
    mDataSize += len;

    err = NO_ERROR;

    return err;
}

status_t Parcel::growData(size_t len)
{
    if (len > INT32_MAX) {
        // don't accept size_t values which may have come from an
        // inadvertent conversion from a negative int.
        return BAD_VALUE;
    }

    size_t newSize = ((mDataSize+len)*3)/2;
    return (newSize <= mDataSize)
           ? (status_t) NO_MEMORY
           : continueWrite(newSize);
}


status_t Parcel::write(const void* data, size_t len)
{
    if (len > INT32_MAX) {
        // don't accept size_t values which may have come from an
        // inadvertent conversion from a negative int.
        return BAD_VALUE;
    }

    void* const d = writeInplace(len);
    if (d) {
        memcpy(d, data, len);
        return NO_ERROR;
    }
    return mError;
}

void* Parcel::writeInplace(size_t len)
{
    if (len > INT32_MAX) {
        // don't accept size_t values which may have come from an
        // inadvertent conversion from a negative int.
        return NULL;
    }

    const size_t padded = pad_size(len);

    // sanity check for integer overflow
    if (mDataPos+padded < mDataPos) {
        return NULL;
    }

    if ((mDataPos+padded) <= mDataCapacity) {
        restart_write:
        //printf("Writing %ld bytes, padded to %ld\n", len, padded);
        uint8_t* const data = mData+mDataPos;

        // Need to pad at end?
        if (padded != len) {
#if BYTE_ORDER == BIG_ENDIAN
            static const uint32_t mask[4] = {
                    0x00000000, 0xffffff00, 0xffff0000, 0xff000000
            };
#endif
#if BYTE_ORDER == LITTLE_ENDIAN
            static const uint32_t mask[4] = {
                    0x00000000, 0x00ffffff, 0x0000ffff, 0x000000ff
            };
#endif
            //printf("Applying pad mask: %p to %p\n", (void*)mask[padded-len],
            //    *reinterpret_cast<void**>(data+padded-4));
            *reinterpret_cast<uint32_t*>(data+padded-4) &= mask[padded-len];
        }

        finishWrite(padded);
        return data;
    }

    status_t err = growData(padded);
    if (err == NO_ERROR) goto restart_write;
    return NULL;
}

status_t Parcel::finishWrite(size_t len)
{
    if (len > INT32_MAX) {
        // don't accept size_t values which may have come from an
        // inadvertent conversion from a negative int.
        return BAD_VALUE;
    }

    //printf("Finish write of %d\n", len);
    mDataPos += len;
    if (mDataPos > mDataSize) {
        mDataSize = mDataPos;
    }
    //printf("New pos=%d, size=%d\n", mDataPos, mDataSize);
    return NO_ERROR;
}

status_t Parcel::writeInt32(int32_t val) {
    return writeAligned(val);
}


template<class T>
status_t Parcel::writeAligned(T val) {
    //COMPILE_TIME_ASSERT_FUNCTION_SCOPE(PAD_SIZE_UNSAFE(sizeof(T)) == sizeof(T));

    if ((mDataPos+sizeof(val)) <= mDataCapacity) {
        restart_write:
        *reinterpret_cast<T*>(mData+mDataPos) = val;
        return finishWrite(sizeof(val));
    }

    status_t err = growData(sizeof(val));
    if (err == NO_ERROR) goto restart_write;
    return err;
}

status_t Parcel::writeInt64(int64_t val) {
    return writeAligned(val);
}


status_t Parcel::writeString16(const String16& str)
{
    return writeString16(str.c_str(), str.size());
}

status_t Parcel::writeString16(const char16_t* str, size_t len)
{
    if (str == NULL) return writeInt32(-1);

    status_t err = writeInt32(len);
    if (err == NO_ERROR) {
        len *= sizeof(char16_t);
        uint8_t* data = (uint8_t*)writeInplace(len+sizeof(char16_t));
        if (data) {
            memcpy(data, str, len);
            *reinterpret_cast<char16_t*>(data+len) = 0;
            return NO_ERROR;
        }
        err = mError;
    }
    return err;
}


status_t Parcel::read(void* outData, size_t len) const
{
    if (len > INT32_MAX) {
        // don't accept size_t values which may have come from an
        // inadvertent conversion from a negative int.
        return BAD_VALUE;
    }

    if ((mDataPos+pad_size(len)) >= mDataPos && (mDataPos+pad_size(len)) <= mDataSize
        && len <= pad_size(len)) {
        memcpy(outData, mData+mDataPos, len);
        mDataPos += pad_size(len);
        return NO_ERROR;
    }
    return NOT_ENOUGH_DATA;
}


const void* Parcel::readInplace(size_t len) const
{
    if (len > INT32_MAX) {
        // don't accept size_t values which may have come from an
        // inadvertent conversion from a negative int.
        return NULL;
    }

    if ((mDataPos+pad_size(len)) >= mDataPos && (mDataPos+pad_size(len)) <= mDataSize
        && len <= pad_size(len)) {
        const void* data = mData+mDataPos;
        mDataPos += pad_size(len);
        return data;
    }
    return NULL;
}

int32_t Parcel::readInt32() const
{
    return readAligned<int32_t>();
}

status_t Parcel::readInt32(int32_t *pArg) const
{
    return readAligned(pArg);
}

template<class T>
status_t Parcel::readAligned(T *pArg) const {
    //COMPILE_TIME_ASSERT_FUNCTION_SCOPE(PAD_SIZE_UNSAFE(sizeof(T)) == sizeof(T));

    if ((mDataPos+sizeof(T)) <= mDataSize) {
        const void* data = mData+mDataPos;
        mDataPos += sizeof(T);
        *pArg =  *reinterpret_cast<const T*>(data);
        return NO_ERROR;
    } else {
        return NOT_ENOUGH_DATA;
    }
}

template<class T>
T Parcel::readAligned() const {
    T result;
    if (readAligned(&result) != NO_ERROR) {
        result = 0;
    }

    return result;
}

String16 Parcel::readString16() const
{
    size_t len;
    const char16_t* str = readString16Inplace(&len);
    if (str) return String16(str, len);
    //Reading a NULL string not supported here
    return String16();
}

const char16_t* Parcel::readString16Inplace(size_t* outLen) const
{
    int32_t size = readInt32();
    // watch for potential int overflow from size+1
    if (size >= 0 && size < INT32_MAX) {
        *outLen = size;
        const char16_t* str = (const char16_t*)readInplace((size+1)*sizeof(char16_t));
        if (str != NULL) {
            return str;
        }
    }
    *outLen = 0;
    return nullptr;
}

const char* Parcel::readCString() const
{
    const size_t avail = mDataSize-mDataPos;
    if (avail > 0) {
        const char* str = reinterpret_cast<const char*>(mData+mDataPos);
        // is the string's trailing NUL within the parcel's valid bounds?
        const char* eos = reinterpret_cast<const char*>(memchr(str, 0, avail));
        if (eos) {
            const size_t len = eos - str;
            mDataPos += pad_size(len+1);
            return str;
        }
    }
    return NULL;
}

status_t Parcel::writeString8AsString16(const char *s8) {
    if (s8 == nullptr) {
        return writeInt32(-1);
    }

    unsigned int x = 1;
    bool isLittleEndian = ((int) (((char *)&x)[0]) == 1) ? true : false;

    std::string source(s8);
    std::u16string u16_conv;

    if (isLittleEndian) {
        u16_conv = std::wstring_convert<std::codecvt_utf8_utf16<char16_t, 0x10ffff,
                std::little_endian>, char16_t>{}.from_bytes(source);
    } else {
        u16_conv = std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>{}
                .from_bytes(source);
    }

    int len = u16_conv.length();
    char16_t* cstr = (char16_t *) malloc (sizeof(char16_t) * (len));

    if (cstr == nullptr) {
        return NO_MEMORY;
    }

    std::copy(u16_conv.c_str(), u16_conv.c_str() + len, cstr);

    writeString16(cstr, len);
    free(cstr);

    return NO_ERROR;
}

status_t Parcel::writeCString(const char* str)
{
    return write(str, strlen(str)+1);
}

status_t Parcel::writeUint64(uint64_t val) {
    return writeAligned(val);
}

status_t Parcel::readUint64(uint64_t *pArg) const {
    return readAligned(pArg);
}

uint64_t Parcel::readUint64() const {
    return readAligned<uint64_t>();
}

status_t Parcel::writeUint32(uint32_t val) {
    return writeAligned(val);
}

status_t Parcel::readUint32(uint32_t *pArg) const {
    return readAligned(pArg);
}
