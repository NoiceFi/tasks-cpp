#include <algorithm>
#include <iostream>
#include <vector>
#include <string>
#include <random>
class BigInteger;

bool operator==(const BigInteger& left, const BigInteger& right);
bool operator!=(const BigInteger& left, const BigInteger& right);
bool operator<(const BigInteger& left, const BigInteger& right);
bool operator>(const BigInteger& left, const BigInteger& right);
bool operator>=(const BigInteger& left, const BigInteger& right);
bool operator<=(const BigInteger& left, const BigInteger& right);
BigInteger operator*(const BigInteger& left, const BigInteger& right);
std::ostream& operator<<(std::ostream& out, const BigInteger& other);

class BigInteger {
  std::vector <int> val;
  bool is_negative;
public:
  static constexpr const int  st_10[] = { 1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000 };
  static const int cMax = 1e9;
  static const int size_of_block = 9;
  static const int size_of_del = 8;

  static int get_last_n(int value, int cnt) { // 0 <= value <= 1e9
    return (value % st_10[cnt]);
  }

  static int get_size(int x) {
    int i = 1;
    while (x >= st_10[i]) {
      i++;
    }
    return i;
  }

  static int get_first_n(int value, int cnt, int type) {
    int size = get_size(value);
    if (type != 0 && size < size_of_block) {
      cnt = std::max(0, cnt - (size_of_block - size));
    }
    if (cnt == 0) return 0;
    int res = 0;
    int j = st_10[size - 1];
    for (int i = 0; i < std::min(cnt, size); ++i) {
      res *= st_10[1];
      res += value / j;
      value -= j * (value / j);
      j = j / st_10[1];
    }
    return res;
  }

  static BigInteger gcd(BigInteger& left, BigInteger& right) {
    if (left.sign()) {
      left *= (-1);
    }
    if (right.sign()) {
      right *= (-1);
    }
    if (left > right) {
      std::swap(left, right);
    }
    if (left == 0) {
      return right;
    }
    right %= left;
    return gcd(left, right);
  }

  BigInteger(long long value) {
    val.clear();
    is_negative = (value < 0);
    value = abs(value);
    val.push_back(value % cMax);
    if (value / cMax > 0) {
      if (value / cMax < cMax) {
        val.push_back(value / cMax);
      }
      else {
        val.push_back((value / cMax) % cMax);
        val.push_back((value / cMax) / cMax);
      }
    }
  }

  BigInteger() {
    val.push_back(0);
    is_negative = 0;
  }

  const bool& sign() const {
    return is_negative;
  }

  size_t size() const {
    return val.size();
  }

  int get_block(int i) const {
    return val[i];
  }

  void sign_update(bool flag) {
    is_negative = flag;
  }

  BigInteger(const std::string& str) {
    BigInteger res;
    res.val.clear();
    std::string copy = str;
    res.is_negative = 0;
    if (copy[0] == '-') {
      res.is_negative = true;
      copy = copy.substr(1, str.size() - 1);
    }
    reverse(copy.begin(), copy.end());
    int j = 1;
    int new_block = 0;
    for (size_t i = 0; i + size_of_block - 1 < copy.size(); i += size_of_block) {
      j = 1;
      new_block = 0;
      for (int l = 0; l < size_of_block; ++l) {
        new_block += j * (copy[i + l] - '0');
        j *= st_10[1];
      }
      res.val.push_back(new_block);
    }
    if (copy.size() % size_of_block != 0) {
      int rem = copy.size() % size_of_block;
      j = 1;
      new_block = 0;
      for (int i = rem; i > 0; --i) {
        new_block += j * (copy[copy.size() - i] - '0');
        j = j * st_10[1];
      }
      res.val.push_back(new_block);
    }
    res.shrink_zero();
    if (res.val.size() == 1 && res.val[0] == 0) {
      res.is_negative = false;
    }
    (*this) = res;
  }

  BigInteger(const BigInteger& other) {
    val = other.val;
    is_negative = other.is_negative;
  }

  BigInteger& operator= (const BigInteger& other) {
    if (&other == this) return *this;
    val = other.val;
    is_negative = other.is_negative;
    return (*this);
  }

