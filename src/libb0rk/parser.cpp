/*
 *  b0rk - The b0rk Embeddable Runtime Environment
 *  Copyright (C) 2015, 2016 GeekProjects.com
 *
 *  This file is part of b0rk.
 *
 *  b0rk is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  b0rk is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with b0rk.  If not, see <http://www.gnu.org/licenses/>.
 */


#undef DEBUG_PARSER
#undef DEBUG_PARSER_DETAILED
#undef DEBUG_PARSER_TYPES

#include <stdio.h>
#include <stdlib.h>

#include <b0rk/parser.h>
#include <b0rk/utils.h>

#include "packages/system/lang/StringClass.h"

using namespace std;
using namespace b0rk;
using namespace Geek;

OpDesc opTable[] = {
    {TOK_ASSIGN, OP_SET, true, true},
    {TOK_PLUS, OP_ADD, true, true},
    {TOK_MINUS, OP_SUB, true, true},
    {TOK_NOT, OP_NOT, false, true},
    {TOK_ASTERISK, OP_MULTIPLY, true, true},
    {TOK_SLASH_FORWARD, OP_DIVIDE, true, true},
    {TOK_LOGICAL_AND, OP_LOGICAL_AND, true, true},
    {TOK_ADD_ASSIGN, OP_SET, true, true},
    {TOK_SUB_ASSIGN, OP_SET, true, true},
    {TOK_EQUALS, OP_EQUALS, true, true},
    {TOK_NOT_EQUAL, OP_NOT_EQUAL, true, true},
    {TOK_LESS_THAN, OP_LESS_THAN, true, true},
    {TOK_LESS_THAN_EQUAL, OP_LESS_THAN_EQUAL, true, true},
    {TOK_GREATER_THAN, OP_GREATER_THAN, true, true},
    {TOK_GREATER_THAN_EQUAL, OP_GREATER_THAN_EQUAL, true, true},
    {TOK_INCREMENT, OP_INCREMENT, true, false},
    {TOK_DECREMENT, OP_DECREMENT, true, false},
    {TOK_DOT, OP_REFERENCE, true, true},
};

#ifdef DEBUG_PARSER
#define EXPECT_ERROR(__name, __expectStr) \
        log(ERROR, "%s: " __name ": Line %d: Expected " __expectStr ", got %ls", __PRETTY_FUNCTION__, token->line, token->string.c_str());
#else
#define EXPECT_ERROR(__name, __expectStr) \
        log(ERROR, __name ": Line %d: Expected " __expectStr ", got %ls", token->line, token->string.c_str());
#endif

#define EXPECT(_name, _expectType, _expectStr) \
    token = nextToken(); \
    if (token->type != _expectType) \
    { \
        EXPECT_ERROR(_name, _expectStr); \
        return false; \
    }

#define EXPECT_BRACKET_LEFT(_name) EXPECT(_name, TOK_BRACKET_LEFT, "(")
#define EXPECT_BRACKET_RIGHT(_name) EXPECT(_name, TOK_BRACKET_RIGHT, ")")
#define EXPECT_BRACE_LEFT(_name) EXPECT(_name, TOK_BRACE_LEFT, "{")
#define EXPECT_BRACE_RIGHT(_name) EXPECT(_name, TOK_BRACE_RIGHT, "}")
#define EXPECT_SEMICOLON(_name) EXPECT(_name, TOK_SEMICOLON, ";")

Parser::Parser(Context* context) : Logger("Parser")
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
    log(DEBUG, "nextToken: %d: %d: %ls", pos, result->type, result->string.c_str());
#endif
    return result;
}

bool Parser::parse(vector<Token> tokens, bool addToExisting)
{
    m_pos = 0;
    m_tokens = tokens;

    Token* token;

    m_imports.insert(make_pair(L"Runtime", Identifier::makeIdentifier(L"system.lang.Runtime")));

    // Imports
    while (moreTokens())
    {
        token = nextToken();
        if (token->type == TOK_IMPORT)
        {
            Identifier id;
            bool res = parseIdentifier(id);
            if (!res)
            {
                return false;
            }

            wstring end = id.end();
            m_imports.insert(make_pair(end, id));

            EXPECT_SEMICOLON("import");
        }
        else
        {
            m_pos--;
            break;
        }
    }

    while (moreTokens())
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
        else
        {
            EXPECT_ERROR("class", "class");
            return false;
        }
    }

    resolveTypes();

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

    // Import ourselves!
    wstring end = classId.end();
    m_imports.insert(make_pair(end, classId));

    wstring name = classId.toString();
    Class* superClass = NULL;

    token = nextToken();
    if (token->type == TOK_EXTENDS)
    {
        Identifier superClassId;
        res = parseIdentifier(superClassId);
        if (!res)
        {
            log(ERROR, "parseClass: Unable to parse superclass");
            return NULL;
        }

        if (superClassId.identifier.size() == 1)
        {
            // Check for imported classes
            // TODO: These will override any local vars
            //       Is that right?
            map<wstring, Identifier>::iterator it = m_imports.find(superClassId.identifier.at(0));
            if (it != m_imports.end())
            {
#ifdef DEBUG_PARSER
                log(
                    DEBUG,
                    "parseClass: NEW: Found imported class: %ls -> %ls",
                    superClassId.identifier.at(0).c_str(),
                    it->second.toString().c_str());
#endif
                superClassId = it->second;
            }
         }

        superClass = m_context->getRuntime()->findClass(m_context, superClassId.toString(), true);
#ifdef DEBUG_PARSER
        log(DEBUG, "parseClass: Class extends: %ls: %p", token->string.c_str(), superClass);
#endif
        if (superClass == NULL)
        {
            log(ERROR, "parseClass: Unable to find super class: %s", Utils::wstring2string(superClassId.toString()).c_str());
            return NULL;
        }

        token = nextToken();
    }
    else
    {
        superClass = m_context->getRuntime()->getObjectClass();
#ifdef DEBUG_PARSER
        log(DEBUG, "parseClass: Class extends: system.lang.Object: %p", superClass);
#endif
    }

    if (token->type != TOK_BRACE_LEFT)
    {
        EXPECT_ERROR("class", "{");
        return NULL;
    }

