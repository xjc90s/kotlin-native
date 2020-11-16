/*
 * Copyright 2010-2017 JetBrains s.r.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package kotlinx.cinterop

import sun.misc.Unsafe

private val NativePointed.address: Long
    get() = this.rawPtr

private enum class DataModel(val pointerSize: Long) {
    _32BIT(4),
    _64BIT(8)
}

private val dataModel: DataModel = when (System.getProperty("sun.arch.data.model")) {
    null -> TODO()
    "32" -> DataModel._32BIT
    "64" -> DataModel._64BIT
    else -> throw IllegalStateException()
}

// Must be only used in interop, contains host pointer size, not target!
@PublishedApi
internal val pointerSize: Int = dataModel.pointerSize.toInt()

@PublishedApi
internal object nativeMemUtils {
    fun getByte(mem: NativePointed) = unsafe.getByte(mem.address)
    fun putByte(mem: NativePointed, value: Byte) = unsafe.putByte(mem.address, value)

    fun getShort(mem: NativePointed) = unsafe.getShort(mem.address)
    fun putShort(mem: NativePointed, value: Short) = unsafe.putShort(mem.address, value)
    
    fun getInt(mem: NativePointed) = unsafe.getInt(mem.address)
    fun putInt(mem: NativePointed, value: Int) = unsafe.putInt(mem.address, value)
    
    fun getLong(mem: NativePointed) = unsafe.getLong(mem.address)
    fun putLong(mem: NativePointed, value: Long) = unsafe.putLong(mem.address, value)

    fun getFloat(mem: NativePointed) = unsafe.getFloat(mem.address)
    fun putFloat(mem: NativePointed, value: Float) = unsafe.putFloat(mem.address, value)

    fun getDouble(mem: NativePointed) = unsafe.getDouble(mem.address)
    fun putDouble(mem: NativePointed, value: Double) = unsafe.putDouble(mem.address, value)

    fun getNativePtr(mem: NativePointed): NativePtr = when (dataModel) {
        DataModel._32BIT -> getInt(mem).toLong()
        DataModel._64BIT -> getLong(mem)
    }

    fun putNativePtr(mem: NativePointed, value: NativePtr) = when (dataModel) {
        DataModel._32BIT -> putInt(mem, value.toInt())
        DataModel._64BIT -> putLong(mem, value)
    }

    fun getByteArray(source: NativePointed, dest: ByteArray, length: Int) {
        unsafe.copyMemory(null, source.address, dest, byteArrayBaseOffset, length.toLong())
    }

    fun putByteArray(source: ByteArray, dest: NativePointed, length: Int) {
        unsafe.copyMemory(source, byteArrayBaseOffset, null, dest.address, length.toLong())
    }

    fun getCharArray(source: NativePointed, dest: CharArray, length: Int) {
        unsafe.copyMemory(null, source.address, dest, charArrayBaseOffset, length.toLong() * 2)
    }

    fun putCharArray(source: CharArray, dest: NativePointed, length: Int) {
        unsafe.copyMemory(source, charArrayBaseOffset, null, dest.address, length.toLong() * 2)
    }

    fun zeroMemory(dest: NativePointed, length: Int): Unit =
            unsafe.setMemory(dest.address, length.toLong(), 0)

    fun copyMemory(dest: NativePointed, length: Int, src: NativePointed) =
            unsafe.copyMemory(src.address, dest.address, length.toLong())


    @Suppress("NON_PUBLIC_CALL_FROM_PUBLIC_INLINE")
    inline fun <reified T> allocateInstance(): T {
        return unsafe.allocateInstance(T::class.java) as T
    }

    private fun alignUp(x: Long, align: Int) = (x + align - 1) and (align - 1).toLong().inv()
    private fun alignUp(x: Int, align: Int) = (x + align - 1) and (align - 1).inv()

    // 256 buckets for sizes <= 2048 padded to 8
    // 256 buckets for sizes <= 64KB padded to 256
    // 256 buckets for sizes <= 1MB padded to 4096
    private const val ChunkBucketSize = 256
    // Alignments are such that overhead is approx 10%.
    private const val SmallChunksSizeAlignment = 8
    private const val MediumChunksSizeAlignment = 256
    private const val BigChunksSizeAlignment = 4096
    private const val MaxSmallSize = ChunkBucketSize * SmallChunksSizeAlignment
    private const val MaxMediumSize = ChunkBucketSize * MediumChunksSizeAlignment
    private const val MaxBigSize = ChunkBucketSize * BigChunksSizeAlignment
    private const val ChunkHeaderSize = 2 * Int.SIZE_BYTES // chunk size + alignment hop size.

    private val smallChunks = LongArray(ChunkBucketSize)
    private val mediumChunks = LongArray(ChunkBucketSize)
    private val bigChunks = LongArray(ChunkBucketSize)

    // Chunk layout: [chunk size,...padding...,diff to start,aligned data start,.....data.....]
    fun alloc(size: Long, align: Int): NativePointed {
        val totalChunkSize = ChunkHeaderSize + size + align
        val ptr = ChunkHeaderSize + when {
            totalChunkSize <= MaxSmallSize -> allocFromFreeList(totalChunkSize.toInt(), SmallChunksSizeAlignment, smallChunks)
            totalChunkSize <= MaxMediumSize -> allocFromFreeList(totalChunkSize.toInt(), MediumChunksSizeAlignment, mediumChunks)
            totalChunkSize <= MaxBigSize -> allocFromFreeList(totalChunkSize.toInt(), BigChunksSizeAlignment, bigChunks)
            else -> unsafe.allocateMemory(totalChunkSize).also {
                // The actual size is not used. Just put value bigger than the biggest threshold.
                unsafe.putInt(it, Int.MAX_VALUE)
            }
        }
        val alignedPtr = alignUp(ptr, align)
        unsafe.putInt(alignedPtr - Int.SIZE_BYTES, (alignedPtr - ptr).toInt())
        return interpretOpaquePointed(alignedPtr)
    }

    private fun allocFromFreeList(size: Int, align: Int, freeList: LongArray): NativePtr {
        val paddedSize = alignUp(size, align)
        val index = paddedSize / align - 1
        val chunk = freeList[index]
        val ptr = if (chunk == 0L)
            allocRaw(paddedSize)
        else {
            val nextChunk = unsafe.getLong(chunk)
            freeList[index] = nextChunk
            chunk
        }
        unsafe.putInt(ptr, paddedSize)
        return ptr
    }

    private fun freeToFreeList(paddedSize: Int, align: Int, freeList: LongArray, chunk: NativePtr) {
        require(paddedSize > 0 && paddedSize % align == 0)
        val index = paddedSize / align - 1
        unsafe.putLong(chunk, freeList[index])
        freeList[index] = chunk
    }

    private const val RawChunkSize: Long = 4 * 1024 * 1024
    private val rawChunks = mutableListOf<NativePtr>()
    private var rawOffset = 0

    private fun allocRaw(size: Int): NativePtr {
        if (rawChunks.isEmpty() || rawOffset + size > RawChunkSize) {
            val newRawChunk = unsafe.allocateMemory(RawChunkSize)
            rawChunks.add(newRawChunk)
            rawOffset = size
            return newRawChunk
        }
        return (rawChunks.last() + rawOffset).also { rawOffset += size }
    }

    fun free(mem: NativePtr) {
        val chunkStart = mem - ChunkHeaderSize - unsafe.getInt(mem - Int.SIZE_BYTES)
        val chunkSize = unsafe.getInt(chunkStart)
        when {
            chunkSize <= MaxSmallSize -> freeToFreeList(chunkSize, SmallChunksSizeAlignment, smallChunks, chunkStart)
            chunkSize <= MaxMediumSize -> freeToFreeList(chunkSize, MediumChunksSizeAlignment, mediumChunks, chunkStart)
            chunkSize <= MaxBigSize -> freeToFreeList(chunkSize, BigChunksSizeAlignment, bigChunks, chunkStart)
            else -> unsafe.freeMemory(chunkStart)
        }
    }

    private val unsafe = with(Unsafe::class.java.getDeclaredField("theUnsafe")) {
        isAccessible = true
        return@with this.get(null) as Unsafe
    }

    private val byteArrayBaseOffset = unsafe.arrayBaseOffset(ByteArray::class.java).toLong()
    private val charArrayBaseOffset = unsafe.arrayBaseOffset(CharArray::class.java).toLong()
}
