#ifndef __FWK_SMARTPTRINTERFACE_H__
#define __FWK_SMARTPTRINTERFACE_H__

template <typename T>
class SmartPtrInterface {
 public:
    void newRef() {
        ++ref_;
    }

    void deleteRef() {
        if ( --ref_ == 0 ) { onZeroReferences(); delete this; }
    }

    unsigned long refCount() const { return ref_; }

    virtual void onZeroReferences() {}
    virtual ~SmartPtrInterface() {}

 private:
    unsigned long ref_;

 protected:
    SmartPtrInterface() : ref_(0) {}
};
#endif /* FWK_PTRINTERFACE_H */
