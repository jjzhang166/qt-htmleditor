/*
** Copyright (C) 2013 Jiří Procházka (Hobrasoft)
** Contact: http://www.hobrasoft.cz/
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file is under the terms of the GNU Lesser General Public License
** version 2.1 as published by the Free Software Foundation and appearing
** in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the
** GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
*/

#include "stdafx.h"
#include "mrichtextedit.h"
#include <QApplication>
#include <QClipboard>
#include <QMimeData>
#include <QFontDatabase>
#include <QInputDialog>
#include <QColorDialog>
#include <QTextList>
#include <QtDebug>
#include <QFileDialog>
#include <QImageReader>
#include <QSettings>
#include <QBuffer>
#include <QUrl>
#include <QPlainTextEdit>
#include <QMenu>
#include <QDialog>
#include "ui_mrichtextedit.h"
#include "sourceeditor.h"


MRichTextEdit::MRichTextEdit(QWidget *parent) 
    : QWidget(parent), ui_(new Ui::MRichTextEdit){
    ui_->setupUi(this);

    m_lastBlockList = 0;
    ui_->f_textedit->setTabStopWidth(40);

    connect(ui_->f_textedit, &MTextEdit::textChanged, this, &MRichTextEdit::textChanged);
    connect(ui_->f_textedit, SIGNAL(currentCharFormatChanged(QTextCharFormat)),
        this, SLOT(slotCurrentCharFormatChanged(QTextCharFormat)));
    connect(ui_->f_textedit, SIGNAL(cursorPositionChanged()),
        this, SLOT(slotCursorPositionChanged()));

    m_fontsize_h1 = 18;
    m_fontsize_h2 = 16;
    m_fontsize_h3 = 14;
    m_fontsize_h4 = 12;

    fontChanged(ui_->f_textedit->font());
    bgColorChanged(ui_->f_textedit->textColor());

    // paragraph formatting

    m_paragraphItems << tr("Standard")
        << tr("Heading 1")
        << tr("Heading 2")
        << tr("Heading 3")
        << tr("Heading 4")
        << tr("Monospace");
    ui_->f_paragraph->addItems(m_paragraphItems);

    connect(ui_->f_paragraph, SIGNAL(activated(int)),
        this, SLOT(textStyle(int)));

    // undo & redo

    ui_->f_undo->setShortcut(QKeySequence::Undo);
    ui_->f_redo->setShortcut(QKeySequence::Redo);

    connect(ui_->f_textedit->document(), SIGNAL(undoAvailable(bool)),
        ui_->f_undo, SLOT(setEnabled(bool)));
    connect(ui_->f_textedit->document(), SIGNAL(redoAvailable(bool)),
        ui_->f_redo, SLOT(setEnabled(bool)));

    ui_->f_undo->setEnabled(ui_->f_textedit->document()->isUndoAvailable());
    ui_->f_redo->setEnabled(ui_->f_textedit->document()->isRedoAvailable());

    connect(ui_->f_undo, SIGNAL(clicked()), ui_->f_textedit, SLOT(undo()));
    connect(ui_->f_redo, SIGNAL(clicked()), ui_->f_textedit, SLOT(redo()));

    // cut, copy & paste

    ui_->f_cut->setShortcut(QKeySequence::Cut);
    ui_->f_copy->setShortcut(QKeySequence::Copy);
    ui_->f_paste->setShortcut(QKeySequence::Paste);

    ui_->f_cut->setEnabled(false);
    ui_->f_copy->setEnabled(false);

    connect(ui_->f_cut, SIGNAL(clicked()), ui_->f_textedit, SLOT(cut()));
    connect(ui_->f_copy, SIGNAL(clicked()), ui_->f_textedit, SLOT(copy()));
    connect(ui_->f_paste, SIGNAL(clicked()), ui_->f_textedit, SLOT(paste()));

    connect(ui_->f_textedit, SIGNAL(copyAvailable(bool)), ui_->f_cut, SLOT(setEnabled(bool)));
    connect(ui_->f_textedit, SIGNAL(copyAvailable(bool)), ui_->f_copy, SLOT(setEnabled(bool)));

#ifndef QT_NO_CLIPBOARD
    connect(QApplication::clipboard(), SIGNAL(dataChanged()), this, SLOT(slotClipboardDataChanged()));
#endif

    // link

    ui_->f_link->setShortcut(Qt::CTRL + Qt::Key_L);

    connect(ui_->f_link, SIGNAL(clicked(bool)), this, SLOT(textLink(bool)));

    // bold, italic & underline

    ui_->f_bold->setShortcut(Qt::CTRL + Qt::Key_B);
    ui_->f_italic->setShortcut(Qt::CTRL + Qt::Key_I);
    ui_->f_underline->setShortcut(Qt::CTRL + Qt::Key_U);

    connect(ui_->f_bold, SIGNAL(clicked()), this, SLOT(textBold()));
    connect(ui_->f_italic, SIGNAL(clicked()), this, SLOT(textItalic()));
    connect(ui_->f_underline, SIGNAL(clicked()), this, SLOT(textUnderline()));
    connect(ui_->f_strikeout, SIGNAL(clicked()), this, SLOT(textStrikeout()));

    QAction *removeFormat = new QAction(tr("Remove character formatting"), this);
    removeFormat->setShortcut(QKeySequence("CTRL+M"));
    connect(removeFormat, SIGNAL(triggered()), this, SLOT(textRemoveFormat()));
    ui_->f_textedit->addAction(removeFormat);

    QAction *removeAllFormat = new QAction(tr("Remove all formatting"), this);
    connect(removeAllFormat, SIGNAL(triggered()), this, SLOT(textRemoveAllFormat()));
    ui_->f_textedit->addAction(removeAllFormat);

    QAction *textsource = new QAction(tr("Edit document source"), this);
    textsource->setShortcut(QKeySequence("CTRL+O"));
    connect(textsource, SIGNAL(triggered()), this, SLOT(textSource()));
    ui_->f_textedit->addAction(textsource);

    QMenu *menu = new QMenu(this);
    menu->addAction(removeAllFormat);
    menu->addAction(removeFormat);
    menu->addAction(textsource);
    ui_->f_menu->setMenu(menu);
    ui_->f_menu->setPopupMode(QToolButton::InstantPopup);

    // lists

    ui_->f_list_bullet->setShortcut(Qt::CTRL + Qt::Key_Minus);
    ui_->f_list_ordered->setShortcut(Qt::CTRL + Qt::Key_Equal);

    connect(ui_->f_list_bullet, SIGNAL(clicked(bool)), this, SLOT(listBullet(bool)));
    connect(ui_->f_list_ordered, SIGNAL(clicked(bool)), this, SLOT(listOrdered(bool)));

    // indentation

    ui_->f_indent_dec->setShortcut(Qt::CTRL + Qt::Key_Comma);
    ui_->f_indent_inc->setShortcut(Qt::CTRL + Qt::Key_Period);

    connect(ui_->f_indent_inc, SIGNAL(clicked()), this, SLOT(increaseIndentation()));
    connect(ui_->f_indent_dec, SIGNAL(clicked()), this, SLOT(decreaseIndentation()));

    // font size

    QFontDatabase db;
    foreach(int size, db.standardSizes())
        ui_->f_fontsize->addItem(QString::number(size));

    connect(ui_->f_fontsize, SIGNAL(activated(QString)),
        this, SLOT(textSize(QString)));
    ui_->f_fontsize->setCurrentIndex(ui_->f_fontsize->findText(QString::number(QApplication::font()
        .pointSize())));

    // text foreground color

    QPixmap pix(16, 16);
    pix.fill(QApplication::palette().foreground().color());
    ui_->f_fgcolor->setIcon(pix);

    connect(ui_->f_fgcolor, SIGNAL(clicked()), this, SLOT(textFgColor()));

    // text background color

    pix.fill(QApplication::palette().background().color());
    ui_->f_bgcolor->setIcon(pix);

    connect(ui_->f_bgcolor, SIGNAL(clicked()), this, SLOT(textBgColor()));

    // images
    connect(ui_->f_image, SIGNAL(clicked()), this, SLOT(insertImage()));
}


