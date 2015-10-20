
#undef DEBUG_PARSER

#include <stdio.h>
#include <stdlib.h>

#include <b0rk/parser.h>

using namespace std;
using namespace b0rk;

struct OpTable
{
    TokenType token;
    OpType oper;
    bool hasRightExpr;
};
OpTable opTable[] = {
    {TOK_ASSIGN, OP_SET, true},
    {TOK_PLUS, OP_ADD, true},
    {TOK_MINUS, OP_SUB, true},
    {TOK_ASTERISK, OP_MULTIPLY, true},
    {TOK_LOGICAL_AND, OP_LOGICAL_AND, true},
    {TOK_ADD_ASSIGN, OP_SET, true},
    {TOK_EQUALS, OP_EQUALS, true},
    {TOK_LESS_THAN, OP_LESS_THAN, true},
    {TOK_LESS_THAN_EQUAL, OP_LESS_THAN_EQUAL, true},
    {TOK_GREATER_THAN, OP_GREATER_THAN, true},
    {TOK_GREATER_THAN_EQUAL, OP_GREATER_THAN_EQUAL, true},
    {TOK_INCREMENT, OP_INCREMENT, false},
    {TOK_DOT, OP_REFERENCE, true},
};

#define EXPECT(_name, _expectType, _expectStr) \
    token = nextToken(); \
    if (token->type != _expectType) \
    { \
        printf("%s: " _name ": Expected " _expectStr ", got %s\n", __PRETTY_FUNCTION__, token->string.c_str()); \
        delete code; \
        return NULL; \
    }

#define EXPECT_BRACKET_LEFT(_name) EXPECT(_name, TOK_BRACKET_LEFT, "(")
#define EXPECT_BRACKET_RIGHT(_name) EXPECT(_name, TOK_BRACKET_RIGHT, ")")
#define EXPECT_BRACE_LEFT(_name) EXPECT(_name, TOK_BRACE_LEFT, "{")
#define EXPECT_BRACE_RIGHT(_name) EXPECT(_name, TOK_BRACE_RIGHT, "}")

Parser::Parser(Context* context)
{
    m_context = context;
}

Parser::~Parser()
{
}

Token* Parser::nextToken()
{
    int pos = m_pos++;
    Token* result = &(m_tokens[pos]);
#ifdef DEBUG_PARSER_DETAILED
    printf("Parser::nextToken: %d: %d: %s\n", pos, result->type, result->string.c_str());
#endif
    return result;
}

bool Parser::parse(vector<Token> tokens, bool addToExisting)
{
    m_pos = 0;
    m_tokens = tokens;

    Token* token;
    while (m_pos < m_tokens.size())
    {
        token = nextToken();

        if (token->type == TOK_CLASS)
        {
            Class* clazz;
            clazz = parseClass(addToExisting);
            if (clazz == NULL)
            {
                return false;
            }
        }
        else if (token->type == TOK_IMPORT)
        {
            Identifier id;
            bool res = parseIdentifier(id);
            if (!res)
            {
                return false;
            }

            string end = id.end();
            Class* clazz = m_context->getRuntime()->findClass(m_context, id.toString(), true);

#ifdef DEBUG_PARSER
            printf("Parser::parser: import: %s: %s -> %p\n", id.toString().c_str(), end.c_str(), clazz);
#endif
            if (clazz == NULL)
            {
                printf("Parser::parser: import: Imported class not found! %s\n", id.toString().c_str());
                return false;
            }
            m_imports.insert(make_pair(end, clazz));

            token = nextToken();
            if (token->type != TOK_SEMICOLON)
            {
                printf("Parser::parser: ERROR: Expected ;, got %s\n", token->string.c_str());
                return false;
            }
        }
        else
        {
            printf("Parser::parser: ERROR: Unexpected symbol: %s (%d)\n", token->string.c_str(), token->type);
            return false;
        }
    }
    return true;
}

