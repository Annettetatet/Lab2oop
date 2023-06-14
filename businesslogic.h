#ifndef BUSINESSLOGIC_H
#define BUSINESSLOGIC_H

#include <QString>

enum ReadingState {
    NONE,

    WAITING_FIELDNAME, READING_FIELDNAME, READING_FIELDNAME_END,

    WAITING_VALUE_SEPARATOR,
    WAITING_VALUE, READING_VALUE, READING_VALUE_END,
    WAITING_ARRAY_VALUE, READING_ARRAY_VALUE, READING_ARRAY_VALUE_END,
    READING_LITERAL_VALUE
};

class BusinessLogic
{
public:
    BusinessLogic();

    void validate(QString filename);

    QString getParseResult();
    QString getFileContents();
private:
    QString parseResult;
    QString fileContents;

    int currentLine;
    int currentPos;
    QString errorText;

    bool isWhiteSpace;
    bool isInQuotes;

    ReadingState currentState;
    QString currentBuffer;
    QString currentFieldName;
    QString currentValue;
    int currentDepth;
    bool isCurrentValueEscaped;

    bool loadFileContents(QString filename);
    void validateLoadedFile();

    void initParingVariables();
    bool processBraceOpen();
    bool processBraceClose();
    bool processSquareBraceOpen();
    bool processSquareBraceClose();
    bool processQuote();
    bool processComma();
    bool processText(QChar c);
    bool processCollon();

    bool validateTextEnd();

    bool processFieldValue();
    void logState(QChar c);
};

#endif // BUSINESSLOGIC_H
