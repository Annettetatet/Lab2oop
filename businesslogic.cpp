#include "businesslogic.h"
#include <QFile>
#include <QTextStream>
#include <QChar>

BusinessLogic::BusinessLogic()
{
}

bool BusinessLogic::loadFileContents(QString filename) {

    QFile file(filename);

    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        this->fileContents = "";
        this->parseResult = "Невозможно открыть файл";
        return false;
    }

    QTextStream textStream(&file);
    this->fileContents = textStream.readAll();
    this->parseResult = "Файл успешно загружен: " + filename + "\n";

    return true;
}

void BusinessLogic::initParingVariables() {
    this->isWhiteSpace = false;
    this->isInQuotes = false;
    this->currentFieldName = NULL;
    this->currentValue = NULL;
    this->currentState = ReadingState::NONE;
}


void BusinessLogic::validateLoadedFile() {

    int contentLength = this->fileContents.length();

    this->initParingVariables();

    this->currentLine = 1;
    this->currentPos = 0;
    this->currentDepth = 0;

    for(int i=0; i<contentLength; i++) {
        QChar c = this->fileContents.at(i);

        bool isValid = true;
        bool isNewLineChar = false;

        switch(c.unicode()) {
        case ' ':
        case '\t':
            if (this->isInQuotes) {
                isValid = processText(c);
            }
            break;
        case '\r':
            isNewLineChar = true;
            break;
        case '\n':
            isNewLineChar = true;
            this->currentLine++;
            this->currentPos = 0;
            break;
        case '{':
            isValid = processBraceOpen();
            break;
        case '}':
            isValid = processBraceClose();
            break;
        case '[':
            isValid = processSquareBraceOpen();
            break;
        case ']':
            isValid = processSquareBraceClose();
            break;
        case ',':
            isValid = processComma();
            break;
        case '\"':
            isValid = processQuote();
            break;
        case ':':
            isValid = processCollon();
            break;
        default:
            isValid = processText(c);
        }

        if (!isNewLineChar) {
            this->currentPos++;
            this->logState(c);
        }

        if (!isValid) {
            this->parseResult += "Ошибка в строке " + QString::number(this->currentLine) +
                    ", позиция " + QString::number(this->currentPos) + ": " + errorText;
            return;
        }
    }

    bool isValid = this->validateTextEnd();
    if (!isValid) {
        this->parseResult += "Ошибка в конце файла: " + errorText;
        return;
    }

    this->parseResult += "JSON успешно проверен";
}

bool BusinessLogic::processBraceOpen() {
    if (isInQuotes) {
        return true;
    }

    if (this->currentState == ReadingState::NONE) {
        this->currentState = ReadingState::WAITING_FIELDNAME;

    } else if (this->currentState == ReadingState::WAITING_VALUE) {
        this->currentFieldName = "";
        this->currentState = ReadingState::WAITING_FIELDNAME;
    }

    this->currentDepth++;
    return true;
}

bool BusinessLogic::processBraceClose() {
    if (isInQuotes) {
        return true;
    }

    if (this->currentState == ReadingState::READING_LITERAL_VALUE) {
        if (!this->processFieldValue()) {
            return false;
        }
        this->currentState = ReadingState::NONE;
    }

    if (this->currentState == ReadingState::READING_VALUE_END ||
        this->currentState == ReadingState::READING_ARRAY_VALUE_END ||
        this->currentState == ReadingState::WAITING_FIELDNAME) {
        this->currentState = ReadingState::NONE;
    }

    if (this->currentState != ReadingState::NONE) {
        this->errorText = "Некорректное расположение закрывающей фигурной скобки";
        return false;
    }

    this->currentDepth--;
    if (this->currentDepth < 0) {
        this->errorText = "Некорректное расположение закрывающей фигурной скобки";
        return false;
    }

    this->currentState = ReadingState::WAITING_FIELDNAME;
    return true;
}

bool BusinessLogic::processSquareBraceOpen() {
    if (this->currentState == ReadingState::WAITING_VALUE) {
        this->currentState = ReadingState::READING_ARRAY_VALUE;
        return true;
    }

    this->errorText = "Некорректное расположение открывающей квадратной скобки";
    return false;
}

bool BusinessLogic::processSquareBraceClose() {

    if (this->currentState == ReadingState::READING_ARRAY_VALUE) {
        this->currentState = ReadingState::READING_ARRAY_VALUE_END;
        if (!this->processFieldValue()) {
            return false;
        }
        this->currentFieldName = "";
        this->currentValue = "";

        return true;
    }

    this->errorText = "Некорректное расположение открывающей квадратной скобки";
    return false;
}

bool BusinessLogic::processQuote() {
    if (this->currentState == ReadingState::WAITING_FIELDNAME) {
        this->currentState = ReadingState::READING_FIELDNAME;
        this->isInQuotes = true;
        return true;
    }

    if (this->currentState == ReadingState::READING_FIELDNAME) {
        this->currentState = ReadingState::WAITING_VALUE_SEPARATOR;
        this->currentValue = "";
        this->isInQuotes = false;
        return true;
    }

    if (this->currentState == ReadingState::WAITING_VALUE) {
        this->currentState = ReadingState::READING_VALUE;
        this->isInQuotes = true;
        return true;
    }

    if (this->currentState == ReadingState::READING_VALUE) {
        this->currentState = ReadingState::READING_VALUE_END;
        this->isInQuotes = false;
        if (!this->processFieldValue()) {
            return false;
        }
        this->currentFieldName = "";
        this->currentValue = "";

        return true;
    }

    if (this->currentState == ReadingState::WAITING_ARRAY_VALUE) {
        this->currentState = ReadingState::READING_ARRAY_VALUE;
        this->isInQuotes = true;
        return true;
    }

    if (this->currentState == ReadingState::READING_ARRAY_VALUE && this->isInQuotes) {
        this->isInQuotes = false;
        return true;
    }

    if (this->currentState == ReadingState::READING_ARRAY_VALUE) {
        this->isInQuotes = true;
        return true;
    }

    this->errorText = "Некорректное расположение кавычки";

    return false;
}

