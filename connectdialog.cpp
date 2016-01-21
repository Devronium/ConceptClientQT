#include "connectdialog.h"
#include "ui_connectdialog.h"
#include <QCompleter>
#include <QMessageBox>
#include "AnsiString.h"
#include <sys/stat.h>
#include <unistd.h>

ConnectDialog::ConnectDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ConnectDialog) {
    ui->setupUi(this);
    setWindowIcon(QIcon(QString::fromUtf8(":/resources/conceptclienticon.png")));
    ui->label->setBuddy(ui->addressEdit);

    QStringList wordList;

    AnsiString history;
#ifdef _WIN32
    h_path = getenv("LOCALAPPDATA");
    if (!h_path.Length())
        h_path = getenv("APPDATA");

    mkdir(h_path + "/ConceptClient-private");
    h_path += (char *)"/ConceptClient-history.dat";
#else
    h_path = getenv("HOME");
    mkdir(h_path + "/.ConceptClient-private", 0777L);

    h_path += (char *)"/.ConceptClient-history";
#endif
    history.LoadFile(h_path.c_str());

    this->history = history;

    AnsiString sep     = "\n";
    int        len_sep = sep.Length();

    int pos   = history.Pos(sep);
    int index = 0;
    int start = 0;
    while (pos > 0) {
        if (pos > 1) {
            history.c_str()[pos - 1] = 0;
            wordList << history.c_str();
        }
        AnsiString temp = history;
        history = temp.c_str() + pos + len_sep - 1;
        pos     = history.Pos(sep);
    }

    QCompleter *completer = new QCompleter(wordList);
    completer->setCaseSensitivity(Qt::CaseInsensitive);

    ui->addressEdit->setCompleter(completer);

    ui->addressEdit->setText(QString::fromUtf8("concept://"));
    ui->addressEdit->addAction(QIcon(QString::fromUtf8(":/resources/gtk-index.png")), QLineEdit::LeadingPosition);
    QAction *action = ui->addressEdit->addAction(QIcon(QString::fromUtf8(":/resources/gtk-delete.png")), QLineEdit::TrailingPosition);

    QStyle *l_style = QApplication::style();

    ui->OkButton->setIcon(l_style->standardIcon(QStyle::SP_DialogOkButton));
    ui->CancelButton->setIcon(l_style->standardIcon(QStyle::SP_DialogCancelButton));

    QObject::connect(action, SIGNAL(triggered()), this, SLOT(clearHistory()));
}

void ConnectDialog::clearHistory() {
    int res = QMessageBox::question(this, "Clear history", "Are you sure you want to delete all the history?");

    if (res == QMessageBox::Yes) {
        ui->addressEdit->setCompleter(NULL);
#ifdef _WIN32
        AnsiString h_path = getenv("LOCALAPPDATA");
        if (!h_path.Length())
            h_path = getenv("APPDATA");

        mkdir(h_path + "/ConceptClient-private");
        h_path += (char *)"/ConceptClient-history.dat";
#else
        AnsiString h_path = getenv("HOME");
        mkdir(h_path + "/.ConceptClient-private", 0777L);

        h_path += (char *)"/.ConceptClient-history";
#endif
        unlink(h_path.c_str());
    }
}

ConnectDialog::~ConnectDialog() {
    delete ui;
}

AnsiString ConnectDialog::getText() {
    QString    str     = ui->addressEdit->text();
    QByteArray ba      = str.toLocal8Bit();
    const char *c_str2 = ba.data();
    AnsiString res((char *)c_str2);

    return res;
}

void ConnectDialog::setText(const char *text) {
    ui->addressEdit->setText(text);
}

void ConnectDialog::on_OkButton_clicked() {
    this->setResult(QDialog::Accepted);
    this->accept();
}

void ConnectDialog::on_CancelButton_clicked() {
    this->setResult(QDialog::Rejected);
    this->reject();
}
