#pragma once

#include <vector>

#include "c--/common/Result.h"
#include "c--/common/Token.h"

namespace cminus {

class Parser {
public:
    ParseResult parse(const std::vector<Token>& tokens);
};

} // namespace cminus
