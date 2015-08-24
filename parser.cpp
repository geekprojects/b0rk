
#include <stdio.h>

#include "parser.h"

using namespace std;

static TokenType g_operatorTokens[] = {
    TOK_EQUALS,
    TOK_PLUS,
    TOK_MINUS
};

Parser::Parser()
{
}

Parser::~Parser()
{
}

Token* Parser::nextToken()
{
    int pos = m_pos++;
    Token* result = &(m_tokens[pos]);
    //printf("Parser::nextToken: %d: %d: %s\n", pos, result->type, result->string.c_str());
    return result;
}

bool Parser::parse(Runtime* runtime, vector<Token> tokens)
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
            clazz = parseClass();
            if (clazz == NULL)
            {
                return false;
            }
            runtime->addClass(clazz);
        }
        else
        {
            printf("Parser::parser: ERROR: Unexpected symbol: %d\n", token->type);
            return false;
        }
    }
    return true;
}

Class* Parser::parseClass()
{
    bool res;
    Token* token;

    token = nextToken();
    if (token->type != TOK_IDENTIFIER)
    {
        printf("Parser::parseClass: Expected class name\n");
        return NULL;
    }

    string name = token->string;

    token = nextToken();
    if (token->type != TOK_BRACE_LEFT)
    {
        printf("Parser::parseClass: Expected {\n");
        return NULL;
    }

    printf("Parser::parseClass: Class name: %s\n", name.c_str());
    Class* clazz = new Class(name);

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
            printf("Parser::parseClass: Variable: %s\n", varName.c_str());

            token = nextToken();
            if (token->type != TOK_SEMICOLON)
            {
                printf("Parser::parseClass: Expected ;\n");
                delete clazz;
                return NULL;
            }
        }
        else if (token->type == TOK_FUNCTION)
        {
            token = nextToken();
            string funcName = token->string;
            printf("Parser::parseClass: Function: %s\n", funcName.c_str());

            ScriptFunction* function = new ScriptFunction(clazz);

            vector<Token*> paramTokens;
            res = parseList(paramTokens, TOK_IDENTIFIER);
            if (!res)
            {
                delete clazz;
                return NULL;
            }

            token = nextToken();
            if (token->type == TOK_STATIC)
            {
                printf("Parser::parseClass: Function: Static function!\n");
                function->setStatic(true);
                token = nextToken();
            }
            if (token->type != TOK_BRACE_LEFT)
            {
                printf("Parser::parseClass: Function: Expected {\n");
                delete clazz;
                return NULL;
            }

            CodeBlock* code;
            code = parseCodeBlock();
            if (code == NULL)
            {
                delete clazz;
                return NULL;
            }
            function->setCode(code);
            clazz->addMethod(funcName, function);
        }
        else
        {
            printf("Parser::parser: ERROR: Unexpected token: %s\n", token->string.c_str());
            delete clazz;
            return NULL;
        }
    }
    return clazz;
}

CodeBlock* Parser::parseCodeBlock()
{
    CodeBlock* code = new CodeBlock();

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
            printf("Parser::parseCodeBlock: VAR: name=%s\n", token->string.c_str());
            code->m_vars.push_back(token->string);

            // TODO: Handle assignment from var definition

            token = nextToken();
            if (token->type != TOK_SEMICOLON)
            {
                printf("Parser::parseCodeBlock: VAR: Expected ;, got %s\n", token->string.c_str());
                delete code;
                return NULL;
            }
        }
        else
        {
            // Expression ?
            m_pos--; // Rewind

            Expression* expression;
            expression = parseExpression();
            if (expression == NULL)
            {
                delete code;
                return NULL;
            }
            printf("Parser::parseCodeBlock: Expression: %s\n", expression->toString().c_str());

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

Expression* Parser::parseExpression()
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
        printf("Parser::parseExpression: NEW: class: %s\n", id.toString().c_str());

        vector<Expression*> newParams;
        res = parseExpressionList(newParams);
        if (!res)
        {
            return NULL;
        }

        NewExpression* newExpr = new NewExpression();
        newExpr->clazz = id;
        newExpr->parameters = newParams;
        expression = newExpr;
    }
    else if (token->type == TOK_IDENTIFIER)
    {
        m_pos--; // Rewind

        Identifier id;
        res = parseIdentifier(id);
        if (!res)
        {
            return NULL;
        }
        printf("Parser::parseExpression: Got identifer: %s\n", id.toString().c_str());

        token = nextToken();
        if (token->type == TOK_BRACKET_LEFT)
        {
            printf("Parser::parseExpression: -> function call!\n");
            m_pos--;
            vector<Expression*> params;
            res = parseExpressionList(params);
            if (!res)
            {
                return NULL;
            }
            printf("Parser::parseExpression: -> function call to %s\n", id.toString().c_str());
            CallExpression* callExpr = new CallExpression();
            callExpr->function = id;
            callExpr->parameters = params;
            expression = callExpr;
        }
        else
        {
            VarExpression* varExpr = new VarExpression();
            varExpr->var = id;
            expression = varExpr;
            m_pos--;
        }
    }
    else if (token->type == TOK_STRING)
    {
        StringExpression* strExpr = new StringExpression();
        strExpr->str = token->string;
        expression = strExpr;
    }
    else
    {
        return NULL;
    }

    // See if we have an operation
    token = nextToken();
    unsigned int i;
    bool isOper = false;
    TokenType oper = TOK_ANY;
    for (i = 0; i < (sizeof(g_operatorTokens) / sizeof(TokenType)); i++)
    {
        if (token->type == g_operatorTokens[i])
        {
            isOper = true;
            oper = g_operatorTokens[i];
            break;
        }
    }

    if (isOper)
    {
        OperationExpression* opExpr = new OperationExpression();
        opExpr->operType = (OpType)oper;
        opExpr->left = expression;
        opExpr->right = parseExpression();
        if (opExpr->right == NULL)
        {
            return NULL;
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
            printf("Parser::parseList: item: %s\n", token->string.c_str());
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

bool Parser::parseExpressionList(vector<Expression*>& list)
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
            Expression* childExp = parseExpression();
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