Class* Parser::parseClass(bool addToExisting)
{
    Token* token;

    Identifier classId;
    bool res = parseIdentifier(classId);
    if (!res)
    {
        return NULL;
    }

    string name = classId.toString();
    Class* superClass = NULL;

    token = nextToken();
    if (token->type == TOK_EXTENDS)
    {
        token = nextToken();
        if (token->type != TOK_IDENTIFIER)
        {
            printf("Parser::parseClass: Expected class name\n");
            return NULL;
        }
        superClass = m_context->getRuntime()->findClass(m_context, token->string, true);
#ifdef DEBUG_PARSER
        printf("Parser::parseClass: Class extends: %s: %p\n", token->string.c_str(), superClass);
#endif
        if (superClass == NULL)
        {
            printf("Parser::parseClass: Unable to find super class: %s\n", token->string.c_str());
            return NULL;
        }

        token = nextToken();
    }
    else
    {
        superClass = m_context->getRuntime()->getObjectClass();
#ifdef DEBUG_PARSER
        printf("Parser::parseClass: Class extends: system.lang.Object: %p\n", superClass);
#endif
    }

    if (token->type != TOK_BRACE_LEFT)
    {
        printf("Parser::parseClass: Expected {\n");
        return NULL;
    }

#ifdef DEBUG_PARSER
    printf("Parser::parseClass: Class name: %s\n", name.c_str());
#endif

    bool existing = false;
    Class* clazz = NULL;
    if (addToExisting)
    {
        clazz = m_context->getRuntime()->findClass(m_context, name, false);
        if (clazz != NULL)
        {
            existing = true;
        }
    }

    if (!existing)
    {
        clazz = new Class(superClass, name);
    }

    while (m_pos < m_tokens.size())
    {
        token = nextToken();
        //printf("Parser::parseClass: %d: token=%d (%s)\n", m_pos, token->type, token->string.c_str());
        if (token->type == TOK_BRACE_RIGHT)
        {
            break;
        }
        else if (token->type == TOK_VAR)
        {
            token = nextToken();
            string varName = token->string;
#ifdef DEBUG_PARSER
            printf("Parser::parseClass: Variable: %s\n", varName.c_str());
#endif
            clazz->addField(varName);

            token = nextToken();
            if (token->type != TOK_SEMICOLON)
            {
                printf("Parser::parseClass: Expected = or ;\n");
                delete clazz;
                return NULL;
            }
        }
        else if (token->type == TOK_FUNCTION)
        {
            token = nextToken();
            string funcName = token->string;
#ifdef DEBUG_PARSER
            printf("Parser::parseClass: Function: %s\n", funcName.c_str());
#endif

            Function* function = parseFunction(clazz);
            if (function == NULL)
            {
                delete clazz;
                return NULL;
            }

            clazz->addMethod(funcName, function);
        }
        else
        {
            printf("Parser::parseClass: ERROR: Unexpected token: %s\n", token->string.c_str());
            delete clazz;
            return NULL;
        }
    }

    if (!existing)
    {
        m_context->getRuntime()->addClass(m_context, clazz);
    }

    return clazz;
}

Function* Parser::parseFunction(Class* clazz)
{
    vector<Token*> paramTokens;
    bool res = parseList(paramTokens, TOK_IDENTIFIER);
    if (!res)
    {
        if (clazz != NULL)
        {
            delete clazz;
        }
        return NULL;
    }

    vector<string> args;
    vector<Token*>::iterator it;
    for (it = paramTokens.begin(); it != paramTokens.end(); it++)
    {
        args.push_back((*it)->string);
#ifdef DEBUG_PARSER
        printf("Parser::parseClass: Function: arg: %s\n", (*it)->string.c_str());
#endif
    }

    ScriptFunction* function = new ScriptFunction(clazz, args);

    Token* token = nextToken();
    if (token->type == TOK_STATIC)
    {
#ifdef DEBUG_PARSER
        printf("Parser::parseClass: Function: Static function!\n");
#endif
        function->setStatic(true);
        token = nextToken();
    }
    if (token->type != TOK_BRACE_LEFT)
    {
        printf("Parser::parseClass: Function: Expected {\n");
        return NULL;
    }

    CodeBlock* code;
    code = parseCodeBlock(function);
    if (code == NULL)
    {
        return NULL;
    }

    code->setStartingVarId(paramTokens.size() + 1);

    resolveTypes();

    function->setCode(code);

#ifdef DEBUG_PARSER
    printf("Parser::parseClass: body:\n%s\n", code->toString().c_str());
#endif

    return function;
}

