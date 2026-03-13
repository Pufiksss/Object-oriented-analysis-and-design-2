using System;
using System.Collections.Generic;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Input;
using Avalonia.Media;

public interface IPrototype
{
    IPrototype Clone();
}


public abstract class ShapeModel : IPrototype
{
    public double X { get; set; }
    public double Y { get; set; }
    public Color FillColor { get; set; }
    public Color BorderColor { get; set; }
    public bool IsSelected { get; set; }

    protected ShapeModel(ShapeModel other)
    {
        X = other.X + 20;
        Y = other.Y + 20;
        FillColor = other.FillColor;
        BorderColor = other.BorderColor;
        IsSelected = false;
    }

    protected ShapeModel() { }

    public abstract IPrototype Clone();

    public abstract void Draw(DrawingContext ctx);

    public abstract bool HitTest(double mx, double my);

    protected void DrawHandles(DrawingContext ctx, double x, double y, double w, double h)
    {
        var pts = new Point[]
        {
            new(x,       y),       new(x+w/2, y),    new(x+w,   y),
            new(x+w,   y+h/2),     new(x+w,   y+h),  new(x+w/2, y+h),
            new(x,     y+h),       new(x,     y+h/2)
        };
        var hFill = new SolidColorBrush(Colors.White);
        var hPen = new Pen(new SolidColorBrush(Color.FromRgb(80, 220, 120)), 1);
        foreach (var p in pts) ctx.FillRectangle(hFill, new Rect(p.X - 4, p.Y - 4, 8, 8));
        foreach (var p in pts) ctx.DrawRectangle(null, hPen, new Rect(p.X - 4, p.Y - 4, 8, 8));
    }
}

public class CircleModel : ShapeModel
{
    public double Radius { get; set; }

    public CircleModel() { }

    private CircleModel(CircleModel other) : base(other)
    {
        Radius = other.Radius;
    }

    public override IPrototype Clone() => new CircleModel(this);

    public override void Draw(DrawingContext ctx)
    {
        var fill = new SolidColorBrush(FillColor);
        var penClr = IsSelected ? Color.FromRgb(80, 220, 120) : BorderColor;
        var pen = new Pen(new SolidColorBrush(penClr), IsSelected ? 3 : 1.5);
        ctx.DrawEllipse(fill, pen, new Point(X, Y), Radius, Radius);
        if (IsSelected) DrawHandles(ctx, X - Radius, Y - Radius, Radius * 2, Radius * 2);
    }

    public override bool HitTest(double mx, double my)
        => Math.Sqrt(Math.Pow(mx - X, 2) + Math.Pow(my - Y, 2)) <= Radius;
}

public class RectModel : ShapeModel
{
    public double W { get; set; }
    public double H { get; set; }

    public RectModel() { }

    private RectModel(RectModel other) : base(other)
    {
        W = other.W;
        H = other.H;
    }

    public override IPrototype Clone() => new RectModel(this);

    public override void Draw(DrawingContext ctx)
    {
        var fill = new SolidColorBrush(FillColor);
        var penClr = IsSelected ? Color.FromRgb(80, 220, 120) : BorderColor;
        var pen = new Pen(new SolidColorBrush(penClr), IsSelected ? 3 : 1.5);
        ctx.FillRectangle(fill, new Rect(X, Y, W, H));
        ctx.DrawRectangle(null, pen, new Rect(X, Y, W, H));
        if (IsSelected) DrawHandles(ctx, X, Y, W, H);
    }

    public override bool HitTest(double mx, double my)
        => mx >= X && mx <= X + W && my >= Y && my <= Y + H;
}

public class TriangleModel : ShapeModel
{
    public double W { get; set; }
    public double H { get; set; }

    public TriangleModel() { }

    private TriangleModel(TriangleModel other) : base(other)
    {
        W = other.W;
        H = other.H;
    }

    public override IPrototype Clone() => new TriangleModel(this);

