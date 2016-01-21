#include "logindialog.h"
#include "ui_logindialog.h"

LoginDialog::LoginDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LoginDialog) {
    ui->setupUi(this);
    setWindowIcon(QIcon(QString::fromUtf8(":/resources/conceptclienticon.png")));

    QStyle *l_style = QApplication::style();
    ui->OkButton->setIcon(l_style->standardIcon(QStyle::SP_DialogOkButton));
    ui->CloseButton->setIcon(l_style->standardIcon(QStyle::SP_DialogCancelButton));
}

LoginDialog::~LoginDialog() {
    delete ui;
}

void LoginDialog::reset(char *username, char *password, char *hint, int remember) {
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

void LoginDialog::on_OkButton_clicked() {
    this->setResult(QDialog::Accepted);
    this->accept();
}

void LoginDialog::on_CloseButton_clicked() {
    this->setResult(QDialog::Rejected);
    this->reject();
}

int LoginDialog::remember() {
    if (ui->radio2->isChecked())
        return 1;
    if (ui->radio3->isChecked())
        return 2;
    return 0;
}

AnsiString LoginDialog::GetPassword() {
    return AnsiString((char *)ui->password->text().toUtf8().constData());
}

AnsiString LoginDialog::GetUsername() {
    return AnsiString((char *)ui->username->text().toUtf8().constData());
}