QString MRichTextEdit::toPlainText() const
{
    return ui_->f_textedit->toPlainText();
}

void MRichTextEdit::textSource() {
    SourceEditor dlg(this);
    dlg.exec();
}


void MRichTextEdit::textRemoveFormat() {
    QTextCharFormat fmt;
    fmt.setFontWeight(QFont::Normal);
    fmt.setFontUnderline(false);
    fmt.setFontStrikeOut(false);
    fmt.setFontItalic(false);
    fmt.setFontPointSize(9);
    //  fmt.setFontFamily     ("Helvetica");
    //  fmt.setFontStyleHint  (QFont::SansSerif);
    //  fmt.setFontFixedPitch (true);

    ui_->f_bold->setChecked(false);
    ui_->f_underline->setChecked(false);
    ui_->f_italic->setChecked(false);
    ui_->f_strikeout->setChecked(false);
    ui_->f_fontsize->setCurrentIndex(ui_->f_fontsize->findText("9"));

    //  QTextBlockFormat bfmt = cursor.blockFormat();
    //  bfmt->setIndent(0);

    fmt.clearBackground();

    mergeFormatOnWordOrSelection(fmt);
}


void MRichTextEdit::textRemoveAllFormat() {
    ui_->f_bold->setChecked(false);
    ui_->f_underline->setChecked(false);
    ui_->f_italic->setChecked(false);
    ui_->f_strikeout->setChecked(false);
    ui_->f_fontsize->setCurrentIndex(ui_->f_fontsize->findText("9"));
    QString text = ui_->f_textedit->toPlainText();
    ui_->f_textedit->setPlainText(text);
}


