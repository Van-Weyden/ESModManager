#ifndef REGEXPPATTERNS_H
#define REGEXPPATTERNS_H

#include <QString>

namespace RegExpPatterns
{
    enum RepeatCount
    {
        ExactlyOnce,
        ZeroOrOnce,
        ZeroOrMore,
        OnceOrMore
    };

    constexpr const char *lineBegin = "^";
    constexpr const char *lineEnd = "$";
    constexpr const char *anySymbol = ".";
    constexpr const char *anySymbolSequence = ".*";
    constexpr const char *backslash = "\\\\";
    constexpr const char *anyEscapedSymbolInExpression = "(\\\\.)";
    constexpr const char *whitespace = "[ \\n\\r]*";

    inline QString escapedSymbol(const QString &symbol);
    inline QString escapedSymbolInExpression(const QString &symbol);
    inline QString putInParentheses(const QString &expression);

    inline QString zeroOrOneOccurences(const QString &expression);
    inline QString oneOrMoreOccurences(const QString &expression);
    inline QString zeroOrMoreOccurences(const QString &expression);
    inline QString applyRepeatCount(const QString &expression,
                                    const RepeatCount count);

    QString oneOf(const QStringList &expressions);
    QString allOf(const QStringList &expressions);
    QString symbolFromSet(const QStringList &symbols);
    QString symbolNotFromSet(const QStringList &symbols);
    QString symbolsUntilNotFromSet(const QStringList &symbols,
                                   const bool skipEscaped = true,
                                   const RepeatCount count = ZeroOrMore);
    QString quotedExpression(const QString &quotationMark);
    QString quotedExpession(const QString &openingQuotationMark,
                            const QString &closingQuotationMark);

    void debugOutput(const QString &pattern);
    void debugOutput(const QRegExp &regExp);
    void debugOutput(const QString &message, const QString &pattern);
    void debugOutput(const QString &message, const QRegExp &regExp);
}

inline QString RegExpPatterns::escapedSymbol(const QString &symbol)
{
    return ('\\' + symbol);
}

inline QString RegExpPatterns::escapedSymbolInExpression(const QString &symbol)
{
    return (backslash + symbol);
}

inline QString RegExpPatterns::putInParentheses(const QString &expression)
{
    return ('(' + expression + ')');
}

inline QString RegExpPatterns::zeroOrOneOccurences(const QString &expression)
{
    return (expression + '?');
}

inline QString RegExpPatterns::oneOrMoreOccurences(const QString &expression)
{
    return (expression + '+');
}

inline QString RegExpPatterns::zeroOrMoreOccurences(const QString &expression)
{
    return (expression + '*');
}

inline QString RegExpPatterns::applyRepeatCount(const QString &expression, const RepeatCount count)
{
    switch (count) {
        case ZeroOrOnce:    return zeroOrOneOccurences(expression);
        case ZeroOrMore:    return zeroOrMoreOccurences(expression);
        case OnceOrMore:    return oneOrMoreOccurences(expression);
        default:            return expression;
    }
}

#endif // REGEXPPATTERNS_H