#ifdef DEBUG_PARSER
    log(DEBUG, "parseClass: Class name: %ls", name.c_str());
#endif

    Class* clazz = m_context->getRuntime()->findClass(m_context, name, false);
    if (clazz != NULL)
    {
        if (clazz->getState() == PARSING)
        {
            return clazz;
        }

        if (!addToExisting)
        {
            log(ERROR, "Attempting to add to existing class?");
            return NULL;
        }
    }

    if (clazz == NULL)
    {
        clazz = new Class(superClass, name);
        m_context->getRuntime()->addClass(m_context, clazz);
    }
    clazz->setState(PARSING);

    while (moreTokens())
    {
        token = nextToken();
        if (token->type == TOK_BRACE_RIGHT)
        {
            break;
        }
        else if (token->type == TOK_VAR)
        {
            token = nextToken();
            wstring varName = token->string;
#ifdef DEBUG_PARSER
            log(DEBUG, "parseClass: Variable: %ls", varName.c_str());
#endif

            bool isStatic = false;
            Value staticValue;
            staticValue.type = VALUE_VOID;

            token = nextToken();
            if (token->type == TOK_STATIC)
            {
                isStatic = true;
                token = nextToken();
#ifdef DEBUG_PARSER
                log(DEBUG, "parseClass:  -> Static (next=%ls)", token->string.c_str());
#endif
                if (token->type == TOK_ASSIGN)
                {
                    token = nextToken();
#ifdef DEBUG_PARSER
                    log(DEBUG, "parseClass: Static Variable: Type=%d", token->type);
#endif
                    switch (token->type)
                    {
                        case TOK_STRING:
                            staticValue.type = VALUE_OBJECT;
                            staticValue.object = String::createString(m_context, token->string);
                            break;

                        case TOK_INTEGER:
                            staticValue.type = VALUE_INTEGER;
                            staticValue.i = token->i;
                            break;

                        case TOK_DOUBLE:
                            staticValue.type = VALUE_DOUBLE;
                            staticValue.i = token->d;
                            break;

                        default:
                            EXPECT_ERROR("var", "STRING, INTEGER or DOUBLE");
                            delete clazz;
                            return NULL;
                            break;
                    }
#ifdef DEBUG_PARSER
                    log(DEBUG, "parseClass: Static Variable: Value=%ls", staticValue.toString().c_str());
#endif
                    token = nextToken();
                }
            }

            if (!isStatic)
            {
                clazz->addField(varName);
            }
            else
            {
                int id = clazz->addStaticField(varName);
#ifdef DEBUG_PARSER
                log(DEBUG, "parseClass: Static Variable id=%d", id);
#endif
                if (staticValue.type != VALUE_VOID)
                {
                    clazz->setStaticField(id, staticValue);
                }
            }
            if (token->type != TOK_SEMICOLON)
            {
                EXPECT_ERROR("var", ";");
                delete clazz;
                return NULL;
            }
        }
        else if (token->type == TOK_FUNCTION)
        {
            token = nextToken();
            wstring funcName = token->string;
#ifdef DEBUG_PARSER
            log(DEBUG, "parseClass: Function: %ls", funcName.c_str());
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
            EXPECT_ERROR("class", "function or var");
            delete clazz;
            return NULL;
        }
    }

    clazz->setState(COMPLETE);

    return clazz;
}

Function* Parser::parseFunction(Class* clazz)
{
    vector<Token*> paramTokens;
    bool res = parseList(paramTokens, TOK_IDENTIFIER);
    if (!res)
    {
        return NULL;
    }

    vector<wstring> args;
    vector<Token*>::iterator it;
    for (it = paramTokens.begin(); it != paramTokens.end(); it++)
    {
        args.push_back((*it)->string);
#ifdef DEBUG_PARSER
        log(DEBUG, "parseFunction: Function: arg: %ls", (*it)->string.c_str());
#endif
    }

    ScriptFunction* function = new ScriptFunction(clazz, args);

    Token* token = nextToken();
    if (token->type == TOK_STATIC)
    {
#ifdef DEBUG_PARSER
        log(DEBUG, "parseFunction: Function: Static function!");
#endif
        function->setStatic(true);
        token = nextToken();
    }
    if (token->type != TOK_BRACE_LEFT)
    {
        EXPECT_ERROR("function", "{");
        return NULL;
    }

    CodeBlock* code;
    code = parseCodeBlock(function);
    if (code == NULL)
    {
        log(ERROR, "parseFunction: Failed to parse function body!");
        return NULL;
    }

    code->setStartingVarId(paramTokens.size() + 1);


    function->setCode(code);

#ifdef DEBUG_PARSER
    log(DEBUG, "parseFunction: body:\n%ls", code->toString().c_str());
#endif

    return function;
}

