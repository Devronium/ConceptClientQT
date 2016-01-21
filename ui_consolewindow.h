/********************************************************************************
** Form generated from reading UI file 'consolewindow.ui'
**
** Created by: Qt User Interface Compiler version 5.2.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_CONSOLEWINDOW_H
#define UI_CONSOLEWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QDialog>
#include <QtWidgets/QFrame>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_ConsoleWindow
{
public:
    QHBoxLayout    *horizontalLayout_2;
    QHBoxLayout    *horizontalLayout_3;
    QVBoxLayout    *verticalLayout;
    QLabel         *splash;
    QProgressBar   *progressBar;
    QPlainTextEdit *console;
    QFrame         *userframe;
    QVBoxLayout    *verticalLayout_2;
    QVBoxLayout    *userlayout_2;
    QLabel         *hint;
    QLabel         *label_5;
    QLineEdit      *username;
    QLabel         *label_6;
    QLineEdit      *password;
    QRadioButton   *radio1;
    QRadioButton   *radio2;
    QRadioButton   *radio3;
    QHBoxLayout    *horizontalLayout_5;
    QPushButton    *OkButton;
    QPushButton    *CancelButton;
    QSpacerItem    *verticalSpacer_2;

    void setupUi(QDialog *ConsoleWindow) {
        if (ConsoleWindow->objectName().isEmpty())
            ConsoleWindow->setObjectName(QStringLiteral("ConsoleWindow"));
        ConsoleWindow->resize(600, 400);
        horizontalLayout_2 = new QHBoxLayout(ConsoleWindow);
        horizontalLayout_2->setSpacing(0);
        horizontalLayout_2->setContentsMargins(11, 11, 11, 11);
        horizontalLayout_2->setObjectName(QStringLiteral("horizontalLayout_2"));
        horizontalLayout_2->setContentsMargins(0, 0, 0, 0);
        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setSpacing(6);
        horizontalLayout_3->setObjectName(QStringLiteral("horizontalLayout_3"));
        verticalLayout = new QVBoxLayout();
        verticalLayout->setSpacing(0);
        verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
        splash = new QLabel(ConsoleWindow);
        splash->setObjectName(QStringLiteral("splash"));
        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(splash->sizePolicy().hasHeightForWidth());
        splash->setSizePolicy(sizePolicy);

        verticalLayout->addWidget(splash);

        progressBar = new QProgressBar(ConsoleWindow);
        progressBar->setObjectName(QStringLiteral("progressBar"));
        QSizePolicy sizePolicy1(QSizePolicy::Expanding, QSizePolicy::Minimum);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(progressBar->sizePolicy().hasHeightForWidth());
        progressBar->setSizePolicy(sizePolicy1);
        progressBar->setCursor(QCursor(Qt::ArrowCursor));
        progressBar->setMaximum(0);
        progressBar->setValue(0);
        progressBar->setTextVisible(false);
        progressBar->setInvertedAppearance(false);
        progressBar->setFormat(QStringLiteral(""));

        verticalLayout->addWidget(progressBar);

        console = new QPlainTextEdit(ConsoleWindow);
        console->setObjectName(QStringLiteral("console"));
        console->setEnabled(true);
        console->setReadOnly(true);
        console->setBackgroundVisible(false);
        console->setCenterOnScroll(false);

        verticalLayout->addWidget(console);


        horizontalLayout_3->addLayout(verticalLayout);

        userframe = new QFrame(ConsoleWindow);
        userframe->setObjectName(QStringLiteral("userframe"));
        userframe->setFrameShape(QFrame::StyledPanel);
        userframe->setFrameShadow(QFrame::Raised);
        verticalLayout_2 = new QVBoxLayout(userframe);
        verticalLayout_2->setSpacing(6);
        verticalLayout_2->setContentsMargins(11, 11, 11, 11);
        verticalLayout_2->setObjectName(QStringLiteral("verticalLayout_2"));
        userlayout_2 = new QVBoxLayout();
        userlayout_2->setSpacing(6);
        userlayout_2->setObjectName(QStringLiteral("userlayout_2"));
        userlayout_2->setSizeConstraint(QLayout::SetMinimumSize);
        hint = new QLabel(userframe);
        hint->setObjectName(QStringLiteral("hint"));

        userlayout_2->addWidget(hint);

        label_5 = new QLabel(userframe);
        label_5->setObjectName(QStringLiteral("label_5"));

        userlayout_2->addWidget(label_5);

        username = new QLineEdit(userframe);
        username->setObjectName(QStringLiteral("username"));
        QSizePolicy sizePolicy2(QSizePolicy::Minimum, QSizePolicy::Fixed);
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(username->sizePolicy().hasHeightForWidth());
        username->setSizePolicy(sizePolicy2);

        userlayout_2->addWidget(username);

        label_6 = new QLabel(userframe);
        label_6->setObjectName(QStringLiteral("label_6"));

        userlayout_2->addWidget(label_6);

        password = new QLineEdit(userframe);
        password->setObjectName(QStringLiteral("password"));
        sizePolicy2.setHeightForWidth(password->sizePolicy().hasHeightForWidth());
        password->setSizePolicy(sizePolicy2);
        password->setEchoMode(QLineEdit::Password);

        userlayout_2->addWidget(password);

        radio1 = new QRadioButton(userframe);
        radio1->setObjectName(QStringLiteral("radio1"));

        userlayout_2->addWidget(radio1);

        radio2 = new QRadioButton(userframe);
        radio2->setObjectName(QStringLiteral("radio2"));

        userlayout_2->addWidget(radio2);

        radio3 = new QRadioButton(userframe);
        radio3->setObjectName(QStringLiteral("radio3"));
        radio3->setChecked(true);

        userlayout_2->addWidget(radio3);

        horizontalLayout_5 = new QHBoxLayout();
        horizontalLayout_5->setSpacing(6);
        horizontalLayout_5->setObjectName(QStringLiteral("horizontalLayout_5"));
        OkButton = new QPushButton(userframe);
        OkButton->setObjectName(QStringLiteral("OkButton"));

        horizontalLayout_5->addWidget(OkButton);

        CancelButton = new QPushButton(userframe);
        CancelButton->setObjectName(QStringLiteral("CancelButton"));

        horizontalLayout_5->addWidget(CancelButton);


        userlayout_2->addLayout(horizontalLayout_5);

        verticalSpacer_2 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        userlayout_2->addItem(verticalSpacer_2);


        verticalLayout_2->addLayout(userlayout_2);


        horizontalLayout_3->addWidget(userframe);


        horizontalLayout_2->addLayout(horizontalLayout_3);


        retranslateUi(ConsoleWindow);

        QMetaObject::connectSlotsByName(ConsoleWindow);
    } // setupUi

    void retranslateUi(QDialog *ConsoleWindow) {
        ConsoleWindow->setWindowTitle(QApplication::translate("ConsoleWindow", "Console", 0));
        splash->setText(QString());
        hint->setText(QApplication::translate("ConsoleWindow", "Please enter your credentials", 0));
        label_5->setText(QApplication::translate("ConsoleWindow", "<b>Username</b>", 0));
        username->setPlaceholderText(QApplication::translate("ConsoleWindow", "Enter username", 0));
        label_6->setText(QApplication::translate("ConsoleWindow", "<b>Password</b>", 0));
        password->setPlaceholderText(QApplication::translate("ConsoleWindow", "Enter password", 0));
        radio1->setText(QApplication::translate("ConsoleWindow", "Don't remember me", 0));
        radio2->setText(QApplication::translate("ConsoleWindow", "Remember username", 0));
        radio3->setText(QApplication::translate("ConsoleWindow", "Remember username and password", 0));
        OkButton->setText(QApplication::translate("ConsoleWindow", "OK", 0));
        CancelButton->setText(QApplication::translate("ConsoleWindow", "Cancel", 0));
    } // retranslateUi
};

namespace Ui {
class ConsoleWindow : public Ui_ConsoleWindow {};
} // namespace Ui

QT_END_NAMESPACE
#endif // UI_CONSOLEWINDOW_H