    public override void Draw(DrawingContext ctx)
    {
        var geo = new StreamGeometry();
        using (var gc = geo.Open())
        {
            gc.BeginFigure(new Point(X + W / 2, Y), true);
            gc.LineTo(new Point(X + W, Y + H));
            gc.LineTo(new Point(X, Y + H));
            gc.EndFigure(true);
        }
        var fill = new SolidColorBrush(FillColor);
        var penClr = IsSelected ? Color.FromRgb(80, 220, 120) : BorderColor;
        var pen = new Pen(new SolidColorBrush(penClr), IsSelected ? 3 : 1.5);
        ctx.DrawGeometry(fill, pen, geo);
        if (IsSelected) DrawHandles(ctx, X, Y, W, H);
    }

    public override bool HitTest(double mx, double my)
        => mx >= X && mx <= X + W && my >= Y && my <= Y + H;
}

public class StarModel : ShapeModel
{
    public double Size { get; set; }

    public StarModel() { }

    private StarModel(StarModel other) : base(other)
    {
        Size = other.Size;
    }

    public override IPrototype Clone() => new StarModel(this);

    public override void Draw(DrawingContext ctx)
    {
        var geo = MakeStarGeometry();
        var fill = new SolidColorBrush(FillColor);
        var penClr = IsSelected ? Color.FromRgb(80, 220, 120) : BorderColor;
        var pen = new Pen(new SolidColorBrush(penClr), IsSelected ? 3 : 1.5);
        ctx.DrawGeometry(fill, pen, geo);
        if (IsSelected) DrawHandles(ctx, X - Size, Y - Size, Size * 2, Size * 2);
    }

    public override bool HitTest(double mx, double my)
        => Math.Sqrt(Math.Pow(mx - X, 2) + Math.Pow(my - Y, 2)) <= Size;

    private StreamGeometry MakeStarGeometry()
    {
        var geo = new StreamGeometry();
        int pts = 5;
        double outer = Size, inner = Size * 0.45;
        using var gc = geo.Open();
        for (int i = 0; i < pts * 2; i++)
        {
            double angle = Math.PI / pts * i - Math.PI / 2;
            double r = i % 2 == 0 ? outer : inner;
            var p = new Point(X + r * Math.Cos(angle), Y + r * Math.Sin(angle));
            if (i == 0) gc.BeginFigure(p, true);
            else gc.LineTo(p);
        }
        gc.EndFigure(true);
        return geo;
    }
}

public class GraphicEditor
{
    private readonly List<ShapeModel> _shapes = new();

    public ShapeModel DuplicateShape(ShapeModel original)
        => (ShapeModel)original.Clone();

    public void Add(ShapeModel s) => _shapes.Add(s);
    public void Remove(ShapeModel s) => _shapes.Remove(s);
    public IReadOnlyList<ShapeModel> Shapes => _shapes;

    public ShapeModel? HitTest(double x, double y)
    {
        for (int i = _shapes.Count - 1; i >= 0; i--)
            if (_shapes[i].HitTest(x, y)) return _shapes[i];
        return null;
    }
}

public class DrawingCanvas : Control
{
    public GraphicEditor? Editor { get; set; }

    public override void Render(DrawingContext ctx)
    {
        if (Editor == null) return;

        ctx.FillRectangle(new SolidColorBrush(Color.FromRgb(28, 38, 32)),
                          new Rect(0, 0, Bounds.Width, Bounds.Height));

        var gridPen = new Pen(new SolidColorBrush(Color.FromArgb(15, 255, 255, 255)), 1);
        for (double x = 0; x < Bounds.Width; x += 30)
            ctx.DrawLine(gridPen, new Point(x, 0), new Point(x, Bounds.Height));
        for (double y = 0; y < Bounds.Height; y += 30)
            ctx.DrawLine(gridPen, new Point(0, y), new Point(Bounds.Width, y));

        foreach (var s in Editor.Shapes)
            s.Draw(ctx);

        var ft = new FormattedText(
            $"Фигур: {Editor.Shapes.Count}",
            System.Globalization.CultureInfo.CurrentCulture,
            FlowDirection.LeftToRight,
            new Typeface("Arial"), 13,
            new SolidColorBrush(Color.FromArgb(80, 255, 255, 255)));
        ctx.DrawText(ft, new Point(Bounds.Width - 90, 8));
    }
}