  void addition(const BigInteger& other) {
    int rem = 0;
    size_t i = 0;
    while (rem != 0 || i < other.size()) {
      if (i == val.size()) {
        val.push_back(0);
      }
      int temp = (i < other.size() ? other.val[i] : 0);
      int new_rem = (temp + val[i] + rem) / cMax;
      val[i] += temp + rem - new_rem * cMax;
      rem = new_rem;
      i++;
    }
  }

  void shrink_zero() {
    if (size() == 0) return;
    for (int i = size() - 1; i > 0; i--) {
      if (val[i] != 0) break;
      val.pop_back();
    }
  }

  void subtraction(const BigInteger& other) {
    int rem = 0;
    bool bigger = (other <= (*this));
    bigger ^= is_negative;
    is_negative ^= !bigger;
    val.resize(std::max(size(), other.size()));
    int coef = (bigger ? -1 : 1);
    for (size_t i = 0; i < size(); ++i) {
      int temp = (i < other.size() ? other.val[i] : 0);
      bool flag = (coef * (temp - val[i]) + rem < 0);
      int signed_temp = coef * temp;
      int signed_val = -coef * val[i];
      val[i] = (flag * cMax + signed_temp + rem + signed_val);
      rem = -flag;
    }
    shrink_zero();
  }

  std::pair<BigInteger, int> get_first_k(int res_size) {
    BigInteger res;
    res.val.clear();
    int sz = res_size;
    int n = val.size();
    auto size_of_this = get_size(val[n - 1]) + (n - 1) * size_of_block;
    if (res_size > size_of_this) {
      res = (*this);
      sz = size_of_this;
    }
    else {
      int count = 0;
      if (res_size > get_size(val[n - 1])) {
        count += get_size(val[n - 1]);
        res.val.push_back(val[n - 1]);
      }
      else {
        count = res_size;
        res.val.push_back(get_first_n(val[n - 1], res_size, 0));
      }
      int j = n - 2;
      int ost = -1;
      while (count < res_size) {
        if (res_size - count > size_of_block) {
          res.val.push_back(val[j]);
          count += size_of_block;
        }
        else {
          res.val.push_back(get_first_n(val[j], res_size - count, 1));
          ost = size_of_block - (res_size - count);
          count = res_size;
        }
        j--;
      }
      if (ost > 0) {
        for (int i = res.val.size() - 1; i > 0; i--) {
          if (i == 1) {
            if (get_size(res.val[0]) < ost) {
              res.val[i] += (res.val[0] * st_10[size_of_block - ost]);
              res.val[0] = 0;
            }
            else {
              res.val[i] += (get_last_n(res.val[0], ost) * st_10[size_of_block - ost]);
              res.val[0] /= st_10[ost];
            }
          }
          else {
            res.val[i] += (get_last_n(res.val[i - 1], ost) * st_10[size_of_block - ost]);
            res.val[i - 1] /= st_10[ost];
          }
        }
      }
      if (res.val[0] == 0) {
        for (size_t i = 1; i < res.val.size(); i++) {
          res.val[i - 1] = res.val[i];
        }
        res.val.pop_back();
      }
      reverse(res.val.begin(), res.val.end());
    }
    return { res, sz };
  }

  void multiplication(int other) {
    int rem = 0;
    for (size_t i = 0; i < val.size(); ++i) {
      long long sum = 1ll * val[i] * other + rem;
      val[i] = (sum % cMax);
      rem = sum / cMax;
    }
    if (rem != 0) {
      val.push_back(rem);
    }
  }

  void shift(int count) {
    int size_of_rem = count % size_of_block;
    int rem = 0;
    int j = 1;
    for (int i = 0; i < size_of_rem; i++) j = j * st_10[1];
    for (size_t i = 0; i < size(); i++) {
      if (i + 1 < size() || 1ll * val[i] * j >= cMax) {
        int res = val[i] % (cMax / j);
        int new_rem = val[i] / (cMax / j);
        val[i] = rem + res * j;
        rem = new_rem;
        if (i + 1 == size() && rem != 0) {
          j = 1;
          while (rem >= j) j = j * st_10[1];
        }
      }
      else {
        val[i] = val[i] * j + rem;
        rem = 0;
      }
    }

    if (rem > 0) {
      val.push_back(rem);
    }
    if (count >= size_of_block) {
      val.resize(size() + count / size_of_block);
      for (int i = size() - 1; i >= count / size_of_block; i--) {
        val[i] = val[i - count / size_of_block];
      }
      for (int i = 0; i < count / size_of_block; i++) val[i] = 0;
    }
  }

