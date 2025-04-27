#include <algorithm>
#include <iostream>
#include <vector>
#include <string>
#include <random>
#include <assert.h>
#include <array>
class BigInteger;
class Rational;

bool operator==(const BigInteger& left, const BigInteger& right);
bool operator!=(const BigInteger& left, const BigInteger& right);
bool operator<(const BigInteger& left, const BigInteger& right);
bool operator>(const BigInteger& left, const BigInteger& right);
bool operator>=(const BigInteger& left, const BigInteger& right);
bool operator<=(const BigInteger& left, const BigInteger& right);
BigInteger operator*(const BigInteger& left, const BigInteger& right);

class BigInteger {
private:
  std::vector <int> val_;
  static constexpr const int  st_10_[] = { 1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000 };
  static const int cMax_ = 1e9;
  static const int size_of_block_ = 9;
  static const int size_of_del_ = 8;
  bool is_negative_;
  static int get_last_n(int value, int cnt) {
    return (value % st_10_[cnt]);
  }

  static int get_size(int x) {
    int i = 1;
    while (x >= st_10_[i]) {
      i++;
    }
    return i;
  }

  static int get_first_n(int value, int cnt, int type) {
    int size = get_size(value);
    if (type != 0 && size < size_of_block_) {
      cnt = std::max(0, cnt - (size_of_block_ - size));
    }
    if (cnt == 0) return 0;
    int res = 0;
    int j = st_10_[size - 1];
    for (int i = 0; i < std::min(cnt, size); ++i) {
      res *= st_10_[1];
      res += value / j;
      value -= j * (value / j);
      j = j / st_10_[1];
    }
    return res;
  }

  void addition(const BigInteger& other) {
    int rem = 0;
    val_.resize(std::max(val_.size(), other.size()));
    for (size_t i = 0; i < other.size(); ++i) {
      int new_rem = (other.val_[i] + val_[i] + rem) / cMax_;
      val_[i] += other.val_[i] + rem - new_rem * cMax_;
      rem = new_rem;
    }
    size_t i = other.size();
    while (rem != 0) {
      if (i == val_.size()) {
        val_.push_back(0);
      }
      int new_rem = (val_[i] + rem) / cMax_;
      val_[i] += rem - new_rem * cMax_;
      rem = new_rem;
      i++;
    }
  }

  void shrink_zero() {
    if (size() == 0) return;
    for (int i = size() - 1; i > 0; i--) {
      if (val_[i] != 0) break;
      val_.pop_back();
    }
  }

  void subtraction(const BigInteger& other) {
    int rem = 0;
    bool bigger = (other <= *this);
    bigger ^= is_negative_;
    is_negative_ ^= !bigger;
    val_.resize(std::max(size(), other.size()));
    int coef = (bigger ? -1 : 1);
    for (size_t i = 0; i < size(); ++i) {
      int temp = (i < other.size() ? other.val_[i] : 0);
      bool flag = (coef * (temp - val_[i]) + rem < 0);
      int signed_temp = coef * temp;
      int signed_val = -coef * val_[i];
      val_[i] = (flag * cMax_ + signed_temp + rem + signed_val);
      rem = -flag;
    }
    shrink_zero();
  }

