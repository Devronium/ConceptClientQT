#ifndef CONSOLEWINDOW_H
#define CONSOLEWINDOW_H

#include <QDialog>
#include "AnsiString.h"
#include "ConceptClient.h"

namespace Ui {
class ConsoleWindow;
}

class ConsoleWindow : public QDialog
{
    Q_OBJECT

public:
    CConceptClient * CC;
    explicit ConsoleWindow(QWidget *parent = 0);
    ~ConsoleWindow();
    void AddText(char *text, int len);
    void Started();

    bool started;

    AnsiString method;
    AnsiString hashsum;
    void reset(char *username, char *password, char *hint, int remember);
    int remember();
    AnsiString GetPassword();
    AnsiString GetUsername();

private slots:
    void on_OkButton_clicked();
    void on_CancelButton_clicked();

private:
    Ui::ConsoleWindow *ui;
};
#endif // CONSOLEWINDOW_H