  static int bin_search(BigInteger& divisible, BigInteger& divider) {
    int left = 0;
    int right = cMax;
    while (right > left + 1) {
      int mid = (right + left) / 2;
      if (divisible < divider * mid) {
        right = mid;
      }
      else {
        left = mid;
      }
    }
    return left;
  }

  std::pair <BigInteger, BigInteger> division(const BigInteger& other) {
    BigInteger divider = other;
    divider.is_negative = 0;
    int n = get_size(divider.val[divider.val.size() - 1]) + (divider.val.size() - 1) * size_of_block;
    BigInteger res = 0;
    BigInteger copy = (*this);
    copy.is_negative = 0;
    while (copy >= divider) {
      auto temp_block = copy.get_first_k(n + size_of_del);
      int temp_size_copy = get_size(copy.val[copy.val.size() - 1]) + (copy.val.size() - 1) * size_of_block;
      int priv = bin_search(temp_block.first, divider);
      BigInteger big_priv(priv);
      big_priv.shift(temp_size_copy - temp_block.second);
      res += big_priv;
      BigInteger new_block = divider * priv;
      new_block.shift(temp_size_copy - temp_block.second);
      copy -= new_block;
    }
    if (res != 0) res.is_negative = (*this).is_negative ^ other.is_negative;
    if (copy != 0) copy.is_negative = (*this).is_negative;
    return { res, copy };
  }

  BigInteger operator-() {
    BigInteger copy = (*this);
    if ((*this) != 0) copy.is_negative ^= 1;
    return copy;
  }

  BigInteger& operator+= (const BigInteger& other) {
    if (is_negative == other.is_negative) {
      addition(other);
    }
    else {
      is_negative ^= 1;
      subtraction(other);
      is_negative ^= 1;
    }
    if (this->val.size() == 1 && val[0] == 0) this->is_negative = false;
    return *this;
  }

  BigInteger& operator-= (const BigInteger& other) {
    if (this == &other) {
      *this = 0;
    }
    else {
      is_negative ^= 1;
      *this += other;
      if (*this != 0) is_negative ^= 1;
    }
    return (*this);
  }

  BigInteger& operator*=(const BigInteger& other) {
    if ((*this) == 0 || other == 0) {
      (*this) = 0;
      return (*this);
    }
    if (this == &other) {
      BigInteger copy = other;
      copy *= other;
      (*this) = copy;
    }
    else {
      BigInteger res;
      int size_of_shift = 0;
      for (size_t i = 0; i < other.size(); ++i) {
        BigInteger mult_by_block = (*this);
        mult_by_block.multiplication(other.val[i]);
        mult_by_block.shift(size_of_shift);
        res += mult_by_block;
        size_of_shift += size_of_block;
      }
      (*this) = res;
      is_negative = is_negative ^ other.is_negative;
    }
    if (this->val.size() == 1 && val[0] == 0) this->is_negative = false;
    return *this;
  }

  BigInteger& operator/=(const BigInteger& other) {
    if (this == &other) {
      (*this) = 1;
    }
    else if ((*this) == 0) {
      (*this) = 0;
    }
    else {
      (*this) = (*this).division(other).first;

    }
    return (*this);
  }

  BigInteger& operator%=(const BigInteger& other) {
    if (this == &other) {
      (*this) = 0;
    }
    else {
      (*this) = (*this).division(other).second;
    }
    return (*this);
  }

  BigInteger operator++(int) {
    BigInteger copy = (*this);
    (*this) += 1;
    return copy;
  }

  BigInteger operator--(int) {
    BigInteger copy = (*this);
    (*this) -= 1;
    return copy;
  }

