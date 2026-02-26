#include "Highlighter.h"

Highlighter::Highlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    HighlightingRule rule;

    // 1. Ключові слова - Світло-блакитний (як у VS Code)
    keywordFormat.setForeground(QColor("#569cd6")); 
    keywordFormat.setFontWeight(QFont::Bold);
    QStringList keywordPatterns;
    keywordPatterns << "\\bprogram\\b" << "\\bvariable\\b" << "\\bstart\\b"
                    << "\\bstop\\b" << "\\bif\\b" << "\\bthen\\b"
                    << "\\belse\\b" << "\\bfor\\b" << "\\bdownto\\b"
                    << "\\bdo\\b" << "\\binput\\b" << "\\boutput\\b";
    
    for (const QString &pattern : keywordPatterns) {
        rule.pattern = QRegularExpression(pattern);
        rule.format = keywordFormat;
        highlightingRules.append(rule);
    }

    // 2. Числа - Світло-зелений
    numberFormat.setForeground(QColor("#b5cea8"));
    rule.pattern = QRegularExpression("\\b[0-9]+\\b");
    rule.format = numberFormat;
    highlightingRules.append(rule);

    // 3. Рядки - Світло-помаранчевий
    quotationFormat.setForeground(QColor("#ce9178")); 
    rule.pattern = QRegularExpression("\".*\"");
    rule.format = quotationFormat;
    highlightingRules.append(rule);

    // 4. Оператори - Рожевий/Фіолетовий
    functionFormat.setForeground(QColor("#c586c0"));
    QStringList opPatterns;
    opPatterns << "\\badd\\b" << "\\bsub\\b" << "\\bdiv\\b" << "\\bmod\\b";
    for (const QString &pattern : opPatterns) {
        rule.pattern = QRegularExpression(pattern);
        rule.format = functionFormat;
        highlightingRules.append(rule);
    }

    // 5. Коментарі - Темно-зелений/Сірий
    multiLineCommentFormat.setForeground(QColor("#6a9955"));
    commentStartExpression = QRegularExpression("/\\$");
    commentEndExpression = QRegularExpression("\\$/");
}

void Highlighter::highlightBlock(const QString &text)
{
    for (const HighlightingRule &rule : qAsConst(highlightingRules)) {
        QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
        while (matchIterator.hasNext()) {
            QRegularExpressionMatch match = matchIterator.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }

    setCurrentBlockState(0);
    int startIndex = 0;
    if (previousBlockState() != 1)
        startIndex = text.indexOf(commentStartExpression);

    while (startIndex >= 0) {
        QRegularExpressionMatch match = commentEndExpression.match(text, startIndex);
        int endIndex = match.capturedStart();
        int commentLength = 0;
        if (endIndex == -1) {
            setCurrentBlockState(1);
            commentLength = text.length() - startIndex;
        } else {
            commentLength = endIndex - startIndex + match.capturedLength();
        }
        setFormat(startIndex, commentLength, multiLineCommentFormat);
        startIndex = text.indexOf(commentStartExpression, startIndex + commentLength);
    }
}
