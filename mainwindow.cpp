#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QFileDialog>
#include <QRegularExpression>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_exitBtn_clicked()
{
    this->close();
}


void MainWindow::on_openFileBtn_clicked()
{
    QString filename = QFileDialog::getOpenFileName(this, tr("Open File"), NULL, "JSON File (*.json)");

    bool isFileSelected = QFile::exists(filename);

    ui->jsonFileText->clear();
    ui->parseInfoTxt->clear();

    if (isFileSelected) {
        ui->filenameEdt->setText(filename);

        BusinessLogic logic;

        logic.validate(filename);

        QString contents = logic.getFileContents();
        QStringList lines = contents.split(QRegularExpression("[\r\n]"));
        int lineNumber = 1;
        for(QString line : lines) {
            ui->jsonFileText->appendPlainText(QString::number(lineNumber++) + ": " + line);
        }

        ui->parseInfoTxt->appendPlainText(logic.getParseResult());

    } else {
        ui->filenameEdt->clear();
    }
}

