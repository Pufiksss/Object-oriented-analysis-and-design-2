#include <QApplication>
#include <QWidget>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QSpinBox>
#include <QColorDialog>
#include <QFileDialog>
#include <QMessageBox>
#include <QToolBar>
#include <QAction>
#include <QStatusBar>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QImage>
#include <QPixmap>
#include <QColor>
#include <QPoint>
#include <QRect>
#include <QString>
#include <QObject>

#include <vector>
#include <stdexcept>


// =============== Подсистема 1: Canvas ===============
//
// Хранит QImage и умеет рисовать на нём примитивы.
// Ничего не знает ни про инструменты, ни про историю, ни про файлы.

enum class ShapeType { Pencil, Line, Rect, Oval, Eraser };

class Canvas {
    QImage image_;
public:
    Canvas(int w, int h) {
        image_ = QImage(w, h, QImage::Format_ARGB32);
        image_.fill(Qt::white);
    }

    QImage image() const { return image_; }

    void setImage(const QImage& img) { image_ = img; }

    void clear() { image_.fill(Qt::white); }

    int width()  const { return image_.width(); }
    int height() const { return image_.height(); }

    // Рисует штрих карандаша/ластика между двумя точками.
    // Ластик — это просто белый карандаш.
    void drawStroke(const QPoint& from, const QPoint& to,
                    const QColor& color, int width) {
        QPainter p(&image_);
        p.setRenderHint(QPainter::Antialiasing, true);
        QPen pen(color, width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        p.setPen(pen);
        p.drawLine(from, to);
    }

    void drawShape(ShapeType type, const QPoint& from, const QPoint& to,
                   const QColor& color, int width) {
        QPainter p(&image_);
        p.setRenderHint(QPainter::Antialiasing, true);
        QPen pen(color, width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);

        if (type == ShapeType::Line) {
            p.drawLine(from, to);
        } else if (type == ShapeType::Rect) {
            p.drawRect(QRect(from, to).normalized());
        } else if (type == ShapeType::Oval) {
            p.drawEllipse(QRect(from, to).normalized());
        }
    }
};


// =============== Подсистема 2: ToolManager ===============
//
// Хранит выбранный инструмент, цвет и толщину линии.
class ToolManager {
    ShapeType tool_ = ShapeType::Pencil;
    QColor    color_ = Qt::black;
    int       width_ = 3;
public:
    ShapeType tool()  const { return tool_; }
    QColor    color() const { return color_; }
    int       width() const { return width_; }

    void setTool(ShapeType t)         { tool_ = t; }
    void setColor(const QColor& c)    { color_ = c; }
    void setWidth(int w)              { width_ = w; }

    // Если выбран ластик, рисуем белым (вне зависимости от цвета).
    QColor effectiveColor() const {
        if (tool_ == ShapeType::Eraser) return Qt::white;
        return color_;
    }
};


// =============== Подсистема 3: History ===============
//
// Хранит снимки QImage (стек undo и стек redo).
// Каждый push фиксирует состояние ДО изменения.

class History {
    std::vector<QImage> undoStack_;
    std::vector<QImage> redoStack_;
    static constexpr int kMaxDepth = 50;
public:
    void push(const QImage& snapshot) {
        undoStack_.push_back(snapshot);
        if (static_cast<int>(undoStack_.size()) > kMaxDepth)
            undoStack_.erase(undoStack_.begin());
        redoStack_.clear();
    }

    bool canUndo() const { return !undoStack_.empty(); }
    bool canRedo() const { return !redoStack_.empty(); }

    // Возвращает состояние, в которое нужно откатиться.
    // Текущее состояние холста сохраняется в redo-стек.
    QImage undo(const QImage& current) {
        QImage prev = undoStack_.back();
        undoStack_.pop_back();
        redoStack_.push_back(current);
        return prev;
    }

    QImage redo(const QImage& current) {
        QImage next = redoStack_.back();
        redoStack_.pop_back();
        undoStack_.push_back(current);
        return next;
    }

    void clear() { undoStack_.clear(); redoStack_.clear(); }
};


// =============== Подсистема 4: FileIO ===============
//
// Сохраняет и загружает QImage как PNG.

class FileIO {
public:
    void savePng(const QImage& image, const QString& path) {
        if (!image.save(path, "PNG"))
            throw std::runtime_error("Не удалось сохранить файл");
    }

    QImage loadPng(const QString& path) {
        QImage img;
        if (!img.load(path))
            throw std::runtime_error("Не удалось загрузить файл");
        return img.convertToFormat(QImage::Format_ARGB32);
    }
};


class Editor {
    Canvas      canvas_;
    ToolManager tools_;
    History     history_;
    FileIO      io_;
public:
    Editor(int w, int h) : canvas_(w, h) {}

    QImage image() const { return canvas_.image(); }
    int    width()  const { return canvas_.width(); }
    int    height() const { return canvas_.height(); }