  std::pair<BigInteger, int> get_first_k(int res_size) {
    BigInteger res;
    res.val_.clear();
    int sz = res_size;
    int n = val_.size();
    auto size_of_this = get_size(val_[n - 1]) + (n - 1) * size_of_block_;
    if (res_size > size_of_this) {
      res = *this;
      sz = size_of_this;
    }
    else {
      int count = 0;
      if (res_size > get_size(val_[n - 1])) {
        count += get_size(val_[n - 1]);
        res.val_.push_back(val_[n - 1]);
      }
      else {
        count = res_size;
        res.val_.push_back(get_first_n(val_[n - 1], res_size, 0));
      }
      int j = n - 2;
      int ost = -1;
      while (count < res_size) {
        if (res_size - count > size_of_block_) {
          res.val_.push_back(val_[j]);
          count += size_of_block_;
        }
        else {
          res.val_.push_back(get_first_n(val_[j], res_size - count, 1));
          ost = size_of_block_ - (res_size - count);
          count = res_size;
        }
        j--;
      }
      if (ost > 0) {
        for (int i = res.val_.size() - 1; i > 0; i--) {
          if (i == 1) {
            if (get_size(res.val_[0]) < ost) {
              res.val_[i] += (res.val_[0] * st_10_[size_of_block_ - ost]);
              res.val_[0] = 0;
            }
            else {
              res.val_[i] += (get_last_n(res.val_[0], ost) * st_10_[size_of_block_ - ost]);
              res.val_[0] /= st_10_[ost];
            }
          }
          else {
            res.val_[i] += (get_last_n(res.val_[i - 1], ost) * st_10_[size_of_block_ - ost]);
            res.val_[i - 1] /= st_10_[ost];
          }
        }
      }
      if (res.val_[0] == 0) {
        for (size_t i = 1; i < res.val_.size(); i++) {
          res.val_[i - 1] = res.val_[i];
        }
        res.val_.pop_back();
      }
      reverse(res.val_.begin(), res.val_.end());
    }
    return { res, sz };
  }

  void multiplication(int other) {
    int rem = 0;
    for (size_t i = 0; i < val_.size(); ++i) {
      long long sum = 1ll * val_[i] * other + rem;
      val_[i] = (sum % cMax_);
      rem = sum / cMax_;
    }
    if (rem != 0) {
      val_.push_back(rem);
    }
  }

  void shift(int count) {
    int size_of_rem = count % size_of_block_;
    int rem = 0;
    int j = 1;
    for (int i = 0; i < size_of_rem; i++) j = j * st_10_[1];
    for (size_t i = 0; i < size(); i++) {
      if (i + 1 < size() || 1ll * val_[i] * j >= cMax_) {
        int res = val_[i] % (cMax_ / j);
        int new_rem = val_[i] / (cMax_ / j);
        val_[i] = rem + res * j;
        rem = new_rem;
        if (i + 1 == size() && rem != 0) {
          j = 1;
          while (rem >= j) j = j * st_10_[1];
        }
      }
      else {
        val_[i] = val_[i] * j + rem;
        rem = 0;
      }
    }

    if (rem > 0) {
      val_.push_back(rem);
    }
    if (count >= size_of_block_) {
      val_.resize(size() + count / size_of_block_);
      for (int i = size() - 1; i >= count / size_of_block_; i--) {
        val_[i] = val_[i - count / size_of_block_];
      }
      for (int i = 0; i < count / size_of_block_; i++) val_[i] = 0;
    }
  }

  static int bin_search(BigInteger& divisible, BigInteger& divider) {
    int left = 0;
    int right = cMax_;
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
    divider.is_negative_ = 0;
    int n = get_size(divider.val_[divider.val_.size() - 1]) + (divider.val_.size() - 1) * size_of_block_;
    BigInteger res = 0;
    BigInteger copy = (*this);
    copy.is_negative_ = 0;
    while (copy >= divider) {
      auto temp_block = copy.get_first_k(n + size_of_del_);
      int temp_size_copy = get_size(copy.val_[copy.val_.size() - 1]) + (copy.val_.size() - 1) * size_of_block_;
      int priv = bin_search(temp_block.first, divider);
      BigInteger big_priv(priv);
      big_priv.shift(temp_size_copy - temp_block.second);
      res += big_priv;
      BigInteger new_block = divider * priv;
      new_block.shift(temp_size_copy - temp_block.second);
      copy -= new_block;
    }
    if (res != 0) res.is_negative_ = this->is_negative_ ^ other.is_negative_;
    if (copy != 0) copy.is_negative_ = this->is_negative_;
    return { res, copy };
  }
public:

  const bool& sign() const {
    return is_negative_;
  }

  size_t size() const {
    return val_.size();
  }

  int get_block(int i) const {
    return val_[i];
  }

  void sign_update(bool flag) {
    is_negative_ = flag;
  }

