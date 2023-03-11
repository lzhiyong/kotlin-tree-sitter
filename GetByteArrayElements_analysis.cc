// the GetByteArrayElements source code analysis

// art/runtime/jni/jni_internal.cc
static jbyte* GetByteArrayElements(JNIEnv* env, jbyteArray array, jboolean* is_copy) {
    return GetPrimitiveArray<jbyteArray, jbyte, mirror::ByteArray>(env, array, is_copy);
}

        |
        |
        v

// art/runtime/jni/jni_internal.cc
template <typename ArrayT, typename ElementT, typename ArtArrayT>
  static ElementT* GetPrimitiveArray(JNIEnv* env, ArrayT java_array, jboolean* is_copy) {
    CHECK_NON_NULL_ARGUMENT(java_array);
    ScopedObjectAccess soa(env);
    ObjPtr<ArtArrayT> array = DecodeAndCheckArrayType<ArrayT, ElementT, ArtArrayT>(
        soa, java_array, "GetArrayElements", "get");
    if (UNLIKELY(array == nullptr)) {
      return nullptr;
    }
    // Only make a copy if necessary.
    if (Runtime::Current()->GetHeap()->IsMovableObject(array)) {
      if (is_copy != nullptr) {
        *is_copy = JNI_TRUE;
      }
      const size_t component_size = sizeof(ElementT);
      size_t size = array->GetLength() * component_size;
      void* data = new uint64_t[RoundUp(size, 8) / 8];
      memcpy(data, array->GetData(), size);
      return reinterpret_cast<ElementT*>(data);
    } else {
      if (is_copy != nullptr) {
        *is_copy = JNI_FALSE;
      }
      return reinterpret_cast<ElementT*>(array->GetData());
    }
  }
  
        |
        |
        v

// art/runtime/gc/heap.cc
bool Heap::IsMovableObject(ObjPtr<mirror::Object> obj) const {
  if (kMovingCollector) {
    space::Space* space = FindContinuousSpaceFromObject(obj.Ptr(), true);
    if (space != nullptr) {
      // TODO: Check large object?
      return space->CanMoveObjects();
    }
  }
  return false;
}

        |
        |
        v

// art/runtime/gc/heap.c
space::ContinuousSpace* Heap::FindContinuousSpaceFromObject(ObjPtr<mirror::Object> obj,
                                                            bool fail_ok) const {
  space::ContinuousSpace* space = FindContinuousSpaceFromAddress(obj.Ptr());
  if (space != nullptr) {
    return space;
  }
  if (!fail_ok) {
    LOG(FATAL) << "object " << obj << " not inside any spaces!";
  }
  return nullptr;
}

        |
        |
        v

// art/runtime/gc/heap-inl.h
inline bool Heap::ShouldAllocLargeObject(ObjPtr<mirror::Class> c, size_t byte_count) const {
  // We need to have a zygote space or else our newly allocated large object can end up in the
  // Zygote resulting in it being prematurely freed.
  // We can only do this for primitive objects since large objects will not be within the card table
  // range. This also means that we rely on SetClass not dirtying the object's card.
  return byte_count >= large_object_threshold_ && (c->IsPrimitiveArray() | c->IsStringClass());
}

// large_object_threshold_ == 4096 * 3 == 12kb
large_object_threshold_ == large_object_threshold == LargeObjectThreshold == kDefaultLargeObjectThreshold 
    ==> kMinLargeObjectThreshold == 12KB

        |
        |
        v

// art/libartbase/base/globals.h
// System page size. We check this against sysconf(_SC_PAGE_SIZE) at runtime, but use a simple
// compile-time constant so the compiler can generate better code.
static constexpr size_t kPageSize = 4096;


// art/runtime/gc/heap.h
// Primitive arrays larger than this size are put in the large object space.
static constexpr size_t kMinLargeObjectThreshold = 3 * kPageSize;
static constexpr size_t kDefaultLargeObjectThreshold = kMinLargeObjectThreshold;


// function call strace(not complete)
GetByteArrayElements --> GetPrimitiveArray --> IsMovableObject(virtual function)
|
| --> FindContinuousSpaceFromObject(Large Object space, Zygote space, Image space) 
|
| ... --> Create heap ... --> ShouldAllocLargeObject