    void setTool(ShapeType t)      { tools_.setTool(t); }
    void setColor(const QColor& c) { tools_.setColor(c); }
    void setWidth(int w)           { tools_.setWidth(w); }
    ShapeType currentTool() const  { return tools_.tool(); }
    QColor    currentColor() const { return tools_.color(); }

    bool canUndo() const { return history_.canUndo(); }
    bool canRedo() const { return history_.canRedo(); }

    void beginAction() {
        history_.push(canvas_.image());
    }

    void undo() {
        if (!history_.canUndo()) return;
        canvas_.setImage(history_.undo(canvas_.image()));
    }

    void redo() {
        if (!history_.canRedo()) return;
        canvas_.setImage(history_.redo(canvas_.image()));
    }

    void strokeTo(const QPoint& from, const QPoint& to) {
        canvas_.drawStroke(from, to,
                           tools_.effectiveColor(), tools_.width());
    }

    void commitShape(const QPoint& from, const QPoint& to) {
        canvas_.drawShape(tools_.tool(), from, to,
                          tools_.effectiveColor(), tools_.width());
    }

    void save(const QString& path) {
        io_.savePng(canvas_.image(), path);
    }

    void load(const QString& path) {
        history_.push(canvas_.image());
        canvas_.setImage(io_.loadPng(path));
    }

    void clearAll() {
        history_.push(canvas_.image());
        canvas_.clear();
    }
};



class EditorApp : public QMainWindow {
    Q_OBJECT

    Editor editor_;

    bool    drawing_ = false;
    QPoint  startPoint_;
    QPoint  lastPoint_;
    QImage  preDragSnapshot_;
    QWidget*  canvasWidget_ = nullptr;
    QAction*  undoAct_ = nullptr;
    QAction*  redoAct_ = nullptr;

public:
    EditorApp() : editor_(800, 600) {
        setWindowTitle("ShapeEditor (with Facade)");
        resize(1100, 720);

        buildToolbar();
        buildCanvas();
        statusBar()->showMessage("Готово");

        applyStyle();
        updateActions();
    }

protected:
    bool eventFilter(QObject* obj, QEvent* ev) override {
        if (obj != canvasWidget_) return QMainWindow::eventFilter(obj, ev);

        if (ev->type() == QEvent::MouseButtonPress) {
            auto* me = static_cast<QMouseEvent*>(ev);
            if (me->button() == Qt::LeftButton) {
                drawing_ = true;
                startPoint_ = me->pos();
                lastPoint_  = me->pos();
                editor_.beginAction();
                preDragSnapshot_ = editor_.image();
                updateActions();
            }
            return true;
        }

        if (ev->type() == QEvent::MouseMove && drawing_) {
            auto* me = static_cast<QMouseEvent*>(ev);
            ShapeType t = editor_.currentTool();
            if (t == ShapeType::Pencil || t == ShapeType::Eraser) {
                editor_.strokeTo(lastPoint_, me->pos());
                lastPoint_ = me->pos();
                canvasWidget_->update();
            } else {
                drawShapePreview(me->pos());
            }
            return true;
        }

        if (ev->type() == QEvent::MouseButtonRelease && drawing_) {
            auto* me = static_cast<QMouseEvent*>(ev);
            ShapeType t = editor_.currentTool();
            if (t == ShapeType::Line || t == ShapeType::Rect || t == ShapeType::Oval) {
                resetToSnapshot();
                editor_.commitShape(startPoint_, me->pos());
                canvasWidget_->update();
            }
            drawing_ = false;
            return true;
        }

        if (ev->type() == QEvent::Paint && obj == canvasWidget_) {
            QPainter p(canvasWidget_);
            p.drawImage(0, 0, editor_.image());
            p.setPen(QColor("#cdd6f4"));
            p.drawRect(0, 0, editor_.width() - 1, editor_.height() - 1);
            return true;
        }

        return QMainWindow::eventFilter(obj, ev);
    }

private:
    void drawShapePreview(const QPoint& current) {

        editor_.undo();
        editor_.beginAction();
        editor_.commitShape(startPoint_, current);
        canvasWidget_->update();
    }

    void resetToSnapshot() {
        editor_.undo();
        editor_.beginAction();
    }