  BigInteger(long long value) {
    val_.clear();
    is_negative_ = (value < 0);
    value = abs(value);
    val_.push_back(value % cMax_);
    if (value / cMax_ > 0) {
      if (value / cMax_ < cMax_) {
        val_.push_back(value / cMax_);
      }
      else {
        val_.push_back((value / cMax_) % cMax_);
        val_.push_back((value / cMax_) / cMax_);
      }
    }
  }

  BigInteger() {
    val_.push_back(0);
    is_negative_ = 0;
  }

  BigInteger(const std::string& str) {
    BigInteger res;
    res.val_.clear();
    std::string copy = str;
    res.is_negative_ = 0;
    if (copy[0] == '-') {
      res.is_negative_ = true;
      copy = copy.substr(1, str.size() - 1);
    }
    reverse(copy.begin(), copy.end());
    int j = 1;
    int new_block = 0;
    for (size_t i = 0; i + size_of_block_ - 1 < copy.size(); i += size_of_block_) {
      j = 1;
      new_block = 0;
      for (int l = 0; l < size_of_block_; ++l) {
        new_block += j * (copy[i + l] - '0');
        j *= st_10_[1];
      }
      res.val_.push_back(new_block);
    }
    if (copy.size() % size_of_block_ != 0) {
      int rem = copy.size() % size_of_block_;
      j = 1;
      new_block = 0;
      for (int i = rem; i > 0; --i) {
        new_block += j * (copy[copy.size() - i] - '0');
        j = j * st_10_[1];
      }
      res.val_.push_back(new_block);
    }
    res.shrink_zero();
    if (res.val_.size() == 1 && res.val_[0] == 0) {
      res.is_negative_ = false;
    }
    *this = res;
  }

  BigInteger(const BigInteger& other) {
    val_ = other.val_;
    is_negative_ = other.is_negative_;
  }

  BigInteger& operator= (const BigInteger& other) {
    if (&other == this) return *this;
    val_ = other.val_;
    is_negative_ = other.is_negative_;
    return *this;
  }
  BigInteger operator-() {
    BigInteger copy = *this;
    if (*this != 0) copy.is_negative_ ^= 1;
    return copy;
  }

  BigInteger& operator+= (const BigInteger& other) {
    if (is_negative_ == other.is_negative_) {
      addition(other);
    }
    else {
      is_negative_ ^= 1;
      subtraction(other);
      is_negative_ ^= 1;
    }
    if (this->val_.size() == 1 && val_[0] == 0) this->is_negative_ = false;
    return *this;
  }

  BigInteger& operator-= (const BigInteger& other) {
    if (this == &other) {
      *this = 0;
    }
    else {
      is_negative_ ^= 1;
      *this += other;
      if (*this != 0) is_negative_ ^= 1;
    }
    return *this;
  }

  BigInteger& operator*=(const BigInteger& other) {
    if (*this == 0 || other == 0) {
      *this = 0;
      return *this;
    }
    if (this == &other) {
      BigInteger copy = other;
      copy *= other;
      *this = copy;
    }
    else {
      BigInteger res;
      int size_of_shift = 0;
      for (size_t i = 0; i < other.size(); ++i) {
        BigInteger mult_by_block = *this;
        mult_by_block.multiplication(other.val_[i]);
        mult_by_block.shift(size_of_shift);
        res += mult_by_block;
        size_of_shift += size_of_block_;
      }
      *this = res;
      is_negative_ = is_negative_ ^ other.is_negative_;
    }
    if (this->val_.size() == 1 && val_[0] == 0) this->is_negative_ = false;
    return *this;
  }

  BigInteger& operator/=(const BigInteger& other) {
    if (this == &other) {
      *this = 1;
    }
    else if (*this == 0) {
      *this = 0;
    }
    else {
      *this = this->division(other).first;

    }
    return *this;
  }

  BigInteger& operator%=(const BigInteger& other) {
    if (this == &other) {
      *this = 0;
    }
    else {
      *this = this->division(other).second;
    }
    return *this;
  }

