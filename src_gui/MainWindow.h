#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTextEdit>
#include <QTableWidget>
#include <QAction>
#include <QToolBar>
#include <QLabel>
#include "CodeEditor.h" 

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void loadFile();         
    void runCompiler();      
    void buildAndRunC();
    void updateCursorPosition(); 

private:
    void setupUi();
    void setupStyle();
    void loadLexemeTable();

    CodeEditor *codeEditor;
    QTextEdit *cCodeViewer;
    QTableWidget *lexemeTable;
    QTextEdit *errorLog;
    QLabel *statusLabel;
    
    QAction *actOpen;
    QAction *actCompile;
    QAction *actBuildRun;
};

#endif