    void buildToolbar() {
        auto* tb = addToolBar("Tools");
        tb->setMovable(false);

        auto addToolAction = [&](const QString& text, ShapeType type) {
            auto* a = tb->addAction(text);
            a->setCheckable(true);
            connect(a, &QAction::triggered, this, [this, type, a, tb]() {
                editor_.setTool(type);
                for (QAction* other : tb->actions()) {
                    if (other->isCheckable()) other->setChecked(other == a);
                }
                statusBar()->showMessage(QString("Инструмент: %1").arg(a->text()));
            });
            return a;
        };

        auto* pencil = addToolAction("✏️ Карандаш", ShapeType::Pencil);
        addToolAction("📏 Линия",        ShapeType::Line);
        addToolAction("▭ Прямоугольник", ShapeType::Rect);
        addToolAction("◯ Овал",          ShapeType::Oval);
        addToolAction("🧽 Ластик",       ShapeType::Eraser);
        pencil->setChecked(true);

        tb->addSeparator();

        auto* colorBtn = new QPushButton("🎨 Цвет");
        connect(colorBtn, &QPushButton::clicked, this, [this, colorBtn]() {
            QColor c = QColorDialog::getColor(editor_.currentColor(), this, "Выбор цвета");
            if (c.isValid()) {
                editor_.setColor(c);
                colorBtn->setStyleSheet(
                    QString("QPushButton{background-color:%1;color:%2;}")
                        .arg(c.name())
                        .arg(c.lightness() > 128 ? "#1e1e2e" : "#ffffff"));
            }
        });
        tb->addWidget(colorBtn);

        tb->addWidget(new QLabel("  Толщина: "));
        auto* widthBox = new QSpinBox();
        widthBox->setRange(1, 40);
        widthBox->setValue(3);
        connect(widthBox, QOverload<int>::of(&QSpinBox::valueChanged),
                this, [this](int v) { editor_.setWidth(v); });
        tb->addWidget(widthBox);

        tb->addSeparator();

        undoAct_ = tb->addAction("↶ Undo");
        redoAct_ = tb->addAction("↷ Redo");
        connect(undoAct_, &QAction::triggered, this, [this]() {
            editor_.undo();
            canvasWidget_->update();
            updateActions();
        });
        connect(redoAct_, &QAction::triggered, this, [this]() {
            editor_.redo();
            canvasWidget_->update();
            updateActions();
        });

        tb->addSeparator();

        auto* openAct = tb->addAction("📂 Открыть");
        auto* saveAct = tb->addAction("💾 Сохранить");
        auto* clearAct = tb->addAction("🧹 Очистить");

        connect(openAct, &QAction::triggered, this, [this]() {
            QString path = QFileDialog::getOpenFileName(
                this, "Открыть PNG", QString(), "PNG (*.png)");
            if (path.isEmpty()) return;
            try {
                editor_.load(path);
                canvasWidget_->update();
                updateActions();
                statusBar()->showMessage("Открыто: " + path);
            } catch (const std::exception& e) {
                QMessageBox::critical(this, "Ошибка", e.what());
            }
        });

        connect(saveAct, &QAction::triggered, this, [this]() {
            QString path = QFileDialog::getSaveFileName(
                this, "Сохранить PNG", "drawing.png", "PNG (*.png)");
            if (path.isEmpty()) return;
            try {
                editor_.save(path);
                statusBar()->showMessage("Сохранено: " + path);
            } catch (const std::exception& e) {
                QMessageBox::critical(this, "Ошибка", e.what());
            }
        });

        connect(clearAct, &QAction::triggered, this, [this]() {
            editor_.clearAll();
            canvasWidget_->update();
            updateActions();
            statusBar()->showMessage("Холст очищен");
        });
    }

    void buildCanvas() {
        canvasWidget_ = new QWidget(this);
        canvasWidget_->setFixedSize(editor_.width(), editor_.height());
        canvasWidget_->setAttribute(Qt::WA_StaticContents);
        canvasWidget_->installEventFilter(this);

        auto* central = new QWidget(this);
        auto* lay = new QVBoxLayout(central);
        lay->setContentsMargins(20, 20, 20, 20);
        lay->addWidget(canvasWidget_, 0, Qt::AlignCenter);
        setCentralWidget(central);
    }

    void updateActions() {
        if (undoAct_) undoAct_->setEnabled(editor_.canUndo());
        if (redoAct_) redoAct_->setEnabled(editor_.canRedo());
    }

    void applyStyle() {
        setStyleSheet(R"(
            QMainWindow, QWidget {
                background-color: #1e1e2e;
                color: #cdd6f4;
                font-family: -apple-system, "Segoe UI", sans-serif;
                font-size: 13px;
            }
            QToolBar {
                background-color: #181825;
                border: none;
                spacing: 4px;
                padding: 6px;
            }
            QToolBar QToolButton {
                background-color: #313244;
                border: 1px solid #45475a;
                border-radius: 5px;
                padding: 6px 10px;
                color: #cdd6f4;
            }
            QToolBar QToolButton:hover   { background-color: #45475a; }
            QToolBar QToolButton:checked { background-color: #a6e3a1; color: #1e1e2e; }
            QToolBar QToolButton:disabled{ color: #6c7086; }
            QPushButton {
                background-color: #313244;
                border: 1px solid #45475a;
                border-radius: 5px;
                padding: 6px 10px;
                color: #cdd6f4;
            }
            QPushButton:hover { background-color: #45475a; }
            QSpinBox {
                background-color: #313244;
                border: 1px solid #45475a;
                border-radius: 5px;
                padding: 4px;
                color: #cdd6f4;
            }
            QStatusBar { background-color: #181825; color: #a6adc8; }
            QLabel     { color: #cdd6f4; }
        )");
    }
};

#include "main_with_facade.moc"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    EditorApp window;
    window.show();
    return app.exec();
}
