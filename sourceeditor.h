#pragma once

#include <QDialog>

class MRichTextEdit;
class HtmlHighlighter;
class MTextEdit;
class SourceEditor : public QDialog
{
    Q_OBJECT

public:
    SourceEditor(MRichTextEdit *parent);
    ~SourceEditor();

private:
    HtmlHighlighter *syntax_ = nullptr;
    MTextEdit * edit_ = nullptr;
};