  BigInteger operator++(int) {
    BigInteger copy = *this;
    *this += 1;
    return copy;
  }

  BigInteger operator--(int) {
    BigInteger copy = *this;
    *this -= 1;
    return copy;
  }

  BigInteger& operator++() {
    *this += 1;
    return *this;
  }

  BigInteger& operator--() {
    *this -= 1;
    return *this;
  }

  explicit operator bool() const {
    return *this != 0;
  }

  std::string toString() const {
    std::string res;
    if (size() == 0 || val_[size() - 1] == 0) {
      res.push_back('0');
      return res;
    }
    if (is_negative_) {
      res.push_back('-');
    }
    if (size() >= 1) res += std::to_string(val_[size() - 1]);
    for (int i = size() - 2; i >= 0; --i) {
      int j = cMax_ / st_10_[1];
      int copy_val_i = val_[i];
      for (int l = 0; l < size_of_block_; ++l) {
        res += ('0' + copy_val_i / j);
        copy_val_i %= j;
        j /= st_10_[1];
      }
    }
    return res;
  }
  friend Rational;
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
private:
  BigInteger num_, den_;
  static const int double_precision_ = 30;
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
public:
  Rational() {
    num_ = 0;
    den_ = 1;
  }

  Rational(int _num) {
    num_ = _num;
    den_ = 1;
  }

  Rational(const BigInteger& _num) {
    num_ = _num;
    den_ = 1;
  }

  Rational(const Rational& other) {
    num_ = other.num_;
    den_ = other.den_;
  }

  void reduction() {
    Rational copy = *this;
    BigInteger gcd_num_den = gcd(copy.num_, copy.den_);
    num_ /= gcd_num_den;
    den_ /= gcd_num_den;
  }

  Rational operator- () {
    Rational copy(*this);
    if (copy.num_ != 0) copy.num_.sign_update(copy.num_.sign() ^ 1);
    return copy;
  }

  Rational& operator= (const Rational& other) {
    num_ = other.num_;
    den_ = other.den_;
    return *this;
  }

  Rational& operator+= (const Rational& other) {
    num_ = (num_ * other.den_ + other.num_ * den_);
    den_ *= other.den_;
    reduction();
    return *this;
  }

  Rational& operator-= (const Rational& other) {
    num_ = (num_ * other.den_ - other.num_ * den_);
    den_ *= other.den_;
    reduction();
    return *this;
  }

  Rational& operator *= (const Rational& other) {
    Rational copy_this = (*this);
    Rational copy_other = other;
    BigInteger gcd_tnum_oden = gcd(copy_this.num_, copy_other.den_);
    BigInteger gcd_onum_tden = gcd(copy_other.num_, copy_this.den_);
    num_ /= gcd_tnum_oden;
    den_ /= gcd_onum_tden;
    num_ *= other.num_ / gcd_onum_tden;
    den_ *= other.den_ / gcd_tnum_oden;
    return *this;
  }

  Rational& operator/= (const Rational& other) {
    if (this == &other) {
      *this = 1;
    }
    Rational copy_this = *this;
    Rational copy_other = other;
    BigInteger gcd_num = gcd(copy_this.num_, copy_other.num_);
    BigInteger gcd_den = gcd(copy_other.num_, copy_this.den_);
    num_ /= gcd_num;
    den_ /= gcd_den;
    num_ *= other.den_ / gcd_den;
    den_ *= other.num_ / gcd_num;
    if (num_ * den_ < 0) {
      num_.sign_update(1);
    }
    else {
      num_.sign_update(0);
    }
    den_.sign_update(0);
    return *this;
  }

  std::string toString() const {
    std::string str_num = num_.toString();
    if (den_ == 1) {
      return str_num;
    }
    std::string str_den = den_.toString();
    return str_num + '/' + str_den;
  }