public class MainWindow : Window
{
    private readonly GraphicEditor _editor = new();
    private readonly DrawingCanvas _canvas;

    private string _tool = "circle";
    private ShapeModel? _selected;
    private bool _dragging;
    private Point _dragOff;

    private Color _fillColor = Color.FromArgb(120, 100, 180, 255);
    private Color _borderColor = Color.FromRgb(60, 100, 200);

    private TextBlock _statusText = new();

    public MainWindow()
    {
        _canvas = new DrawingCanvas { Editor = _editor };
        Title = "Графический редактор  [С паттерном Прототип]";
        Width = 1100;
        Height = 720;
        BuildUI();
        AddSamples();
    }

    private void BuildUI()
    {
        var grid = new Grid();
        grid.ColumnDefinitions.Add(new ColumnDefinition(200, GridUnitType.Pixel));
        grid.ColumnDefinitions.Add(new ColumnDefinition(1, GridUnitType.Star));
        grid.RowDefinitions.Add(new RowDefinition(1, GridUnitType.Star));
        grid.RowDefinitions.Add(new RowDefinition(28, GridUnitType.Pixel));

        var toolbar = new StackPanel
        {
            Background = new SolidColorBrush(Color.FromRgb(15, 25, 20)),
            Spacing = 3
        };
        Grid.SetColumn(toolbar, 0);
        Grid.SetRow(toolbar, 0);

        toolbar.Children.Add(MakeLabel("⬡  ИНСТРУМЕНТЫ", Color.FromRgb(80, 220, 140)));
        toolbar.Children.Add(MakeToolBtn("⬤  Круг", "circle"));
        toolbar.Children.Add(MakeToolBtn("■  Прямоугольник", "rectangle"));
        toolbar.Children.Add(MakeToolBtn("▲  Треугольник", "triangle"));
        toolbar.Children.Add(MakeToolBtn("★  Звезда", "star"));   // ✅ новый тип — не трогали редактор!
        toolbar.Children.Add(MakeToolBtn("↖  Выбрать", "select"));
        toolbar.Children.Add(MakeSep());
        toolbar.Children.Add(MakeActionBtn("⧉  Дублировать  [D]", DoDuplicate, Color.FromRgb(30, 80, 50)));
        toolbar.Children.Add(MakeActionBtn("✕  Удалить  [Del]", DoDelete, Color.FromRgb(80, 30, 30)));
        toolbar.Children.Add(MakeActionBtn("↺  Очистить всё", DoClear, Color.FromRgb(40, 50, 60)));
        toolbar.Children.Add(MakeSep());
        toolbar.Children.Add(MakeLabel("ЦВЕТ ЗАЛИВКИ", Color.FromRgb(80, 220, 140)));
        foreach (var (c, n) in Colors_())
            toolbar.Children.Add(MakeColorBtn(c, n));
        toolbar.Children.Add(MakeSep());
        toolbar.Children.Add(new TextBlock
        {
            Text = "✅ С паттерном:\nDuplicate = 1 строка.\nНовый тип — редактор\nне трогать!",
            Foreground = new SolidColorBrush(Color.FromRgb(80, 220, 140)),
            FontStyle = Avalonia.Media.FontStyle.Italic,
            FontSize = 11,
            Margin = new Thickness(8, 4),
            TextWrapping = TextWrapping.Wrap
        });

        grid.Children.Add(toolbar);

        _canvas.Cursor = new Cursor(StandardCursorType.Cross);
        _canvas.PointerPressed += OnPointerPressed;
        _canvas.PointerMoved += OnPointerMoved;
        _canvas.PointerReleased += OnPointerReleased;
        Grid.SetColumn(_canvas, 1);
        Grid.SetRow(_canvas, 0);
        grid.Children.Add(_canvas);

        _statusText = new TextBlock
        {
            Text = "Готово. Нажмите на холст, чтобы добавить фигуру.",
            Foreground = new SolidColorBrush(Color.FromRgb(100, 200, 130)),
            FontSize = 12,
            VerticalAlignment = Avalonia.Layout.VerticalAlignment.Center,
            Margin = new Thickness(12, 0)
        };
        var statusBar = new Border
        {
            Background = new SolidColorBrush(Color.FromRgb(12, 22, 17)),
            Child = _statusText
        };
        Grid.SetColumn(statusBar, 0);
        Grid.SetColumnSpan(statusBar, 2);
        Grid.SetRow(statusBar, 1);
        grid.Children.Add(statusBar);

        Content = new Border
        {
            Background = new SolidColorBrush(Color.FromRgb(20, 30, 25)),
            Child = grid
        };

        KeyDown += (_, e) =>
        {
            if (e.Key == Key.D) DoDuplicate();
            if (e.Key == Key.Delete) DoDelete();
            if (e.Key == Key.Escape) { Deselect(); Refresh(); }
        };
    }

