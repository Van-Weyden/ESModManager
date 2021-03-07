#ifndef REGEXPPATTERNS_H
#define REGEXPPATTERNS_H

#include <QString>

namespace RegExpPatterns
{
	constexpr const char *lineBegin = "^";
	constexpr const char *lineEnd = "$";
	constexpr const char *anySymbol = ".";
	constexpr const char *anySymbolSequence = ".*";
	constexpr const char *backslash = "\\\\";
	constexpr const char *anyEscapedSymbolInExpression = "(\\\\.)";
	constexpr const char *whitespace = "[ \\n\\r]*";

	inline QString escapedSymbol(const QString &symbol);
	inline QString escapedSymbolInExpression(const QString &symbol);
	inline QString zeroOrOneOccurences(const QString &expression);
	inline QString oneOrMoreOccurences(const QString &expression);
	inline QString zeroOrMoreOccurences(const QString &expression);
	inline QString putInParentheses(const QString &expression);

	QString oneOf(const QStringList &expressions);
	QString allOf(const QStringList &expressions);
	QString symbolFrom(const QStringList &symbols);
	QString symbolExcept(const QStringList &symbols);
	QString whileExcept(const QStringList &symbols, const bool skipEscaped = true);
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

inline QString RegExpPatterns::putInParentheses(const QString &expression)
{
	return ('(' + expression + ')');
}

#endif // REGEXPPATTERNS_H
