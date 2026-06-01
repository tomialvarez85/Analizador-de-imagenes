#ifndef UIHELPERS_H
#define UIHELPERS_H

#include <QFrame>
#include <QLabel>
#include <QWidget>

class QLineEdit;
class QPushButton;
class QVBoxLayout;

namespace UiHelpers {

constexpr int kAuthCardWidth = 440;

QWidget *createAppRoot(QWidget *parent);
QVBoxLayout *createOuterLayout(QWidget *appRoot);
QFrame *createCard(QWidget *parent, int fixedWidth = 0);
QLabel *createEmoji(const QString &emoji, QWidget *parent);
QLabel *createTitle(const QString &text, QWidget *parent);
QLabel *createSubtitle(const QString &text, QWidget *parent);
QLabel *createStatusLabel(QWidget *parent);
QWidget *createFieldGroup(QWidget *parent, const QString &labelText, QLineEdit *field);
void addFullWidthButton(QVBoxLayout *layout, QPushButton *button);

} // namespace UiHelpers

#endif // UIHELPERS_H