  std::string asDecimal(size_t precision)const {
    BigInteger integer = num_ / den_;
    std::string res;
    if (integer == 0 && num_ < 0) res += '-';
    res += integer.toString();
    size_t res_size = res.size();
    integer = num_ % den_;
    size_t den_size = den_.toString().size();
    size_t integer_size = integer.toString().size();
    if (integer < 0) integer_size--;
    integer.shift(den_size - integer_size);
    int count_zeros = den_size - integer_size - 1;
    bool integer_isneg = integer.sign();
    integer.sign_update(0);
    if (integer < den_) count_zeros++;
    if (integer_isneg) integer.sign_update(1);
    res += '.';
    for (int i = 0; i < count_zeros; i++) res += '0';
    integer.shift(precision + den_size - integer_size + 1);
    integer = integer / den_;
    integer.sign_update(0);
    res += integer.toString();
    res.resize(res_size + 1 + precision);
    return res;
  }

  explicit operator double() const {
    return atof(asDecimal(double_precision_).c_str());
  }

  BigInteger get_num() const {
    return num_;
  }

  BigInteger get_den() const {
    return den_;
  }

};

std::istream& operator>>(std::istream& in, Rational& x) {
  std::string s;
  in >> s;
  x = Rational(BigInteger(s));
  return in;
}

std::ostream& operator<<(std::ostream& out, const Rational& x) {
  out << x.toString();
  return out;
}

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

template <size_t l, size_t d, size_t n>
struct my_sqrt {
  static const size_t value = ((l + l + d) * (l + l + d) >= 4 * n ? my_sqrt<l, d / 2, n>::value : my_sqrt<l + d / 2, d / 2, n>::value);
};

template<size_t l, size_t n>
struct my_sqrt<l, 1, n> {
  static const size_t value = l + 1;
};

template<size_t N, size_t D>
struct isPrimeHelp {
  static const bool value = (N % D == 0 ? false : isPrimeHelp<N, D - 1>::value);
};

template<size_t N>
struct isPrimeHelp<N, 1> {
  static const bool value = true;
};

template<size_t N>
struct isPrime {
  static const bool value = isPrimeHelp<N, my_sqrt<0, N, N>::value>::value;
};

template <int N>
class Residue {
private:
  size_t value_ = 0;
  static Residue<N> bin_pow(Residue<N> x, size_t st) {
    if (st == 1) return x;
    if (st % 2 == 0) {
      Residue<N> temp = bin_pow(x, st / 2);
      return temp * temp;
    }
    else {
      return x * bin_pow(x, st - 1);
    }
  }
public:
  Residue() : value_(0) {}

  Residue(int x) {
    value_ = ((x % N + N) % N);
  }

  Residue& operator+=(const Residue& other) {
    value_ = (value_ + other.value_) % N;
    return *this;
  }

  Residue& operator-=(const Residue& other) {
    value_ = (value_ - other.value_ + N) % N;
    return *this;
  }

  Residue& operator*=(const Residue& other) {
    value_ = (1ll * value_ * other.value_) % N;
    return *this;
  }

  Residue operator- () const {
    return Residue((N - value_) % N);
  }

  Residue& operator/=(const Residue& other) {
    static_assert(isPrime<N>::value);
    operator*=(bin_pow(other, N - 2));
    return *this;
  }
  size_t get_val() const { return value_; }
};

template<int N>
bool operator==(const Residue<N>& left, const Residue<N>& right) {
  return ((left.get_val() - right.get_val()) % N == 0);
}

template<int N>
bool operator!=(const Residue<N>& left, const Residue<N>& right) {
  return !(left == right);
}

template <int N>
Residue<N> operator*(const Residue<N>& left, const Residue<N>& right) {
  Residue<N> copy = left;
  copy *= right;
  return copy;
}

template<int N>
Residue<N> operator/(const Residue<N>& left, const Residue<N>& right) {
  Residue<N> copy = left;
  copy /= right;
  return copy;
}

template<int N>
std::ostream& operator<<(std::ostream& out, const Residue<N>& other) {
  out << other.value_;
  return out;
}


template <int N>
Residue<N> operator+(const Residue<N>& left, const Residue<N>& right) {
  Residue<N> copy = left;
  copy += right;
  return copy;
}

