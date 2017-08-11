#pragma once

#include <QtWidgets/QDialog>
#include "ui_test.h"

class test : public QDialog
{
    Q_OBJECT

public:
    test(QWidget *parent = Q_NULLPTR);

private:
    Ui::testClass ui;
};
