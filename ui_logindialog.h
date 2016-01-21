/********************************************************************************
** Form generated from reading UI file 'logindialog.ui'
**
** Created by: Qt User Interface Compiler version 5.2.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_LOGINDIALOG_H
#define UI_LOGINDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QDialog>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_LoginDialog
{
public:
    QVBoxLayout  *verticalLayout;
    QLabel       *hint;
    QLabel       *label_8;
    QLineEdit    *username;
    QLabel       *label_9;
    QLineEdit    *password;
    QRadioButton *radio1;
    QRadioButton *radio2;
    QRadioButton *radio3;
    QHBoxLayout  *horizontalLayout;
    QSpacerItem  *horizontalSpacer;
    QPushButton  *OkButton;
    QPushButton  *CloseButton;

    void setupUi(QDialog *LoginDialog) {
        if (LoginDialog->objectName().isEmpty())
            LoginDialog->setObjectName(QStringLiteral("LoginDialog"));
        LoginDialog->resize(212, 221);
        LoginDialog->setLayoutDirection(Qt::LeftToRight);
        verticalLayout = new QVBoxLayout(LoginDialog);
        verticalLayout->setSpacing(6);
        verticalLayout->setContentsMargins(11, 11, 11, 11);
        verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
        hint = new QLabel(LoginDialog);
        hint->setObjectName(QStringLiteral("hint"));

        verticalLayout->addWidget(hint);

        label_8 = new QLabel(LoginDialog);
        label_8->setObjectName(QStringLiteral("label_8"));

        verticalLayout->addWidget(label_8);

        username = new QLineEdit(LoginDialog);
        username->setObjectName(QStringLiteral("username"));
        QSizePolicy sizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(username->sizePolicy().hasHeightForWidth());
        username->setSizePolicy(sizePolicy);

        verticalLayout->addWidget(username);

        label_9 = new QLabel(LoginDialog);
        label_9->setObjectName(QStringLiteral("label_9"));

        verticalLayout->addWidget(label_9);

        password = new QLineEdit(LoginDialog);
        password->setObjectName(QStringLiteral("password"));
        sizePolicy.setHeightForWidth(password->sizePolicy().hasHeightForWidth());
        password->setSizePolicy(sizePolicy);
        password->setEchoMode(QLineEdit::Password);

        verticalLayout->addWidget(password);

        radio1 = new QRadioButton(LoginDialog);
        radio1->setObjectName(QStringLiteral("radio1"));

        verticalLayout->addWidget(radio1);

        radio2 = new QRadioButton(LoginDialog);
        radio2->setObjectName(QStringLiteral("radio2"));

        verticalLayout->addWidget(radio2);

        radio3 = new QRadioButton(LoginDialog);
        radio3->setObjectName(QStringLiteral("radio3"));
        radio3->setChecked(true);

        verticalLayout->addWidget(radio3);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setSpacing(6);
        horizontalLayout->setObjectName(QStringLiteral("horizontalLayout"));
        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);

        OkButton = new QPushButton(LoginDialog);
        OkButton->setObjectName(QStringLiteral("OkButton"));

        horizontalLayout->addWidget(OkButton);

        CloseButton = new QPushButton(LoginDialog);
        CloseButton->setObjectName(QStringLiteral("CloseButton"));

        horizontalLayout->addWidget(CloseButton);


        verticalLayout->addLayout(horizontalLayout);


        retranslateUi(LoginDialog);

        QMetaObject::connectSlotsByName(LoginDialog);
    } // setupUi

    void retranslateUi(QDialog *LoginDialog) {
        LoginDialog->setWindowTitle(QApplication::translate("LoginDialog", "Login required", 0));
        hint->setText(QApplication::translate("LoginDialog", "Please enter your credentials", 0));
        label_8->setText(QApplication::translate("LoginDialog", "<b>Username</b>", 0));
        username->setPlaceholderText(QApplication::translate("LoginDialog", "Enter username", 0));
        label_9->setText(QApplication::translate("LoginDialog", "<b>Password</b>", 0));
        password->setPlaceholderText(QApplication::translate("LoginDialog", "Enter password", 0));
        radio1->setText(QApplication::translate("LoginDialog", "Don't remember me", 0));
        radio2->setText(QApplication::translate("LoginDialog", "Remember username", 0));
        radio3->setText(QApplication::translate("LoginDialog", "Remember username and password", 0));
        OkButton->setText(QApplication::translate("LoginDialog", "OK", 0));
        CloseButton->setText(QApplication::translate("LoginDialog", "Cancel", 0));
    } // retranslateUi
};

namespace Ui {
class LoginDialog : public Ui_LoginDialog {};
} // namespace Ui

QT_END_NAMESPACE
#endif // UI_LOGINDIALOG_H