bool Parser::resolveTypes()
{
    vector<Expression*>::iterator exprIt;
    for (exprIt = m_expressions.begin(); exprIt != m_expressions.end(); exprIt++)
    {
        Expression* expr = *exprIt;
#ifdef DEBUG_PARSER
        printf("Parser::resolveTypes: resolve: type=%d\n", expr->type);
#endif
        if (expr->type == EXPR_OPER)
        {
            OperationExpression* opExpr = (OperationExpression*)expr;
#ifdef DEBUG_PARSER
            printf("Parser::resolveTypes: resolve: OPER: opType=%d, value type=%d\n", opExpr->operType, opExpr->valueType);
#endif
            if (opExpr->valueType != VALUE_UNKNOWN)
            {
#ifdef DEBUG_PARSER
                printf("Parser::resolveTypes: resolve:  -> left type=%d\n", opExpr->left->type);
#endif
                if (opExpr->operType == OP_SET && opExpr->left->type == EXPR_VAR)
                {
                    VarExpression* varExpr = (VarExpression*)opExpr->left;
#ifdef DEBUG_PARSER
                    printf("Parser::resolveTypes: resolve: OPER SET: var=%s\n", varExpr->var.c_str());
#endif
                        int varId = varExpr->block->getVarId(varExpr->var);
#ifdef DEBUG_PARSER
                        printf(
                            "Parser::resolveTypes: var %p: %s, id=%d, type=%d\n",
varExpr,
                            varExpr->var.c_str(),
                            varId,
                            opExpr->valueType);
#endif
                        if (varId != -1)
                        {
                            varExpr->block->setVarType(varId, opExpr->valueType);
                        }
                }
            }
        }
    }


    for (exprIt = m_expressions.begin(); exprIt != m_expressions.end(); exprIt++)
    {
        Expression* expr = *exprIt;
        if (expr->type == EXPR_VAR && expr->parent != NULL)
        {
            if (expr->parent->type == EXPR_OPER && ((OperationExpression*)(expr->parent))->operType == OP_REFERENCE)
            {
                continue;
            }
            VarExpression* varExpr = (VarExpression*)expr;
            int varId = varExpr->block->getVarId(varExpr->var);
            if (varId == -1)
            {
#ifdef DEBUG_PARSER
                printf(
                    "Parser::resolveTypes: VAR %p: %s, id=%d\n",
                    varExpr,
                    varExpr->var.c_str(),
                    varId);
#endif
                continue;
            }
            ValueType varType = varExpr->block->getVarType(varId);
#ifdef DEBUG_PARSER
            printf(
                "Parser::resolveTypes: VAR %p: %s, id=%d, type=%d\n",
                varExpr,
                varExpr->var.c_str(),
                varId,
                varType);
#endif
            varExpr->valueType = varType;
        }
    }


    for (exprIt = m_expressions.begin(); exprIt != m_expressions.end(); exprIt++)
    {
        Expression* expr = *exprIt;
        if (expr->type == EXPR_OPER)
        {
            OperationExpression* opExpr = (OperationExpression*)expr;
            opExpr->resolveType();
        }
    }
 
    return true;
}

