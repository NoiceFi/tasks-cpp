#include <iostream>
#include <cstring>
class String {
private:
  char* buffer;
  size_t sz;
  size_t cap;

public:

  String(const char* str) {
    int len = strlen(str);
    buffer = new char[len + 1];
    memcpy(buffer, str, len + 1);
    sz = len;
    cap = len;
  }

  String(size_t n, char c)
    : buffer(new char[n + 1])
    , sz(n)
    , cap(n)
  {
    memset(buffer, c, n);
    buffer[n] = '\0';
  }

  String() : buffer(new char[1]), sz(0), cap(0) {
    buffer[0] = '\0';
  }

  String(const String& x)
    : buffer(new char[x.cap + 1])
    , sz(x.sz)
    , cap(x.cap)
  {
    memcpy(buffer, x.buffer, sz + 1);
  }

  void swap(String& x) {
    std::swap(buffer, x.buffer);
    std::swap(sz, x.sz);
    std::swap(cap, x.cap);
  }

  void extend(size_t len) {
    char* nw = nullptr;
    if (len <= 2 * cap) {
      nw = new char[2 * cap + 1];
      cap = 2 * cap;
    }
    else {
      nw = new char[len + 1];
      cap = len;
    }
    memcpy(nw, buffer, sz + 1);
    delete[] buffer;
    buffer = nw;
  }

  size_t length() const {
    return sz;
  }

  size_t size() const {
    return sz;
  }

  size_t capacity() const {
    return cap;
  }

  char& operator[](size_t n) {
    return buffer[n];
  }

  const char& operator[](size_t n) const {
    return buffer[n];
  }

  String& operator=(const String& x) {
    String copy = x;
    swap(copy);
    return *this;
  }

  String& operator+=(const String& x) {
    if (cap - sz < x.sz) {
      extend(sz + x.sz);
    }
    memcpy(buffer + sz, x.buffer, x.sz + 1);
    sz += x.sz;
    return *this;
  }

  String& operator+=(char c) {
    if (cap == sz) {
      extend(sz + 1);
    }
    buffer[sz] = c;
    buffer[sz + 1] = '\0';
    sz++;
    return *this;
  }

  void push_back(char c) {
    *this += c;
  }

  void pop_back() {
    sz--;
    buffer[sz] = '\0';
  }

  char& front() {
    return buffer[0];
  }

  const char& front() const {
    return buffer[0];
  }

  char& back() {
    return buffer[sz - 1];
  }

  const char& back() const {
    return buffer[sz - 1];
  }

  size_t find(const String& x) const {
    if (sz < x.sz) return length();
    size_t j = 0;
    size_t i = 0;
    for (size_t l = 0; l < sz; ++l) {
      i = l;
      j = 0;
      while (i < sz && j < x.sz && buffer[i] == x[j]) {
        j++;
        i++;
      }
      if (j == x.sz) return i - x.sz;
    }
    return length();
  }

  size_t rfind(const String& x) const {
    if (sz < x.sz) return length();
    size_t ans = length();
    for (size_t i = 0; i < sz - x.sz + 1; ++i) {
      size_t id = 0;
      while (id < x.sz && buffer[id + i] == x[id]) {
        id++;
      }
      if (id == x.sz) ans = i;
    }
    return ans;
  }

  String substr(size_t start, size_t count) const {
    String copy(count, '\0');
    memcpy(copy.buffer, buffer + start, count);
    copy.buffer[count] = '\0';
    copy.sz = copy.cap = count;
    return copy;
  }

  bool empty() const {
    return sz == 0;
  }

  void clear() {
    sz = 0;
    buffer[0] = '\0';
    return;
  }

  void shrink_to_fit() {
    char* nw = new char[sz + 1];
    memcpy(nw, buffer, sz + 1);
    std::swap(nw, buffer);
    cap = sz;
    delete[] nw;
  }

  ~String() {
    delete[] buffer;
  }

  char* data() {
    return buffer;
  }

  const char* data() const {
    return buffer;
  }
};

String operator+(const String& left, const String& right) {
  String copy = left;
  copy += right;
  return copy;
}

String operator+(const String& left, char c) {
  String copy = left;
  copy += c;
  return copy;
}

String operator+(char c, const String& right) {
  String copy(1, c);
  copy += right;
  return copy;
}

bool operator<(const String& a, const String& b) {
  for (size_t i = 0; i < std::min(a.size(), b.size()); ++i) {
    if (a[i] < b[i]) return true;
    if (a[i] > b[i]) return false;
  }
  return a.size() < b.size();
}

bool operator==(const String& a, const String& b) {
  if (a.size() != b.size()) return false;
  for (size_t i = 0; i < a.size(); ++i) {
    if (a[i] != b[i]) return false;
  }
  return true;
}

bool operator>(const String& a, const String& b) {
  return b < a;
}

bool operator<=(const String& a, const String& b) {
  return !(a > b);
}

bool operator>=(const String& a, const String& b) {
  return !(a < b);
}

bool operator!=(const String& a, const String& b) {
  return !(a == b);
}

std::ostream& operator<<(std::ostream& out, const String& a) {
  return out << a.data();
}

std::istream& operator>>(std::istream& in, String& a) {
  a.clear();
  char c;
  while (1) {
    c = in.get();
    if (!isspace(c)) break;
  }
  if (!isspace(c)) a += c;
  while (1) {
    c = in.get();
    if (isspace(c) || c == EOF) break;
    a += c;
  }
  return in;
}