void MRichTextEdit::textBold() {
    QTextCharFormat fmt;
    fmt.setFontWeight(ui_->f_bold->isChecked() ? QFont::Bold : QFont::Normal);
    mergeFormatOnWordOrSelection(fmt);
}


void MRichTextEdit::focusInEvent(QFocusEvent *) {
    ui_->f_textedit->setFocus(Qt::TabFocusReason);
}


void MRichTextEdit::textUnderline() {
    QTextCharFormat fmt;
    fmt.setFontUnderline(ui_->f_underline->isChecked());
    mergeFormatOnWordOrSelection(fmt);
}

void MRichTextEdit::textItalic() {
    QTextCharFormat fmt;
    fmt.setFontItalic(ui_->f_italic->isChecked());
    mergeFormatOnWordOrSelection(fmt);
}

void MRichTextEdit::textStrikeout() {
    QTextCharFormat fmt;
    fmt.setFontStrikeOut(ui_->f_strikeout->isChecked());
    mergeFormatOnWordOrSelection(fmt);
}

void MRichTextEdit::textSize(const QString &p) {
    qreal pointSize = p.toFloat();
    if (p.toFloat() > 0) {
        QTextCharFormat fmt;
        fmt.setFontPointSize(pointSize);
        mergeFormatOnWordOrSelection(fmt);
    }
}

void MRichTextEdit::textLink(bool checked) {
    bool unlink = false;
    QTextCharFormat fmt;
    if (checked) {
        QString url = ui_->f_textedit->currentCharFormat().anchorHref();
        bool ok;
        QString newUrl = QInputDialog::getText(this, tr("Create a link"),
            tr("Link URL:"), QLineEdit::Normal,
            url,
            &ok);
        if (ok) {
            fmt.setAnchor(true);
            fmt.setAnchorHref(newUrl);
            fmt.setForeground(QApplication::palette().color(QPalette::Link));
            fmt.setFontUnderline(true);
        }
        else {
            unlink = true;
        }
    }
    else {
        unlink = true;
    }
    if (unlink) {
        fmt.setAnchor(false);
        fmt.setForeground(QApplication::palette().color(QPalette::Text));
        fmt.setFontUnderline(false);
    }
    mergeFormatOnWordOrSelection(fmt);
}