bool BusinessLogic::processComma() {
    if (isInQuotes) {
        return true;
    }

    if (this->currentState == ReadingState::READING_LITERAL_VALUE) {
        this->currentState = ReadingState::WAITING_FIELDNAME;

        if (!this->processFieldValue()) {
            return false;
        }

        this->currentFieldName = "";
        this->currentValue = "";
        return true;
    }

    if (this->currentState == ReadingState::READING_VALUE_END) {
        this->currentState = ReadingState::WAITING_FIELDNAME;
        return true;
    }

    if (this->currentState == ReadingState::READING_ARRAY_VALUE_END) {
        this->currentState = ReadingState::WAITING_FIELDNAME;
        return true;
    }

    if (this->currentState == ReadingState::READING_ARRAY_VALUE) {
        this->processFieldValue();
        this->currentValue = "";
        this->currentState = ReadingState::WAITING_ARRAY_VALUE;
        return true;
    }

    this->errorText = "Неверное расположение запятой";
    return false;
}

bool BusinessLogic::processText(QChar c) {
    if (this->currentState == ReadingState::WAITING_FIELDNAME) {
        this->currentState = ReadingState::READING_FIELDNAME;
        this->currentFieldName += c;
        return true;
    }

    if (this->currentState == ReadingState::READING_FIELDNAME) {
        this->currentFieldName += c;
        return true;
    }

    this->isCurrentValueEscaped = this->isInQuotes;

    if (this->currentState == ReadingState::WAITING_VALUE) {
        this->currentState = ReadingState::READING_LITERAL_VALUE;
        this->currentValue += c;
        return true;
    }

    if (this->currentState == ReadingState::READING_VALUE) {
        this->currentValue += c;
        return true;
    }

    if (this->currentState == ReadingState::READING_LITERAL_VALUE) {
        this->currentValue += c;
        return true;
    }

    if (this->currentState == ReadingState::WAITING_ARRAY_VALUE) {
        this->currentState = ReadingState::READING_ARRAY_VALUE;
        this->currentValue += c;
        return true;
    }

    if (this->currentState == ReadingState::READING_ARRAY_VALUE) {
        this->currentValue += c;
        return true;
    }

    this->errorText = "Неверный символ";
    return false;
}

bool BusinessLogic::processCollon() {
    if (this->currentState == ReadingState::WAITING_VALUE_SEPARATOR) {
        this->currentState = ReadingState::WAITING_VALUE;
        return true;
    }

    this->errorText = "Неверное расположение двуеточия";
    return false;
}


bool BusinessLogic::validateTextEnd() {
    return true;
}

void BusinessLogic::validate(QString filename) {
    if (!this->loadFileContents(filename)) {
        return;
    }

    this->validateLoadedFile();
}

void BusinessLogic::logState(QChar c) {
    QString state;

    switch(this->currentState) {
    case NONE: state = "NONE"; break;

    case WAITING_FIELDNAME: state = "WAITING_FIELDNAME"; break;
    case READING_FIELDNAME: state = "READING_FIELDNAME"; break;
    case READING_FIELDNAME_END: state = "READING_FIELDNAME_END"; break;

    case WAITING_VALUE_SEPARATOR: state = "WAITING_VALUE_SEPARATOR"; break;

    case WAITING_VALUE: state = "WAITING_VALUE"; break;
    case READING_VALUE: state = "READING_VALUE"; break;
    case READING_VALUE_END: state = "READING_VALUE_END"; break;

    case WAITING_ARRAY_VALUE: state = "WAITING_ARRAY_VALUE"; break;
    case READING_ARRAY_VALUE: state = "READING_ARRAY_VALUE"; break;
    case READING_ARRAY_VALUE_END: state = "READING_ARRAY_VALUE_END"; break;

    case READING_LITERAL_VALUE: state = "READING_LITERAL_VALUE"; break;
    default: state = "UNKNOWN";
    }

    this->parseResult += "state " + state +
            ", line = " + QString::number(this->currentLine) +
            ", pos = " + QString::number(this->currentPos) +
            ", char = " + c +
            ", field = " + this->currentFieldName +
            ", value = " + this->currentValue +
            "\n";
}

bool BusinessLogic::processFieldValue() {
    this->parseResult += "Прочитано поле " + this->currentFieldName +
            ", значение " + this->currentValue + ", экранировано " +
            (this->isCurrentValueEscaped ? "да" : "нет") + "\n";

    if (this->isCurrentValueEscaped) {
        return true;
    }

    if (QString::compare(this->currentValue, "null", Qt::CaseSensitive) == 0) {
        return true;
    }

    if (QString::compare(this->currentValue, "true", Qt::CaseSensitive) == 0 ||
        QString::compare(this->currentValue, "false", Qt::CaseSensitive) == 0) {
        return true;
    }

    bool isNumberOk;
    this->currentValue.toDouble(&isNumberOk);

    if (isNumberOk) {
        return true;
    }

    this->errorText = "Некоректное число - [" + this->currentValue + "]";
    return false;
}

QString BusinessLogic::getFileContents() {
    return this->fileContents;
}

QString BusinessLogic::getParseResult() {
    return this->parseResult;
}