template <int N>
Residue<N> operator-(const Residue<N>& left, const Residue<N>& right) {
  Residue<N> copy = left;
  copy -= right;
  return copy;
}

template <size_t N, size_t M, typename Field = Rational>
class Matrix {
private:
  std::array<std::array<Field, M>, N> array_;
public:
  Matrix() {
    for (size_t i = 0; i < N; i++) {
      for (size_t j = 0; j < M; j++) {
        array_[i][j] = Field(0);
      }
    }
  }

  Matrix(std::initializer_list<std::initializer_list<Field>> list) {
    size_t i = 0;
    for (auto raw : list) {
      size_t j = 0;
      for (auto val : raw) {
        array_[i][j] = val;
        j++;
      }
      i++;
    }
  }

  const std::array<Field, M>& operator[](size_t i) const {
    return array_[i];
  }

  std::array<Field, M>& operator[](size_t i) {
    return array_[i];
  }

  Matrix<N, M, Field>& UnityMatrix() {
    static_assert(N == M);
    for (size_t i = 0; i < N; i++) {
      for (size_t j = 0; j < N; j++) {
        if (i == j) {
          array_[i][j] = Field(1);
        }
        else {
          array_[i][j] = Field(0);
        }
      }
    }
    return *this;
  }

  Matrix<N, M, Field>& operator+=(const Matrix<N, M, Field>& other) {
    for (size_t i = 0; i < N; i++) {
      for (size_t j = 0; j < M; j++) {
        array_[i][j] += other[i][j];
      }
    }
    return *this;
  }

  Matrix<N, M, Field>& operator-=(const Matrix<N, M, Field>& other) {
    for (size_t i = 0; i < N; i++) {
      for (size_t j = 0; j < M; j++) {
        array_[i][j] -= other[i][j];
      }
    }
    return *this;
  }

  Matrix<M, N, Field> transposed() const {
    Matrix<M, N, Field> result;
    for (size_t i = 0; i < N; i++) {
      for (size_t j = 0; j < M; j++) {
        result[j][i] = array_[i][j];
      }
    }
    return result;
  }

  std::array<Field, N> getColumn(unsigned j) {
    std::array<Field, N> result;
    for (size_t i = 0; i < N; i++) {
      result[i] = array_[i][j];
    }
    return result;
  }

  std::array<Field, M> getRow(unsigned i) {
    std::array<Field, M> result;
    for (size_t j = 0; j < M; j++) {
      result[j] = array_[i][j];
    }
    return result;
  }

  Field trace() const {
    static_assert(N == M);
    Field res(0);
    for (size_t i = 0; i < N; i++) {
      res += array_[i][i];
    }
    return res;
  }

  Matrix<N, N, Field> inverted() const {
    static_assert(N == M);
    Matrix<N, N, Field> invert;
    invert.UnityMatrix();
    Matrix<N, N, Field> copy = *this;
    for (size_t j = 0; j < N; j++) {
      for (size_t i = j + 1; i < N; i++) {
        if (copy[j][j] == Field(0) && copy[i][j] != Field(0)) {
          std::swap(copy[i], copy[j]);
          std::swap(invert[i], invert[j]);
        }
      }
      for (size_t i = 0; i < N; i++) {
        if (i == j) continue;
        Field coef = -copy[i][j] / copy[j][j];
        for (size_t l = 0; l < N; l++) {
          copy[i][l] += coef * copy[j][l];
          invert[i][l] += coef * invert[j][l];
        }
      }
      Field coef = Field(1) / copy[j][j];
      for (size_t i = 0; i < N; i++) {
        copy[j][i] *= coef;
        invert[j][i] *= coef;
      }
    }
    return invert;
  }

  Matrix<N, N, Field>& invert() {
    *this = inverted();
    return *this;
  }

