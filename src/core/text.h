#pragma once

#include <QString>

namespace Text {

// The separator to put before a phrase that follows another: none before punctuation that
// belongs to the previous phrase, a space otherwise.
inline QString separatorBefore(const QString &phrase)
{
    static const QString attached = QStringLiteral(",.!?;:…)»");
    return !phrase.isEmpty() && attached.contains(phrase.front()) ? QString() : QStringLiteral(" ");
}

// A phrase as it continues `previous`. Every phrase is recognised on its own, so the models start
// each one with a capital letter; after a pause in the middle of a sentence that capital is wrong.
// Lower it unless the sentence before has ended, raise it if it has. Acronyms (NASA, ГОСТ) stay.
inline QString continuing(const QString &previous, QString phrase)
{
    if (previous.isEmpty() || phrase.isEmpty())
        return phrase;
    qsizetype end = previous.size();
    while (end > 0 && (previous[end - 1].isSpace() || QStringLiteral("\"»)”'").contains(previous[end - 1])))
        --end;
    const bool sentenceEnded = end > 0 && QStringLiteral(".!?…").contains(previous[end - 1]);
    qsizetype first = 0;
    while (first < phrase.size() && !phrase[first].isLetter())
        ++first;
    if (first == phrase.size())
        return phrase;
    qsizetype wordEnd = first;
    while (wordEnd < phrase.size() && phrase[wordEnd].isLetter())
        ++wordEnd;
    const QString word = phrase.mid(first, wordEnd - first);
    if (word.size() > 1 && word == word.toUpper())
        return phrase; // an acronym
    phrase[first] = sentenceEnded ? phrase[first].toUpper() : phrase[first].toLower();
    return phrase;
}

// Appends a recognised phrase to the text dictated so far.
inline void append(QString &text, const QString &phrase)
{
    if (phrase.isEmpty())
        return;
    const QString next = continuing(text, phrase);
    text += text.isEmpty() ? next : separatorBefore(next) + next;
}

} // namespace Text
