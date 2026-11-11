# Лабораторная работа №2 — Структурные паттерны

**Паттерн:** Фасад (Facade)
**Предметная область:** графический редактор

---

## 1. Постановка проблемы

Приложение «ShapeEditor» — простой графический редактор. Поддерживает карандаш, линию, прямоугольник, овал, ластик, выбор цвета и толщины, undo/redo, сохранение и загрузку PNG.

Внутри редактора выделяются четыре независимые подсистемы:

- **`Canvas`** — холст и собственно отрисовка графических элементов.
- **`ToolManager`** — текущий инструмент, выбранный цвет, толщина линии.
- **`History`** — undo/redo, хранящие снимки холста.
- **`FileIO`** — сохранение и загрузка изображения в PNG.

**Постановка проблемы** Если клиент (`EditorApp`) работает со всеми четырьмя подсистемами напрямую, возникает **сильная зависимость** между классом клиента и классами подсистем:

- Клиент вынужден сам держать у себя объекты `Canvas`, `ToolManager`, `History` и `FileIO`.
- Клиент сам координирует их взаимодействие — практически каждое действие пользователя требует сразу нескольких подсистем (например, штрих карандаша = снимок в `History` + чтение настроек из `ToolManager` + рисование на `Canvas`).
- Любое изменение в подсистемах заставит переписывать весь UI.
- Если нужно добавить нового клиента (например, консольную или веб-версию) — придётся дублировать всю координацию.

---

## 3. Реализация без паттерна

Клиент `EditorApp` напрямую держит все четыре подсистемы и сам их координирует:

```cpp
class EditorApp : public QMainWindow {
    Canvas      canvas_;
    ToolManager tools_;
    History     history_;
    FileIO      io_;
    ...
    void onMousePress(QMouseEvent* e) {
        // клиент сам добавляет снимок в историю
        history_.push(canvas_.image());
        startPoint_ = e->pos();
    }

    void onMouseMove(QMouseEvent* e) {
        // клиент сам спрашивает ToolManager и сам вызывает Canvas
        canvas_.drawStroke(lastPoint_, e->pos(),
                           tools_.effectiveColor(), tools_.width());
    }

    void onOpen() {
        QImage loaded = io_.loadPng(path);   
        history_.push(canvas_.image());      
        canvas_.setImage(loaded);           
    }
};
```

### Минусы
- **Сильная зависимость** клиента сразу с четырьмя подсистемами.
- Координация размазана по UI: один обработчик события мыши знает про `Canvas`, `ToolManager` и `History` одновременно.
- При изменении интерфейса любой из подсистем переписывать придётся весь `EditorApp`.
- Появление новых клиентов приведёт к дублированию кода.
- Затрудненное тестирование

---

## 4. Реализация с паттерном Фасад

![Диаграмма классов](docs/diagram.png)

Вводим класс `Editor` — **фасад**, который прячет все четыре подсистемы:

```cpp
class Editor {
    Canvas      canvas_;
    ToolManager tools_;
    History     history_;
    FileIO      io_;
public:
    void setTool(ShapeType t);
    void setColor(const QColor& c);
    void setWidth(int w);

    void beginAction();                               
    void strokeTo(const QPoint& from, const QPoint& to);
    void commitShape(const QPoint& from, const QPoint& to);

    void undo();
    void redo();

    void save(const QString& path);
    void load(const QString& path);
    void clearAll();
};
```

Теперь клиент работает **только с фасадом**:

```cpp
class EditorApp : public QMainWindow {
    Editor editor_;
    ...
    void onMousePress(QMouseEvent* e) {
        editor_.beginAction();
        startPoint_ = e->pos();
    }

    void onMouseMove(QMouseEvent* e) {
        editor_.strokeTo(lastPoint_, e->pos());
    }

    void onOpen() {
        editor_.load(path);
    }
};
```

### Плюсы
- **Слабая зависимостьь** клиента с подсистемой: `EditorApp` зависит только от `Editor`.
- Координация подсистем находится в одном месте — внутри фасада.
- Легко добавить новый UI
- При изменении `Canvas`, `ToolManager`, `History` или `FileIO` клиент трогать не нужно, правим только фасад.