CodeBlock* Parser::parseCodeBlock(ScriptFunction* function)
{
    CodeBlock* code = new CodeBlock();
    code->m_function = function;

    while (moreTokens())
    {
        Token* token = nextToken();
        if (token->type == TOK_BRACE_RIGHT)
        {
            return code;
        }
        else if (token->type == TOK_VAR)
        {
            token = nextToken();
            if (token->type != TOK_IDENTIFIER)
            {
                printf("Parser::parseCodeBlock: VAR: Expected variable name, got %s\n", token->string.c_str());
                delete code;
                return NULL;
            }
#ifdef DEBUG_PARSER
            printf("Parser::parseCodeBlock: VAR: name=%s\n", token->string.c_str());
#endif
            code->m_vars.push_back(token->string);

            // TODO: Handle assignment from var definition

            token = nextToken();

            if (token->type == TOK_ASSIGN)
            {
#ifdef DEBUG_PARSER
                printf("Parser::parseClass: VAR equals!\n");
#endif

                // Reverse back up to the variable name and pretend it was just an assignment!
                m_pos -= 2;
                Expression* expr = parseExpression(code);
                if (expr == NULL)
                {
                    return NULL;
                }
                code->m_code.push_back(expr);

                token = nextToken();
            }
            if (token->type != TOK_SEMICOLON)
            {
                printf("Parser::parseCodeBlock: VAR: Expected ;, got %s\n", token->string.c_str());
                delete code;
                return NULL;
            }
        }
        else if (token->type == TOK_IF)
        {
            EXPECT_BRACKET_LEFT("IF");

            IfExpression* ifExpr = new IfExpression(code);
            m_expressions.push_back(ifExpr);

            ifExpr->testExpr = parseExpression(code);
            if (ifExpr->testExpr == NULL)
            {
                delete code;
                return NULL;
            }

            EXPECT_BRACKET_RIGHT("IF");
            EXPECT_BRACE_LEFT("IF");

            ifExpr->trueBlock = parseCodeBlock(function);
            if (ifExpr->trueBlock == NULL)
            {
                delete code;
                return NULL;
            }
            ifExpr->trueBlock->m_parent = code;
            code->m_childBlocks.push_back(ifExpr->trueBlock);

            token = nextToken();
            if (token->type == TOK_ELSE)
            {
                EXPECT_BRACE_LEFT("IF");

                ifExpr->falseBlock = parseCodeBlock(function);
                if (ifExpr->trueBlock == NULL)
                {
                    delete code;
                    return NULL;
                }
                ifExpr->falseBlock->m_parent = code;
                code->m_childBlocks.push_back(ifExpr->falseBlock);
            }
            else
            {
                m_pos--;
            }
            code->m_code.push_back(ifExpr);
        }
        else if (token->type == TOK_FOR)
        {
            token = nextToken();
            if (token->type != TOK_BRACKET_LEFT)
            {
                printf("Parser::parseCodeBlock: FOR: Expected (, got %s\n", token->string.c_str());
                delete code;
                return NULL;
            }

            ForExpression* forExpr = new ForExpression(code);
            m_expressions.push_back(forExpr);

            forExpr->initExpr = parseExpression(code);
            if (forExpr->initExpr == NULL)
            {
                delete code;
                return NULL;
            }
            forExpr->initExpr->parent = forExpr;
#ifdef DEBUG_PARSER
            printf("Parser::parseCodeBlock: FOR: Init Expression: %s\n", forExpr->initExpr->toString().c_str());
#endif

            token = nextToken();
            if (token->type != TOK_SEMICOLON)
            {
                printf("Parser::parseCodeBlock: FOR: Expected ;, got %s\n", token->string.c_str());
                delete code;
                return NULL;
            }

            forExpr->testExpr = parseExpression(code);
            if (forExpr->testExpr == NULL)
            {
                delete code;
                return NULL;
            }
            forExpr->testExpr->parent = forExpr;
#ifdef DEBUG_PARSER
            printf("Parser::parseCodeBlock: FOR: Test Expression: %s\n", forExpr->testExpr->toString().c_str());
#endif

            token = nextToken();
            if (token->type != TOK_SEMICOLON)
            {
                printf("Parser::parseCodeBlock: FOR: Expected ;, got %s\n", token->string.c_str());
                delete code;
                return NULL;
            }

            forExpr->incExpr = parseExpression(code);
            if (forExpr->incExpr == NULL)
            {
                delete code;
                return NULL;
            }
            forExpr->incExpr->parent = forExpr;

#ifdef DEBUG_PARSER
            printf("Parser::parseCodeBlock: FOR: Inc Expression: %s\n", forExpr->incExpr->toString().c_str());
#endif

            token = nextToken();
            if (token->type != TOK_BRACKET_RIGHT)
            {
                printf("Parser::parseCodeBlock: FOR: Expected ), got %s\n", token->string.c_str());
                delete code;
                return NULL;
            }

            token = nextToken();
            if (token->type != TOK_BRACE_LEFT)
            {
                printf("Parser::parseCodeBlock: FOR: Expected {, got %s\n", token->string.c_str());
                delete code;
                return NULL;
            }


#ifdef DEBUG_PARSER
            printf("Parser::parseCodeBlock: FOR: Parsing Body...\n");
#endif
            forExpr->body = parseCodeBlock(function);
            if (forExpr->body == NULL)
            {
                delete code;
                return NULL;
            }
            forExpr->body->m_parent = code;
            code->m_childBlocks.push_back(forExpr->body);
#ifdef DEBUG_PARSER
            printf("Parser::parseCodeBlock: FOR: Got body!\n");
#endif

            code->m_code.push_back(forExpr);
        }
        else if (token->type == TOK_WHILE)
        {
            token = nextToken();
            if (token->type != TOK_BRACKET_LEFT)
            {
                printf("Parser::parseCodeBlock: WHILE: Expected (, got %s\n", token->string.c_str());
                delete code;
                return NULL;
            }

// A While expression is just a simple For loop
            ForExpression* forExpr = new ForExpression(code);
            m_expressions.push_back(forExpr);

            forExpr->initExpr = NULL;
            forExpr->incExpr = NULL;

            forExpr->testExpr = parseExpression(code);
            if (forExpr->testExpr == NULL)
            {
                delete code;
                return NULL;
            }
            forExpr->testExpr->parent = forExpr;
#ifdef DEBUG_PARSER
            printf("Parser::parseCodeBlock: WHILE: Test Expression: %s\n", forExpr->testExpr->toString().c_str());
#endif

            token = nextToken();
            if (token->type != TOK_BRACKET_RIGHT)
            {
                printf("Parser::parseCodeBlock: WHILE: Expected ), got %s\n", token->string.c_str());
                delete code;
                return NULL;
            }

            token = nextToken();
            if (token->type != TOK_BRACE_LEFT)
            {
                printf("Parser::parseCodeBlock: WHILE: Expected {, got %s\n", token->string.c_str());
                delete code;
                return NULL;
            }

#ifdef DEBUG_PARSER
            printf("Parser::parseCodeBlock: WHILE: Parsing Body...\n");
#endif
            forExpr->body = parseCodeBlock(function);
            if (forExpr->body == NULL)
            {
                delete code;
                return NULL;
            }

            forExpr->body->m_parent = code;
            code->m_childBlocks.push_back(forExpr->body);
#ifdef DEBUG_PARSER
            printf("Parser::parseCodeBlock: WHILE: Got body!\n");
#endif

            code->m_code.push_back(forExpr);
        }
        else if (token->type == TOK_RETURN)
        {
#ifdef DEBUG_PARSER
            printf("Parser::parseCodeBlock: RETURN\n");
#endif
            ReturnExpression* retExpr = new ReturnExpression(code);
            m_expressions.push_back(retExpr);
            code->m_code.push_back(retExpr);
            token = nextToken();
            if (token->type == TOK_SEMICOLON)
            {
                retExpr->returnValue = NULL;
            }
            else
            {
                m_pos--;
                retExpr->returnValue = parseExpression(code);

                token = nextToken();
                if (token->type != TOK_SEMICOLON)
                {
                    printf("Parser::parseCodeBlock: RETURN: Expected ;, got %s\n", token->string.c_str());
                    return NULL;
                }
            }
        }
        else
        {
            // Expression ?
            m_pos--; // Rewind

            Expression* expression;
            expression = parseExpression(code);
            if (expression == NULL)
            {
                delete code;
                return NULL;
            }
#ifdef DEBUG_PARSER
            printf("Parser::parseCodeBlock: Expression: %s\n", expression->toString().c_str());
#endif

            token = nextToken();
            if (token->type != TOK_SEMICOLON)
            {
                printf("Parser::parseCodeBlock: EXPRESSION: Expected ;, got %s\n", token->string.c_str());
                delete code;
                return NULL;
            }
            code->m_code.push_back(expression);
        }
    }
    return code;
}

