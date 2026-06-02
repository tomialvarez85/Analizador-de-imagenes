#include "uihelpers.h"

#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QSizePolicy>
#include <QVBoxLayout>

namespace UiHelpers {

QWidget *createAppRoot(QWidget *parent)
{
    auto *root = new QWidget(parent);
    root->setObjectName(QStringLiteral("appRoot"));
    root->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    return root;
}

QVBoxLayout *createOuterLayout(QWidget *appRoot)
{
    auto *outer = new QVBoxLayout(appRoot);
    outer->setContentsMargins(32, 32, 32, 32);
    outer->setSpacing(0);
    return outer;
}

QFrame *createCard(QWidget *parent, int fixedWidth)
{
    auto *card = new QFrame(parent);
    card->setObjectName(QStringLiteral("cardPanel"));
    card->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
    if (fixedWidth > 0) {
        card->setMaximumWidth(fixedWidth);
    }
    return card;
}

QLabel *createEmoji(const QString &emoji, QWidget *parent)
{
    auto *label = new QLabel(emoji, parent);
    label->setObjectName(QStringLiteral("emojiLabel"));
    label->setAlignment(Qt::AlignCenter);
    label->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    return label;
}

QLabel *createTitle(const QString &text, QWidget *parent)
{
    auto *label = new QLabel(text, parent);
    label->setObjectName(QStringLiteral("titleLabel"));
    label->setAlignment(Qt::AlignCenter);
    label->setWordWrap(true);
    label->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    return label;
}

QLabel *createSubtitle(const QString &text, QWidget *parent)
{
    auto *label = new QLabel(text, parent);
    label->setObjectName(QStringLiteral("subtitleLabel"));
    label->setAlignment(Qt::AlignCenter);
    label->setWordWrap(true);
    label->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    return label;
}

QLabel *createStatusLabel(QWidget *parent)
{
    auto *label = new QLabel(parent);
    label->setObjectName(QStringLiteral("statusLabel"));
    label->setAlignment(Qt::AlignCenter);
    label->setWordWrap(true);
    label->setVisible(false);
    label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    return label;
}

QWidget *createFieldGroup(QWidget *parent, const QString &labelText, QLineEdit *field)
{
    auto *group = new QWidget(parent);
    auto *layout = new QVBoxLayout(group);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    auto *label = new QLabel(labelText, group);
    label->setObjectName(QStringLiteral("formLabel"));
    label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    field->setParent(group);
    field->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    field->setMinimumHeight(48);

    layout->addWidget(label);
    layout->addWidget(field);

    group->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    return group;
}

void addFullWidthButton(QVBoxLayout *layout, QPushButton *button)
{
    button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    button->setMinimumHeight(54);
    button->setMaximumHeight(58);
    button->setMinimumWidth(0);
    layout->addWidget(button);
}

} // namespace UiHelpers