    private void DoDuplicate()
    {
        if (_selected == null) { Status("Сначала выберите фигуру!"); return; }
        var copy = _editor.DuplicateShape(_selected);
        _editor.Add(copy);
        Deselect();
        _selected = copy;
        _selected.IsSelected = true;
        Status("Клонировано через Clone() — без if/else!");
        Refresh();
    }

    private void DoDelete()
    {
        if (_selected == null) { Status("Сначала выберите фигуру!"); return; }
        _editor.Remove(_selected);
        _selected = null;
        Status("Удалено");
        Refresh();
    }

    private void DoClear()
    {
        while (_editor.Shapes.Count > 0) _editor.Remove(_editor.Shapes[0]);
        _selected = null;
        Status("Холст очищен");
        Refresh();
    }

    private void OnPointerPressed(object? s, PointerPressedEventArgs e)
    {
        var pos = e.GetPosition(_canvas);

        if (_tool == "select")
        {
            Deselect();
            _selected = _editor.HitTest(pos.X, pos.Y);
            if (_selected != null)
            {
                _selected.IsSelected = true;
                _dragging = true;
                _dragOff = new Point(pos.X - _selected.X, pos.Y - _selected.Y);
                Status($"Выбрано: {_selected.GetType().Name}");
            }
            Refresh();
            return;
        }

        ShapeModel? ns = null;
        if (_tool == "circle")
            ns = new CircleModel { X = pos.X, Y = pos.Y, Radius = 45, FillColor = _fillColor, BorderColor = _borderColor };
        else if (_tool == "rectangle")
            ns = new RectModel { X = pos.X - 50, Y = pos.Y - 35, W = 100, H = 70, FillColor = _fillColor, BorderColor = _borderColor };
        else if (_tool == "triangle")
            ns = new TriangleModel { X = pos.X - 50, Y = pos.Y - 40, W = 100, H = 80, FillColor = _fillColor, BorderColor = _borderColor };
        else if (_tool == "star")
            ns = new StarModel { X = pos.X, Y = pos.Y, Size = 50, FillColor = _fillColor, BorderColor = _borderColor };

        if (ns != null)
        {
            Deselect();
            _editor.Add(ns);
            _selected = ns;
            _selected.IsSelected = true;
            Status($"Добавлено: {_tool}");
            Refresh();
        }
    }

    private void OnPointerMoved(object? s, PointerEventArgs e)
    {
        if (!_dragging || _selected == null) return;
        var pos = e.GetPosition(_canvas);
        _selected.X = pos.X - _dragOff.X;
        _selected.Y = pos.Y - _dragOff.Y;
        Refresh();
    }

    private void OnPointerReleased(object? s, PointerReleasedEventArgs e) => _dragging = false;

    private void Deselect()
    {
        if (_selected != null) { _selected.IsSelected = false; _selected = null; }
    }

    private void Refresh() => _canvas.InvalidateVisual();
    private void Status(string msg) => _statusText.Text = msg;

    private static (Color, string)[] Colors_() => new[]
    {
        (Color.FromRgb(100, 180, 255), "Синий"),
        (Color.FromRgb(100, 220, 140), "Зелёный"),
        (Color.FromRgb(255, 120, 100), "Красный"),
        (Color.FromRgb(255, 200, 80),  "Жёлтый"),
        (Color.FromRgb(200, 120, 255), "Фиолетовый"),
    };

