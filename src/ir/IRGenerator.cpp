#include "c--/ir/IRGenerator.h"

namespace cminus {

IRResult IRGenerator::generate(const ASTNode* root) {
    IRResult result;
    if (root == nullptr) {
        result.success = false;
        result.errorMessage = "IRGenerator received null AST root.";
        return result;
    }

    result.success = false;
    result.errorMessage = "IR generation is not implemented yet.";
    return result;
}

} // namespace cminus
