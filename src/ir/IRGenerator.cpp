#include "c--/ir/IRGenerator.h"
#include "third_part/compiler_ir/include/Module.h"
#include "third_part/compiler_ir/include/Function.h"
#include "third_part/compiler_ir/include/BasicBlock.h"
#include "third_part/compiler_ir/include/Constant.h"
#include "third_part/compiler_ir/include/IRbuilder.h"
#include "third_part/compiler_ir/include/IRprinter.h"
#include "third_part/compiler_ir/include/Type.h"
#include <stdexcept>
#include <map>
#include <vector>
#include <string>
#include <cassert>

namespace cminus
{
    namespace
    {
        class IRVisitor
        {
        private:
            Module *module = nullptr;
            Function *currentFunc = nullptr;
            BasicBlock *currentBB = nullptr;
            IRBuilder *builder = nullptr;
            std::vector<std::map<std::string, Value *>> symbolTableStack; // 符号表栈
            std::vector<Value *> valueStack;                              // 表达式值栈

        public:
            IRVisitor() = default;
            Value *popValue() // 取出表达式值栈栈顶
            {
                if (valueStack.empty())
                {
                    return nullptr;
                }
                Value *val = valueStack.back();
                valueStack.pop_back();
                return val;
            }

            void pushValue(Value *val) // 将值压入表达式值栈栈顶
            {
                valueStack.push_back(val);
            }
            std::string generate(const ASTNode *root)
            {
                // 调试部分
                //  std::cerr << "=== DEBUG: Starting IR generation ===" << std::endl;
                //  static int generateCount = 0;
                //  generateCount++;
                //  std::cerr << "DEBUG generate call #" << generateCount << std::endl;

                if (!root)
                {
                    std::cerr << "ERROR: AST root is null" << std::endl;
                    throw std::runtime_error("AST root is null");
                }
                // 调试部分
                // std::cerr << "DEBUG: Root node: name=" << root->name
                //           << ", value=" << root->value
                //           << ", children=" << root->children.size() << std::endl;

                // 只调用一次reset()
                reset(); // 重置状态支持多次调用

                if (module)
                {
                    // std::cerr << "WARNING: module already exists, deleting..." << std::endl;
                    delete module;
                    module = nullptr;
                }

                module = new Module("main");
                // std::cerr << "DEBUG: Created module at " << module << std::endl;
                builder = new IRBuilder(nullptr, module);

                try
                {
                    visit(root);
                }
                catch (const std::exception &e)
                {
                    std::cerr << "ERROR in visit: " << e.what() << std::endl;
                    throw;
                }

                std::string irText = module->print();
                // std::cerr << "DEBUG: Raw IR text length: " << irText.length() << std::endl;

                return irText;
            }
            void visit(const ASTNode *node)
            {
                // 调试使用
                //  static int callCount = 0;
                //  callCount++;
                //  std::cerr << "DEBUG visit #" << callCount << ": " << node->name
                //            << " value=" << node->value << std::endl;
                if (!node)
                    return;
                if (node->name == "CompUnit")
                {
                    // std::cerr << "DEBUG: Visiting CompUnit with " << node->children.size()
                    //           << " children" << std::endl;
                    visitCompUnit(node);
                    return;
                }
                else if (node->name == "FuncDef")
                {
                    visitFuncDef(node);
                    return;
                }
                else if (node->name == "Type")
                {
                    visitType(node);
                    return;
                }
                else if (node->name == "Block")
                {
                    visitBlock(node);
                    return;
                }
                else if (node->name == "ReturnStmt")
                {
                    visitReturnStmt(node);
                    return;
                }
                else if (node->name == "IntLiteral")
                {
                    visitIntLiteral(node);
                    return;
                }
                else if (node->name == "BinaryExpr")
                {
                    visitBinaryExpr(node);
                    return;
                }
                else if (node->name == "VarDecl")
                {
                    visitVarDecl(node);
                    return;
                }
                else if (node->name == "ConstDecl")
                {
                    visitConstDecl(node);
                    return;
                }
                else if (node->name == "AssignStmt")
                {
                    visitAssignStmt(node);
                    return;
                }
                else if (node->name == "Ident")
                {
                    visitIdent(node);
                    return;
                }
                else if (node->name == "LVal")
                {
                    visitLVal(node);
                    return;
                }
                else if (node->name == "FloatLiteral")
                {
                    visitFloatLiteral(node);
                    return;
                }
                else if (node->name == "UnaryExpr")
                {
                    visitUnaryExpr(node);
                    return;
                }
                else if (node->name == "CallExpr")
                {
                    visitCallExpr(node);
                    return;
                }
                else if (node->name == "ExprStmt")
                {
                    visitExprStmt(node);
                    return;
                }
                else if (node->name == "IfStmt")
                {
                    visitIfStmt(node);
                    return;
                }
                else if (node->name == "ParamList")
                {
                    visitParamList(node);
                    return;
                }
                else if (node->name == "Param")
                {
                    visitParam(node);
                    return;
                }
                else
                {
                    // 对于未知节点类型，递归访问子节点
                    for (auto &child : node->children)
                    {
                        visit(child.get());
                    }
                }
            }
            void visitCompUnit(const ASTNode *node)
            {
                // 创建Module程序根节点
                for (auto &child : node->children)
                {
                    visit(child.get());
                }
            }
            void visitFuncDef(const ASTNode *node)
            {
                // 调试使用
                // static int funcDefCount = 0;
                // funcDefCount++;
                // std::cerr << "DEBUG visitFuncDef #" << funcDefCount
                //           << ": " << node->value << std::endl;
                // 提取函数名
                std::string funcName = node->value; // 直接从节点value获取

                // 检查子节点结构
                if (node->children.size() < 3)
                {
                    throw std::runtime_error("FuncDef must have at least 3 children: Type, ParamList, Block");
                }

                // 第0个子节点是返回类型
                const ASTNode *returnTypeNode = node->children[0].get();
                if (returnTypeNode->name != "Type")
                {
                    throw std::runtime_error("First child of FuncDef must be Type");
                }
                std::string returnTypeName = returnTypeNode->value;
                Type *returnType = getTypeFromName(returnTypeName);

                // 第1个子节点是参数列表
                const ASTNode *paramListNode = node->children[1].get();
                if (paramListNode->name != "ParamList")
                {
                    throw std::runtime_error("Second child of FuncDef must be ParamList");
                }

                // 处理参数
                std::vector<Type *> paramTypes;
                std::vector<std::string> paramNames;

                for (auto &paramChild : paramListNode->children)
                {
                    if (paramChild->name == "Param")
                    {
                        // Param节点：value=参数名，第0个子节点是Type
                        if (!paramChild->children.empty())
                        {
                            const ASTNode *paramTypeNode = paramChild->children[0].get();
                            if (paramTypeNode->name == "Type")
                            {
                                Type *paramType = getTypeFromName(paramTypeNode->value);
                                paramTypes.push_back(paramType);
                                paramNames.push_back(paramChild->value);
                            }
                        }
                    }
                }

                // 创建函数类型
                FunctionType *funcType = FunctionType::get(returnType, paramTypes);
                Function *func = Function::create(funcType, funcName, module);
                // module->add_function(func);
                currentFunc = func;

                // 创建入口基本块
                BasicBlock *entryBB = BasicBlock::create(module, "entry", func); // 输出是label_entry
                // BasicBlock *entryBB = BasicBlock::create(module, "", func);// 输出是label
                // func->add_basic_block(entryBB);
                currentBB = entryBB;
                builder->set_insert_point(entryBB);

                // 进入函数作用域
                enterScope();

                // 为参数创建alloca指令并加入符号表
                auto argIt = func->get_args().begin();
                for (size_t i = 0; i < paramNames.size() && argIt != func->get_args().end(); i++, ++argIt)
                {
                    AllocaInst *paramAlloca = builder->create_alloca(paramTypes[i]);

                    // Argument* 继承自 Value* 可以直接使用
                    Value *argValue = *argIt; // 这已经是Value*了

                    // 存储参数值到alloca空间
                    builder->create_store(argValue, paramAlloca);

                    // 将alloca指令加入符号表，而不是参数值
                    addSymbol(paramNames[i], paramAlloca);
                }

                // 处理函数体（第2个子节点是Block）
                if (node->children.size() > 2)
                {
                    const ASTNode *blockNode = node->children[2].get();
                    if (blockNode->name == "Block")
                    {
                        visitBlock(blockNode);
                    }
                }

                // 如果基本块没有终止指令，添加默认返回
                // if (!currentBB->get_terminator())
                // {
                //     if (returnType->is_int32_type())
                //     {
                //         ConstantInt *zero = ConstantInt::get(0, module);
                //         builder->create_ret(zero);
                //     }
                //     else if (returnType->is_void_type())
                //     {
                //         builder->create_void_ret();
                //     }
                // }

                // 退出函数作用域
                exitScope();

                currentFunc = nullptr;
                currentBB = nullptr;
            }
            void visitType(const ASTNode *node)
            {
                // 确定函数返回类型
            }
            void visitBlock(const ASTNode *node)
            {
                // 创建BasicBlock
                // 进入新的作用域
                enterScope();

                // 处理块内的所有语句
                for (auto &child : node->children)
                {
                    visit(child.get());
                }

                // 退出作用域
                exitScope();
            }
            void visitReturnStmt(const ASTNode *node)
            {
                // 创建返回指令
                if (!node->children.empty())
                {
                    visit(node->children[0].get());
                    Value *retValue = popValue(); // 从值栈弹出返回值
                    if (!retValue)
                    {
                        throw std::runtime_error("Return value not found in value stack");
                    }
                    builder->create_ret(retValue);
                }
                else
                {
                    builder->create_void_ret(); // void返回
                }
            }
            void visitIntLiteral(const ASTNode *node)
            {
                // 创建整数常量
                int intValue = std::stoi(node->value); // AST节点获得整数值
                ConstantInt *constInt = ConstantInt::get(intValue, module);
                pushValue(constInt); // 将常量值压入值栈
            }
            void visitBinaryExpr(const ASTNode *node)
            {
                // 处理二元表达式
                std::string op = node->value;

                // 检查子节点数量
                if (node->children.size() < 2)
                {
                    throw std::runtime_error("BinaryExpr must have 2 children");
                }

                // 计算左表达式
                visit(node->children[0].get());
                Value *left = popValue();

                // 计算右表达式
                visit(node->children[1].get());
                Value *right = popValue();

                if (!left || !right)
                {
                    throw std::runtime_error("Missing operand in binary expression");
                }

                // 检查类型：不支持float运算
                if (left->get_type()->is_float_type() || right->get_type()->is_float_type())
                {
                    throw std::runtime_error("Float operations are not supported in this version");
                }

                // 只支持整数类型
                if (!left->get_type()->is_integer_type() || !right->get_type()->is_integer_type())
                {
                    throw std::runtime_error("Binary expression only supports integer types");
                }

                Value *result = nullptr;
                Type *type = left->get_type();

                // 整数类型运算
                if (type->is_integer_type())
                {
                    if (op == "+")
                    {
                        result = builder->create_iadd(left, right);
                    }
                    else if (op == "-")
                    {
                        result = builder->create_isub(left, right);
                    }
                    else if (op == "*")
                    {
                        result = builder->create_imul(left, right);
                    }
                    else if (op == "/")
                    {
                        result = builder->create_isdiv(left, right);
                    }
                    else if (op == "%")
                    {
                        result = builder->create_irem(left, right);
                    }
                    // 比较运算返回i1类型（布尔）
                    else if (op == "==")
                    {
                        result = builder->create_icmp_eq(left, right);
                    }
                    else if (op == "!=")
                    {
                        result = builder->create_icmp_ne(left, right);
                    }
                    else if (op == "<")
                    {
                        result = builder->create_icmp_lt(left, right);
                    }
                    else if (op == "<=")
                    {
                        result = builder->create_icmp_le(left, right);
                    }
                    else if (op == ">")
                    {
                        result = builder->create_icmp_gt(left, right);
                    }
                    else if (op == ">=")
                    {
                        result = builder->create_icmp_ge(left, right);
                    }
                    // 逻辑运算（需要转换为布尔值）
                    else if (op == "&&" || op == "||")
                    {
                        // 先将整数转换为布尔值
                        Value *leftBool = builder->create_icmp_ne(left, ConstantInt::get(0, module));
                        Value *rightBool = builder->create_icmp_ne(right, ConstantInt::get(0, module));

                        if (op == "&&")
                        {
                            result = builder->create_iand(leftBool, rightBool);
                        }
                        else
                        { // "||"
                            result = builder->create_ior(leftBool, rightBool);
                        }
                    }
                    else
                    {
                        throw std::runtime_error("Unsupported binary operator for integer: " + op);
                    }
                }
                // 浮点数类型运算（需要先实现FloatLiteral）
                else if (type->is_float_type())
                {
                    // 暂时不支持浮点数
                    throw std::runtime_error("Float binary operations not implemented yet");
                }
                else
                {
                    throw std::runtime_error("Unsupported type for binary expression");
                }

                pushValue(result);
            }
            void visitVarDecl(const ASTNode *node)
            {
                // 处理变量声明
                //  第0个子节点是Type
                if (node->children.empty())
                    return;
                const ASTNode *typeNode = node->children[0].get();
                Type *varType = getTypeFromName(typeNode->value);

                // 处理后续的VarDef节点
                for (size_t i = 1; i < node->children.size(); i++)
                {
                    const ASTNode *varDefNode = node->children[i].get();
                    if (varDefNode->name == "VarDef")
                    {
                        std::string varName = varDefNode->value;

                        // 创建alloca指令
                        AllocaInst *alloca = builder->create_alloca(varType);
                        addSymbol(varName, alloca);

                        // 如果有初始化表达式
                        if (!varDefNode->children.empty())
                        {
                            visit(varDefNode->children[0].get()); // 计算初始化表达式
                            Value *initValue = popValue();
                            if (initValue)
                            {
                                builder->create_store(initValue, alloca);
                            }
                        }
                    }
                }
            }

