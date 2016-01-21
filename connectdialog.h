#ifndef CONNECTDIALOG_H
#define CONNECTDIALOG_H

#include <QDialog>
#include "AnsiString.h"

namespace Ui {
class ConnectDialog;
}

class ConnectDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ConnectDialog(QWidget *parent = 0);
    ~ConnectDialog();
    AnsiString getText();
    void setText(const char *text);

    AnsiString history;
    AnsiString h_path;

private slots:
    void on_OkButton_clicked();
    void clearHistory();
    void on_CancelButton_clicked();

private:
    Ui::ConnectDialog *ui;
};
#endif // CONNECTDIALOG_H
