#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QListWidget>
#include <QLabel>
#include <QMessageBox>
#include <QStatusBar>
#include <QFrame>
#include <QObject>

#include <string>
#include <vector>
#include <map>
#include <optional>
#include <algorithm>
#include <cctype>


struct Book {
    std::string title;
    std::string author;
};

class Driver {
    std::map<std::string, Book> data_ = {
        {"https://akniga.org/war",     {"War and Peace",  "Tolstoy"}},
        {"https://akniga.org/crime",   {"Crime and Punishment", "Dostoevsky"}},
        {"https://akniga.org/1984",    {"1984",           "Orwell"}},
    };
public:
    std::optional<Book> getBook(const std::string& url) {
        auto it = data_.find(url);
        if (it != data_.end()) return it->second;
        return std::nullopt;
    }
};

class DB {
    std::vector<Book> data_ = {
        {"The Lord of the Rings", "J.R.R. Tolkien"},
        {"Hamlet", "William Shakespeare"},
    };
public:
    std::vector<Book> getLibrary() { return data_; }

    Book addBook(const Book& book) {
        data_.push_back(book);
        return book;
    }

    bool removeBook(int index) {
        if (index < 0 || index >= static_cast<int>(data_.size()))
            return false;
        data_.erase(data_.begin() + index);
        return true;
    }

    std::vector<Book> searchBooks(const std::string& query) {
        std::vector<Book> result;
        std::string q = query;
        std::transform(q.begin(), q.end(), q.begin(),
                       [](unsigned char c){ return std::tolower(c); });

        for (const auto& book : data_) {
            std::string t = book.title;
            std::string a = book.author;
            std::transform(t.begin(), t.end(), t.begin(),
                           [](unsigned char c){ return std::tolower(c); });
            std::transform(a.begin(), a.end(), a.begin(),
                           [](unsigned char c){ return std::tolower(c); });
            if (t.find(q) != std::string::npos || a.find(q) != std::string::npos)
                result.push_back(book);
        }
        return result;
    }

    int count() { return static_cast<int>(data_.size()); }
};


class BookApp : public QWidget {
    Q_OBJECT

    Driver driver_;
    DB     db_;

