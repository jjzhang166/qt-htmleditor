#include "stdafx.h"
#include "sourceeditor.h"
#include <QHBoxLayout>
#include "mtextedit.h"
#include "HtmlHighlighter.h"
#include "mrichtextedit.h"

SourceEditor::SourceEditor(MRichTextEdit *parent)
    : QDialog(nullptr)
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    edit_ = new MTextEdit(this);
    layout->addWidget(edit_);

    syntax_ = new HtmlHighlighter(edit_->document());
    edit_->setPlainText(parent->toHtml());

    connect(edit_, &MTextEdit::textChanged, [=]() {
        parent->setText(edit_->toPlainText(), true);
    });
}

SourceEditor::~SourceEditor()
{
    if (syntax_) {
        delete syntax_;
        syntax_ = nullptr;
    }
}