  BigInteger& operator++() {
    (*this) += 1;
    return (*this);
  }

  BigInteger& operator--() {
    (*this) -= 1;
    return (*this);
  }

  explicit operator bool() const {
    return (*this) != 0;
  }

  std::string toString() const {
    std::string res;
    if (size() == 0 || val[size() - 1] == 0) {
      res.push_back('0');
      return res;
    }
    if (is_negative) {
      res.push_back('-');
    }
    if (size() >= 1) res += std::to_string(val[size() - 1]);
    for (int i = size() - 2; i >= 0; --i) {
      int j = cMax / st_10[1];
      int copy_val_i = val[i];
      for (int l = 0; l < size_of_block; ++l) {
        res += ('0' + copy_val_i / j);
        copy_val_i %= j;
        j /= st_10[1];
      }
    }
    return res;
  }
};

bool operator<(const BigInteger& left, const BigInteger& right) {
  if (left.sign() != right.sign()) {
    return left.sign() > right.sign();
  }
  if (left.size() != right.size()) return left.sign() ? (left.size() > right.size()) : (left.size() < right.size());
  for (int i = left.size() - 1; i >= 0; --i) {
    if (left.get_block(i) != right.get_block(i)) return left.sign() ? (left.get_block(i) > right.get_block(i)) : (left.get_block(i) < right.get_block(i));
  }
  return false;
}

bool operator>(const BigInteger& left, const BigInteger& right) {
  return right < left;
}

bool operator==(const BigInteger& left, const BigInteger& right) {
  if (left.sign() != right.sign()) {
    return false;
  }
  if (left.size() != right.size()) return false;
  for (int i = left.size() - 1; i >= 0; --i) {
    if (left.get_block(i) != right.get_block(i)) return false;
  }
  return true;
}

bool operator!=(const BigInteger& left, const BigInteger& right) {
  return !(left == right);
}

bool operator<=(const BigInteger& left, const BigInteger& right) {
  return !(left > right);
}

bool operator>=(const BigInteger& left, const BigInteger& right) {
  return !(left < right);
}

BigInteger operator"" _bi(const char* buffer, size_t) {
  std::string str_buf(buffer);
  BigInteger copy(str_buf);
  return copy;
}

BigInteger operator"" _bi(const char* buffer) {
  std::string str_buf(buffer);
  BigInteger copy(str_buf);
  return copy;
}

BigInteger operator"" _bi(unsigned long long number) {
  std::string str_number = std::to_string(number);
  BigInteger copy(str_number);
  return copy;
}

BigInteger operator+ (const BigInteger& left, const BigInteger& right) {
  BigInteger copy = left;
  copy += right;
  return copy;
}

BigInteger operator-(const BigInteger& left, const BigInteger& right) {
  BigInteger copy = left;
  copy -= right;
  return copy;
}

std::ostream& operator<< (std::ostream& out, const BigInteger& x) {
  out << x.toString();
  return out;
}

BigInteger operator*(const BigInteger& left, const BigInteger& right) {
  BigInteger copy = left;
  copy *= right;
  return copy;
}

BigInteger operator/(const BigInteger& left, const BigInteger& right) {
  BigInteger copy = left;
  copy /= right;
  return copy;
}

BigInteger operator%(const BigInteger& left, const BigInteger& right) {
  BigInteger copy = left;
  copy %= right;
  return copy;
}

std::istream& operator>>(std::istream& in, BigInteger& a) {
  std::string str;
  in >> str;
  a = static_cast<BigInteger>(str);
  return in;
}

class Rational {
  BigInteger num, den;
public:
  static const int double_precision = 30;

  Rational() {
    num = 0;
    den = 1;
  }

  Rational(int _num) {
    num = _num;
    den = 1;
  }

  Rational(const BigInteger& _num) {
    num = _num;
    den = 1;
  }

  Rational(const Rational& other) {
    num = other.num;
    den = other.den;
  }

  void reduction() {
    Rational copy = (*this);
    BigInteger gcd_num_den = BigInteger::gcd(copy.num, copy.den);
    num /= gcd_num_den;
    den /= gcd_num_den;
  }

