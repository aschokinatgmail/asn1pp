#include "codec/version.hpp"
#include <gtest/gtest.h>

namespace asn1pp {

TEST(VersionTest, ComponentsDefined) {
    // Version 0.x is valid semver for pre-release / early development
    EXPECT_GE(version::major, 0u);
    EXPECT_GE(version::minor, 0u);
    EXPECT_GE(version::patch, 0u);
}

TEST(VersionTest, StringNotEmpty) {
    EXPECT_FALSE(version::string.empty()) << "Version string must not be empty";
}

TEST(VersionTest, StringMatchesComponents) {
    // Verify version.string starts with "MAJOR.MINOR.PATCH"
    auto expected = std::to_string(version::major) + "." +
                    std::to_string(version::minor) + "." +
                    std::to_string(version::patch);
    EXPECT_EQ(version::string, expected);
}

TEST(VersionTest, ShaNotEmpty) {
    EXPECT_FALSE(version::sha.empty()) << "Git SHA must not be empty";
}

TEST(VersionTest, AtLeastSameOrHigher) {
    const auto m = version::major;
    const auto n = version::minor;
    const auto p = version::patch;

    // Same version must be at least itself
    EXPECT_TRUE(version::at_least(m, n, p));

    // Lower version must be at least current
    EXPECT_TRUE(version::at_least(m, 0, 0));

    if (p > 0) {
        EXPECT_TRUE(version::at_least(m, n, p - 1));
    }
    if (n > 0) {
        EXPECT_TRUE(version::at_least(m, n - 1, 0));
    }
}

TEST(VersionTest, AtLeastHigherFails) {
    // A higher version must NOT be "at least" satisfied
    EXPECT_FALSE(version::at_least(version::major + 1, 0, 0));
    EXPECT_FALSE(version::at_least(version::major, version::minor + 1, 0));
    EXPECT_FALSE(version::at_least(999, 0, 0));
}

TEST(VersionTest, IsExactlySelf) {
    EXPECT_TRUE(version::is_exactly(
        version::major, version::minor, version::patch));
}

TEST(VersionTest, IsExactlyDifferent) {
    EXPECT_FALSE(version::is_exactly(
        version::major + 1, version::minor, version::patch));
    EXPECT_FALSE(version::is_exactly(
        version::major, version::minor, version::patch + 1));
}

}
