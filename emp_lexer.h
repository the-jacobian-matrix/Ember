#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// A view into source bytes (not null-terminated).
typedef struct EmpSlice {
    const char *ptr;
    size_t len;
} EmpSlice;

// 1-based line/column; byte offsets are 0-based, end is exclusive.
typedef struct EmpSpan {
    size_t start;
    size_t end;
    uint32_t line;
    uint32_t col;
} EmpSpan;

typedef enum EmpTokenKind {
    EMP_TOK_EOF = 0,
    EMP_TOK_ERROR,

    // Identifiers / keywords
    EMP_TOK_IDENT,

    EMP_TOK_KW_FN,
    EMP_TOK_KW_AUTO,
    EMP_TOK_KW_LET,
    EMP_TOK_KW_MUT,
    EMP_TOK_KW_IF,
    EMP_TOK_KW_ELSE,
    EMP_TOK_KW_WHILE,
    EMP_TOK_KW_FOR,
    EMP_TOK_KW_IN,
    EMP_TOK_KW_RETURN,
    EMP_TOK_KW_BREAK,
    EMP_TOK_KW_CONTINUE,
    EMP_TOK_KW_STRUCT,
    EMP_TOK_KW_ENUM,
    EMP_TOK_KW_MATCH,
        EMP_TOK_KW_DEFER,
    EMP_TOK_KW_TRUE,
    EMP_TOK_KW_FALSE,
    EMP_TOK_KW_NULL,
    EMP_TOK_KW_EMP,
    EMP_TOK_KW_MM,
    EMP_TOK_KW_OFF,
    EMP_TOK_KW_EXPORT,
    EMP_TOK_KW_USE,
    EMP_TOK_KW_FROM,
    EMP_TOK_KW_AS,
    EMP_TOK_KW_EXTERN,
    EMP_TOK_KW_UNSAFE,
    EMP_TOK_KW_CLASS,
    EMP_TOK_KW_TRAIT,
    EMP_TOK_KW_VIRTUAL,
    EMP_TOK_KW_NEW,
    EMP_TOK_KW_IMPL,
    EMP_TOK_KW_CONST,
    EMP_TOK_KW_DYN,

    // Literals
    EMP_TOK_INT,
    EMP_TOK_FLOAT,
    EMP_TOK_STRING,
    EMP_TOK_FSTRING, // $"..." or $`...`
    EMP_TOK_CHAR,

    // Delimiters / punctuation
    EMP_TOK_LPAREN,
    EMP_TOK_RPAREN,
    EMP_TOK_LBRACE,
    EMP_TOK_RBRACE,
    EMP_TOK_LBRACKET,
    EMP_TOK_RBRACKET,
    EMP_TOK_COMMA,
    EMP_TOK_DOT,
    EMP_TOK_DOTDOT,
    EMP_TOK_DOTDOT_EQ,
    EMP_TOK_DOTDOTDOT,
    EMP_TOK_SEMI,
    EMP_TOK_COLON,
    EMP_TOK_COLONCOLON,

    // Operators
    EMP_TOK_PLUS,
    EMP_TOK_MINUS,
    EMP_TOK_STAR,
    EMP_TOK_SLASH,
    EMP_TOK_PERCENT,

    EMP_TOK_CARET,
    EMP_TOK_AMP,
    EMP_TOK_PIPE,
    EMP_TOK_BANG,
    EMP_TOK_TILDE,

    EMP_TOK_EQ,
    EMP_TOK_LT,
    EMP_TOK_GT,

    EMP_TOK_PLUS_EQ,
    EMP_TOK_MINUS_EQ,
    EMP_TOK_STAR_EQ,
    EMP_TOK_SLASH_EQ,
    EMP_TOK_PERCENT_EQ,
    EMP_TOK_CARET_EQ,
    EMP_TOK_AMP_EQ,
    EMP_TOK_PIPE_EQ,

    EMP_TOK_EQ_EQ,
    EMP_TOK_BANG_EQ,
    EMP_TOK_LT_EQ,
    EMP_TOK_GT_EQ,

    EMP_TOK_AMP_AMP,
    EMP_TOK_PIPE_PIPE,

    EMP_TOK_SHL,
    EMP_TOK_SHR,
    EMP_TOK_SHL_EQ,
    EMP_TOK_SHR_EQ,

    EMP_TOK_ARROW,     // ->
    EMP_TOK_FAT_ARROW, // =>

    // Misc
    EMP_TOK_QUESTION,
    EMP_TOK_AT,
    EMP_TOK_HASH,
} EmpTokenKind;

typedef enum EmpLexErrorKind {
    EMP_LEXERR_NONE = 0,
    EMP_LEXERR_UNEXPECTED_CHAR,
    EMP_LEXERR_UNTERMINATED_STRING,
    EMP_LEXERR_UNTERMINATED_CHAR,
    EMP_LEXERR_INVALID_ESCAPE,
    EMP_LEXERR_INVALID_NUMBER,
    EMP_LEXERR_UNTERMINATED_BLOCK_COMMENT,
    EMP_LEXERR_EMPTY_CHAR,
    EMP_LEXERR_MULTI_CHAR_LITERAL,
} EmpLexErrorKind;

typedef struct EmpLexError {
    EmpLexErrorKind kind;
    char found; // when meaningful (0 if unknown)
} EmpLexError;

typedef struct EmpToken {
    EmpTokenKind kind;
    EmpSpan span;
    EmpSlice lexeme;
    EmpLexError error;
} EmpToken;

typedef struct EmpLexer {
    const char *src;
    size_t len;
    size_t pos;
    uint32_t line;
    uint32_t col;

    // single-token lookahead cache
    bool has_peek;
    EmpToken peek;
} EmpLexer;

EmpLexer emp_lexer_new(const char *src, size_t len);
EmpToken emp_lexer_next(EmpLexer *lex);
EmpToken emp_lexer_peek(EmpLexer *lex);

const char *emp_token_kind_name(EmpTokenKind kind);
const char *emp_lex_error_kind_name(EmpLexErrorKind kind);

#ifdef __cplusplus
}
#endif
