#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include "AnsiString.h"

namespace Ui {
class LoginDialog;
}

class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoginDialog(QWidget *parent = 0);

    void reset(char *username, char *password, char *hint, int remember);
    int remember();
    AnsiString GetPassword();
    AnsiString GetUsername();

    ~LoginDialog();

private slots:
    void on_OkButton_clicked();
    void on_CloseButton_clicked();

private:
    Ui::LoginDialog *ui;
};
#endif // LOGINDIALOG_H