    QLineEdit*   urlInput_;
    QLineEdit*   searchInput_;
    QListWidget* bookList_;
    QLabel*      statusLabel_;

public:
    BookApp(QWidget* parent = nullptr) : QWidget(parent) {
        setWindowTitle("DiBooks");
        resize(600, 520);

        setStyleSheet(R"(
            QWidget {
                background-color: #1e1e2e;
                color: #cdd6f4;
                font-family: -apple-system, "Segoe UI", sans-serif;
                font-size: 14px;
            }
            QLabel#header {
                font-size: 20px;
                font-weight: bold;
                color: #f5c2e7;
                padding: 8px 0px;
            }
            QLabel#status {
                color: #a6adc8;
                font-size: 12px;
                padding: 4px 8px;
                background-color: #181825;
                border-radius: 4px;
            }
            QLineEdit {
                background-color: #313244;
                border: 1px solid #45475a;
                border-radius: 6px;
                padding: 8px 10px;
                color: #cdd6f4;
                selection-background-color: #89b4fa;
            }
            QLineEdit:focus {
                border: 1px solid #89b4fa;
            }
            QPushButton {
                background-color: #89b4fa;
                color: #1e1e2e;
                border: none;
                border-radius: 6px;
                padding: 8px 14px;
                font-weight: bold;
            }
            QPushButton:hover {
                background-color: #b4befe;
            }
            QPushButton:pressed {
                background-color: #74a8f5;
            }
            QPushButton#danger {
                background-color: #f38ba8;
            }
            QPushButton#danger:hover {
                background-color: #eba0b7;
            }
            QPushButton#secondary {
                background-color: #585b70;
                color: #cdd6f4;
            }
            QPushButton#secondary:hover {
                background-color: #6c7086;
            }
            QListWidget {
                background-color: #181825;
                border: 1px solid #313244;
                border-radius: 6px;
                padding: 4px;
            }
            QListWidget::item {
                padding: 8px;
                border-radius: 4px;
            }
            QListWidget::item:selected {
                background-color: #89b4fa;
                color: #1e1e2e;
            }
            QListWidget::item:hover {
                background-color: #313244;
            }
            QFrame#separator {
                background-color: #313244;
                max-height: 1px;
            }
        )");

        auto* root = new QVBoxLayout(this);
        root->setContentsMargins(16, 16, 16, 12);
        root->setSpacing(10);

        auto* header = new QLabel("📚 DiBooks");
        header->setObjectName("header");
        root->addWidget(header);

        auto* searchRow = new QHBoxLayout();
        auto* searchLabel = new QLabel("🔍");
        searchInput_ = new QLineEdit();
        searchInput_->setPlaceholderText("Поиск по автору или названию...");
        auto* searchBtn = new QPushButton("Найти");
        auto* resetBtn  = new QPushButton("Сброс");
        resetBtn->setObjectName("secondary");
        searchRow->addWidget(searchLabel);
        searchRow->addWidget(searchInput_);
        searchRow->addWidget(searchBtn);
        searchRow->addWidget(resetBtn);
        root->addLayout(searchRow);

        auto* sep = new QFrame();
        sep->setObjectName("separator");
        sep->setFrameShape(QFrame::HLine);
        root->addWidget(sep);

        auto* addRow = new QHBoxLayout();
        auto* urlLabel = new QLabel("🔗");
        urlInput_ = new QLineEdit();
        urlInput_->setPlaceholderText("url");
        auto* addBtn = new QPushButton("➕ Добавить");
        addRow->addWidget(urlLabel);
        addRow->addWidget(urlInput_);
        addRow->addWidget(addBtn);
        root->addLayout(addRow);

        bookList_ = new QListWidget();
        root->addWidget(bookList_, 1);

        auto* bottomRow = new QHBoxLayout();
        statusLabel_ = new QLabel();
        statusLabel_->setObjectName("status");
        auto* deleteBtn = new QPushButton("🗑️ Удалить");
        deleteBtn->setObjectName("danger");
        bottomRow->addWidget(statusLabel_, 1);
        bottomRow->addWidget(deleteBtn);
        root->addLayout(bottomRow);

        connect(addBtn,    &QPushButton::clicked, this, &BookApp::onAddClicked);
        connect(deleteBtn, &QPushButton::clicked, this, &BookApp::onDeleteClicked);
        connect(searchBtn, &QPushButton::clicked, this, &BookApp::onSearchClicked);
        connect(resetBtn,  &QPushButton::clicked, this, &BookApp::onResetClicked);
        connect(searchInput_, &QLineEdit::returnPressed, this, &BookApp::onSearchClicked);

        loadLibrary();
    }

private slots:
    void loadLibrary() {
        bookList_->clear();
        for (const auto& book : db_.getLibrary())
            bookList_->addItem(
                QString::fromStdString(book.author + " — " + book.title));
        updateStatus();
    }

    void onAddClicked() {
        std::string url = urlInput_->text().trimmed().toStdString();
        if (url.empty()) {
            QMessageBox::warning(this, "Ошибка", "Введите URL книги");
            return;
        }

        std::optional<Book> maybeBook = driver_.getBook(url);

        if (!maybeBook.has_value()) {
            QMessageBox::critical(this, "Ошибка",
                                  "Не удалось получить данные книги");
            return;
        }

        Book added = db_.addBook(maybeBook.value());

        bookList_->addItem(
            QString::fromStdString(added.author + " — " + added.title));
        urlInput_->clear();
        updateStatus();
    }

    void onDeleteClicked() {
        int row = bookList_->currentRow();
        if (row < 0) {
            QMessageBox::warning(this, "Ошибка", "Выберите книгу для удаления");
            return;
        }

        onResetClicked();

        bool ok = db_.removeBook(row);
        if (ok) {
            delete bookList_->takeItem(row);
        }
        updateStatus();
    }

    void onSearchClicked() {
        std::string query = searchInput_->text().trimmed().toStdString();
        if (query.empty()) {
            onResetClicked();
            return;
        }

        std::vector<Book> found = db_.searchBooks(query);

        bookList_->clear();
        for (const auto& book : found)
            bookList_->addItem(
                QString::fromStdString(book.author + " — " + book.title));

        statusLabel_->setText(
            QString("Найдено: %1 из %2")
                .arg(found.size())
                .arg(db_.count()));
    }

    void onResetClicked() {
        searchInput_->clear();
        loadLibrary();
    }

    void updateStatus() {
        statusLabel_->setText(QString("Всего книг: %1").arg(db_.count()));
    }
};

#include "main_without_facade.moc"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    BookApp window;
    window.show();
    return app.exec();
}