/*
 *    E
 *  /  \
 * L    R
 *
 *
 * If E is UNKNOWN and 
 */

bool Parser::resolveTypes()
{
    vector<Expression*>::iterator exprIt;
int changes = 0;

    /*
     * Infer types of OperationExpressions from their operands
     */
    for (exprIt = m_expressions.begin(); exprIt != m_expressions.end(); exprIt++)
    {
        Expression* expr = *exprIt;
#ifdef DEBUG_PARSER_TYPES
        log(DEBUG, "resolveTypes: resolve: type=%d", expr->type);
#endif
        if (expr->type == EXPR_OPER)
        {
            OperationExpression* opExpr = (OperationExpression*)expr;
#ifdef DEBUG_PARSER_TYPES
            log(DEBUG, "resolveTypes: resolve: OPER: opType=%d, value type=%d", opExpr->operType, opExpr->valueType);
#endif
            if (opExpr->valueType != VALUE_UNKNOWN)
            {
#ifdef DEBUG_PARSER_TYPES
                //log(DEBUG, "resolveTypes: resolve:  -> left type=%d", opExpr->left->type);
#endif
                if (opExpr->operType == OP_SET && opExpr->left->type == EXPR_VAR)
                {
                    VarExpression* varExpr = (VarExpression*)opExpr->left;
#ifdef DEBUG_PARSER_TYPES
                    log(DEBUG, "resolveTypes: resolve: OPER SET: var=%ls", varExpr->var.c_str());
#endif
                        int varId = varExpr->block->getVarId(varExpr->var);
#ifdef DEBUG_PARSER_TYPES
                        log(
                            DEBUG,
                            "resolveTypes: var %p: %ls, id=%d, type=%d",
                            varExpr,
                            varExpr->var.c_str(),
                            varId,
                            opExpr->valueType);
#endif
                        if (varId != -1)
                        {
                            varExpr->block->setVarType(varId, opExpr->valueType);
changes++;
                        }
                }
            }
            else if (opExpr->left != NULL && opExpr->right != NULL &&
                opExpr->left->valueType != VALUE_UNKNOWN &&
                opExpr->left->valueType == opExpr->right->valueType)
            {
                opExpr->valueType = opExpr->left->valueType;
changes++;
            }
        }
    }
#ifdef DEBUG_PARSER
    log(DEBUG, "resolveTypes: resolve: changes=%d", changes);
#endif

    /*
     * Attempt to find the type of var expressions from what they get assigned to
     */
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
#ifdef DEBUG_PARSER_TYPES
                log(
                    DEBUG,
                    "resolveTypes: VAR %p: %ls, id=%d",
                    varExpr,
                    varExpr->var.c_str(),
                    varId);
#endif
                continue;
            }
            ValueType varType = varExpr->block->getVarType(varId);
