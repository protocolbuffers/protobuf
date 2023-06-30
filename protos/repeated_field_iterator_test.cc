#include "protos/repeated_field_iterator.h"

#include <algorithm>
#include <array>
#include <numeric>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

using ::testing::ElementsAre;

namespace protos {
namespace internal {

struct IteratorTestPeer {
  template <typename T>
  static ReferenceProxy<T> MakeRefProxy(T& ref) {
    return ReferenceProxy<T>(ref);
  }

  template <typename T>
  static RepeatedScalarIterator<T> MakeIterator(T* ptr) {
    return RepeatedScalarIterator<T>(ptr);
  }
};

namespace {

TEST(ReferenceProxyTest, BasicOperationsWork) {
  int i = 0;
  ReferenceProxy<int> p = IteratorTestPeer::MakeRefProxy(i);
  ReferenceProxy<const int> cp =
      IteratorTestPeer::MakeRefProxy(std::as_const(i));
  EXPECT_EQ(i, 0);
  p = 17;
  EXPECT_EQ(i, 17);
  EXPECT_EQ(p, 17);
  EXPECT_EQ(cp, 17);
  i = 13;
  EXPECT_EQ(p, 13);
  EXPECT_EQ(cp, 13);

  EXPECT_FALSE((std::is_assignable<decltype(cp), int>::value));

  // Check that implicit conversion works T -> const T
  ReferenceProxy<const int> cp2 = p;
  EXPECT_EQ(cp2, 13);

  EXPECT_FALSE((std::is_convertible<decltype(cp), ReferenceProxy<int>>::value));
}

TEST(ReferenceProxyTest, AssignmentAndSwap) {
  int i = 3;
  int j = 5;
  ReferenceProxy<int> p = IteratorTestPeer::MakeRefProxy(i);
  ReferenceProxy<int> p2 = IteratorTestPeer::MakeRefProxy(j);

  EXPECT_EQ(p, 3);
  EXPECT_EQ(p2, 5);
  swap(p, p2);
  EXPECT_EQ(p, 5);
  EXPECT_EQ(p2, 3);

  p = p2;
  EXPECT_EQ(p, 3);
  EXPECT_EQ(p2, 3);
}

template <typename T>
std::array<bool, 6> RunCompares(const T& a, const T& b) {
  // Verify some basic properties here.
  // Equivalencies
  EXPECT_EQ((a == b), (b == a));
  EXPECT_EQ((a != b), (b != a));
  EXPECT_EQ((a < b), (b > a));
  EXPECT_EQ((a > b), (b < a));
  EXPECT_EQ((a <= b), (b >= a));
  EXPECT_EQ((a >= b), (b <= a));

  // Opposites
  EXPECT_NE((a == b), (a != b));
  EXPECT_NE((a < b), (a >= b));
  EXPECT_NE((a > b), (a <= b));

  return {{
      (a == b),
      (a != b),
      (a < b),
      (a <= b),
      (a > b),
      (a >= b),
  }};
}

template <typename T>
void TestScalarIterator(T* array) {
  RepeatedScalarIterator<T> it = IteratorTestPeer::MakeIterator(array);
  // Copy
  auto it2 = it;

  EXPECT_THAT(RunCompares(it, it2),
              ElementsAre(true, false, false, true, false, true));

  // Increment
  EXPECT_EQ(*++it, 11);
  EXPECT_EQ(*it2, 10);
  EXPECT_EQ(*it++, 11);
  EXPECT_EQ(*it2, 10);
  EXPECT_EQ(*it, 12);
  EXPECT_EQ(*it2, 10);

  EXPECT_THAT(RunCompares(it, it2),
              ElementsAre(false, true, false, false, true, true));

  // Assign
  it2 = it;
  EXPECT_EQ(*it, 12);
  EXPECT_EQ(*it2, 12);

  // Decrement
  EXPECT_EQ(*--it, 11);
  EXPECT_EQ(*it--, 11);
  EXPECT_EQ(*it, 10);

  it += 5;
  EXPECT_EQ(*it, 15);
  EXPECT_EQ(it - it2, 3);
  EXPECT_EQ(it2 - it, -3);
  it -= 3;
  EXPECT_EQ(*it, 12);
  EXPECT_EQ(it[6], 18);
  EXPECT_EQ(it[-1], 11);
}

TEST(ScalarIteratorTest, BasicOperationsWork) {
  int array[10] = {10, 11, 12, 13, 14, 15, 16, 17, 18, 19};
  TestScalarIterator<const int>(array);
  TestScalarIterator<int>(array);
}

TEST(ScalarIteratorTest, Convertibility) {
  int array[10] = {10, 11, 12, 13, 14, 15, 16, 17, 18, 19};
  RepeatedScalarIterator<int> it = IteratorTestPeer::MakeIterator(array);
  it += 4;
  RepeatedScalarIterator<const int> cit = it;
  EXPECT_EQ(*it, 14);
  EXPECT_EQ(*cit, 14);
  it += 2;
  EXPECT_EQ(*it, 16);
  EXPECT_EQ(*cit, 14);
  cit = it;
  EXPECT_EQ(*it, 16);
  EXPECT_EQ(*cit, 16);

  EXPECT_FALSE((std::is_convertible<RepeatedScalarIterator<const int>,
                                    RepeatedScalarIterator<int>>::value));
  EXPECT_FALSE((std::is_assignable<RepeatedScalarIterator<int>,
                                   RepeatedScalarIterator<const int>>::value));
}

TEST(ScalarIteratorTest, MutabilityOnlyWorksOnMutable) {
  int array[10] = {10, 11, 12, 13, 14, 15, 16, 17, 18, 19};
  RepeatedScalarIterator<int> it = IteratorTestPeer::MakeIterator(array);
  EXPECT_EQ(array[3], 13);
  it[3] = 113;
  EXPECT_EQ(array[3], 113);
  RepeatedScalarIterator<const int> cit = it;
  EXPECT_FALSE((std::is_assignable<decltype(*cit), int>::value));
  EXPECT_FALSE((std::is_assignable<decltype(cit[1]), int>::value));
}

TEST(ScalarIteratorTest, IteratorReferenceInteraction) {
  int array[10] = {10, 11, 12, 13, 14, 15, 16, 17, 18, 19};
  RepeatedScalarIterator<int> it = IteratorTestPeer::MakeIterator(array);
  EXPECT_EQ(it[4], 14);
  // op& from references goes back to iterator.
  RepeatedScalarIterator<int> it2 = &it[4];
  EXPECT_EQ(it + 4, it2);
}

TEST(ScalarIteratorTest, IteratorBasedAlgorithmsWork) {
  // We use a vector here to make testing it easier.
  std::vector<int> v(10, 0);
  RepeatedScalarIterator<int> it = IteratorTestPeer::MakeIterator(v.data());
  EXPECT_THAT(v, ElementsAre(0, 0, 0, 0, 0, 0, 0, 0, 0, 0));
  std::iota(it, it + 10, 10);
  EXPECT_THAT(v, ElementsAre(10, 11, 12, 13, 14, 15, 16, 17, 18, 19));
  EXPECT_EQ(it + 5, std::find(it, it + 10, 15));
  EXPECT_EQ(145, std::accumulate(it, it + 10, 0));
  std::sort(it, it + 10, [](int a, int b) {
    return std::tuple(a % 2, a) < std::tuple(b % 2, b);
  });
  EXPECT_THAT(v, ElementsAre(10, 12, 14, 16, 18, 11, 13, 15, 17, 19));
}

}  // namespace
}  // namespace internal
}  // namespace protos
