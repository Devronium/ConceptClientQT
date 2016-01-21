/********************************************************************************
** Form generated from reading UI file 'connectdialog.ui'
**
** Created by: Qt User Interface Compiler version 5.2.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_CONNECTDIALOG_H
#define UI_CONNECTDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QDialog>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>

QT_BEGIN_NAMESPACE

class Ui_ConnectDialog
{
public:
    QHBoxLayout *horizontalLayout;
    QGridLayout *gridLayout;
    QLabel      *label;
    QLineEdit   *addressEdit;
    QHBoxLayout *horizontalLayout_3;
    QSpacerItem *horizontalSpacer;
    QPushButton *OkButton;
    QPushButton *CancelButton;

    void setupUi(QDialog *ConnectDialog) {
        if (ConnectDialog->objectName().isEmpty())
            ConnectDialog->setObjectName(QStringLiteral("ConnectDialog"));
        ConnectDialog->setWindowModality(Qt::ApplicationModal);
        ConnectDialog->resize(509, 90);
        horizontalLayout = new QHBoxLayout(ConnectDialog);
        horizontalLayout->setSpacing(6);
        horizontalLayout->setContentsMargins(11, 11, 11, 11);
        horizontalLayout->setObjectName(QStringLiteral("horizontalLayout"));
        horizontalLayout->setSizeConstraint(QLayout::SetDefaultConstraint);
        horizontalLayout->setContentsMargins(5, 5, 5, -1);
        gridLayout = new QGridLayout();
        gridLayout->setSpacing(6);
        gridLayout->setObjectName(QStringLiteral("gridLayout"));
        gridLayout->setSizeConstraint(QLayout::SetDefaultConstraint);
        label = new QLabel(ConnectDialog);
        label->setObjectName(QStringLiteral("label"));

        gridLayout->addWidget(label, 0, 0, 1, 1);

        addressEdit = new QLineEdit(ConnectDialog);
        addressEdit->setObjectName(QStringLiteral("addressEdit"));
        addressEdit->setClearButtonEnabled(true);

        gridLayout->addWidget(addressEdit, 1, 0, 1, 1);

        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setSpacing(6);
        horizontalLayout_3->setObjectName(QStringLiteral("horizontalLayout_3"));
        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_3->addItem(horizontalSpacer);

        OkButton = new QPushButton(ConnectDialog);
        OkButton->setObjectName(QStringLiteral("OkButton"));

        horizontalLayout_3->addWidget(OkButton);

        CancelButton = new QPushButton(ConnectDialog);
        CancelButton->setObjectName(QStringLiteral("CancelButton"));
        CancelButton->setFlat(false);

        horizontalLayout_3->addWidget(CancelButton);


        gridLayout->addLayout(horizontalLayout_3, 2, 0, 1, 1);


        horizontalLayout->addLayout(gridLayout);


        retranslateUi(ConnectDialog);

        QMetaObject::connectSlotsByName(ConnectDialog);
    } // setupUi

    void retranslateUi(QDialog *ConnectDialog) {
        ConnectDialog->setWindowTitle(QApplication::translate("ConnectDialog", "Navigate to ...", 0));
        label->setText(QApplication::translate("ConnectDialog", "<html><head/><body><p>Enter <span style=\" font-weight:600;\">shortcut path</span>, <span style=\" font-weight:600;\">concept://</span> or <span style=\" font-weight:600;\">concepts://</span> address</p></body></html>", 0));
        addressEdit->setPlaceholderText(QApplication::translate("ConnectDialog", "enter address here", 0));
        OkButton->setText(QApplication::translate("ConnectDialog", "OK", 0));
        CancelButton->setText(QApplication::translate("ConnectDialog", "Cancel", 0));
    } // retranslateUi
};

namespace Ui {
class ConnectDialog : public Ui_ConnectDialog {};
} // namespace Ui

QT_END_NAMESPACE
#endif // UI_CONNECTDIALOG_H
