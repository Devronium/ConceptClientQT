#include "consolewindow.h"
#include "ui_consolewindow.h"
#include "callback.h"

#ifdef _WIN32
 #include <QtWin>
#endif

ConsoleWindow::ConsoleWindow(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ConsoleWindow) {
    ui->setupUi(this);
    this->resize(500, 300);
    setWindowIcon(QIcon(QString::fromUtf8(":/resources/conceptclienticon.png")));
    this->ui->userframe->hide();
    QApplication::setWindowIcon(QIcon(QString::fromUtf8(":/resources/conceptclienticon.png")));

    QStyle *l_style = QApplication::style();
    ui->OkButton->setIcon(l_style->standardIcon(QStyle::SP_DialogOkButton));
    ui->CancelButton->setIcon(l_style->standardIcon(QStyle::SP_DialogCancelButton));
    this->CC = NULL;

    ui->splash->setPixmap(QPixmap(":/resources/splash.jpg"));

    setAutoFillBackground(true);
    QPalette p = palette();
    p.setColor(backgroundRole(), QColor(14, 14, 14));
    p.setColor(foregroundRole(), QColor(0xD0, 0xD0, 0xD8));
    setPalette(p);
    this->ui->label_5->setPalette(p);
    this->ui->label_6->setPalette(p);
    this->ui->radio1->setPalette(p);
    this->ui->radio2->setPalette(p);
    this->ui->radio3->setPalette(p);
    this->ui->console->hide();
    this->ui->progressBar->setMaximumHeight(3);
    this->ui->progressBar->setStyleSheet(QString("QProgressBar { border: 0px; background-color: #0E0E0E;} QProgressBar::chunk:horizontal { background: qlineargradient(x1: 0, y1: 0.5, x2: 1, y2: 0.5, stop: 0 #0E0E0E, stop: 0.8 #D0D0FF, stop: 1 #0E0E0E);}"));
    this->ui->console->setStyleSheet(QString("QWidget { border: 0px; background-color: #0E0E0E; color: #D0D0D8;}"));
    this->ui->console->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    this->ui->console->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    this->setWindowFlags(Qt::FramelessWindowHint);
    started = false;
}

void ConsoleWindow::AddText(char *text, int len) {
    QString str(text);

    if (!started) {
        this->ui->progressBar->hide();
        ui->splash->hide();
        ui->console->show();
        this->setWindowFlags(Qt::Dialog);
        started = true;
    }
    ui->console->moveCursor(QTextCursor::End);
    ui->console->insertPlainText(str);
    ui->console->moveCursor(QTextCursor::End);
}

void ConsoleWindow::Started() {
    if (started)
        return;
    started = true;
    ui->console->show();
    ui->splash->hide();
    this->ui->progressBar->hide();
    this->setWindowFlags(Qt::Dialog);
}

void ConsoleWindow::reset(char *username, char *password, char *hint, int remember) {
    this->ui->userframe->show();
    if ((remember < 0) || (remember > 2))
        remember = 0;
    ui->username->setText(username);
    ui->password->setText(password);
    ui->hint->setText(hint);

    switch (remember) {
        case 1:
            ui->radio2->setChecked(true);
            break;

        case 2:
            ui->radio3->setChecked(true);
            break;

        default:
            ui->radio1->setChecked(true);
    }

    if ((username) && (username[0]))
        ui->password->setFocus();
    else
        ui->username->setFocus();
}

void ConsoleWindow::on_OkButton_clicked() {
    this->ui->userframe->hide();
    if (CC) {
        if (this->method == (char *)"MD5") {
            CC->SendMessageNoWait("%CLIENT", MSG_MESSAGE_LOGIN, "MD5", AnsiString((long)RESPONSE_OK));
            CC->SendMessageNoWait("%CLIENT", MSG_MESSAGE_LOGIN, this->GetUsername(), do_md5(CC->POST_TARGET + do_md5(this->GetPassword())));
        } else
        if (this->method == (char *)"SHA1") {
            CC->SendMessageNoWait("%CLIENT", MSG_MESSAGE_LOGIN, "SHA1", AnsiString((long)RESPONSE_OK));
            CC->SendMessageNoWait("%CLIENT", MSG_MESSAGE_LOGIN, this->GetUsername(), do_sha1(CC->POST_TARGET + do_sha1(this->GetPassword())));
        } else {
            CC->SendMessageNoWait("%CLIENT", MSG_MESSAGE_LOGIN, "PLAIN", AnsiString((long)RESPONSE_OK));
            CC->SendMessageNoWait("%CLIENT", MSG_MESSAGE_LOGIN, this->GetUsername(), this->GetPassword());
        }
        if (hashsum.Length())
            SetCachedLogin(this->GetUsername(), this->GetPassword(), this->remember(), &hashsum);
    }
    adjustSize();
}

void ConsoleWindow::on_CancelButton_clicked() {
    this->ui->userframe->hide();
    if (CC) {
        if (this->method == (char *)"MD5") {
            CC->SendMessageNoWait("%CLIENT", MSG_MESSAGE_LOGIN, "MD5", AnsiString((long)RESPONSE_CLOSE));
            CC->SendMessageNoWait("%CLIENT", MSG_MESSAGE_LOGIN, this->GetUsername(), do_md5(CC->POST_TARGET + do_md5(this->GetPassword())));
        } else
        if (this->method == (char *)"SHA1") {
            CC->SendMessageNoWait("%CLIENT", MSG_MESSAGE_LOGIN, "SHA1", AnsiString((long)RESPONSE_CLOSE));
            CC->SendMessageNoWait("%CLIENT", MSG_MESSAGE_LOGIN, this->GetUsername(), do_sha1(CC->POST_TARGET + do_sha1(this->GetPassword())));
        } else {
            CC->SendMessageNoWait("%CLIENT", MSG_MESSAGE_LOGIN, "PLAIN", AnsiString((long)RESPONSE_CLOSE));
            CC->SendMessageNoWait("%CLIENT", MSG_MESSAGE_LOGIN, this->GetUsername(), this->GetPassword());
        }
    }
    adjustSize();
}

int ConsoleWindow::remember() {
    if (ui->radio2->isChecked())
        return 1;
    if (ui->radio3->isChecked())
        return 2;
    return 0;
}

AnsiString ConsoleWindow::GetPassword() {
    return AnsiString((char *)ui->password->text().toUtf8().constData());
}

AnsiString ConsoleWindow::GetUsername() {
    return AnsiString((char *)ui->username->text().toUtf8().constData());
}

ConsoleWindow::~ConsoleWindow() {
    delete ui;
}
