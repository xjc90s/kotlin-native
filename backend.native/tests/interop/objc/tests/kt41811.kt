import kotlin.native.ref.WeakReference
import kotlinx.cinterop.*
import kotlin.test.*
import objcTests.*

// Note: these tests rely on GC assertions: without the fix and the assertions it won't actually crash.
// GC should fire an assertion if it obtains a reference to Kotlin object that is being (or has been) deallocated.

@Test
fun testKT41811() {
    // Attempt to make the state predictable:
    kotlin.native.internal.GC.collect()

    deallocRetainReleaseDeallocated = false
    assertFalse(deallocRetainReleaseDeallocated)

    createGarbageDeallocRetainRelease()

    // Runs [DeallocRetainRelease dealloc]:
    kotlin.native.internal.GC.collect()

    assertTrue(deallocRetainReleaseDeallocated)

    // Might crash due to double-dispose if the dealloc applied addRef/releaseRef to reclaimed Kotlin object:
    kotlin.native.internal.GC.collect()
}

private fun createGarbageDeallocRetainRelease() {
    autoreleasepool {
        object : DeallocRetainRelease() {}
    }
}

@Test
fun testKT41811LoadWeak() {
    testKT41811LoadWeak(ObjCWeakReference())
}

@Test
fun testKT41811LoadKotlinWeak() {
    val kotlinWeak = object : NSObject(), WeakReferenceProtocol {
        lateinit var weakReference: WeakReference<Any>

        override fun referent(): Any? {
            return weakReference.value
        }

        override fun setReferent(referent: Any?) {
            weakReference = WeakReference(referent!!)
        }
    }

    testKT41811LoadWeak(kotlinWeak)
}

private fun testKT41811LoadWeak(weakRef: WeakReferenceProtocol) {
    // Attempt to make the state predictable:
    kotlin.native.internal.GC.collect()

    deallocLoadWeakDeallocated = false
    assertFalse(deallocLoadWeakDeallocated)

    createGarbageDeallocLoadWeak(weakRef)

    // Runs [DeallocLoadWeak dealloc]:
    kotlin.native.internal.GC.collect()

    assertTrue(deallocLoadWeakDeallocated)

    // Might crash due to double-dispose if the dealloc applied addRef/releaseRef to reclaimed Kotlin object:
    kotlin.native.internal.GC.collect()

    weakDeallocLoadWeak = null
}

private fun createGarbageDeallocLoadWeak(weakRef: WeakReferenceProtocol) {
    autoreleasepool {
        val obj = object : DeallocLoadWeak() {}
        weakDeallocLoadWeak = weakRef.apply { referent = obj }
        obj.checkWeak()
        assertSame(obj, weakDeallocLoadWeak!!.referent)
    }
}
