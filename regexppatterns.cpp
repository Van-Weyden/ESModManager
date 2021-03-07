#include <QDebug>
#include <QStringList>

#include "regexppatterns.h"

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

	QString symbolFrom(const QStringList &symbols)
	{
		QString result('[');

		for (const QString &symbol : symbols) {
			result += symbol;
		}

		return (result + ']');
	}

	QString symbolExcept(const QStringList &symbols)
	{
		QString result("[^");

		for (const QString &symbol : symbols) {
			result += symbol;
		}

		return (result + ']');
	}

	QString whileExcept(const QStringList &symbols, const bool skipEscaped)
	{
		QString result("[^");

		for (const QString &symbol : symbols) {
			result += symbol;
		}

		if (skipEscaped) {
			result += backslash;
			result += "]*";

			return zeroOrMoreOccurences(oneOf({
				result,
				anyEscapedSymbolInExpression
			}));
		} else {
			return (result + "]*");
		}
	}

	QString quotedExpression(const QString &quotationMark)
	{
		return allOf({
			quotationMark,
			zeroOrMoreOccurences(
				oneOf({
					anyEscapedSymbolInExpression,
					symbolExcept({backslash, quotationMark})
				})
			),
			quotationMark
		});
	}

	QString quotedExpession(const QString &openingQuotationMark,
							const QString &closingQuotationMark)
	{
		return allOf({
			openingQuotationMark,
			zeroOrMoreOccurences(
				oneOf({
					anyEscapedSymbolInExpression,
					symbolExcept({backslash, openingQuotationMark, closingQuotationMark})
				})
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
