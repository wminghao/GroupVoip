#ifndef __FWK_SMARTPTR_H__
#define __FWK_SMARTPTR_H__

namespace  mixcoder
{

template <typename T>
class SmartPtr {
 public:
 SmartPtr() : ptr_(0) {}
 SmartPtr(T *t) : ptr_(t) { if ( ptr_) ptr_->newRef(); }
 SmartPtr(const SmartPtr<T> &a) : ptr_(a.ptr_) { if ( ptr_ ) ptr_->newRef(); }

    template<typename S>
        SmartPtr(const SmartPtr<S> &a) : ptr_(a.ptr()) { if ( ptr_ ) ptr_->newRef(); }

    ~SmartPtr() { if ( ptr_ ) ptr_->deleteRef(); }

    T *ptr() const { return ptr_; }
    T *operator->() const { return ptr_; }

    operator bool() const { return ptr_ != 0; }

    const SmartPtr<T> &operator=(const SmartPtr<T> &a) { return operator=(a.ptr_); }

    template <typename S>
        const SmartPtr<T> &operator=(const SmartPtr<S> &a) { return operator=(a.ptr()); }
  
    const SmartPtr<T> &operator=(T *a) {
        if ( a ) a->newRef();
        if ( ptr_ ) ptr_->deleteRef();
        ptr_ = a;
        return *this;
    }

 private:
    T *ptr_;

    operator int() const;
 };

 template <typename T, typename S>
     bool operator==(const SmartPtr<T> &a, const SmartPtr<S> &b) {
     return a.ptr() == b.ptr();
 }

 template <typename T, typename S>
     bool operator!=(const SmartPtr<T> &a, const SmartPtr<S> &b) {
     return a.ptr() != b.ptr();
 }

template <typename T>
    bool operator==(const SmartPtr<T> &a, const void *b) {
    return a.ptr() == b;
}

template <typename T>
    bool operator!=(const SmartPtr<T> &a, const void *b) {
    return a.ptr() != b;
 }

 template <typename A, typename B>
     SmartPtr<A> ptr_cast( const SmartPtr<B> &b ) {
     return dynamic_cast<A *>( b.ptr() );
 }


} // end namespace mixcoder

#endif /* FWK_SMARTPTR_H */