void MRichTextEdit::textStyle(int index) {
    QTextCursor cursor = ui_->f_textedit->textCursor();
    cursor.beginEditBlock();

    // standard
    if (!cursor.hasSelection()) {
        cursor.select(QTextCursor::BlockUnderCursor);
    }
    QTextCharFormat fmt;
    cursor.setCharFormat(fmt);
    ui_->f_textedit->setCurrentCharFormat(fmt);

    if (index == ParagraphHeading1
        || index == ParagraphHeading2
        || index == ParagraphHeading3
        || index == ParagraphHeading4) {
        if (index == ParagraphHeading1) {
            fmt.setFontPointSize(m_fontsize_h1);
        }
        if (index == ParagraphHeading2) {
            fmt.setFontPointSize(m_fontsize_h2);
        }
        if (index == ParagraphHeading3) {
            fmt.setFontPointSize(m_fontsize_h3);
        }
        if (index == ParagraphHeading4) {
            fmt.setFontPointSize(m_fontsize_h4);
        }
        if (index == ParagraphHeading2 || index == ParagraphHeading4) {
            fmt.setFontItalic(true);
        }

        fmt.setFontWeight(QFont::Bold);
    }
    if (index == ParagraphMonospace) {
        fmt = cursor.charFormat();
        fmt.setFontFamily("Monospace");
        fmt.setFontStyleHint(QFont::Monospace);
        fmt.setFontFixedPitch(true);
    }
    cursor.setCharFormat(fmt);
    ui_->f_textedit->setCurrentCharFormat(fmt);

    cursor.endEditBlock();
}

void MRichTextEdit::textFgColor() {
    QColor col = QColorDialog::getColor(ui_->f_textedit->textColor(), this);
    QTextCursor cursor = ui_->f_textedit->textCursor();
    if (!cursor.hasSelection()) {
        cursor.select(QTextCursor::WordUnderCursor);
    }
    QTextCharFormat fmt = cursor.charFormat();
    if (col.isValid()) {
        fmt.setForeground(col);
    }
    else {
        fmt.clearForeground();
    }
    cursor.setCharFormat(fmt);
    ui_->f_textedit->setCurrentCharFormat(fmt);
    fgColorChanged(col);
}

void MRichTextEdit::textBgColor() {
    QColor col = QColorDialog::getColor(ui_->f_textedit->textBackgroundColor(), this);
    QTextCursor cursor = ui_->f_textedit->textCursor();
    if (!cursor.hasSelection()) {
        cursor.select(QTextCursor::WordUnderCursor);
    }
    QTextCharFormat fmt = cursor.charFormat();
    if (col.isValid()) {
        fmt.setBackground(col);
    }
    else {
        fmt.clearBackground();
    }
    cursor.setCharFormat(fmt);
    ui_->f_textedit->setCurrentCharFormat(fmt);
    bgColorChanged(col);
}

void MRichTextEdit::listBullet(bool checked) {
    if (checked) {
        ui_->f_list_ordered->setChecked(false);
    }
    list(checked, QTextListFormat::ListDisc);
}

void MRichTextEdit::listOrdered(bool checked) {
    if (checked) {
        ui_->f_list_bullet->setChecked(false);
    }
    list(checked, QTextListFormat::ListDecimal);
}

