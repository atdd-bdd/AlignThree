#pragma once

#include <QTabWidget>
#include <QMap>

class BaseEditor;
class Project;

class EditorTabWidget : public QTabWidget
{
    Q_OBJECT

public:
    explicit EditorTabWidget(QWidget* parent = nullptr);

    void openFile(const QString& absolutePath, Project* project = nullptr);
    BaseEditor* currentEditor() const;
    BaseEditor* editorForPath(const QString& absolutePath) const;

    bool saveCurrentFile();
    bool saveAllFiles();
    void closeFile(const QString& absolutePath);

signals:
    void currentFileChanged(const QString& absolutePath);
    void fileOpenRequested(const QString& absolutePath);

private slots:
    void onTabCloseRequested(int index);
    void onCurrentChanged(int index);

private:
    bool closeTab(int index);

    // One spelling of a path, for the map below. Windows names the same file
    // several ways -- either slash, any case -- and the callers build their
    // paths differently, so a map keyed on whatever string arrived misses a
    // tab that is already open and puts up a second one on the same file.
    static QString pathKey(const QString& path);

    // Keyed by pathKey(), not by the caller's spelling.
    QMap<QString, BaseEditor*> m_openFiles;
};
