#pragma once

#include <string>
#include <vector>

// Placeholder for the SHA256 utility as per T065.
// Implementation will be provided by the user.

namespace util {
    std::string sha256(const std::string& data);
    std::string sha256(const std::vector<unsigned char>& data);
}
