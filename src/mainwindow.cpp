/*
 *  Copyright (c) 2008, 2013 Kelly Schrock, John Hammen
 *
 *  This file is part of SDDM.
 *
 *  SDDM is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  SDDM is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with SDDM.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QTimer>
#include "model.h"
#include "sddm.h"

MainWindow::MainWindow(App *a, QWidget *parent) :
    QMainWindow(parent), app(a),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    QObject::connect(app, SIGNAL(midiNoteOn(int)), this, SLOT(setMeter(int)));
    QObject::connect(app, SIGNAL(openNsm(QString,QString,QString)),
                     this, SLOT(showSession(QString,QString,QString)));
    QTimer * timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(meterDecay()));
    timer->start(50);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_openImportButton_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"), QString(),
                       tr("XML Files (*.xml)"));

    if (!fileName.isEmpty()) {
        bool success = app->loadFile(fileName);
        if(success && !app->isInSession()) {
         ui->fileNameLineEdit->setText(fileName);
        }
    }

}

void MainWindow::showSession(QString fileName, QString displayName, QString clientId) {
    this->setWindowTitle(clientId + ": " + displayName);
    ui->openImportButton->setText("Import...");
    ui->fileNameLineEdit->setText(fileName);
}

void MainWindow::setMeter(int value) {
    ui->progressBar->setValue(value);
}

const int SDDM_METER_DECAY_VALUE = 3;

void MainWindow::meterDecay() {
    int value = ui->progressBar->value();
    if(value >= SDDM_METER_DECAY_VALUE) {
        ui->progressBar->setValue(value - SDDM_METER_DECAY_VALUE);
    } else if(value > 0) {
        ui->progressBar->setValue(0);
    }
}