void MRichTextEdit::list(bool checked, QTextListFormat::Style style) {
    QTextCursor cursor = ui_->f_textedit->textCursor();
    cursor.beginEditBlock();
    if (!checked) {
        QTextBlockFormat obfmt = cursor.blockFormat();
        QTextBlockFormat bfmt;
        bfmt.setIndent(obfmt.indent());
        cursor.setBlockFormat(bfmt);
    }
    else {
        QTextListFormat listFmt;
        if (cursor.currentList()) {
            listFmt = cursor.currentList()->format();
        }
        listFmt.setStyle(style);
        cursor.createList(listFmt);
    }
    cursor.endEditBlock();
}

void MRichTextEdit::mergeFormatOnWordOrSelection(const QTextCharFormat &format) {
    QTextCursor cursor = ui_->f_textedit->textCursor();
    if (!cursor.hasSelection()) {
        cursor.select(QTextCursor::WordUnderCursor);
    }
    cursor.mergeCharFormat(format);
    ui_->f_textedit->mergeCurrentCharFormat(format);
    ui_->f_textedit->setFocus(Qt::TabFocusReason);
}

void MRichTextEdit::slotCursorPositionChanged() {
    QTextList *l = ui_->f_textedit->textCursor().currentList();
    if (m_lastBlockList && (l == m_lastBlockList || (l != 0 && m_lastBlockList != 0
        && l->format().style() == m_lastBlockList->format().style()))) {
        return;
    }
    m_lastBlockList = l;
    if (l) {
        QTextListFormat lfmt = l->format();
        if (lfmt.style() == QTextListFormat::ListDisc) {
            ui_->f_list_bullet->setChecked(true);
            ui_->f_list_ordered->setChecked(false);
        }
        else if (lfmt.style() == QTextListFormat::ListDecimal) {
            ui_->f_list_bullet->setChecked(false);
            ui_->f_list_ordered->setChecked(true);
        }
        else {
            ui_->f_list_bullet->setChecked(false);
            ui_->f_list_ordered->setChecked(false);
        }
    }
    else {
        ui_->f_list_bullet->setChecked(false);
        ui_->f_list_ordered->setChecked(false);
    }
}

void MRichTextEdit::fontChanged(const QFont &f) {
    ui_->f_fontsize->setCurrentIndex(ui_->f_fontsize->findText(QString::number(f.pointSize())));
    ui_->f_bold->setChecked(f.bold());
    ui_->f_italic->setChecked(f.italic());
    ui_->f_underline->setChecked(f.underline());
    ui_->f_strikeout->setChecked(f.strikeOut());
    if (f.pointSize() == m_fontsize_h1) {
        ui_->f_paragraph->setCurrentIndex(ParagraphHeading1);
    }
    else if (f.pointSize() == m_fontsize_h2) {
        ui_->f_paragraph->setCurrentIndex(ParagraphHeading2);
    }
    else if (f.pointSize() == m_fontsize_h3) {
        ui_->f_paragraph->setCurrentIndex(ParagraphHeading3);
    }
    else if (f.pointSize() == m_fontsize_h4) {
        ui_->f_paragraph->setCurrentIndex(ParagraphHeading4);
    }
    else {
        if (f.fixedPitch() && f.family() == "Monospace") {
            ui_->f_paragraph->setCurrentIndex(ParagraphMonospace);
        }
        else {
            ui_->f_paragraph->setCurrentIndex(ParagraphStandard);
        }
    }
    if (ui_->f_textedit->textCursor().currentList()) {
        QTextListFormat lfmt = ui_->f_textedit->textCursor().currentList()->format();
        if (lfmt.style() == QTextListFormat::ListDisc) {
            ui_->f_list_bullet->setChecked(true);
            ui_->f_list_ordered->setChecked(false);
        }
        else if (lfmt.style() == QTextListFormat::ListDecimal) {
            ui_->f_list_bullet->setChecked(false);
            ui_->f_list_ordered->setChecked(true);
        }
        else {
            ui_->f_list_bullet->setChecked(false);
            ui_->f_list_ordered->setChecked(false);
        }
    }
    else {
        ui_->f_list_bullet->setChecked(false);
        ui_->f_list_ordered->setChecked(false);
    }
}

