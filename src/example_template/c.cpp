#include <bits/stdc++.h>
#include "h.h"
using namespace std;

template <typename type>
class B {
protected:
    void fImpl();
    void aImpl();
public:
    void f() {
        return static_cast<type*>(this)->fImpl();
    }
    B() {}
    ~B(){}
};

template <typename type>
void B<type>::fImpl() {
    cout << "bImpla" <<"\n";
}
template <typename type>
void B<type>::aImpl() {
    cout << "zxcvs" <<"\n";
}

class D : public B<D> {
public:
    void fImpl();
    D(){}
    ~D(){}
};

class C : public B<C> {
public:
    void fImpl();
    C(){}
    ~C(){}
};

template <>
void B<D>::fImpl() {
    cout << "bImplalll" <<"\n";
}

void C::fImpl() {
    B::fImpl();
    cout << "------cImpl" << endl;
}

void D::fImpl() {
    B::fImpl();
    cout << "fImpl" << endl;
}

int main() {
    D d;
    d.f();
    C c;
    c.f();
    return 0;
}