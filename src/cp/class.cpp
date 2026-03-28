#include <bits/stdc++.h>
using namespace std;

template<class Rec>
class Base {
public:
    int x;
    Base():x(11){}
    Base(int y):x(y){}
    void fun() {
        static_cast<Rec*>(this)->funImpl();
    }
};

template <class Rec>
class Class : public Base<Class<Rec>> {
public:
    Class():Base<Class<Rec>>(55) {this->x = 99;}
    void funImpl() {
        cout << "Base<Rec>::funImpl" << " " << this->x << endl;
    }
};

template <>
class Class<int> : public Base<Class<int>> {
    public:
    Class():Base<Class<int>>(44) {this->x = 33;}
    void funImpl() {
        cout << "Base<int>::funImpl" << " " << this->x << endl;
    }
};



int main () {
    Class<int> c;
    Class<unsigned> u;
    c.fun();
    u.fun();
    return 0;
}