            void visitConstDecl(const ASTNode *node)
            {
                // 处理常量声明
            }
            void visitAssignStmt(const ASTNode *node)
            {
                // 处理赋值语句
                if (node->children.size() < 2)
                {
                    throw std::runtime_error("AssignStmt must have 2 children");
                }

                // 先处理右表达式
                const ASTNode *exprNode = node->children[1].get();
                visit(exprNode);
                Value *exprValue = popValue();

                if (!exprValue)
                {
                    throw std::runtime_error("No value from expression in assignment");
                }

                // 处理左值
                const ASTNode *lvalNode = node->children[0].get();
                if (lvalNode->name != "LVal")
                {
                    throw std::runtime_error("First child of AssignStmt must be LVal");
                }

                std::string varName = lvalNode->value;
                Value *varAddr = lookupSymbol(varName);
                if (!varAddr)
                {
                    throw std::runtime_error("Undefined variable in assignment: " + varName);
                }
                // 类型检查
                if (varAddr->get_type()->is_pointer_type())
                {
                    PointerType *ptrType = static_cast<PointerType *>(varAddr->get_type());
                    Type *pointedType = ptrType->get_element_type();

                    // 检查赋值类型是否匹配
                    if (exprValue->get_type() != pointedType)
                    {
                        throw std::runtime_error("Type mismatch in assignment");
                    }
                }
                // 存储值到变量
                builder->create_store(exprValue, varAddr);
            }
            void visitLVal(const ASTNode *node)
            {
                // 处理变量名
                std::string varName = node->value;
                Value *varAddr = lookupSymbol(varName);

                if (!varAddr)
                {
                    throw std::runtime_error("Undefined variable: " + varName);
                }
                // 加载变量的值
                Value *loadedValue = builder->create_load(varAddr);
                pushValue(loadedValue);
            }
            void visitFloatLiteral(const ASTNode *node)
            {
                // 处理浮点数常量 不支持浮点数常量
                throw std::runtime_error("Float literals are not supported in this version");
            }
            void visitUnaryExpr(const ASTNode *node)
            {
                // 处理一元运算符
                std::string op = node->value; // '+', '-', '!'

                if (node->children.empty())
                {
                    throw std::runtime_error("UnaryExpr must have 1 child");
                }

                // 计算操作数
                visit(node->children[0].get());
                Value *operand = popValue();

                if (!operand)
                {
                    throw std::runtime_error("Missing operand in unary expression");
                }

                // 检查类型：不支持float
                if (operand->get_type()->is_float_type())
                {
                    throw std::runtime_error("Float operations are not supported in this version");
                }
                Value *result = nullptr;

                if (op == "+")
                {
                    // 正号，直接返回操作数
                    result = operand;
                }
                else if (op == "-")
                {
                    // 负号：0 - operand
                    if (operand->get_type()->is_integer_type())
                    {
                        Value *zero = ConstantInt::get(0, module);
                        result = builder->create_isub(zero, operand);
                    }
                    else
                    {
                        throw std::runtime_error("Unary minus only supports integer type");
                    }
                }
                else if (op == "!")
                {
                    // 逻辑非：operand == 0 ? 1 : 0
                    if (operand->get_type()->is_integer_type())
                    {
                        Value *zero = ConstantInt::get(0, module);
                        Value *isZero = builder->create_icmp_eq(operand, zero);
                        // 将i1转换为i32
                        result = builder->create_zext(isZero, Type::get_int32_type(module));
                    }
                    else
                    {
                        throw std::runtime_error("Logical not only supports integer type");
                    }
                }
                else
                {
                    throw std::runtime_error("Unsupported unary operator: " + op);
                }

                pushValue(result);
            }
            void visitCallExpr(const ASTNode *node)
            {
                // 处理函数调用
            }
            void visitExprStmt(const ASTNode *node)
            {
                // 处理表达式语句
            }
            void visitIfStmt(const ASTNode *node)
            {
                // 处理条件语句
            }
            void visitParamList(const ASTNode *node)
            {
                // 处理参数序列
                for (auto &child : node->children)
                {
                    visit(child.get());
                }
            }
            void visitParam(const ASTNode *node)
            {
                // 处理单个参数
            }
            Type *getTypeFromName(const std::string &typeName)
            {
                if (typeName == "int")
                {
                    return Type::get_int32_type(module);
                }
                else if (typeName == "float")
                {
                    return Type::get_float_type(module);
                }
                else if (typeName == "void")
                {
                    return Type::get_void_type(module);
                }
                throw std::runtime_error("Unsupported type: " + typeName);
            }

