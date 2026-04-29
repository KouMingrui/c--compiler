#include "c--/lexer/lexer.h"

namespace cminus {

LexResult Lexer::tokenize(const std::string& source) {
    (void)source;
    LexResult result;
    result.success = false;
    result.errorMessage = "Lexer is not implemented yet.";
    return result;
}

} // namespace cminus