    private TextBlock MakeLabel(string text, Color color) => new()
    {
        Text = text,
        Foreground = new SolidColorBrush(color),
        FontSize = 11,
        FontWeight = FontWeight.Bold,
        Margin = new Thickness(8, 8, 8, 2)
    };

    private Border MakeSep() => new()
    {
        Height = 1,
        Background = new SolidColorBrush(Color.FromRgb(40, 70, 50)),
        Margin = new Thickness(8, 4)
    };

    private Button MakeToolBtn(string text, string toolName)
    {
        var btn = new Button
        {
            Content = text,
            Background = new SolidColorBrush(Color.FromRgb(25, 40, 30)),
            Foreground = new SolidColorBrush(Color.FromRgb(200, 230, 210)),
            Margin = new Thickness(6, 2),
            FontSize = 13,
            HorizontalContentAlignment = Avalonia.Layout.HorizontalAlignment.Left
        };
        btn.Click += (_, _) =>
        {
            _tool = toolName;
            _canvas.Cursor = toolName == "select"
                ? new Cursor(StandardCursorType.Arrow)
                : new Cursor(StandardCursorType.Cross);
            Status($"Инструмент: {text.Trim()}");
        };
        return btn;
    }

    private Button MakeActionBtn(string text, Action action, Color bg)
    {
        var btn = new Button
        {
            Content = text,
            Background = new SolidColorBrush(bg),
            Foreground = new SolidColorBrush(Colors.White),
            Margin = new Thickness(6, 2),
            FontSize = 12,
            HorizontalContentAlignment = Avalonia.Layout.HorizontalAlignment.Left
        };
        btn.Click += (_, _) => action();
        return btn;
    }

    private Button MakeColorBtn(Color color, string name)
    {
        var btn = new Button
        {
            Content = $"  ■  {name}",
            Background = new SolidColorBrush(Color.FromRgb(25, 40, 30)),
            Foreground = new SolidColorBrush(color),
            Margin = new Thickness(6, 1),
            FontSize = 12,
            HorizontalContentAlignment = Avalonia.Layout.HorizontalAlignment.Left
        };
        btn.Click += (_, _) =>
        {
            _fillColor = Color.FromArgb(120, color.R, color.G, color.B);
            _borderColor = color;
            Status($"Цвет: {name}");
        };
        return btn;
    }

    private void AddSamples()
    {
        _editor.Add(new CircleModel
        {
            X = 150,
            Y = 150,
            Radius = 50,
            FillColor = Color.FromArgb(100, 100, 180, 255),
            BorderColor = Color.FromRgb(60, 100, 200)
        });
        _editor.Add(new RectModel
        {
            X = 280,
            Y = 120,
            W = 140,
            H = 90,
            FillColor = Color.FromArgb(100, 100, 220, 140),
            BorderColor = Color.FromRgb(60, 160, 100)
        });
        _editor.Add(new TriangleModel
        {
            X = 460,
            Y = 110,
            W = 120,
            H = 100,
            FillColor = Color.FromArgb(100, 255, 120, 100),
            BorderColor = Color.FromRgb(200, 80, 60)
        });
        _editor.Add(new StarModel
        {
            X = 660,
            Y = 160,
            Size = 55,
            FillColor = Color.FromArgb(100, 255, 200, 80),
            BorderColor = Color.FromRgb(200, 160, 40)
        });
    }
}

class App : Application
{
    public override void Initialize()
        => Styles.Add(new Avalonia.Themes.Fluent.FluentTheme());

    public override void OnFrameworkInitializationCompleted()
    {
        if (ApplicationLifetime is
            Avalonia.Controls.ApplicationLifetimes.IClassicDesktopStyleApplicationLifetime desktop)
            desktop.MainWindow = new MainWindow();
        base.OnFrameworkInitializationCompleted();
    }
}

class Program
{
    [STAThread]
    static void Main(string[] args) =>
        AppBuilder.Configure<App>()
            .UsePlatformDetect()
            .WithInterFont()
            .StartWithClassicDesktopLifetime(args);
}
