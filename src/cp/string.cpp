#include <bits/stdc++.h>
#include <cstring>
using namespace std;

template <typename CharType, size_t SSOThreshold = 15> class String {
  static constexpr CharType nullChar = '\0';

  CharType *__restrict data;
  size_t capacity;
  union {
    CharType sso[SSOThreshold + 1]; // +1 for null terminator
    size_t len;
  } sso;

public:
  String() : capacity(0) { sso.sso[0] = nullChar; }

  String(const CharType *oStr) {
    size_t strLen = strlen(oStr);
    capacity = strLen;
    if (strLen <= SSOThreshold) {
      strcpy(sso.sso, oStr); // +1 for null terminator
    } else {
      capacity = sso.len = strLen;
      data = new CharType[capacity + 1];
      strcpy(data, oStr); // +1 for null terminator
    }
  }

  String(const String &&other) {
    size_t strLen = other.capacity;
    capacity = strLen;
    if (strLen <= SSOThreshold) {
      strcpy(sso.sso, other.sso.sso); // +1 for null terminator
    } else {
      capacity = sso.len = strLen;
      data = new CharType[capacity + 1];
      strcpy(data, other.data); // +1 for null terminator
    }
  }

  ~String() { delete[] data; }

  void doubleCapacity() {
    capacity *= 2;
    CharType *newData = new CharType[capacity + 1];
    strcpy(newData, data); // +1 for null terminator
    delete[] data;
    data = newData;
  }

  void moveToHeap() {
    auto len = getLen();
    capacity = SSOThreshold;
    data = new CharType[capacity + 1];
    strcpy(data, sso.sso);
    sso.len = len;
  }

  void push(const CharType &&c) {
    if (capacity < SSOThreshold) {
      sso.sso[capacity++] = c;
      sso.sso[capacity] = nullChar;
      return;
    }
    if (capacity == SSOThreshold) {
      cout << "Moving to heap with capacity " << SSOThreshold << endl;
      moveToHeap();
    }
    if (capacity == sso.len) {
      cout << "Doubling capacity from " << SSOThreshold << " to ";
      doubleCapacity();
      cout << capacity << endl;
    }
    data[sso.len++] = c;
    data[sso.len] = nullChar;
  }

  size_t getLen() const noexcept {
    return capacity <= SSOThreshold ? capacity : sso.len;
  }

  void push(const String &s) {
    size_t newLen = getLen() + s.getLen();
    if (newLen <= SSOThreshold) {
      strcpy(data + capacity, s.data);
      capacity = newLen;
      return;
    }
    if (capacity < SSOThreshold && newLen >= SSOThreshold) {
      cout << "Moving to heap with capacity " << SSOThreshold << endl;
      moveToHeap();
    }
    while (newLen > capacity) {
      cout << "Doubling capacity from " << capacity << " to ";
      doubleCapacity();
      cout << capacity << endl;
    }
    strcpy(data + sso.len, s.data);
    sso.len = newLen;
    data[sso.len] = nullChar;
  }

  friend ostream &operator<<(ostream &os, const String &&str) {
    if (str.capacity <= SSOThreshold) {
      os << str.sso.sso;
    } else {
      os << str.data;
    }
    return os;
  }

  friend ostream &operator<<(ostream &os, const String &str) {
    if (str.capacity <= SSOThreshold) {
      os << str.sso.sso;
    } else {
      os << str.data;
    }
    return os;
  }
};

int main() {
  ios_base::sync_with_stdio(false);
  cin.tie(NULL);

  cout << sizeof("1234") << endl;

  String<char> s1("Hello, World!");
  cout << s1 << endl;
  cout << sizeof(s1) << " " << sizeof(string) << endl;

  String<char> s2;
  for (int i = 0; i < 26; i++) {
    s2.push('a' + i);
    cout << s2 << endl;
  }
  String<char> s3;
  for (int i = 0; i < 26; i++) {
    const char *ch = new char('a' + i);
    s3.push(s2);
    delete ch;
    cout << s3 << endl;
  }
  return 0;
}