Expression* Parser::parseExpression(CodeBlock* code)
{
    Expression* expression;

    bool res;
    //printf("Parser::parseExpression: Here\n");

    Token* token = nextToken();
    if (token->type == TOK_NEW)
    {
        Identifier id;

        res = parseIdentifier(id);
        if (!res)
        {
            return NULL;
        }
#ifdef DEBUG_PARSER
        printf("Parser::parseExpression: NEW: class: %s\n", id.toString().c_str());
#endif

        vector<Expression*> newParams;
        res = parseExpressionList(code, newParams);
        if (!res)
        {
            printf("Parser::parseExpression: NEW: Failed to parse args?\n");
            return NULL;
        }

        NewExpression* newExpr = new NewExpression(code);
        m_expressions.push_back(newExpr);
        newExpr->clazz = id;
        newExpr->parameters = newParams;
        newExpr->valueType = VALUE_OBJECT;
        expression = newExpr;
    }
    else if (token->type == TOK_IDENTIFIER)
    {
/*
        m_pos--; // Rewind

        Identifier id;
        res = parseIdentifier(id);
        if (!res)
        {
            return NULL;
        }
*/

        string id = token->string;
        Class* clazz = NULL;

#ifdef DEBUG_PARSER
        printf("Parser::parseExpression: Got identifer: %s\n", id.c_str());
#endif

        token = nextToken();
#ifdef DEBUG_PARSER
        printf("Parser::parseExpression: checking for class names...\n");
#endif
        if (token->type == TOK_DOT)
        {
#ifdef DEBUG_PARSER
            printf("Parser::parseExpression:  -> got a dot!\n");
#endif
            string clazzname = id;
            token = nextToken();

            // Check for imported classes
            // TODO: These will override any local vars
            //       Is that right?
            map<string, Class*>::iterator it = m_imports.find(clazzname);
            if (it != m_imports.end())
            {
                clazz = it->second;
#ifdef DEBUG_PARSER
                printf(
                    "Parser::parseExpression: Found imported class: %s -> %s\n",
                    clazzname.c_str(),
                    clazz->getName().c_str());
#endif
                m_pos -= 2;
            }
            int count = 2;
            while (clazz == NULL && token->type == TOK_IDENTIFIER)
            {
#ifdef DEBUG_PARSER
                printf("Parser::parseExpression:  -> got an identifier: %s\n", token->string.c_str());
#endif
                clazzname += ".";
                clazzname += token->string;
                clazz = m_context->getRuntime()->findClass(m_context, clazzname, false);
#ifdef DEBUG_PARSER
                printf("Parser::parseExpression:  -> class? %s = %p\n", clazzname.c_str(), clazz);
#endif
                if (clazz != NULL)
                {
                    break;
                }
                token = nextToken();
                count++;
                if (token->type != TOK_DOT)
                {
                    break;
                }
                token = nextToken();
                count++;
            }

            if (clazz != NULL)
            {
                token = nextToken(); // DOT
                token = nextToken();
                id = token->string;
            }
            else
            {
                m_pos -= count;
            }
            token = nextToken();
        }

        if (token->type == TOK_BRACKET_LEFT)
        {
#ifdef DEBUG_PARSER
            printf("Parser::parseExpression: -> function call!\n");
#endif
            m_pos--;
            vector<Expression*> params;
            res = parseExpressionList(code, params);
            if (!res)
            {
            printf("Parser::parseExpression: -> Error in expression list\n");
                return NULL;
            }
#ifdef DEBUG_PARSER
            printf("Parser::parseExpression: -> function call to %s class=%p\n", id.c_str(), clazz);
#endif
            CallExpression* callExpr = new CallExpression(code);
            m_expressions.push_back(callExpr);
            callExpr->clazz = clazz;
            callExpr->function = id;
            callExpr->parameters = params;
            callExpr->valueType = VALUE_UNKNOWN;
            expression = callExpr;
        }
        else if (token->type == TOK_SQUARE_BRACKET_LEFT)
        {
            ArrayExpression* arrExpr = new ArrayExpression(code);
            m_expressions.push_back(arrExpr);
            arrExpr->clazz = clazz;
            arrExpr->var = id;
            arrExpr->indexExpr = parseExpression(code);
#ifdef DEBUG_PARSER
            printf("Parser::parseExpression: -> Array: %s[%s]\n", id.c_str(), arrExpr->indexExpr->toString().c_str());
#endif
            expression = arrExpr;
            token = nextToken();
            if (token->type != TOK_SQUARE_BRACKET_RIGHT)
            {
                printf("Parser::parseExpression: ERROR: Array: Expected ], got %s\n", token->string.c_str());
                return NULL;
            }
        }
        else
        {
            VarExpression* varExpr = new VarExpression(code);
            m_expressions.push_back(varExpr);
            varExpr->clazz = clazz;
            varExpr->var = id;
#ifdef DEBUG_PARSER
            printf("Parser::parseExpression: -> Variable!\n");
#endif
            expression = varExpr;
            m_pos--;
        }
    }
    else if (token->type == TOK_FUNCTION)
    {
        Function* func = parseFunction(NULL);
#ifdef DEBUG_PARSER
        printf("Parser::parseExpression: Function: %p\n", func);
#endif
        FunctionExpression* funcExpr = new FunctionExpression(code);
        m_expressions.push_back(funcExpr);
        funcExpr->function = func;
        funcExpr->valueType = VALUE_OBJECT;
        expression = funcExpr;
    }
    else if (token->type == TOK_STRING)
    {
        StringExpression* strExpr = new StringExpression(code);
        m_expressions.push_back(strExpr);
        strExpr->str = token->string;
        strExpr->valueType = VALUE_OBJECT;
        expression = strExpr;
    }
    else if (token->type == TOK_INTEGER)
    {
        IntegerExpression* intExpr = new IntegerExpression(code);
        m_expressions.push_back(intExpr);
        intExpr->i = token->i;
        intExpr->valueType = VALUE_INTEGER;
        expression = intExpr;
    }
    else if (token->type == TOK_DOUBLE)
    {
        DoubleExpression* doubleExpr = new DoubleExpression(code);
        m_expressions.push_back(doubleExpr);
        doubleExpr->d = token->d;
        doubleExpr->valueType = VALUE_DOUBLE;
        expression = doubleExpr;
    }
    else if (token->type == TOK_BRACKET_LEFT)
    {
#ifdef DEBUG_PARSER
        printf("Parser::parseExpression: Left bracket!\n");
#endif
        expression = parseExpression(code);
        Token* token = nextToken();
        if (token->type != TOK_BRACKET_RIGHT)
        {
            printf("Parser::parseExpression: Expected ), got %s\n", token->string.c_str());
            return NULL;
        }
    }
    else
    {
        printf("Parser::parseExpression: Unhandled token type: %d: %s\n", token->type, token->string.c_str());
        return NULL;
    }

    // See if we have an operation
    token = nextToken();

    OpTable* entry = NULL;
    int i;
    for (i = 0; i < sizeof(opTable) / sizeof(OpTable); i++)
    {
        if (token->type == opTable[i].token)
        {
            entry = &(opTable[i]);
        }
    }

    if (entry != NULL)
    {
        OperationExpression* opExpr = new OperationExpression(code);
        m_expressions.push_back(opExpr);
        opExpr->operType = entry->oper;
        opExpr->left = expression;
        opExpr->left->parent = opExpr;

        if (token->type == TOK_ADD_ASSIGN)
        {
            // l += r -> l = (l + r)

            // Make a copy of the left hand side rather than reuse or cleaning up
            // at the end tries to free it twice!
            Expression* copy;
            if (expression->type == EXPR_VAR)
            {
                VarExpression* varExpr = new VarExpression(code);
                m_expressions.push_back(varExpr);
                varExpr->var = ((VarExpression*)expression)->var;
                copy = varExpr;
            }
            else
            {
                printf("Parser::parseExpression: TOK_ADD_ASSIGN: Unhandled left type: %d: %s\n", expression->type, expression->toString().c_str());
                return NULL;
            }

            OperationExpression* addExpr = new OperationExpression(code);
            m_expressions.push_back(addExpr);
            addExpr->operType = OP_ADD;
            addExpr->left = copy;
            addExpr->left->parent = addExpr;
            addExpr->right = parseExpression(code);
            if (addExpr->right == NULL)
            {
                return NULL;
            }
            addExpr->right->parent = addExpr;
            addExpr->resolveType();

            opExpr->right = addExpr;
            addExpr->parent = opExpr;
        }
        else if (entry->hasRightExpr)
        {
            opExpr->right = parseExpression(code);
            if (opExpr->right == NULL)
            {
                return NULL;
            }
            opExpr->right->parent = opExpr;
        }
        else
        {
            opExpr->right = NULL;
        }

        if (opExpr->operType == OP_REFERENCE)
        {
            // To ensure the references get loaded in the correct order,
            // we must swap the left and right!
            // There must be a better, more correct way of doing this?
            Expression* tmp = opExpr->left;
            opExpr->left = opExpr->right;
            opExpr->right = tmp;
        }

        if (opExpr->left != NULL && opExpr->right != NULL)
        {
            if (opExpr->left->type == EXPR_INTEGER && opExpr->right->type == EXPR_INTEGER)
            {
#ifdef DEBUG_PARSER
                printf("Parser::parseExpression: TODO: Integers on both sides, optimise away!\n");
#endif
            }
            else if (opExpr->left->type == EXPR_DOUBLE && opExpr->right->type == EXPR_DOUBLE)
            {
                switch (opExpr->operType)
                {
                    case OP_MULTIPLY:
                    {
                        DoubleExpression* dblExpr = new DoubleExpression(code);
                        dblExpr->d = ((DoubleExpression*)opExpr->left)->d;
                        dblExpr->d *= ((DoubleExpression*)opExpr->left)->d;
                        expression = dblExpr;
#ifdef DEBUG_PARSER
                        printf("Parser::parseExpression: Doubles on both sides, optimised! (%0.2f)\n", dblExpr->d);
#endif
                    } break;
                    default:
#ifdef DEBUG_PARSER
                        printf("Parser::parseExpression: TODO: Doubles on both sides, optimise away!\n");
#endif
                        break;
                }
            }
        }

        expression = opExpr;
    }
    else
    {
        // Not for us to deal with, then!
        m_pos--;
    }

    return expression;
}