            void reset() // 清理状态
            {
                currentFunc = nullptr;
                currentBB = nullptr;
                symbolTableStack.clear();
                valueStack.clear();
            }
            void enterScope()
            {
                symbolTableStack.emplace_back();
            }

            void exitScope()
            {
                if (!symbolTableStack.empty())
                {
                    symbolTableStack.pop_back();
                }
            }

            Value *lookupSymbol(const std::string &name)
            {
                for (auto it = symbolTableStack.rbegin(); it != symbolTableStack.rend(); ++it)
                {
                    auto found = it->find(name);
                    if (found != it->end())
                    {
                        return found->second;
                    }
                }
                return nullptr;
            }

            void addSymbol(const std::string &name, Value *value)
            {
                if (symbolTableStack.empty())
                {
                    symbolTableStack.emplace_back();
                }
                symbolTableStack.back()[name] = value;
            }
            void visitIdent(const ASTNode *node)
            {
                // 从符号表中查找变量
                std::string varName = node->value;
                Value *varAddr = lookupSymbol(varName);

                if (!varAddr)
                {
                    throw std::runtime_error("Undefined variable: " + varName);
                }

                // 确保varAddr是AllocaInst*指针类型
                if (varAddr->get_type()->is_pointer_type())
                {
                    // 创建load指令加载变量值
                    Value *loadedValue = builder->create_load(varAddr);
                    pushValue(loadedValue);
                }
                else
                {
                    // 如果不是指针则直接使用
                    pushValue(varAddr);
                }
            }
            ~IRVisitor()
            {
                delete builder;
                delete module;
            }
        };
    }

    IRResult
    IRGenerator::generate(const ASTNode *root)
    {
        IRResult result;

        try
        {
            if (root == nullptr)
            {
                result.success = false;
                result.errorMessage = "IRGenerator received null AST root.";
                return result;
            }

            IRVisitor visitor;
            std::string irText = visitor.generate(root);

            result.success = true;
            result.irText = irText;
            result.errorMessage = "";
        }
        catch (const std::exception &e)
        {
            result.success = false;
            result.errorMessage = "IR generation failed: " + std::string(e.what());
        }

        return result;
    }

} // namespace cminus
