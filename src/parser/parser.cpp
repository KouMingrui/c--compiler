#include "c--/parser/parser.h"

namespace cminus {

ParseResult Parser::parse(const std::vector<Token>& tokens) {
    (void)tokens;
    ParseResult result;
    result.success = false;
    result.errorMessage = "Parser is not implemented yet.";
    return result;
}

} // namespace cminus