#ifdef DEBUG_PARSER_TYPES
            log(
                DEBUG,
                "resolveTypes: VAR %p: %ls, id=%d, type=%d",
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

CodeBlock* Parser::parseCodeBlock(ScriptFunction* function, bool single)
{
    bool success = true;

    pushDepth();

    CodeBlock* code = new CodeBlock();
    code->m_function = function;

    while (moreTokens())
    {
        bool end = false;
        success = parseStatement(code, end);
        if (!success || end)
        {
            break;
        }
    }

    if (success)
    {
#ifdef DEBUG_PARSER
        log(DEBUG, "parseCodeBlock: DONE: %ls", code->toString().c_str());
#endif
    }
    else
    {
        log(ERROR, "parseCodeBlock: Failed to parse code block");
        delete code;
        code = NULL;
    }

    popDepth();
    return code;
}

bool Parser::parseStatement(CodeBlock* code, bool& end)
{
    end = false;

    Token* token = nextToken();
    if (token->type == TOK_BRACE_RIGHT)
    {
        end = true;
    }
    else if (token->type == TOK_VAR)
    {
        EXPECT("var", TOK_IDENTIFIER, "identifier");

#ifdef DEBUG_PARSER
        log(DEBUG, "parseCodeBlock: VAR: name=%ls", token->string.c_str());
#endif
        code->m_vars.push_back(token->string);

        // TODO: Handle assignment from var definition

        token = nextToken();

        if (token->type == TOK_ASSIGN)
        {
#ifdef DEBUG_PARSER
            log(DEBUG, "parseCodeBlock: VAR equals!");
#endif

            // Reverse back up to the variable name and pretend it was just an assignment!
            m_pos -= 2;
            Expression* expr = parseExpression(code);
            if (expr == NULL)
            {
                return false;
            }
            code->m_code.push_back(expr);

            token = nextToken();
        }
        if (token->type != TOK_SEMICOLON)
        {
            EXPECT_ERROR("var", ";");
            return false;
        }
    }
    else if (token->type == TOK_IF)
    {
#ifdef DEBUG_PARSER
        log(DEBUG, "parseCodeBlock: *** IF ***");
#endif
        EXPECT_BRACKET_LEFT("IF");

        IfExpression* ifExpr = new IfExpression(code);
        m_expressions.push_back(ifExpr);

        ifExpr->testExpr = parseExpression(code);
        if (ifExpr->testExpr == NULL)
        {
            return false;
        }

        EXPECT_BRACKET_RIGHT("IF");
        EXPECT_BRACE_LEFT("IF");

#ifdef DEBUG_PARSER
        log(DEBUG, "Parser::parseCodeBlock: IF: TRUE BLOCK...");
#endif

        ifExpr->trueBlock = parseCodeBlock(code->m_function);
        if (ifExpr->trueBlock == NULL)
        {
            return false;
        }
        ifExpr->trueBlock->m_parent = code;
        code->m_childBlocks.push_back(ifExpr->trueBlock);

        token = nextToken();
        if (token->type == TOK_ELSE)
        {
#ifdef DEBUG_PARSER
            log(DEBUG, "parseCodeBlock: IF: ELSE...");
#endif
            token = nextToken();
            if (token->type != TOK_IF && token->type != TOK_BRACE_LEFT)
            {
                EXPECT_ERROR("else", "if or {");
                return false;
            }

            if (token->type == TOK_IF)
            {
                // We have ...else if (..)...
#ifdef DEBUG_PARSER
                log(DEBUG, "parseCodeBlock: IF: ELSE IF!");
#endif
                m_pos--;
                CodeBlock* elseIf = new CodeBlock();
                elseIf->m_function = code->m_function;

                // Parse it as a single IF statement
                bool end;
                bool res = parseStatement(elseIf, end);
                if (res)
                {
                    ifExpr->falseBlock = elseIf;
                }
                else
                {
                    delete elseIf;
                }
            }
            else
            {
                // Just an else
                ifExpr->falseBlock = parseCodeBlock(code->m_function);
            }

            if (ifExpr->falseBlock == NULL)
            {
                return false;
            }
            ifExpr->falseBlock->m_parent = code;
            code->m_childBlocks.push_back(ifExpr->falseBlock);
#ifdef DEBUG_PARSER
            log(DEBUG, "parseCodeBlock: IF: ELSE DONE: %ls", ifExpr->falseBlock->toString().c_str());
#endif

        }
        else
        {
            m_pos--;
        }
        code->m_code.push_back(ifExpr);
#ifdef DEBUG_PARSER
        log(DEBUG, "parseCodeBlock: *** IF: DONE ***");
#endif
    }
    else if (token->type == TOK_FOR)
    {
        EXPECT_BRACKET_LEFT("for");

        ForExpression* forExpr = new ForExpression(code);
        m_expressions.push_back(forExpr);

        forExpr->initExpr = parseExpression(code);
        if (forExpr->initExpr == NULL)
        {
            return false;
        }
        forExpr->initExpr->parent = forExpr;
#ifdef DEBUG_PARSER
        log(DEBUG, "parseCodeBlock: FOR: Init Expression: %ls", forExpr->initExpr->toString().c_str());
#endif

        EXPECT_SEMICOLON("for");

        forExpr->testExpr = parseExpression(code);
        if (forExpr->testExpr == NULL)
        {
            return false;
        }
        forExpr->testExpr->parent = forExpr;
#ifdef DEBUG_PARSER
        log(DEBUG, "parseCodeBlock: FOR: Test Expression: %ls", forExpr->testExpr->toString().c_str());
#endif

        EXPECT_SEMICOLON("for");

        forExpr->incExpr = parseExpression(code);
        if (forExpr->incExpr == NULL)
        {
            return false;
        }
        forExpr->incExpr->parent = forExpr;

#ifdef DEBUG_PARSER
        log(DEBUG, "parseCodeBlock: FOR: Inc Expression: %ls", forExpr->incExpr->toString().c_str());
#endif

        EXPECT_BRACKET_RIGHT("for");
        EXPECT_BRACE_LEFT("for");

#ifdef DEBUG_PARSER
        log(DEBUG, "parseCodeBlock: FOR: Parsing Body...");
#endif
        forExpr->body = parseCodeBlock(code->m_function);
        if (forExpr->body == NULL)
        {
            return false;
        }
        forExpr->body->m_parent = code;
        code->m_childBlocks.push_back(forExpr->body);
#ifdef DEBUG_PARSER
        log(DEBUG, "parseCodeBlock: FOR: Got body!");
#endif

        code->m_code.push_back(forExpr);
    }
    else if (token->type == TOK_WHILE)
    {
        // A While expression is just a simple For loop

        EXPECT_BRACKET_LEFT("for");

        ForExpression* forExpr = new ForExpression(code);
        m_expressions.push_back(forExpr);

        forExpr->initExpr = NULL;
        forExpr->incExpr = NULL;

        forExpr->testExpr = parseExpression(code);
        if (forExpr->testExpr == NULL)
        {
            return false;
        }
        forExpr->testExpr->parent = forExpr;
#ifdef DEBUG_PARSER
        log(DEBUG, "parseCodeBlock: WHILE: Test Expression: %ls", forExpr->testExpr->toString().c_str());
#endif

        EXPECT_BRACKET_RIGHT("for");
        EXPECT_BRACE_LEFT("for");

#ifdef DEBUG_PARSER
        log(DEBUG, "parseCodeBlock: WHILE: Parsing Body...");
#endif
        forExpr->body = parseCodeBlock(code->m_function);
        if (forExpr->body == NULL)
        {
            return false;
        }

        forExpr->body->m_parent = code;
        code->m_childBlocks.push_back(forExpr->body);
#ifdef DEBUG_PARSER
        log(DEBUG, "parseCodeBlock: WHILE: Got body!");
#endif

        code->m_code.push_back(forExpr);
    }
    else if (token->type == TOK_RETURN)
    {
#ifdef DEBUG_PARSER
        log(DEBUG, "parseCodeBlock: RETURN");
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

            EXPECT_SEMICOLON("return");
        }
    }
    else if (token->type == TOK_TRY)
    {
#ifdef DEBUG_PARSER
        log(DEBUG, "parseCodeBlock: Got try!");
#endif

        TryExpression* tryExpr = new TryExpression(code);
        m_expressions.push_back(tryExpr);
        code->m_code.push_back(tryExpr);

        EXPECT_BRACE_LEFT("try");

#ifdef DEBUG_PARSER
        log(DEBUG, "parseCodeBlock: TRY: Parsing try body...");
#endif
        tryExpr->tryBlock = parseCodeBlock(code->m_function);
        if (tryExpr->tryBlock == NULL)
        {
            return false;
        }
        tryExpr->tryBlock->m_parent = code;

        EXPECT("try", TOK_CATCH, "catch");
        EXPECT_BRACKET_LEFT("try");

        EXPECT("try", TOK_IDENTIFIER, "identifier");
        tryExpr->exceptionVar = token->string;

#ifdef DEBUG_PARSER
        log(DEBUG, "parseCodeBlock: TRY: catch var name=%ls", token->string.c_str());
#endif
        EXPECT_BRACKET_RIGHT("try");
        EXPECT_BRACE_LEFT("try");

#ifdef DEBUG_PARSER
        log(DEBUG, "parseCodeBlock: TRY: Parsing catch body...");
#endif
        tryExpr->catchBlock = parseCodeBlock(code->m_function);
        if (tryExpr->catchBlock == NULL)
        {
            return false;
        }
        tryExpr->catchBlock->m_parent = code;

        // Define the exception variable
        tryExpr->catchBlock->m_vars.push_back(tryExpr->exceptionVar);

#ifdef DEBUG_PARSER
        log(DEBUG, "parseCodeBlock: TRY: %ls", tryExpr->toString().c_str());
#endif
    }
    else if (token->type == TOK_THROW)
    {
        ThrowExpression* throwExpr = new ThrowExpression(code);

        throwExpr->throwValue = parseExpression(code);
        if (throwExpr->throwValue == NULL)
        {
            log(ERROR, "parseCodeBlock: Expression expected after \"throw\"");
            return false;
        }
#ifdef DEBUG_PARSER
        log(DEBUG, "parseCodeBlock: Got throw: %ls", throwExpr->toString().c_str());
#endif

        m_expressions.push_back(throwExpr);
        code->m_code.push_back(throwExpr);

        EXPECT_SEMICOLON("throw");
    }
    else
    {
        // Expression ?
        m_pos--; // Rewind

        Expression* expression;
        expression = parseExpression(code);
        if (expression == NULL)
        {
            return false;
        }
#ifdef DEBUG_PARSER
        log(DEBUG, "parseCodeBlock: Expression: %ls", expression->toString().c_str());
#endif

        EXPECT_SEMICOLON("EXPRESSION");
        code->m_code.push_back(expression);
    }

    return true;
}


