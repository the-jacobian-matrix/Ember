#include "emp_lexer.h"

#include <ctype.h>
#include <string.h>

typedef struct EmpMark {
    size_t pos;
    uint32_t line;
    uint32_t col;
} EmpMark;

static bool emp_is_eof(const EmpLexer *lex) { return lex->pos >= lex->len; }

static bool emp_is_newline_byte(char ch) { return ch == '\n' || ch == '\r'; }

static char emp_peek_ch(const EmpLexer *lex) {
    if (emp_is_eof(lex)) return 0;
    return lex->src[lex->pos];
}

static char emp_peek_next_ch(const EmpLexer *lex) {
    if (lex->pos + 1 >= lex->len) return 0;
    return lex->src[lex->pos + 1];
}

static size_t emp_utf8_codepoint_len(const char *s, size_t len, size_t at) {
    unsigned char b0 = (unsigned char)s[at];
    if (b0 < 0x80) return 1;
    if ((b0 & 0xE0u) == 0xC0u) {
        if (at + 1 >= len) return 1;
        unsigned char b1 = (unsigned char)s[at + 1];
        if ((b1 & 0xC0u) != 0x80u) return 1;
        return 2;
    }
    if ((b0 & 0xF0u) == 0xE0u) {
        if (at + 2 >= len) return 1;
        unsigned char b1 = (unsigned char)s[at + 1];
        unsigned char b2 = (unsigned char)s[at + 2];
        if ((b1 & 0xC0u) != 0x80u || (b2 & 0xC0u) != 0x80u) return 1;
        return 3;
    }
    if ((b0 & 0xF8u) == 0xF0u) {
        if (at + 3 >= len) return 1;
        unsigned char b1 = (unsigned char)s[at + 1];
        unsigned char b2 = (unsigned char)s[at + 2];
        unsigned char b3 = (unsigned char)s[at + 3];
        if ((b1 & 0xC0u) != 0x80u || (b2 & 0xC0u) != 0x80u || (b3 & 0xC0u) != 0x80u) return 1;
        return 4;
    }
    return 1;
}

static char emp_advance(EmpLexer *lex) {
    if (emp_is_eof(lex)) return 0;

    char ch = lex->src[lex->pos];

    // Normalize Windows CRLF (and treat lone CR) as newline.
    if (ch == '\r') {
        lex->pos += 1;
        if (!emp_is_eof(lex) && lex->src[lex->pos] == '\n') {
            lex->pos += 1;
        }
        lex->line += 1;
        lex->col = 1;
        return '\n';
    }

    if (ch == '\n') {
        lex->pos += 1;
        lex->line += 1;
        lex->col = 1;
        return '\n';
    }

    // Advance by one UTF-8 codepoint to keep columns sane for non-ASCII.
    size_t step = emp_utf8_codepoint_len(lex->src, lex->len, lex->pos);
    lex->pos += step;
    lex->col += 1;
    return ch;
}

static EmpMark emp_mark(const EmpLexer *lex) {
    EmpMark m;
    m.pos = lex->pos;
    m.line = lex->line;
    m.col = lex->col;
    return m;
}

static EmpToken emp_make_token(EmpLexer *lex, EmpTokenKind kind, EmpMark start) {
    EmpToken t;
    t.kind = kind;
    t.span.start = start.pos;
    t.span.end = lex->pos;
    t.span.line = start.line;
    t.span.col = start.col;
    t.lexeme.ptr = lex->src + start.pos;
    t.lexeme.len = lex->pos - start.pos;
    t.error.kind = EMP_LEXERR_NONE;
    t.error.found = 0;
    return t;
}

static EmpToken emp_make_error_at(EmpLexer *lex, EmpLexErrorKind kind, char found, EmpMark start) {
    EmpToken t = emp_make_token(lex, EMP_TOK_ERROR, start);
    t.error.kind = kind;
    t.error.found = found;
    return t;
}

