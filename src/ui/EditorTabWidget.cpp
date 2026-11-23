#include "EditorTabWidget.h"
#include "../editors/BaseEditor.h"
#include "../editors/EditorFactory.h"

#include <QLabel>
#include <QMessageBox>
#include <QDir>
#include <QFileInfo>

EditorTabWidget::EditorTabWidget(QWidget* parent)
    : QTabWidget(parent)
{
    setTabsClosable(true);
    setMovable(true);
    setDocumentMode(true);

    connect(this, &QTabWidget::tabCloseRequested, this, &EditorTabWidget::onTabCloseRequested);
    connect(this, &QTabWidget::currentChanged,    this, &EditorTabWidget::onCurrentChanged);
}

QString EditorTabWidget::pathKey(const QString& path)
{
    // Both slashes, '.' and '..', and a relative path all collapse here. The
    // spellings that reach this class really do differ: a file just made by
    // File > New File arrives as rootPath + QDir::separator() + name, with a
    // backslash in it, while the Solution Explorer hands over what
    // QDirIterator produced, which is all forward slashes.
    const QString abs =
        QDir::cleanPath(QFileInfo(QDir::fromNativeSeparators(path)).absoluteFilePath());
#ifdef Q_OS_WIN
    // Case names no file of its own on Windows.
    return abs.toLower();
#else
    return abs;
#endif
}

void EditorTabWidget::openFile(const QString& absolutePath, Project* /*project*/)
{
    const QString key = pathKey(absolutePath);
    if (m_openFiles.contains(key)) {
        setCurrentWidget(m_openFiles[key]);
        return;
    }

    auto* editor = EditorFactory::create(absolutePath, nullptr, this);
    m_openFiles[key] = editor;

    QString title = QFileInfo(absolutePath).fileName();
    int idx = addTab(editor, title);
    setCurrentIndex(idx);

    connect(editor, &BaseEditor::fileOpenRequested,
            this,   &EditorTabWidget::fileOpenRequested);

    connect(editor, &BaseEditor::modificationChanged, this, [this, editor](bool dirty) {
        int i = indexOf(editor);
        if (i < 0) return;
        QString t = tabText(i);
        if (dirty && !t.endsWith('*'))
            setTabText(i, t + '*');
        else if (!dirty && t.endsWith('*'))
            setTabText(i, t.chopped(1));
    });
}

BaseEditor* EditorTabWidget::currentEditor() const
{
    return qobject_cast<BaseEditor*>(currentWidget());
}

BaseEditor* EditorTabWidget::editorForPath(const QString& absolutePath) const
{
    return m_openFiles.value(pathKey(absolutePath), nullptr);
}

bool EditorTabWidget::saveCurrentFile()
{
    if (auto* ed = currentEditor())
        return ed->save();
    return false;
}

bool EditorTabWidget::saveAllFiles()
{
    bool ok = true;
    for (auto* ed : m_openFiles)
        if (!ed->save()) ok = false;
    return ok;
}

void EditorTabWidget::closeFile(const QString& absolutePath)
{
    auto* ed = m_openFiles.value(pathKey(absolutePath), nullptr);
    if (!ed) return;
    closeTab(indexOf(ed));
}

bool EditorTabWidget::closeTab(int index)
{
    auto* ed = qobject_cast<BaseEditor*>(widget(index));
    if (!ed) return true;

    if (ed->isDirty()) {
        auto btn = QMessageBox::question(this, tr("Unsaved Changes"),
            tr("'%1' has unsaved changes. Save before closing?")
                .arg(QFileInfo(ed->filePath()).fileName()),
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        if (btn == QMessageBox::Cancel) return false;
        if (btn == QMessageBox::Save)   ed->save();
    }

    // By key, and by value as well: an editor whose path changed while it was
    // open (a rename) would otherwise leave a dangling pointer in the map for
    // the next openFile() to switch to.
    m_openFiles.remove(pathKey(ed->filePath()));
    for (auto it = m_openFiles.begin(); it != m_openFiles.end(); ) {
        if (it.value() == ed) it = m_openFiles.erase(it);
        else                  ++it;
    }

    removeTab(index);
    ed->deleteLater();
    return true;
}

void EditorTabWidget::onTabCloseRequested(int index)
{
    closeTab(index);
}

void EditorTabWidget::onCurrentChanged(int index)
{
    auto* ed = qobject_cast<BaseEditor*>(widget(index));
    emit currentFileChanged(ed ? ed->filePath() : QString());
}
