#include <QDebug>
#include <QStringList>

#include "RegExpPatterns.h"

namespace RegExpPatterns
{
    QString oneOf(const QStringList &expressions)
    {
        QString result('(');

        for (const QString &expression : expressions) {
            result += expression + '|';
        }

        return (result.chopped(1) + ')');
    }

    QString allOf(const QStringList &expressions)
    {
        QString result('(');

        for (const QString &expression : expressions) {
            result += expression;
        }

        return (result + ')');
    }

    QString symbolFromSet(const QStringList &symbols)
    {
        QString result('[');

        for (const QString &symbol : symbols) {
            result += symbol;
        }

        return (result + ']');
    }

    QString symbolNotFromSet(const QStringList &symbols)
    {
        QString result("[^");

        for (const QString &symbol : symbols) {
            result += symbol;
        }

        return (result + ']');
    }

    QString symbolsUntilNotFromSet(const QStringList &symbols,
                                   const bool skipEscaped,
                                   const RepeatCount count)
    {
        QString result = symbolNotFromSet(symbols);

        if (skipEscaped) {
            result = oneOf({
                anyEscapedSymbolInExpression,
                result,
            });
        }

        return applyRepeatCount(result, count);
    }

    QString quotedExpression(const QString &quotationMark)
    {
        return allOf({
            quotationMark,
            symbolsUntilNotFromSet(
                {
                    quotationMark
                }, true, ZeroOrMore
            ),
            quotationMark
        });
    }

    QString quotedExpession(const QString &openingQuotationMark,
                            const QString &closingQuotationMark)
    {
        return allOf({
            openingQuotationMark,
            symbolsUntilNotFromSet(
                {
                    openingQuotationMark,
                    closingQuotationMark
                }, true, ZeroOrMore
            ),
            closingQuotationMark
        });
    }

    void debugOutput(const QString &pattern)
    {
        qDebug().noquote() << pattern;
    }

    void debugOutput(const QRegExp &regExp)
    {
        qDebug().noquote() << regExp.pattern();
    }

    void debugOutput(const QString &message, const QString &pattern)
    {
        qDebug().noquote() << message << pattern;
    }

    void debugOutput(const QString &message, const QRegExp &regExp)
    {
        qDebug().noquote() << message << regExp.pattern();
    }
}