  Rational operator- () {
    Rational copy((*this));
    if (copy.num != 0) copy.num.sign_update(copy.num.sign() ^ 1);
    return copy;
  }

  Rational& operator= (const Rational& other) {
    num = other.num;
    den = other.den;
    return (*this);
  }

  Rational& operator+= (const Rational& other) {
    num = (num * other.den + other.num * den);
    den *= other.den;
    reduction();
    return (*this);
  }

  Rational& operator-= (const Rational& other) {
    num = (num * other.den - other.num * den);
    den *= other.den;
    reduction();
    return (*this);
  }

  Rational& operator *= (const Rational& other) {
    Rational copy_this = (*this);
    Rational copy_other = other;
    BigInteger gcd_tnum_oden = BigInteger::gcd(copy_this.num, copy_other.den);
    BigInteger gcd_onum_tden = BigInteger::gcd(copy_other.num, copy_this.den);
    num /= gcd_tnum_oden;
    den /= gcd_onum_tden;
    num *= other.num / gcd_onum_tden;
    den *= other.den / gcd_tnum_oden;
    return (*this);
  }

  Rational& operator/= (const Rational& other) {
    if (this == &other) {
      *this = 1;
    }
    Rational copy_this = (*this);
    Rational copy_other = other;
    BigInteger gcd_num = BigInteger::gcd(copy_this.num, copy_other.num);
    BigInteger gcd_den = BigInteger::gcd(copy_other.num, copy_this.den);
    num /= gcd_num;
    den /= gcd_den;
    num *= other.den / gcd_den;
    den *= other.num / gcd_num;
    if (num * den < 0) {
      num.sign_update(1);
    }
    else {
      num.sign_update(0);
    }
    den.sign_update(0);
    return (*this);
  }

  std::string toString() const {
    std::string str_num = num.toString();
    if (den == 1) {
      return str_num;
    }
    std::string str_den = den.toString();
    return str_num + '/' + str_den;
  }

  std::string asDecimal(size_t precision)const {
    BigInteger integer = num / den;
    std::string res;
    if (integer == 0 && num < 0) res += '-';
    res += integer.toString();
    size_t res_size = res.size();
    integer = num % den;
    size_t den_size = den.toString().size();
    size_t integer_size = integer.toString().size();
    if (integer < 0) integer_size--;
    integer.shift(den_size - integer_size);
    int count_zeros = den_size - integer_size - 1;
    bool integer_isneg = integer.sign();
    integer.sign_update(0);
    if (integer < den) count_zeros++;
    if (integer_isneg) integer.sign_update(1);
    res += '.';
    for (int i = 0; i < count_zeros; i++) res += '0';
    integer.shift(precision + den_size - integer_size + 1);
    integer = integer / den;
    integer.sign_update(0);
    res += integer.toString();
    res.resize(res_size + 1 + precision);
    return res;
  }

  explicit operator double() const {
    return atof(asDecimal(double_precision).c_str());
  }

  BigInteger get_num() const {
    return num;
  }

  BigInteger get_den() const {
    return den;
  }

};

bool operator< (const Rational& left, const Rational& right) {
  return left.get_num() * right.get_den() < left.get_den() * right.get_num();
}

bool operator== (const Rational& left, const Rational& right) {
  return left.get_num() * right.get_den() == left.get_den() * right.get_num();
}

Rational operator* (const Rational& left, const Rational& right) {
  Rational copy = left;
  copy *= right;
  return copy;
}

Rational operator+ (const Rational& left, const Rational& right) {
  Rational copy = left;
  copy += right;
  return copy;
}

Rational operator- (const Rational& left, const Rational& right) {
  Rational copy = left;
  copy -= right;
  return copy;
}

Rational operator/ (const Rational& left, const Rational& right) {
  Rational copy = left;
  copy /= right;
  return copy;
}

bool operator!= (const Rational& left, const Rational& right) {
  return !(left == right);
}

bool operator> (const Rational& left, const Rational& right) {
  return right < left;
}
bool operator<= (const Rational& left, const Rational& right) {
  return !(left > right);
}

bool operator>= (const Rational& left, const Rational& right) {
  return !(left < right);
}