Expression* Parser::parseExpression(CodeBlock* code, TokenType endToken)
{
    vector<Expression*> outputQueue;
    vector<OperationExpression*> operatorStack;

    int brackets = 0;

    /*
     * This is basically the Shunting Yard algorithm:
     * https://en.wikipedia.org/wiki/Shunting-yard_algorithm
     */
    while (true)
    {
        Token* token = nextToken();
#ifdef DEBUG_PARSER
        log(DEBUG, "parseExpression: token: %ls", token->string.c_str());
#endif

        if (token->type == TOK_BRACKET_LEFT)
        {
            brackets++;
        }
        else if (token->type == TOK_BRACKET_RIGHT)
        {
            brackets--;
            if (brackets < 0)
            {
                m_pos--;
                break;
            }
        }
        else if (token->type == TOK_SEMICOLON || token->type == TOK_COMMA)
        {
            m_pos--;
            break;
        }
        else if (token->type != TOK_ANY && token->type == endToken)
        {
            m_pos--;
            break;
        }

        OpDesc* entry = NULL;
        unsigned int i;
        for (i = 0; i < sizeof(opTable) / sizeof(OpDesc); i++)
        {
            if (token->type == opTable[i].token)
            {
                entry = &(opTable[i]);
            }
        }

        if (entry != NULL)
        {
            OperationExpression* o1 = new OperationExpression(code, *entry);
            m_expressions.push_back(o1);
            o1->operType = entry->oper;
#ifdef DEBUG_PARSER
            log(DEBUG, "parseExpression:  -> is operator");
#endif

            if (operatorStack.size() > 0)
            {
                OperationExpression* o2 = (operatorStack.back());

                if (o2 != NULL)
                {

#ifdef DEBUG_PARSER
                    log(DEBUG, "parseExpression:  -> o1=%d, o2=%d", o1->operType, o2->operType);
#endif

                    if (o1->operType <= o2->operType)
                    {
#ifdef DEBUG_PARSER
                        log(DEBUG, "parseExpression:  -> o1=%d, o2=%d", o1->operType, o2->operType);
#endif
                        outputQueue.push_back(o2);
                        operatorStack.pop_back();
                    }
                }
                else
                {
#ifdef DEBUG_PARSER
                    log(DEBUG, "parseExpression: Popped left bracket!");
#endif
                }
            }
            operatorStack.push_back(o1);
 
        }
        else if (token->type == TOK_BRACKET_LEFT)
        {
#ifdef DEBUG_PARSER
            log(DEBUG, "parseExpression:  -> Left bracket");
#endif
            operatorStack.push_back(NULL);
        }
        else if (token->type == TOK_BRACKET_RIGHT)
        {
#ifdef DEBUG_PARSER
            log(DEBUG, "parseExpression:  -> Right bracket");
#endif
            while (true)
            {
                if (operatorStack.size() == 0)
                {
                    return NULL;
                }
                OperationExpression* op = operatorStack.back();
                operatorStack.pop_back();
                if (op == NULL)
                {
#ifdef DEBUG_PARSER
                    log(DEBUG, "parseExpression:    -> Found left bracket!");
#endif
                    break;
                }
                outputQueue.push_back(op);
            }
        }
        else
        {
            m_pos--;
            Expression* valueExpr = parseExpressionValue(code);
            if (valueExpr == NULL)
            {
                return NULL;
            }
            outputQueue.push_back(valueExpr);
#ifdef DEBUG_PARSER
            log(DEBUG, "parseExpression:  -> value=%ls", valueExpr->toString().c_str());
#endif
        }
    }

    vector<OperationExpression*>::reverse_iterator opIt;
    for (opIt = operatorStack.rbegin(); opIt != operatorStack.rend(); opIt++)
    {
        outputQueue.push_back(*opIt);
    }
#ifdef DEBUG_PARSER
    log(DEBUG, "parseExpression: Finished expression, building tree");
#endif

    Expression* expr = buildTree(code, outputQueue);
#ifdef DEBUG_PARSER
    if (expr != NULL)
    {
        log(DEBUG, "parseExpression: DONE: Tree: %ls", expr->toString().c_str());
    }
#endif

    return expr;
}

