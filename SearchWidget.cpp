#include "stdafx.h"
#include "SearchWidget.h"
#include <QVBoxLayout>

SearchWidget::SearchWidget(QWidget* parent, bool inDirs)
    : QWidget(parent)
    , mSearchWordLineEdit(new QLineEdit(this))
    , mReplaceWordLineEdit(new QLineEdit(this))
    , mSearchButton(new QPushButton(QIcon(":/search.png"), "Find", this))
    , mReplaceButton(new QPushButton(QIcon(":/replace.png"), "Replace", this))
    , mInDirs(inDirs)
{
    QVBoxLayout* layout = new QVBoxLayout(this);

    layout->addWidget(createSearchPanel());
    layout->addSpacerItem(new QSpacerItem(4, 4, QSizePolicy::Minimum, QSizePolicy::Minimum));
    layout->addWidget(createReplacePanel());
    layout->setMargin(0);
    layout->setStretchFactor(this, 0);

    connect(mSearchButton, &QPushButton::clicked, this, &SearchWidget::onSearchClicked);
    connect(mReplaceButton, &QPushButton::clicked, this, &SearchWidget::onReplaceClicked);

    refresh();
}

QWidget* SearchWidget::createSearchPanel() {
    QWidget* widget = new QWidget(this);
    QHBoxLayout* layout = new QHBoxLayout(widget);

    layout->addWidget(mSearchWordLineEdit);
    layout->addWidget(mSearchButton);
    layout->setMargin(0);
    return widget;
}

QWidget* SearchWidget::createReplacePanel() {
    QWidget* widget = new QWidget(this);
    QHBoxLayout* layout = new QHBoxLayout(widget);
    layout->addWidget(mReplaceWordLineEdit);
    layout->addWidget(mReplaceButton);
    layout->setMargin(0);
    return widget;
}

void SearchWidget::refresh() {
    mSearchWordLineEdit->clear();
    mReplaceWordLineEdit->clear();
}

QWidget* SearchWidget::docWidget()
{
    return nullptr;
    //return qobject_cast<DocumentWidget*>(parent());
}

void SearchWidget::onSearchClicked()
{
}

void SearchWidget::onReplaceClicked()
{
//     QString src = mSearchWordLineEdit->text();
//     QString dst = mReplaceWordLineEdit->text();
//     if (auto *p = docWidget()) {
//         QString all = p->getSourceText();
//         p->setText(all.replace(src, dst));
//     }
}