static bool emp_is_ident_start(char ch) {
    return (ch == '_') || (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z');
}

static bool emp_is_ident_continue(char ch) {
    return emp_is_ident_start(ch) || (ch >= '0' && ch <= '9');
}

static bool emp_is_hex_digit(char ch) {
    return (ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F');
}

static bool emp_is_digit_in_base(char ch, int base) {
    if (base <= 10) return ch >= '0' && ch < (char)('0' + base);
    if (ch >= '0' && ch <= '9') return true;
    return base == 16 && emp_is_hex_digit(ch);
}

static void emp_skip_whitespace(EmpLexer *lex) {
    for (;;) {
        char ch = emp_peek_ch(lex);
        if (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n') {
            emp_advance(lex);
            continue;
        }
        break;
    }
}

static EmpToken emp_skip_comments(EmpLexer *lex) {
    // returns EMP_TOK_ERROR token if unterminated block comment, otherwise kind=EOF? no: caller ignores non-error.
    for (;;) {
        char ch = emp_peek_ch(lex);
        char nx = emp_peek_next_ch(lex);

        if (ch == '/' && nx == '/') {
            // line comment
            emp_advance(lex);
            emp_advance(lex);
            while (!emp_is_eof(lex) && !emp_is_newline_byte(emp_peek_ch(lex))) {
                emp_advance(lex);
            }
            continue;
        }

        if (ch == '/' && nx == '*') {
            // nested block comment
            EmpMark start = emp_mark(lex);
            emp_advance(lex);
            emp_advance(lex);
            uint32_t depth = 1;

            while (!emp_is_eof(lex)) {
                char c0 = emp_peek_ch(lex);
                char c1 = emp_peek_next_ch(lex);
                if (c0 == '/' && c1 == '*') {
                    emp_advance(lex);
                    emp_advance(lex);
                    depth++;
                    continue;
                }
                if (c0 == '*' && c1 == '/') {
                    emp_advance(lex);
                    emp_advance(lex);
                    depth--;
                    if (depth == 0) break;
                    continue;
                }
                emp_advance(lex);
            }

            if (depth != 0) {
                return emp_make_error_at(lex, EMP_LEXERR_UNTERMINATED_BLOCK_COMMENT, 0, start);
            }
            continue;
        }

        break;
    }

    EmpToken ok;
    memset(&ok, 0, sizeof(ok));
    ok.kind = EMP_TOK_EOF; // sentinel meaning "no error" for this helper
    ok.error.kind = EMP_LEXERR_NONE;
    return ok;
}

static EmpTokenKind emp_keyword_kind(EmpSlice s) {
    // Keep this list small/obvious; user can customize later.
    #define KW(lit, kindv) \
        do { \
            static const char k[] = lit; \
            if (s.len == sizeof(k) - 1 && memcmp(s.ptr, k, sizeof(k) - 1) == 0) return kindv; \
        } while (0)

    KW("fn", EMP_TOK_KW_FN);
    KW("auto", EMP_TOK_KW_AUTO);
    KW("let", EMP_TOK_KW_LET);
    KW("mut", EMP_TOK_KW_MUT);
    KW("if", EMP_TOK_KW_IF);
    KW("else", EMP_TOK_KW_ELSE);
    KW("while", EMP_TOK_KW_WHILE);
    KW("for", EMP_TOK_KW_FOR);
    KW("in", EMP_TOK_KW_IN);
    KW("return", EMP_TOK_KW_RETURN);
    KW("break", EMP_TOK_KW_BREAK);
    KW("continue", EMP_TOK_KW_CONTINUE);
    KW("struct", EMP_TOK_KW_STRUCT);
    KW("enum", EMP_TOK_KW_ENUM);
    KW("match", EMP_TOK_KW_MATCH);
    KW("defer", EMP_TOK_KW_DEFER);
    KW("true", EMP_TOK_KW_TRUE);
    KW("false", EMP_TOK_KW_FALSE);
    KW("null", EMP_TOK_KW_NULL);
    KW("emp", EMP_TOK_KW_EMP);
    KW("mm", EMP_TOK_KW_MM);
    KW("off", EMP_TOK_KW_OFF);
    KW("export", EMP_TOK_KW_EXPORT);
    KW("use", EMP_TOK_KW_USE);
    KW("from", EMP_TOK_KW_FROM);
    KW("as", EMP_TOK_KW_AS);
    KW("extern", EMP_TOK_KW_EXTERN);
    KW("unsafe", EMP_TOK_KW_UNSAFE);
    KW("class", EMP_TOK_KW_CLASS);
    KW("trait", EMP_TOK_KW_TRAIT);
    KW("virtual", EMP_TOK_KW_VIRTUAL);
    KW("new", EMP_TOK_KW_NEW);
    KW("impl", EMP_TOK_KW_IMPL);
    KW("const", EMP_TOK_KW_CONST);
    KW("dyn", EMP_TOK_KW_DYN);

    #undef KW
    return EMP_TOK_IDENT;
}

static bool emp_consume_if(EmpLexer *lex, char expected) {
    if (emp_peek_ch(lex) == expected) {
        emp_advance(lex);
        return true;
    }
    return false;
}

static EmpToken emp_lex_identifier_or_keyword(EmpLexer *lex) {
    EmpMark start = emp_mark(lex);
    emp_advance(lex); // first char
    while (emp_is_ident_continue(emp_peek_ch(lex))) {
        emp_advance(lex);
    }

    EmpToken tok = emp_make_token(lex, EMP_TOK_IDENT, start);
    tok.kind = emp_keyword_kind(tok.lexeme);
    return tok;
}

static bool emp_scan_digits_strict(EmpLexer *lex, int base, bool require_one_digit) {
    bool saw_digit = false;
    bool last_underscore = false;

    for (;;) {
        char ch = emp_peek_ch(lex);
        if (ch == '_') {
            // no leading underscore, no consecutive underscores
            if (!saw_digit || last_underscore) return false;
            last_underscore = true;
            emp_advance(lex);
            continue;
        }
        if (!emp_is_digit_in_base(ch, base)) break;
        saw_digit = true;
        last_underscore = false;
        emp_advance(lex);
    }

    if (require_one_digit && !saw_digit) return false;
    if (last_underscore) return false; // no trailing underscore
    return true;
}

static bool emp_scan_digits_strict_tail(EmpLexer *lex, int base, bool already_saw_digit) {
    bool saw_digit = already_saw_digit;
    bool last_underscore = false;

    for (;;) {
        char ch = emp_peek_ch(lex);
        if (ch == '_') {
            // no leading underscore, no consecutive underscores
            if (!saw_digit || last_underscore) return false;
            last_underscore = true;
            emp_advance(lex);
            continue;
        }
        if (!emp_is_digit_in_base(ch, base)) break;
        saw_digit = true;
        last_underscore = false;
        emp_advance(lex);
    }

    if (last_underscore) return false; // no trailing underscore
    return true;
}

static EmpToken emp_lex_number(EmpLexer *lex) {
    EmpMark start = emp_mark(lex);

    bool is_float = false;

    char first = emp_peek_ch(lex);
    if (first == '.') {
        // Leading-dot float: .123
        emp_advance(lex);
        if (!isdigit((unsigned char)emp_peek_ch(lex))) return emp_make_token(lex, EMP_TOK_DOT, start);
        if (!emp_scan_digits_strict(lex, 10, true)) {
            return emp_make_error_at(lex, EMP_LEXERR_INVALID_NUMBER, emp_peek_ch(lex), start);
        }
        is_float = true;
    } else {
        // 0x / 0b / 0o or decimal
        if (first == '0') {
            emp_advance(lex);
            char p = emp_peek_ch(lex);
            if (p == 'x' || p == 'X') {
                emp_advance(lex);
                if (!emp_scan_digits_strict(lex, 16, true)) {
                    return emp_make_error_at(lex, EMP_LEXERR_INVALID_NUMBER, emp_peek_ch(lex), start);
                }
                return emp_make_token(lex, EMP_TOK_INT, start);
            }
            if (p == 'b' || p == 'B') {
                emp_advance(lex);
                // binary digits (strict underscores)
                bool saw_digit = false;
                bool last_underscore = false;
                for (;;) {
                    char ch = emp_peek_ch(lex);
                    if (ch == '_') {
                        if (!saw_digit || last_underscore) return emp_make_error_at(lex, EMP_LEXERR_INVALID_NUMBER, ch, start);
                        last_underscore = true;
                        emp_advance(lex);
                        continue;
                    }
                    if (ch == '0' || ch == '1') {
                        saw_digit = true;
                        last_underscore = false;
                        emp_advance(lex);
                        continue;
                    }
                    break;
                }
                if (!saw_digit || last_underscore) {
                    return emp_make_error_at(lex, EMP_LEXERR_INVALID_NUMBER, emp_peek_ch(lex), start);
                }
                return emp_make_token(lex, EMP_TOK_INT, start);
            }
            if (p == 'o' || p == 'O') {
                emp_advance(lex);
                bool saw_digit = false;
                bool last_underscore = false;
                for (;;) {
                    char ch = emp_peek_ch(lex);
                    if (ch == '_') {
                        if (!saw_digit || last_underscore) return emp_make_error_at(lex, EMP_LEXERR_INVALID_NUMBER, ch, start);
                        last_underscore = true;
                        emp_advance(lex);
                        continue;
                    }
                    if (ch >= '0' && ch <= '7') {
                        saw_digit = true;
                        last_underscore = false;
                        emp_advance(lex);
                        continue;
                    }
                    break;
                }
                if (!saw_digit || last_underscore) {
                    return emp_make_error_at(lex, EMP_LEXERR_INVALID_NUMBER, emp_peek_ch(lex), start);
                }
                return emp_make_token(lex, EMP_TOK_INT, start);
            }
            // fallthrough: decimal starting with 0 (already consumed leading 0)
            if (!emp_scan_digits_strict_tail(lex, 10, /*already_saw_digit*/ true)) {
                return emp_make_error_at(lex, EMP_LEXERR_INVALID_NUMBER, emp_peek_ch(lex), start);
            }
        } else {
            // decimal starting with non-zero: scan full integer part
            if (!emp_scan_digits_strict(lex, 10, true)) {
                return emp_make_error_at(lex, EMP_LEXERR_INVALID_NUMBER, emp_peek_ch(lex), start);
            }
        }

        // fractional part
        if (emp_peek_ch(lex) == '.' && isdigit((unsigned char)emp_peek_next_ch(lex))) {
            is_float = true;
            emp_advance(lex); // '.'
            if (!emp_scan_digits_strict(lex, 10, true)) {
                return emp_make_error_at(lex, EMP_LEXERR_INVALID_NUMBER, emp_peek_ch(lex), start);
            }
        }
    }

    // exponent part
    char e = emp_peek_ch(lex);
    if (e == 'e' || e == 'E') {
        is_float = true;
        emp_advance(lex);
        char sign = emp_peek_ch(lex);
        if (sign == '+' || sign == '-') emp_advance(lex);

        if (!emp_scan_digits_strict(lex, 10, true)) {
            return emp_make_error_at(lex, EMP_LEXERR_INVALID_NUMBER, emp_peek_ch(lex), start);
        }
    }

    EmpToken tok = emp_make_token(lex, is_float ? EMP_TOK_FLOAT : EMP_TOK_INT, start);
    return tok;
}

static bool emp_lex_escape(EmpLexer *lex) {
    // assumes leading '\\' already consumed
    char esc = emp_peek_ch(lex);
    if (esc == 0) return false;

    switch (esc) {
        case '\\':
        case '\"':
        case '\'':
        case 'n':
        case 'r':
        case 't':
        case '0':
            emp_advance(lex);
            return true;
        case 'x': {
            emp_advance(lex);
            char a = emp_peek_ch(lex);
            char b = emp_peek_next_ch(lex);
            if (!emp_is_hex_digit(a) || !emp_is_hex_digit(b)) return false;
            emp_advance(lex);
            emp_advance(lex);
            return true;
        }
        case 'u': {
            emp_advance(lex);
            if (!emp_consume_if(lex, '{')) return false;

            int digits = 0;
            while (!emp_is_eof(lex)) {
                char ch = emp_peek_ch(lex);
                if (ch == '}') {
                    emp_advance(lex);
                    return digits > 0;
                }
                if (!emp_is_hex_digit(ch)) return false;
                digits++;
                if (digits > 6) return false;
                emp_advance(lex);
            }
            return false;
        }
        default:
            return false;
    }
}

static EmpToken emp_lex_string(EmpLexer *lex) {
    EmpMark start = emp_mark(lex);
    emp_advance(lex); // opening '"'

    while (!emp_is_eof(lex)) {
        char ch = emp_peek_ch(lex);
        if (ch == '"') {
            emp_advance(lex);
            return emp_make_token(lex, EMP_TOK_STRING, start);
        }
        if (emp_is_newline_byte(ch)) {
            return emp_make_error_at(lex, EMP_LEXERR_UNTERMINATED_STRING, 0, start);
        }
        if (ch == '\\') {
            emp_advance(lex);
            if (!emp_lex_escape(lex)) {
                // ensure progress if escape is invalid
                if (!emp_is_eof(lex)) emp_advance(lex);
                return emp_make_error_at(lex, EMP_LEXERR_INVALID_ESCAPE, emp_peek_ch(lex), start);
            }
            continue;
        }
        emp_advance(lex);
    }

    return emp_make_error_at(lex, EMP_LEXERR_UNTERMINATED_STRING, 0, start);
}

static EmpToken emp_lex_raw_string(EmpLexer *lex) {
    // Backtick-delimited raw string. Allows newlines. No escapes.
    EmpMark start = emp_mark(lex);
    emp_advance(lex); // opening '`'

    while (!emp_is_eof(lex)) {
        char ch = emp_peek_ch(lex);
        if (ch == '`') {
            emp_advance(lex);
            return emp_make_token(lex, EMP_TOK_STRING, start);
        }
        emp_advance(lex);
    }

    return emp_make_error_at(lex, EMP_LEXERR_UNTERMINATED_STRING, 0, start);
}

static EmpToken emp_lex_fstring(EmpLexer *lex) {
    // $"..." (escaped, single-line) or $`...` (raw, multiline)
    EmpMark start = emp_mark(lex);
    emp_advance(lex); // '$'

    char delim = emp_peek_ch(lex);
    if (delim != '"' && delim != '`') {
        return emp_make_error_at(lex, EMP_LEXERR_UNEXPECTED_CHAR, delim, start);
    }

    if (delim == '"') {
        // Scan like normal string, but keep lexeme starting at '$'.
        emp_advance(lex); // opening '"'
        while (!emp_is_eof(lex)) {
            char ch = emp_peek_ch(lex);
            if (ch == '"') {
                emp_advance(lex);
                return emp_make_token(lex, EMP_TOK_FSTRING, start);
            }
            if (emp_is_newline_byte(ch)) {
                return emp_make_error_at(lex, EMP_LEXERR_UNTERMINATED_STRING, 0, start);
            }
            if (ch == '\\') {
                emp_advance(lex);
                if (!emp_lex_escape(lex)) {
                    if (!emp_is_eof(lex)) emp_advance(lex);
                    return emp_make_error_at(lex, EMP_LEXERR_INVALID_ESCAPE, emp_peek_ch(lex), start);
                }
                continue;
            }
            emp_advance(lex);
        }
        return emp_make_error_at(lex, EMP_LEXERR_UNTERMINATED_STRING, 0, start);
    }

    // Backtick-delimited raw fstring.
    emp_advance(lex); // opening '`'
    while (!emp_is_eof(lex)) {
        char ch = emp_peek_ch(lex);
        if (ch == '`') {
            emp_advance(lex);
            return emp_make_token(lex, EMP_TOK_FSTRING, start);
        }
        emp_advance(lex);
    }

    return emp_make_error_at(lex, EMP_LEXERR_UNTERMINATED_STRING, 0, start);
}

static EmpToken emp_lex_char(EmpLexer *lex) {
    EmpMark start = emp_mark(lex);
    emp_advance(lex); // opening '\''

    if (emp_is_eof(lex) || emp_is_newline_byte(emp_peek_ch(lex))) {
        return emp_make_error_at(lex, EMP_LEXERR_UNTERMINATED_CHAR, 0, start);
    }

    int payload_units = 0;

    char ch = emp_peek_ch(lex);
    if (ch == '\\') {
        emp_advance(lex);
        if (!emp_lex_escape(lex)) {
            if (!emp_is_eof(lex)) emp_advance(lex);
            return emp_make_error_at(lex, EMP_LEXERR_INVALID_ESCAPE, emp_peek_ch(lex), start);
        }
        payload_units = 1;
    } else if (ch == '\'') {
        // empty ''
        return emp_make_error_at(lex, EMP_LEXERR_EMPTY_CHAR, 0, start);
    } else {
        emp_advance(lex);
        payload_units = 1;
    }

    // Now require closing quote.
    if (!emp_consume_if(lex, '\'')) {
        // If we see more payload before closing, call it multi-char.
        while (!emp_is_eof(lex) && !emp_is_newline_byte(emp_peek_ch(lex)) && emp_peek_ch(lex) != '\'') {
            emp_advance(lex);
            payload_units++;
            if (payload_units > 1) break;
        }
        return emp_make_error_at(lex, EMP_LEXERR_MULTI_CHAR_LITERAL, emp_peek_ch(lex), start);
    }

    return emp_make_token(lex, EMP_TOK_CHAR, start);
}

EmpLexer emp_lexer_new(const char *src, size_t len) {
    EmpLexer lex;
    lex.src = src;
    lex.len = len;
    lex.pos = 0;
    lex.line = 1;
    lex.col = 1;
    lex.has_peek = false;
    memset(&lex.peek, 0, sizeof(lex.peek));
    return lex;
}

static EmpToken emp_lexer_next_impl(EmpLexer *lex) {
    for (;;) {
        emp_skip_whitespace(lex);
        EmpToken cmt = emp_skip_comments(lex);
        if (cmt.kind == EMP_TOK_ERROR) return cmt;

        // if we consumed a comment, there might be more whitespace/comments
        // Loop until stable.
        char ch = emp_peek_ch(lex);
        if (!(ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n' || (ch == '/' && (emp_peek_next_ch(lex) == '/' || emp_peek_next_ch(lex) == '*')))) {
            break;
        }
    }

    EmpMark start = emp_mark(lex);

    if (emp_is_eof(lex)) {
        EmpToken t;
        memset(&t, 0, sizeof(t));
        t.kind = EMP_TOK_EOF;
        t.span.start = lex->pos;
        t.span.end = lex->pos;
        t.span.line = lex->line;
        t.span.col = lex->col;
        t.lexeme.ptr = lex->src + lex->pos;
        t.lexeme.len = 0;
        t.error.kind = EMP_LEXERR_NONE;
        return t;
    }

    char ch = emp_peek_ch(lex);

    // identifier/keyword
    if (emp_is_ident_start(ch)) {
        return emp_lex_identifier_or_keyword(lex);
    }

    // number (digit or leading-dot float)
    if (isdigit((unsigned char)ch) || (ch == '.' && isdigit((unsigned char)emp_peek_next_ch(lex)))) {
        return emp_lex_number(lex);
    }

    // strings / chars
    if (ch == '$' && (emp_peek_next_ch(lex) == '"' || emp_peek_next_ch(lex) == '`')) return emp_lex_fstring(lex);
    if (ch == '"') return emp_lex_string(lex);
    if (ch == '`') return emp_lex_raw_string(lex);
    if (ch == '\'') return emp_lex_char(lex);

    // punctuation/operators (maximal munch)
    switch (ch) {
        case '(':
            emp_advance(lex);
            return emp_make_token(lex, EMP_TOK_LPAREN, start);
        case ')':
            emp_advance(lex);
            return emp_make_token(lex, EMP_TOK_RPAREN, start);
        case '{':
            emp_advance(lex);
            return emp_make_token(lex, EMP_TOK_LBRACE, start);
        case '}':
            emp_advance(lex);
            return emp_make_token(lex, EMP_TOK_RBRACE, start);
        case '[':
            emp_advance(lex);
            return emp_make_token(lex, EMP_TOK_LBRACKET, start);
        case ']':
            emp_advance(lex);
            return emp_make_token(lex, EMP_TOK_RBRACKET, start);
        case ',':
            emp_advance(lex);
            return emp_make_token(lex, EMP_TOK_COMMA, start);
        case ';':
            emp_advance(lex);
            return emp_make_token(lex, EMP_TOK_SEMI, start);
        case '?':
            emp_advance(lex);
            return emp_make_token(lex, EMP_TOK_QUESTION, start);
        case '@':
            emp_advance(lex);
            return emp_make_token(lex, EMP_TOK_AT, start);
        case '#':
            emp_advance(lex);
            return emp_make_token(lex, EMP_TOK_HASH, start);
        case '.':
            emp_advance(lex);
            if (emp_consume_if(lex, '.')) {
                if (emp_consume_if(lex, '.')) return emp_make_token(lex, EMP_TOK_DOTDOTDOT, start);
                if (emp_consume_if(lex, '=')) return emp_make_token(lex, EMP_TOK_DOTDOT_EQ, start);
                return emp_make_token(lex, EMP_TOK_DOTDOT, start);
            }
            return emp_make_token(lex, EMP_TOK_DOT, start);
        case ':':
            emp_advance(lex);
            if (emp_consume_if(lex, ':')) return emp_make_token(lex, EMP_TOK_COLONCOLON, start);
            return emp_make_token(lex, EMP_TOK_COLON, start);
        case '+':
            emp_advance(lex);
            if (emp_consume_if(lex, '=')) return emp_make_token(lex, EMP_TOK_PLUS_EQ, start);
            return emp_make_token(lex, EMP_TOK_PLUS, start);
        case '-':
            emp_advance(lex);
            if (emp_consume_if(lex, '>')) return emp_make_token(lex, EMP_TOK_ARROW, start);
            if (emp_consume_if(lex, '=')) return emp_make_token(lex, EMP_TOK_MINUS_EQ, start);
            return emp_make_token(lex, EMP_TOK_MINUS, start);
        case '*':
            emp_advance(lex);
            if (emp_consume_if(lex, '=')) return emp_make_token(lex, EMP_TOK_STAR_EQ, start);
            return emp_make_token(lex, EMP_TOK_STAR, start);
        case '/':
            emp_advance(lex);
            if (emp_consume_if(lex, '=')) return emp_make_token(lex, EMP_TOK_SLASH_EQ, start);
            return emp_make_token(lex, EMP_TOK_SLASH, start);
        case '%':
            emp_advance(lex);
            if (emp_consume_if(lex, '=')) return emp_make_token(lex, EMP_TOK_PERCENT_EQ, start);
            return emp_make_token(lex, EMP_TOK_PERCENT, start);
        case '^':
            emp_advance(lex);
            if (emp_consume_if(lex, '=')) return emp_make_token(lex, EMP_TOK_CARET_EQ, start);
            return emp_make_token(lex, EMP_TOK_CARET, start);
        case '~':
            emp_advance(lex);
            return emp_make_token(lex, EMP_TOK_TILDE, start);
        case '!':
            emp_advance(lex);
            if (emp_consume_if(lex, '=')) return emp_make_token(lex, EMP_TOK_BANG_EQ, start);
            return emp_make_token(lex, EMP_TOK_BANG, start);
        case '=':
            emp_advance(lex);
            if (emp_consume_if(lex, '=')) return emp_make_token(lex, EMP_TOK_EQ_EQ, start);
            if (emp_consume_if(lex, '>')) return emp_make_token(lex, EMP_TOK_FAT_ARROW, start);
            return emp_make_token(lex, EMP_TOK_EQ, start);
        case '&':
            emp_advance(lex);
            if (emp_consume_if(lex, '&')) return emp_make_token(lex, EMP_TOK_AMP_AMP, start);
            if (emp_consume_if(lex, '=')) return emp_make_token(lex, EMP_TOK_AMP_EQ, start);
            return emp_make_token(lex, EMP_TOK_AMP, start);
        case '|':
            emp_advance(lex);
            if (emp_consume_if(lex, '|')) return emp_make_token(lex, EMP_TOK_PIPE_PIPE, start);
            if (emp_consume_if(lex, '=')) return emp_make_token(lex, EMP_TOK_PIPE_EQ, start);
            return emp_make_token(lex, EMP_TOK_PIPE, start);
        case '<':
            emp_advance(lex);
            if (emp_consume_if(lex, '<')) {
                if (emp_consume_if(lex, '=')) return emp_make_token(lex, EMP_TOK_SHL_EQ, start);
                return emp_make_token(lex, EMP_TOK_SHL, start);
            }
            if (emp_consume_if(lex, '=')) return emp_make_token(lex, EMP_TOK_LT_EQ, start);
            return emp_make_token(lex, EMP_TOK_LT, start);
        case '>':
            emp_advance(lex);
            if (emp_consume_if(lex, '>')) {
                if (emp_consume_if(lex, '=')) return emp_make_token(lex, EMP_TOK_SHR_EQ, start);
                return emp_make_token(lex, EMP_TOK_SHR, start);
            }
            if (emp_consume_if(lex, '=')) return emp_make_token(lex, EMP_TOK_GT_EQ, start);
            return emp_make_token(lex, EMP_TOK_GT, start);
        default:
            emp_advance(lex);
            return emp_make_error_at(lex, EMP_LEXERR_UNEXPECTED_CHAR, ch, start);
    }
}

EmpToken emp_lexer_next(EmpLexer *lex) {
    if (lex->has_peek) {
        lex->has_peek = false;
        return lex->peek;
    }
    return emp_lexer_next_impl(lex);
}

EmpToken emp_lexer_peek(EmpLexer *lex) {
    if (!lex->has_peek) {
        lex->peek = emp_lexer_next_impl(lex);
        lex->has_peek = true;
    }
    return lex->peek;
}

const char *emp_token_kind_name(EmpTokenKind kind) {
    switch (kind) {
        case EMP_TOK_EOF: return "EOF";
        case EMP_TOK_ERROR: return "ERROR";

        case EMP_TOK_IDENT: return "IDENT";
        case EMP_TOK_KW_FN: return "KW_FN";
        case EMP_TOK_KW_AUTO: return "KW_AUTO";
        case EMP_TOK_KW_LET: return "KW_LET";
        case EMP_TOK_KW_MUT: return "KW_MUT";
        case EMP_TOK_KW_IF: return "KW_IF";
        case EMP_TOK_KW_ELSE: return "KW_ELSE";
        case EMP_TOK_KW_WHILE: return "KW_WHILE";
        case EMP_TOK_KW_FOR: return "KW_FOR";
        case EMP_TOK_KW_IN: return "KW_IN";
        case EMP_TOK_KW_RETURN: return "KW_RETURN";
        case EMP_TOK_KW_BREAK: return "KW_BREAK";
        case EMP_TOK_KW_CONTINUE: return "KW_CONTINUE";
        case EMP_TOK_KW_STRUCT: return "KW_STRUCT";
        case EMP_TOK_KW_ENUM: return "KW_ENUM";
        case EMP_TOK_KW_MATCH: return "KW_MATCH";
        case EMP_TOK_KW_DEFER: return "KW_DEFER";
        case EMP_TOK_KW_TRUE: return "KW_TRUE";
        case EMP_TOK_KW_FALSE: return "KW_FALSE";
        case EMP_TOK_KW_NULL: return "KW_NULL";
        case EMP_TOK_KW_EMP: return "KW_EMP";
        case EMP_TOK_KW_MM: return "KW_MM";
        case EMP_TOK_KW_OFF: return "KW_OFF";
        case EMP_TOK_KW_EXPORT: return "KW_EXPORT";
        case EMP_TOK_KW_USE: return "KW_USE";
        case EMP_TOK_KW_FROM: return "KW_FROM";
        case EMP_TOK_KW_AS: return "KW_AS";
        case EMP_TOK_KW_EXTERN: return "KW_EXTERN";
        case EMP_TOK_KW_UNSAFE: return "KW_UNSAFE";
        case EMP_TOK_KW_CLASS: return "KW_CLASS";
        case EMP_TOK_KW_TRAIT: return "KW_TRAIT";
        case EMP_TOK_KW_VIRTUAL: return "KW_VIRTUAL";
        case EMP_TOK_KW_NEW: return "KW_NEW";
        case EMP_TOK_KW_IMPL: return "KW_IMPL";
        case EMP_TOK_KW_CONST: return "KW_CONST";
        case EMP_TOK_KW_DYN: return "KW_DYN";

        case EMP_TOK_INT: return "INT";
        case EMP_TOK_FLOAT: return "FLOAT";
        case EMP_TOK_STRING: return "STRING";
        case EMP_TOK_FSTRING: return "FSTRING";
        case EMP_TOK_CHAR: return "CHAR";

        case EMP_TOK_LPAREN: return "LPAREN";
        case EMP_TOK_RPAREN: return "RPAREN";
        case EMP_TOK_LBRACE: return "LBRACE";
        case EMP_TOK_RBRACE: return "RBRACE";
        case EMP_TOK_LBRACKET: return "LBRACKET";
        case EMP_TOK_RBRACKET: return "RBRACKET";
        case EMP_TOK_COMMA: return "COMMA";
        case EMP_TOK_DOT: return "DOT";
        case EMP_TOK_DOTDOT: return "DOTDOT";
        case EMP_TOK_DOTDOT_EQ: return "DOTDOT_EQ";
        case EMP_TOK_DOTDOTDOT: return "DOTDOTDOT";
        case EMP_TOK_SEMI: return "SEMI";
        case EMP_TOK_COLON: return "COLON";
        case EMP_TOK_COLONCOLON: return "COLONCOLON";

        case EMP_TOK_PLUS: return "PLUS";
        case EMP_TOK_MINUS: return "MINUS";
        case EMP_TOK_STAR: return "STAR";
        case EMP_TOK_SLASH: return "SLASH";
        case EMP_TOK_PERCENT: return "PERCENT";
        case EMP_TOK_CARET: return "CARET";
        case EMP_TOK_AMP: return "AMP";
        case EMP_TOK_PIPE: return "PIPE";
        case EMP_TOK_BANG: return "BANG";
        case EMP_TOK_TILDE: return "TILDE";
        case EMP_TOK_EQ: return "EQ";
        case EMP_TOK_LT: return "LT";
        case EMP_TOK_GT: return "GT";

        case EMP_TOK_PLUS_EQ: return "PLUS_EQ";
        case EMP_TOK_MINUS_EQ: return "MINUS_EQ";
        case EMP_TOK_STAR_EQ: return "STAR_EQ";
        case EMP_TOK_SLASH_EQ: return "SLASH_EQ";
        case EMP_TOK_PERCENT_EQ: return "PERCENT_EQ";
        case EMP_TOK_CARET_EQ: return "CARET_EQ";
        case EMP_TOK_AMP_EQ: return "AMP_EQ";
        case EMP_TOK_PIPE_EQ: return "PIPE_EQ";

        case EMP_TOK_EQ_EQ: return "EQ_EQ";
        case EMP_TOK_BANG_EQ: return "BANG_EQ";
        case EMP_TOK_LT_EQ: return "LT_EQ";
        case EMP_TOK_GT_EQ: return "GT_EQ";

        case EMP_TOK_AMP_AMP: return "AMP_AMP";
        case EMP_TOK_PIPE_PIPE: return "PIPE_PIPE";

        case EMP_TOK_SHL: return "SHL";
        case EMP_TOK_SHR: return "SHR";
        case EMP_TOK_SHL_EQ: return "SHL_EQ";
        case EMP_TOK_SHR_EQ: return "SHR_EQ";

        case EMP_TOK_ARROW: return "ARROW";
        case EMP_TOK_FAT_ARROW: return "FAT_ARROW";

        case EMP_TOK_QUESTION: return "QUESTION";
        case EMP_TOK_AT: return "AT";
        case EMP_TOK_HASH: return "HASH";
        default: return "<unknown>";
    }
}

const char *emp_lex_error_kind_name(EmpLexErrorKind kind) {
    switch (kind) {
        case EMP_LEXERR_NONE: return "NONE";
        case EMP_LEXERR_UNEXPECTED_CHAR: return "UNEXPECTED_CHAR";
        case EMP_LEXERR_UNTERMINATED_STRING: return "UNTERMINATED_STRING";
        case EMP_LEXERR_UNTERMINATED_CHAR: return "UNTERMINATED_CHAR";
        case EMP_LEXERR_INVALID_ESCAPE: return "INVALID_ESCAPE";
        case EMP_LEXERR_INVALID_NUMBER: return "INVALID_NUMBER";
        case EMP_LEXERR_UNTERMINATED_BLOCK_COMMENT: return "UNTERMINATED_BLOCK_COMMENT";
        case EMP_LEXERR_EMPTY_CHAR: return "EMPTY_CHAR";
        case EMP_LEXERR_MULTI_CHAR_LITERAL: return "MULTI_CHAR_LITERAL";
        default: return "<unknown>";
    }
}