Expression* Parser::buildTree(CodeBlock* code, vector<Expression*>& queue)
{
    if (queue.size() == 0)
    {
        log(ERROR, "buildTree: queue is empty!");
        return NULL;
    }

    Expression* expr = queue.back();
    queue.pop_back();

    if (expr == NULL)
    {
        log(ERROR, "buildTree: expr is NULL!");
        return NULL;
    }

    if (expr->type == EXPR_OPER)
    {
        OperationExpression* opExpr = (OperationExpression*)expr;

        if (opExpr->operDesc.hasRightExpr)
        {
            opExpr->right = buildTree(code, queue);
            if (opExpr->right == NULL)
            {
                return NULL;
            }
            opExpr->right->parent = opExpr;
        }

        if (opExpr->operDesc.hasLeftExpr)
        {
            opExpr->left = buildTree(code, queue);
            if (opExpr->left == NULL)
            {
                log(ERROR, "buildTree: No left: %ls", opExpr->toString().c_str());
                return NULL;
            }
            opExpr->left->parent = opExpr;
        }

        if (opExpr->operType == OP_MULTIPLY || opExpr->operType == OP_ADD)
        {
            if (opExpr->left->type == EXPR_DOUBLE && opExpr->right->type == EXPR_DOUBLE)
            {
#ifdef DEBUG_PARSER
                log(DEBUG, "buildTree: Optmise two Doubles: %ls", opExpr->toString().c_str());
#endif
                DoubleExpression* dblExpr = new DoubleExpression(code);
                double leftd = ((DoubleExpression*)opExpr->left)->d;
                double rightd = ((DoubleExpression*)opExpr->right)->d;
                switch (opExpr->operType)
                {
                    case OP_ADD:
                        dblExpr->d = leftd + rightd;
                        break;
                    case OP_MULTIPLY:
                        dblExpr->d = leftd * rightd;
                        break;
                    default:
                        // Should never get here!
                        break;
                }
                dblExpr->parent = opExpr->parent;
                expr = dblExpr;
            }
            else if (opExpr->left->type == EXPR_INTEGER && opExpr->right->type == EXPR_INTEGER)
            {
#ifdef DEBUG_PARSER
                log(DEBUG, "buildTree: Optmise two Integers: %ls", opExpr->toString().c_str());
#endif
                IntegerExpression* intExpr = new IntegerExpression(code);
                int64_t lefti = ((IntegerExpression*)opExpr->left)->i;
                int64_t righti = ((IntegerExpression*)opExpr->right)->i;
                switch (opExpr->operType)
                {
                    case OP_ADD:
                        intExpr->i = lefti + righti;
                        break;
                    case OP_MULTIPLY:
                        intExpr->i = lefti * righti;
                        break;
                    default:
                        // Should never get here!
                        break;
                }
                intExpr->parent = opExpr->parent;
                expr = intExpr;
            }
        }
        else if (opExpr->operDesc.token == TOK_ADD_ASSIGN ||
                 opExpr->operDesc.token == TOK_SUB_ASSIGN)
        {
            // l += r -> l = (l + r)

            // Make a copy of the left hand side rather than reuse or cleaning up
            // at the end tries to free it twice!
            Expression* copy;
            if (opExpr->left->type == EXPR_VAR)
            {
                VarExpression* varExpr = new VarExpression(code);
                m_expressions.push_back(varExpr);
                varExpr->var = ((VarExpression*)opExpr->left)->var;
                copy = varExpr;
            }
            else
            {
                log(ERROR, "buildTree: TOK_x_ASSIGN: Unhandled left type: %d: %ls", opExpr->left->type, opExpr->left->toString().c_str());
                return NULL;
            }

OpType op;
            if (opExpr->operDesc.token == TOK_ADD_ASSIGN)
            {
                op = OP_ADD;
            }
            else
            {
                op = OP_SUB;
            }

            OpDesc addDesc = {TOK_PLUS, op, true};
            OperationExpression* addExpr = new OperationExpression(code, addDesc);
            m_expressions.push_back(addExpr);
            addExpr->operType = op;
            addExpr->left = copy;
            addExpr->left->parent = addExpr;
            addExpr->right = opExpr->right;
            addExpr->right->parent = addExpr;
            addExpr->resolveType();

            opExpr->right = addExpr;
            addExpr->parent = opExpr;
        }
    }
    else
    {
    }

    return expr;
}

