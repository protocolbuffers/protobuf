// Protocol Buffers - Google's data interchange format
// Copyright 2014 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

package com.google.protobuf.nano;


/**
 * A custom version of {@link android.util.SparseArray} with the minimal API
 * for storing {@link FieldData} objects.
 *
 * <p>This class is an internal implementation detail of nano and should not
 * be called directly by clients.
 *
 * Based on {@link android.support.v4.util.SpareArrayCompat}.
 */
public final class FieldArray implements Cloneable {
    private static final FieldData DELETED = new FieldData();
    private boolean mGarbage = false;

    private int[] mFieldNumbers;
    private FieldData[] mData;
    private int mSize;

    /**
     * Creates a new FieldArray containing no fields.
     */
    FieldArray() {
        this(10);
    }

    /**
     * Creates a new FieldArray containing no mappings that will not
     * require any additional memory allocation to store the specified
     * number of mappings.
     */
    FieldArray(int initialCapacity) {
        initialCapacity = idealIntArraySize(initialCapacity);
        mFieldNumbers = new int[initialCapacity];
        mData = new FieldData[initialCapacity];
        mSize = 0;
    }

    /**
     * Gets the FieldData mapped from the specified fieldNumber, or <code>null</code>
     * if no such mapping has been made.
     */
    FieldData get(int fieldNumber) {
        int i = binarySearch(fieldNumber);

        if (i < 0 || mData[i] == DELETED) {
            return null;
        } else {
            return mData[i];
        }
    }

    /**
     * Removes the data from the specified fieldNumber, if there was any.
     */
    void remove(int fieldNumber) {
        int i = binarySearch(fieldNumber);

        if (i >= 0 && mData[i] != DELETED) {
            mData[i] = DELETED;
            mGarbage = true;
        }
    }

    private void gc() {
        int n = mSize;
        int o = 0;
        int[] keys = mFieldNumbers;
        FieldData[] values = mData;

        for (int i = 0; i < n; i++) {
            FieldData val = values[i];

            if (val != DELETED) {
                if (i != o) {
                    keys[o] = keys[i];
                    values[o] = val;
                    values[i] = null;
                }

                o++;
            }
        }

        mGarbage = false;
        mSize = o;
    }

    /**
     * Adds a mapping from the specified fieldNumber to the specified data,
     * replacing the previous mapping if there was one.
     */
    void put(int fieldNumber, FieldData data) {
        int i = binarySearch(fieldNumber);

        if (i >= 0) {
            mData[i] = data;
        } else {
            i = ~i;

            if (i < mSize && mData[i] == DELETED) {
                mFieldNumbers[i] = fieldNumber;
                mData[i] = data;
                return;
            }

            if (mGarbage && mSize >= mFieldNumbers.length) {
                gc();

                // Search again because indices may have changed.
                i = ~ binarySearch(fieldNumber);
            }

            if (mSize >= mFieldNumbers.length) {
                int n = idealIntArraySize(mSize + 1);

                int[] nkeys = new int[n];
                FieldData[] nvalues = new FieldData[n];

                System.arraycopy(mFieldNumbers, 0, nkeys, 0, mFieldNumbers.length);
                System.arraycopy(mData, 0, nvalues, 0, mData.length);

                mFieldNumbers = nkeys;
                mData = nvalues;
            }

            if (mSize - i != 0) {
                System.arraycopy(mFieldNumbers, i, mFieldNumbers, i + 1, mSize - i);
                System.arraycopy(mData, i, mData, i + 1, mSize - i);
            }

            mFieldNumbers[i] = fieldNumber;
            mData[i] = data;
            mSize++;
        }
    }

    /**
     * Returns the number of key-value mappings that this FieldArray
     * currently stores.
     */
    int size() {
        if (mGarbage) {
            gc();
        }

        return mSize;
    }

    public boolean isEmpty() {
        return size() == 0;
    }

    /**
     * Given an index in the range <code>0...size()-1</code>, returns
     * the value from the <code>index</code>th key-value mapping that this
     * FieldArray stores.
     */
    FieldData dataAt(int index) {
        if (mGarbage) {
            gc();
        }

        return mData[index];
    }

    @Override
    public boolean equals(Object o) {
        if (o == this) {
            return true;
        }
        if (!(o instanceof FieldArray)) {
            return false;
        }

        FieldArray other = (FieldArray) o;
        if (size() != other.size()) {  // size() will call gc() if necessary.
            return false;
        }
        return arrayEquals(mFieldNumbers, other.mFieldNumbers, mSize) &&
                arrayEquals(mData, other.mData, mSize);
    }

    @Override
    public int hashCode() {
        if (mGarbage) {
            gc();
        }
        int result = 17;
        for (int i = 0; i < mSize; i++) {
            result = 31 * result + mFieldNumbers[i];
            result = 31 * result + mData[i].hashCode();
        }
        return result;
    }

    private int idealIntArraySize(int need) {
        return idealByteArraySize(need * 4) / 4;
    }

    private int idealByteArraySize(int need) {
        for (int i = 4; i < 32; i++)
            if (need <= (1 << i) - 12)
                return (1 << i) - 12;

        return need;
    }

    private int binarySearch(int value) {
        int lo = 0;
        int hi = mSize - 1;

        while (lo <= hi) {
            int mid = (lo + hi) >>> 1;
            int midVal = mFieldNumbers[mid];

            if (midVal < value) {
                lo = mid + 1;
            } else if (midVal > value) {
                hi = mid - 1;
            } else {
                return mid; // value found
            }
        }
        return ~lo; // value not present
    }

    private boolean arrayEquals(int[] a, int[] b, int size) {
        for (int i = 0; i < size; i++) {
            if (a[i] != b[i]) {
                return false;
            }
        }
        return true;
    }

    private boolean arrayEquals(FieldData[] a, FieldData[] b, int size) {
        for (int i = 0; i < size; i++) {
            if (!a[i].equals(b[i])) {
                return false;
            }
        }
        return true;
    }

    @Override
    public final FieldArray clone() {
        // Trigger GC so we compact and don't copy DELETED elements.
        int size = size();
        FieldArray clone = new FieldArray(size);
        System.arraycopy(mFieldNumbers, 0, clone.mFieldNumbers, 0, size);
        for (int i = 0; i < size; i++) {
            if (mData[i] != null) {
                clone.mData[i] = mData[i].clone();
            }
        }
        clone.mSize = size;
        return clone;
    }
}
