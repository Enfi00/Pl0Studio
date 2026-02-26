#include "MainWindow.h"
#include "Highlighter.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QProcess>
#include <QFile>
#include <QTextStream>
#include <QHeaderView>
#include <QFontDatabase>
#include <QDebug>
#include <QDir>
#include <QMessageBox>
#include <QFileDialog>
#include <QStyle> 
#include <QStatusBar>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setupUi();
    setupStyle();
}

MainWindow::~MainWindow() {}

void MainWindow::setupStyle() {
    QString qss = R"(
        QMainWindow { background-color: #1e1e1e; }
        QToolBar { background-color: #252526; border-bottom: 1px solid #3e3e42; spacing: 10px; }
        QToolButton { color: #d4d4d4; border-radius: 3px; padding: 6px; font-weight: bold; }
        QToolButton:hover { background-color: #3e3e42; }
        QPlainTextEdit, QTextEdit, QTableWidget { background-color: #1e1e1e; color: #d4d4d4; border: 0.8px solid #3e3e42; }
        QHeaderView::section { background-color: #252526; color: #d4d4d4; border: none; border-right: 0.8px solid #3e3e42; border-bottom: 1px solid #3e3e42; }
        QTableWidget { gridline-color: #3e3e42; alternate-background-color: #252526; }
        QStatusBar { background-color: #001829; color: white; }
        QSplitter::handle { background-color: #2d2d2d; }
    )";
    this->setStyleSheet(qss);
}

void MainWindow::setupUi() {
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    // --- Toolbar ---
    QToolBar *toolbar = addToolBar("Main Toolbar");
    toolbar->setMovable(false);
    
    actOpen = new QAction(QIcon::fromTheme("document-open"), " Open", this);
    actCompile = new QAction(QIcon::fromTheme("system-run"), " Compile", this);
    actBuildRun = new QAction(QIcon::fromTheme("utilities-terminal"), " Run (C)", this);
    actBuildRun->setEnabled(false);

    toolbar->addAction(actOpen);
    toolbar->addSeparator();
    toolbar->addAction(actCompile);
    toolbar->addAction(actBuildRun);

    // Connecting Actions
    connect(actOpen, &QAction::triggered, this, &MainWindow::loadFile);
    connect(actCompile, &QAction::triggered, this, &MainWindow::runCompiler);
    connect(actBuildRun, &QAction::triggered, this, &MainWindow::buildAndRunC);

    // --- Fonts ---
    QString fontFamily = "Courier New";
    if (QFontDatabase::systemFont(QFontDatabase::FixedFont).family() != "") 
        fontFamily = QFontDatabase::systemFont(QFontDatabase::FixedFont).family();
    QFont codeFont(fontFamily, 11);
    codeFont.setStyleHint(QFont::Monospace);

    // --- Editor (CodeEditor) ---
    codeEditor = new CodeEditor();
    codeEditor->setFont(codeFont);
    codeEditor->setPlaceholderText("// Code Editor\n //Write a code or select an existing one" );
    
    new Highlighter(codeEditor->document());
    connect(codeEditor, &CodeEditor::cursorPositionChanged, this, &MainWindow::updateCursorPosition);

    cCodeViewer = new QTextEdit();
    cCodeViewer->setFont(codeFont);
    cCodeViewer->setReadOnly(true);
    cCodeViewer->setPlaceholderText("// C Output...");

    // --- Table ---
    lexemeTable = new QTableWidget();
    lexemeTable->setColumnCount(3);
    lexemeTable->setHorizontalHeaderLabels({"Line", "Type", "Value"});
    lexemeTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    lexemeTable->verticalHeader()->setVisible(false);
    lexemeTable->setAlternatingRowColors(true);

    // --- Error Log ---
    errorLog = new QTextEdit();
    errorLog->setFont(codeFont);
    errorLog->setReadOnly(true);
    errorLog->setMaximumHeight(120);
    errorLog->setStyleSheet("background-color: #181818; color: #f48771; border-top: 1.5px solid #3e3e42;");

    // --- Status Bar ---
    statusLabel = new QLabel("Ln 1, Col 1");
    statusLabel->setStyleSheet("color: white; padding: 0 10px;");
    statusBar()->addWidget(statusLabel);

    // --- Layouts ---
    QSplitter *rightSplitter = new QSplitter(Qt::Vertical);
    rightSplitter->addWidget(lexemeTable);
    rightSplitter->addWidget(cCodeViewer);
    rightSplitter->setStretchFactor(0, 1);
    rightSplitter->setStretchFactor(1, 2);

    QSplitter *mainSplitter = new QSplitter(Qt::Horizontal);
    mainSplitter->addWidget(codeEditor);
    mainSplitter->addWidget(rightSplitter);
    mainSplitter->setStretchFactor(0, 3);
    mainSplitter->setStretchFactor(1, 2);

    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0,0,0,0);
    mainLayout->addWidget(mainSplitter);
    mainLayout->addWidget(errorLog);

    resize(1200, 800);
    setWindowTitle("Igor's Studio - Pro Edition");
}

void MainWindow::updateCursorPosition() {
    QTextCursor cursor = codeEditor->textCursor();
    int y = cursor.blockNumber() + 1;
    int x = cursor.columnNumber() + 1;
    statusLabel->setText(QString("Ln %1, Col %2").arg(y).arg(x));
}

void MainWindow::loadFile() {
    QString fileName = QFileDialog::getOpenFileName(this, "Open", QDir::currentPath(), " (*.O13);;All (*)");
    if (fileName.isEmpty()) return;
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;
    codeEditor->setPlainText(file.readAll());
    file.close();
    errorLog->append("> Loaded: " + fileName);
    actBuildRun->setEnabled(false);
}

void MainWindow::runCompiler() {
    errorLog->clear();
    cCodeViewer->clear();
    lexemeTable->setRowCount(0);
    actBuildRun->setEnabled(false);

    QString tempFileName = "temp.O13";
    QFile file(tempFileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return;
    QTextStream out(&file);
    out << codeEditor->toPlainText();
    file.close();

    QProcess process;
    process.start("./pl0_core", QStringList() << tempFileName);
    if (!process.waitForFinished(3000)) { errorLog->append("Error: Timeout"); return; }

    QString stdOut = process.readAllStandardOutput();
    QString stdErr = process.readAllStandardError();

    if (process.exitCode() == 0) {
        cCodeViewer->setText(stdOut);
        errorLog->setTextColor(QColor("#6a9955"));
        errorLog->append("> Build SUCCESS.");
        actBuildRun->setEnabled(true);
    } else {
        errorLog->setTextColor(QColor("#f48771"));
        errorLog->append("> Build FAILED.");
    }
    if (!stdErr.isEmpty()) {
        errorLog->setTextColor(QColor("#d4d4d4"));
        errorLog->append(stdErr);
    }
    loadLexemeTable();
}

void MainWindow::buildAndRunC() {
    QString cCode = cCodeViewer->toPlainText();
    if (cCode.isEmpty()) return;
    QFile file("output.c");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return;
    QTextStream out(&file); out << cCode; file.close();
    
    QProcess gcc;
    gcc.start("gcc", QStringList() << "output.c" << "-o" << "prog" << "-lm");
    if (!gcc.waitForFinished(5000)) return;
    
    if (gcc.exitCode() != 0) {
        errorLog->append("GCC Error:\n" + gcc.readAllStandardError());
        return;
    }
    QProcess::startDetached("gnome-terminal", QStringList() << "--" << "bash" << "-c" << "./prog; echo; echo 'Press Enter...'; read line");
}

void MainWindow::loadLexemeTable() {
    QFile file("lexeme_table.txt");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;
    QTextStream in(&file);
    bool headerSkipped = false, errorSection = false;
    while (!in.atEnd()) {
        QString line = in.readLine();
        if (line.contains("--- Errors ---")) { errorSection = true; errorLog->append("\n [ERRORS]"); continue; }
        if (errorSection) { if (!line.trimmed().isEmpty()) errorLog->append(">> " + line.trimmed()); continue; }
        if (line.trimmed().isEmpty() || line.startsWith("-")) { if (line.startsWith("-")) headerSkipped = true; continue; }
        if (!headerSkipped || line.contains("Type")) continue;
        QStringList parts = line.split('\t', Qt::SkipEmptyParts);
        if (parts.size() >= 2) {
            int row = lexemeTable->rowCount();
            lexemeTable->insertRow(row);
            lexemeTable->setItem(row, 0, new QTableWidgetItem(parts[0].trimmed()));
            lexemeTable->setItem(row, 1, new QTableWidgetItem(parts[1].trimmed()));
            lexemeTable->setItem(row, 2, new QTableWidgetItem(parts.size()>=3?parts[2].trimmed():""));
        }
    }
    file.close();
    if (errorSection) errorLog->moveCursor(QTextCursor::End);
}
