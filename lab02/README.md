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

**Постановка проблемы** Если клиент (`EditorApp`) работает со всеми четырьмя подсистемами напрямую, возникает **сильная связанность** между классом клиента и классами подсистем:

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
        // клиент сам пушит снимок в историю
        history_.push(canvas_.image());
        startPoint_ = e->pos();
    }

    void onMouseMove(QMouseEvent* e) {
        // клиент сам спрашивает ToolManager и сам зовёт Canvas
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
- **Сильная связанность** клиента сразу с четырьмя подсистемами.
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
    // настройки инструмента
    void setTool(ShapeType t);
    void setColor(const QColor& c);
    void setWidth(int w);

    // рисование
    void beginAction();                               
    void strokeTo(const QPoint& from, const QPoint& to);
    void commitShape(const QPoint& from, const QPoint& to);

    // история
    void undo();
    void redo();

    // файлы
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
- **Слабая связанность** клиента с подсистемой: `EditorApp` зависит только от `Editor`.
- Координация подсистем находится в одном месте — внутри фасада. Например, метод `Editor::load()` сам сохраняет снимок в `History` перед заменой изображения, и UI про это знать не должен.
- Легко добавить новый UI (консольный, веб) — он будет использовать тот же фасад.
- При изменении `Canvas`, `ToolManager`, `History` или `FileIO` клиент трогать не нужно, правим только фасад.

### Минусы
- Фасад рискует превратиться в «божественный объект», если через него тянуть вообще всё. С ростом системы его нужно декомпозировать.
- Если клиенту всё-таки нужен прямой доступ к низкоуровневым возможностям подсистемы, их придётся либо «пробрасывать» через фасад, либо оставлять подсистему публичной.