Expression* Parser::parseExpressionValue(CodeBlock* code)
{
    Expression* expression;

    bool res;
#ifdef DEBUG_PARSER
    log(DEBUG, "parseExpressionValue: Here");
#endif

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
        log(DEBUG, "parseExpressionValue: NEW: class: %ls", id.toString().c_str());
#endif

        if (id.identifier.size() == 1)
        {
            // Check for imported classes
            // TODO: These will override any local vars
            //       Is that right?
            map<wstring, Identifier>::iterator it = m_imports.find(id.identifier.at(0));
            if (it != m_imports.end())
            {
#ifdef DEBUG_PARSER
                log(
                    DEBUG,
                    "parseExpressionValue: NEW: Found imported class: %ls -> %ls",
                    id.identifier.at(0).c_str(),
                    it->second.toString().c_str());
#endif

                id = it->second;
            }
         }

        vector<Expression*> newParams;
        res = parseExpressionList(code, newParams);
        if (!res)
        {
            log(ERROR, "parseExpressionValue: NEW: Failed to parse args?");
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
        wstring id = token->string;
        Class* clazz = NULL;

#ifdef DEBUG_PARSER
        log(DEBUG, "parseExpressionValue: Got identifer: %ls", id.c_str());
#endif

        token = nextToken();
#ifdef DEBUG_PARSER
        log(DEBUG, "parseExpressionValue: checking for class names...");
#endif
        if (token->type == TOK_DOT)
        {
#ifdef DEBUG_PARSER
            log(DEBUG, "parseExpressionValue:  -> got a dot!");
#endif
            wstring clazzname = id;
            token = nextToken();

            // Check for imported classes
            // TODO: These will override any local vars
            //       Is that right?
            map<wstring, Identifier>::iterator it = m_imports.find(clazzname);
            if (it != m_imports.end())
            {
#ifdef DEBUG_PARSER
                log(
                    DEBUG,
                    "parseExpressionValue: Found imported class: %ls -> %ls",
                    clazzname.c_str(),
                    it->second.toString().c_str());
#endif
                clazz = m_context->getRuntime()->findClass(m_context, it->second.toString(), true);
                if (clazz == NULL)
                {
                    log(ERROR, "parseExpressionValue: Unable to find imported class: %ls", it->second.toString().c_str());
                    return NULL;
                }
                m_pos -= 2;
            }
            int count = 2;
            while (clazz == NULL && token->type == TOK_IDENTIFIER)
            {
#ifdef DEBUG_PARSER
                log(DEBUG, "parseExpressionValue:  -> got an identifier: %ls", token->string.c_str());
#endif
                clazzname += '.';
                clazzname += token->string;
                clazz = m_context->getRuntime()->findClass(m_context, clazzname, true);
#ifdef DEBUG_PARSER
                log(DEBUG, "parseExpressionValue:  -> class? %ls = %p", clazzname.c_str(), clazz);
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
            log(DEBUG, "parseExpressionValue: -> function call!");
#endif
            m_pos--;
            vector<Expression*> params;
            res = parseExpressionList(code, params);
            if (!res)
            {
            log(DEBUG, "parseExpressionValue: -> Error in expression list");
                return NULL;
            }
#ifdef DEBUG_PARSER
            log(DEBUG, "parseExpressionValue: -> function call to %ls class=%p", id.c_str(), clazz);
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
            arrExpr->indexExpr = parseExpression(code, TOK_SQUARE_BRACKET_RIGHT);
            if (arrExpr->indexExpr == NULL)
            {
                log(DEBUG, "parseExpressionValue: -> Error in array expression");
                return NULL;
            }

#ifdef DEBUG_PARSER
            log(DEBUG, "parseExpressionValue: Array: Index expression=%ls", arrExpr->indexExpr->toString().c_str());
#endif
            expression = arrExpr;
            token = nextToken();
            if (token->type != TOK_SQUARE_BRACKET_RIGHT)
            {
                EXPECT_ERROR("array", "]");
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
            log(DEBUG, "parseExpressionValue: -> Variable!");
#endif
            expression = varExpr;
            m_pos--;
        }
    }
    else if (token->type == TOK_FUNCTION)
    {
        Function* func = parseFunction(NULL);
#ifdef DEBUG_PARSER
        log(DEBUG, "parseExpressionValue: Function: %p", func);
#endif
        FunctionExpression* funcExpr = new FunctionExpression(code);
        m_expressions.push_back(funcExpr);
        funcExpr->function = func;
        funcExpr->valueType = VALUE_OBJECT;
        expression = funcExpr;
    }
    else if (token->type == TOK_LITERAL)
    {
        if (token->string == L"true" || token->string == L"false")
        {
            IntegerExpression* intExpr = new IntegerExpression(code);
            m_expressions.push_back(intExpr);
            intExpr->i = (token->string == L"true");
            intExpr->valueType = VALUE_INTEGER;
            expression = intExpr;
        }
        else
        {
            log(ERROR, "parseExpressionValue: Unhandled Literal type: %d: %ls", token->type, token->string.c_str());
            return NULL;
        }
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
    else
    {
        log(DEBUG, "parseExpressionValue: Unhandled token type: %d: %ls", token->type, token->string.c_str());
        return NULL;
    }
    return expression;
}

bool Parser::parseIdentifier(Identifier& id)
{

    Token* token = nextToken();
    if (token->type != TOK_IDENTIFIER)
    {
        log(ERROR, "parseIdentifier: Invalid identifier");
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
            EXPECT_ERROR("identifier", "identifier");
            return false;
        }
        id.identifier.push_back(token->string);
    }

    return true;
}

bool Parser::parseList(vector<Token*>& list, TokenType type)
{
    Token* token = nextToken();
    if (token->type != TOK_BRACKET_LEFT)
    {
        EXPECT_ERROR("list", "(");
        return false;
    }

    while (moreTokens())
    {
        token = nextToken();
        if (token->type == TOK_BRACKET_RIGHT)
        {
            break;
        }

        if (token->type == type)
        {
#ifdef DEBUG_PARSER
            log(DEBUG, "parseList: item: %ls", token->string.c_str());
#endif
            list.push_back(token);
        }
        else
        {
            EXPECT_ERROR("list", "identifier or )");
            return false;
        }

        token = nextToken();
        if (token->type == TOK_BRACKET_RIGHT)
        {
            break;
        }
        else if (token->type != TOK_COMMA)
        {
            EXPECT_ERROR("list", ", or )");
            return false;
        }
    }
    return true;
}

bool Parser::parseExpressionList(CodeBlock* code, vector<Expression*>& list)
{
    Token* token = nextToken();
    if (token->type != TOK_BRACKET_LEFT)
    {
        EXPECT_ERROR("list", "(");
        return false;
    }

    while (moreTokens())
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
            EXPECT_ERROR("list", ", or )");
            return false;
        }
    }

    return true;
}

wstring Identifier::end()
{
    return identifier.at(identifier.size() - 1);
}

wstring Identifier::toString()
{
    wstring str;
    bool comma = false;
    vector<wstring>::iterator it;
    for (it = identifier.begin(); it != identifier.end(); it++)
    {
        if (comma)
        {
            str += '.';
        }
        comma = true;
        str += *it;
    }
    return str;
}

Identifier Identifier::makeIdentifier(wstring name)
{
    Identifier res;
    unsigned int pos;
    wstring buf;
    for (pos = 0; pos < name.length(); pos++)
    {
        wchar_t c = name.at(pos);
        if (c == '.')
        {
            if (buf.length() > 0)
            {
                res.identifier.push_back(buf);
                buf = wstring();
            }
        }
        else
        {
            buf += c;
        }
    }
    if (buf.length() > 0)
    {
        res.identifier.push_back(buf);
    }
    return res;
}

