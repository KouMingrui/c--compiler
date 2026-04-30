## 开发日志（kmr）

### 2026-04-29
先尝试使用一下fork后仓库的拉取和提交


### 2026-04-30
实现实验指导书中要求的visitor框架
为中间代码生成阶段加上符号表处理和值栈处理
对于正确样例ast.txt的所有节点实现了visit处理
实现visitIntLiteral 创建整数常量并压入值栈
实现visitReturnStmt 创建返回指令
实现visitIdent 变量查找和加载
实现visitCompUnit visitFuncDef visitBlock框架