bool Parser::parseIdentifier(Identifier& id)
{
    //printf("Parser::parseIdentifier: Here\n");

    Token* token = nextToken();
    if (token->type != TOK_IDENTIFIER)
    {
        printf("Parser::parseIdentifier: Invalid identifier\n");
        return false;
    }
    id.identifier.push_back(token->string);

    while (moreTokens())
    {
        Token* token = nextToken();
        if (token->type != TOK_DOT)
        {
            m_pos--; // Rewind
            return true;
        }

        token = nextToken();
        if (token->type != TOK_IDENTIFIER)
        {
            printf("Parser::parseIdentifier: Invalid identifier\n");
            return false;
        }
        id.identifier.push_back(token->string);
    }

    //printf("Parser::parseIdentifier: Done\n");
    return true;
}

bool Parser::parseList(vector<Token*>& list, TokenType type)
{
    //printf("Parser::parseList: Here\n");
    Token* token = nextToken();
    if (token->type != TOK_BRACKET_LEFT)
    {
        printf("Parser::parseList: Expected (\n");
        return false;
    }

    while (m_pos < m_tokens.size())
    {
        token = nextToken();
        if (token->type == TOK_BRACKET_RIGHT)
        {
            break;
        }

        if (token->type == type)
        {
#ifdef DEBUG_PARSER
            printf("Parser::parseList: item: %s\n", token->string.c_str());
#endif
            list.push_back(token);
        }
        else
        {
            printf("Parser::parseList: Expected identifier or )\n");
            return false;
        }

        token = nextToken();
        if (token->type == TOK_BRACKET_RIGHT)
        {
            break;
        }
        else if (token->type != TOK_COMMA)
        {
            printf("Parser::parseList: Function: Expected , or )\n");
            return false;
        }
    }
//printf("Parser::parseList: Done\n");
    return true;
}

bool Parser::parseExpressionList(CodeBlock* code, vector<Expression*>& list)
{
    Token* token = nextToken();
    if (token->type != TOK_BRACKET_LEFT)
    {
        printf("Parser::parseExpressionList: Expected (\n");
        return false;
    }

    while (m_pos < m_tokens.size())
    {
        token = nextToken();
        if (token->type == TOK_BRACKET_RIGHT)
        {
            break;
        }

            m_pos--;
            Expression* childExp = parseExpression(code);
            if (childExp == NULL)
            {
                return false;
            }
            list.push_back(childExp);

        token = nextToken();
        if (token->type == TOK_BRACKET_RIGHT)
        {
            break;
        }
        else if (token->type != TOK_COMMA)
        {
            printf("Parser::parseList: Function: Expected , or )\n");
            return false;
        }
    }

    return true;
}

string Identifier::end()
{
    return identifier.at(identifier.size() - 1);
}

string Identifier::toString()
{
    string str = "";
    bool comma = false;
    vector<string>::iterator it;
    for (it = identifier.begin(); it != identifier.end(); it++)
    {
        if (comma)
        {
            str += ".";
        }
        comma = true;
        str += *it;
    }
    return str;
}