void MRichTextEdit::fgColorChanged(const QColor &c) {
    QPixmap pix(16, 16);
    if (c.isValid()) {
        pix.fill(c);
    }
    else {
        pix.fill(QApplication::palette().foreground().color());
    }
    ui_->f_fgcolor->setIcon(pix);
}

void MRichTextEdit::bgColorChanged(const QColor &c) {
    QPixmap pix(16, 16);
    if (c.isValid()) {
        pix.fill(c);
    }
    else {
        pix.fill(QApplication::palette().background().color());
    }
    ui_->f_bgcolor->setIcon(pix);
}

void MRichTextEdit::slotCurrentCharFormatChanged(const QTextCharFormat &format) {
    fontChanged(format.font());
    bgColorChanged((format.background().isOpaque()) ? format.background().color() : QColor());
    fgColorChanged((format.foreground().isOpaque()) ? format.foreground().color() : QColor());
    ui_->f_link->setChecked(format.isAnchor());
}

void MRichTextEdit::slotClipboardDataChanged() {
#ifndef QT_NO_CLIPBOARD
    if (const QMimeData *md = QApplication::clipboard()->mimeData())
        ui_->f_paste->setEnabled(md->hasText());
#endif
}

QString MRichTextEdit::toHtml() const {
    QString s = ui_->f_textedit->toHtml();
    // convert emails to links
    s = s.replace(QRegExp("(<[^a][^>]+>(?:<span[^>]+>)?|\\s)([a-zA-Z\\d]+@[a-zA-Z\\d]+\\.[a-zA-Z]+)"), "\\1<a href=\"mailto:\\2\">\\2</a>");
    // convert links
    s = s.replace(QRegExp("(<[^a][^>]+>(?:<span[^>]+>)?|\\s)((?:https?|ftp|file)://[^\\s'\"<>]+)"), "\\1<a href=\"\\2\">\\2</a>");
    // see also: Utils::linkify()
    return s;
}

QTextDocument * MRichTextEdit::document()
{
    return ui_->f_textedit->document();
}

QTextCursor MRichTextEdit::textCursor() const
{
    return ui_->f_textedit->textCursor();
}

void MRichTextEdit::setTextCursor(const QTextCursor& cursor)
{
    ui_->f_textedit->setTextCursor(cursor);
}

void MRichTextEdit::increaseIndentation() {
    indent(+1);
}

void MRichTextEdit::decreaseIndentation() {
    indent(-1);
}

void MRichTextEdit::indent(int delta) {
    QTextCursor cursor = ui_->f_textedit->textCursor();
    cursor.beginEditBlock();
    QTextBlockFormat bfmt = cursor.blockFormat();
    int ind = bfmt.indent();
    if (ind + delta >= 0) {
        bfmt.setIndent(ind + delta);
    }
    cursor.setBlockFormat(bfmt);
    cursor.endEditBlock();
}

void MRichTextEdit::setText(const QString& text, bool html/* = false*/) {
    if (text.isEmpty()) {
        setPlainText(text);
        return;
    }

    if (html) {
        setHtml(text);
    }
    else {
        setPlainText(text);
    }
}

void MRichTextEdit::setPlainText(const QString &text)
{
    ui_->f_textedit->setPlainText(text);
}

void MRichTextEdit::setHtml(const QString &text)
{
    ui_->f_textedit->setHtml(text);
}

void MRichTextEdit::insertImage() {
    QSettings s;
    QString attdir = s.value("general/filedialog-path").toString();
    QString file = QFileDialog::getOpenFileName(this,
        tr("Select an image"),
        attdir,
        tr("JPEG (*.jpg);; GIF (*.gif);; PNG (*.png);; BMP (*.bmp);; All (*)"));
    QImage image = QImageReader(file).read();

    ui_->f_textedit->dropImage(image, QFileInfo(file).suffix().toUpper().toLocal8Bit().data());

}