  Field det() const {
    static_assert(N == M);
    Field res = 1;
    Matrix<N, N, Field> copy = *this;
    for (size_t j = 0; j < N; j++) {
      for (size_t i = j + 1; i < N; i++) {
        if (copy[j][j] == Field(0) && copy[i][j] != Field(0)) {
          std::swap(copy[i], copy[j]);
          res *= -1;
        }
      }
      for (size_t i = j + 1; i < N; i++) {
        Field coef = -copy[i][j] / copy[j][j];
        for (size_t l = 0; l < N; l++) {
          copy[i][l] += coef * copy[j][l];
        }
      }
      res *= copy[j][j];
    }
    return res;
  }

  int rank() const {
    Matrix<N, M, Field> copy = *this;
    size_t ans = M;
    int count_zeroes = 0;
    for (size_t j = 0; j < M; j++) {
      if (j + count_zeroes == M) break;
      for (size_t i = j + 1; i < N; i++) {
        if (copy[j][j + count_zeroes] == Field(0) && copy[i][j + count_zeroes] != Field(0)) {
          std::swap(copy[i], copy[j]);
        }
      }
      if (j < N && copy[j][j + count_zeroes] == Field(0)) {
        ans--;
        count_zeroes++;
        if (j + count_zeroes == M) break;
        continue;
      }
      for (size_t i = j + 1; i < N; i++) {
        Field coef = -copy[i][j + count_zeroes] / copy[j][j + count_zeroes];
        for (size_t l = 0; l < M; l++) {
          copy[i][l] += coef * copy[j][l];
        }
      }
    }
    return std::min(ans, std::min(N, M));
  }

  template<size_t k>
  Matrix<N, k, Field> operator*=(const Matrix<M, k, Field>& other) {
    Matrix<N, k, Field> result;
    for (size_t i = 0; i < N; i++) {
      for (size_t l = 0; l < k; l++) {
        for (size_t j = 0; j < M; j++) {
          result[i][l] += array_[i][j] * other[j][l];
        }
      }
    }
    *this = result;
    return *this;
  }
};

template<size_t n, size_t m, typename Field = Rational>
bool operator==(const Matrix<n, m, Field>& left, const Matrix<n, m, Field>& right) {
  for (size_t i = 0; i < n; i++) {
    for (size_t j = 0; j < m; j++) {
      if (left[i][j] != right[i][j]) return false;
    }
  }
  return true;
}

template<size_t n, size_t m, typename Field = Rational>
Matrix<n, m, Field> operator*(const Field coef, const Matrix<n, m, Field>& matrix) {
  Matrix<n, m, Field> copy = matrix;
  for (size_t i = 0; i < n; i++) {
    for (size_t j = 0; j < m; j++) {
      copy[i][j] *= coef;
    }
  }
  return copy;
}

template<size_t n, size_t m, typename Field = Rational>
Matrix<n, m, Field> operator*(const Matrix<n, m, Field>& matrix, const Field x) {
  Matrix<n, m, Field> copy = matrix;
  for (size_t i = 0; i < n; i++) {
    for (size_t j = 0; j < m; j++) {
      copy[i][j] *= x;
    }
  }
  return copy;
}

template<size_t n, size_t m, typename Field = Rational>
Matrix<n, m, Field> operator+(const Matrix<n, m, Field>& left, const Matrix<n, m, Field>& right) {
  Matrix<n, m, Field> copy = left;
  copy += right;
  return copy;
}

template<size_t n, size_t m, typename Field = Rational>
Matrix<n, m, Field> operator-(const Matrix<n, m, Field>& left, const Matrix<n, m, Field>& right) {
  Matrix<n, m, Field> copy = left;
  copy -= right;
  return copy;
}

template<size_t n, size_t m, size_t k, typename Field = Rational>
Matrix<n, k, Field> operator*(Matrix<n, m, Field> left, Matrix<m, k, Field> right) {
  Matrix<n, k, Field> result;
  for (size_t i = 0; i < n; i++) {
    for (size_t l = 0; l < k; l++) {
      for (size_t j = 0; j < m; j++) {
        result[i][l] += left[i][j] * right[j][l];
      }
    }
  }
  return result;
}

template<size_t N, typename Field = Rational>
using SquareMatrix = Matrix<N, N, Field>;
