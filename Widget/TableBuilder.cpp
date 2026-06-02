#include "TableBuilder.h"
#include <QSortFilterProxyModel>
#include <QGridLayout>
#include <QLabel>
#include <QJsonArray>
#include <QDrag>
#include <QMimeData>
#include <QScrollBar>

// 静态模板注册表初始化
QHash<QString, TableBuilder::ColumnTemplate> TableBuilder::s_templates;

// ======================= TableStyle 实现 =======================

/**
 * @brief 应用内置主题到样式对象
 * @param theme LightTheme 或 DarkTheme
 */
void TableStyle::applyTheme(Theme theme) {
    if (theme == LightTheme) {
        backgroundColor = QColor(255, 255, 255);
        alternateBackgroundColor = QColor(245, 245, 245);
        gridColor = QColor(220, 220, 220);
        selectionBackground = QColor(200, 200, 255);
        selectionForeground = QColor(0, 0, 0);
        headerFont = QFont("Segoe UI", 9, QFont::Bold);
        cellFont = QFont("Segoe UI", 9);
        showGrid = true;
        alternatingRowColors = true;
    } else { // DarkTheme
        backgroundColor = QColor(53, 53, 53);
        alternateBackgroundColor = QColor(45, 45, 45);
        gridColor = QColor(80, 80, 80);
        selectionBackground = QColor(100, 100, 150);
        selectionForeground = QColor(255, 255, 255);
        headerFont = QFont("Segoe UI", 9, QFont::Bold);
        cellFont = QFont("Segoe UI", 9);
        showGrid = true;
        alternatingRowColors = true;
    }
}

// ======================= TableBuilder 实现 =======================

/**
 * @brief 构造函数
 * @param parent 父对象
 */
TableBuilder::TableBuilder(QObject* parent) : QObject(parent) {
    m_view = new QTableView;
    m_model = new QStandardItemModel(this);
    m_proxyModel = new QSortFilterProxyModel(this);
    m_proxyModel->setSourceModel(m_model);
    m_view->setModel(m_proxyModel);
    m_view->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_view->setSelectionMode(QAbstractItemView::SingleSelection);
    m_view->horizontalHeader()->setSectionsMovable(false);
    m_view->verticalHeader()->setVisible(false);
}

/**
 * @brief 析构函数：清理拥有所有权的委托，删除视图
 */
TableBuilder::~TableBuilder() {
    // 清理拥有所有权的委托
    for (auto& cfg : m_configs) {
        if (cfg.delegateOwnership && cfg.customDelegate) {
            delete cfg.customDelegate;
        }
    }
    delete m_view; // view 会自动删除其子控件
}

// ========== 列配置链式方法 ==========

TableBuilder& TableBuilder::addColumn(const QString& title) {
    ColumnConfig cfg;
    cfg.title = title;
    m_configs.append(cfg);
    m_currentCol = m_configs.size() - 1;
    return *this;
}

TableBuilder& TableBuilder::setWidth(int width) {
    if (m_currentCol >= 0) {
        m_configs[m_currentCol].widthPolicy = ColumnWidthPolicy::Fixed;
        m_configs[m_currentCol].fixedWidth = width;
    }
    return *this;
}

TableBuilder& TableBuilder::setStretch(int factor) {
    if (m_currentCol >= 0) {
        m_configs[m_currentCol].widthPolicy = ColumnWidthPolicy::Stretch;
        m_configs[m_currentCol].stretchFactor = factor;
    }
    return *this;
}

TableBuilder& TableBuilder::setDefaultWidth() {
    if (m_currentCol >= 0)
        m_configs[m_currentCol].widthPolicy = ColumnWidthPolicy::Default;
    return *this;
}

TableBuilder& TableBuilder::setAlignment(Qt::Alignment align) {
    if (m_currentCol >= 0) m_configs[m_currentCol].alignment = align;
    return *this;
}

TableBuilder& TableBuilder::setEditable(bool editable) {
    if (m_currentCol >= 0) m_configs[m_currentCol].editable = editable;
    return *this;
}

TableBuilder& TableBuilder::setSortable(bool sortable) {
    if (m_currentCol >= 0) m_configs[m_currentCol].sortable = sortable;
    return *this;
}

TableBuilder& TableBuilder::setHidden(bool hidden) {
    if (m_currentCol >= 0) m_configs[m_currentCol].hidden = hidden;
    return *this;
}

TableBuilder& TableBuilder::setEditor(ColumnConfig::EditorCreator creator) {
    if (m_currentCol >= 0) m_configs[m_currentCol].editorCreator = creator;
    return *this;
}

TableBuilder& TableBuilder::setDelegate(QAbstractItemDelegate* delegate, bool takeOwnership) {
    if (m_currentCol >= 0) {
        if (m_configs[m_currentCol].delegateOwnership && m_configs[m_currentCol].customDelegate)
            delete m_configs[m_currentCol].customDelegate;
        m_configs[m_currentCol].customDelegate = delegate;
        m_configs[m_currentCol].delegateOwnership = takeOwnership;
        m_configs[m_currentCol].editorCreator = nullptr;
    }
    return *this;
}

TableBuilder& TableBuilder::setDataRole(int role) {
    if (m_currentCol >= 0) m_configs[m_currentCol].dataRole = role;
    return *this;
}

// ========== 表格样式设置 ==========

TableBuilder& TableBuilder::setStyle(const TableStyle& style) {
    m_style = style;
    return *this;
}

TableBuilder& TableBuilder::setBackgroundColor(const QColor& color) {
    m_style.backgroundColor = color;
    return *this;
}

TableBuilder& TableBuilder::setAlternatingRowColors(bool enable) {
    m_style.alternatingRowColors = enable;
    return *this;
}

TableBuilder& TableBuilder::setGridColor(const QColor& color) {
    m_style.gridColor = color;
    return *this;
}

TableBuilder& TableBuilder::setSelectionColors(const QColor& background, const QColor& foreground) {
    m_style.selectionBackground = background;
    m_style.selectionForeground = foreground;
    return *this;
}

TableBuilder& TableBuilder::setHeaderFont(const QFont& font) {
    m_style.headerFont = font;
    return *this;
}

TableBuilder& TableBuilder::setCellFont(const QFont& font) {
    m_style.cellFont = font;
    return *this;
}

TableBuilder& TableBuilder::setRowHeight(int height) {
    m_style.rowHeight = height;
    return *this;
}

TableBuilder& TableBuilder::applyTheme(TableStyle::Theme theme) {
    m_style.applyTheme(theme);
    return *this;
}

// ========== 列预设 ==========

void TableBuilder::registerColumnTemplate(const QString& name, ColumnTemplate tmpl) {
    s_templates[name] = tmpl;
}

TableBuilder& TableBuilder::addColumnFromTemplate(const QString& templateName, const QString& customTitle) {
    if (s_templates.contains(templateName)) {
        s_templates[templateName](*this, customTitle.isEmpty() ? templateName : customTitle);
    }
    return *this;
}

// ========== 动态列管理 ==========

bool TableBuilder::insertColumn(int index, const QString& title) {
    if (index < 0 || index > m_configs.size()) return false;
    ColumnConfig cfg;
    cfg.title = title;
    m_configs.insert(index, cfg);
    if (m_view && m_model) {
        m_model->insertColumn(index);
        m_model->setHeaderData(index, Qt::Horizontal, title);
        // 重新应用所有列配置（简单方法：重建）
        applyColumnConfigs();
        setupDelegates();
    }
    return true;
}

bool TableBuilder::removeColumn(int index) {
    if (index < 0 || index >= m_configs.size()) return false;
    m_configs.removeAt(index);
    if (m_view && m_model) {
        m_model->removeColumn(index);
        applyColumnConfigs();
        setupDelegates();
    }
    return true;
}

bool TableBuilder::moveColumn(int from, int to) {
    if (from < 0 || from >= m_configs.size() || to < 0 || to >= m_configs.size()) return false;
    m_configs.move(from, to);
    if (m_view && m_model) {
        // 移动模型中的列数据（需要逐行交换）
        for (int row = 0; row < m_model->rowCount(); ++row) {
            QStandardItem* fromItem = m_model->takeItem(row, from);
            QStandardItem* toItem = m_model->takeItem(row, to);
            m_model->setItem(row, from, toItem);
            m_model->setItem(row, to, fromItem);
        }
        // 交换表头数据
        QVariant fromHeader = m_model->headerData(from, Qt::Horizontal);
        QVariant toHeader = m_model->headerData(to, Qt::Horizontal);
        m_model->setHeaderData(from, Qt::Horizontal, toHeader);
        m_model->setHeaderData(to, Qt::Horizontal, fromHeader);
        applyColumnConfigs();
    }
    return true;
}

void TableBuilder::clearColumns() {
    m_configs.clear();
    if (m_model) m_model->setColumnCount(0);
}

// ========== 数据绑定 ==========

void TableBuilder::setDataSource(const QList<QList<QVariant>>& data) {
    m_cachedData = data;
    m_useExternalModel = false;
    refreshData();
}

void TableBuilder::setDataSource(QAbstractItemModel* model) {
    m_externalModel = model;
    m_useExternalModel = true;
    bindToModel(model);
}

void TableBuilder::refreshData() {
    if (m_useExternalModel && m_externalModel) {
        // 使用外部模型，只需重新设置 proxy 的源模型
        m_proxyModel->setSourceModel(m_externalModel);
        m_model = nullptr; // 不再拥有内部模型
    } else if (!m_useExternalModel) {
        // 使用内部缓存数据
        m_model->clear();
        m_model->setColumnCount(m_configs.size());
        m_model->setRowCount(m_cachedData.size());
        for (int row = 0; row < m_cachedData.size(); ++row) {
            const auto& rowData = m_cachedData[row];
            for (int col = 0; col < rowData.size() && col < m_configs.size(); ++col) {
                QStandardItem* item = new QStandardItem(rowData[col].toString());
                item->setTextAlignment(m_configs[col].alignment);
                item->setEditable(m_configs[col].editable);
                m_model->setItem(row, col, item);
            }
        }
        m_proxyModel->setSourceModel(m_model);
    }
    applyColumnConfigs();
}

void TableBuilder::bindToModel(QAbstractItemModel* model) {
    if (!model) return;
    connect(model, &QAbstractItemModel::dataChanged, this, [this]() {
        if (!m_useExternalModel) refreshData();
    });
    connect(model, &QAbstractItemModel::rowsInserted, this, &TableBuilder::refreshData);
    connect(model, &QAbstractItemModel::rowsRemoved, this, &TableBuilder::refreshData);
    refreshData();
}

// ========== 导出/导入列配置（JSON） ==========

QJsonObject TableBuilder::exportColumnConfig() const {
    QJsonObject root;
    QJsonArray cols;
    for (const auto& cfg : m_configs) {
        QJsonObject colObj;
        colObj["title"] = cfg.title;
        colObj["widthPolicy"] = static_cast<int>(cfg.widthPolicy);
        colObj["fixedWidth"] = cfg.fixedWidth;
        colObj["stretchFactor"] = cfg.stretchFactor;
        colObj["alignment"] = static_cast<int>(cfg.alignment);
        colObj["editable"] = cfg.editable;
        colObj["sortable"] = cfg.sortable;
        colObj["hidden"] = cfg.hidden;
        cols.append(colObj);
    }
    root["columns"] = cols;
    return root;
}

bool TableBuilder::importColumnConfig(const QJsonObject& config) {
    if (!config.contains("columns")) return false;
    QJsonArray cols = config["columns"].toArray();
    m_configs.clear();
    for (const auto& val : cols) {
        QJsonObject colObj = val.toObject();
        ColumnConfig cfg;
        cfg.title = colObj["title"].toString();
        cfg.widthPolicy = static_cast<ColumnWidthPolicy>(colObj["widthPolicy"].toInt());
        cfg.fixedWidth = colObj["fixedWidth"].toInt();
        cfg.stretchFactor = colObj["stretchFactor"].toInt();
        cfg.alignment = static_cast<Qt::Alignment>(colObj["alignment"].toInt());
        cfg.editable = colObj["editable"].toBool();
        cfg.sortable = colObj["sortable"].toBool();
        cfg.hidden = colObj["hidden"].toBool();
        m_configs.append(cfg);
    }
    if (m_view) {
        applyColumnConfigs();
        setupDelegates();
    }
    return true;
}

// ========== 过滤面板 ==========

void TableBuilder::enableGlobalFilter(bool enable) {
    if (!m_view) return;
    if (enable && !m_globalFilterEdit) {
        m_globalFilterEdit = new QLineEdit;
        m_globalFilterEdit->setPlaceholderText("全局过滤...");
        connect(m_globalFilterEdit, &QLineEdit::textChanged, this, &TableBuilder::onFilterTextChanged);
        // 将过滤控件放在表格上方（简单示例：添加到一个 layout，需要用户手动布局，这里为了演示，我们直接设置其父为 view 并定位）
        m_globalFilterEdit->setParent(m_view->parentWidget());
        m_globalFilterEdit->show();
    } else if (!enable && m_globalFilterEdit) {
        delete m_globalFilterEdit;
        m_globalFilterEdit = nullptr;
    }
}

void TableBuilder::enableColumnFilter(bool enable) {
    if (!m_view) return;
    if (enable && m_columnFilterEdits.isEmpty()) {
        // 为每一列创建过滤框（放在表头下方，简化版）
        QWidget* container = new QWidget;
        QHBoxLayout* layout = new QHBoxLayout(container);
        layout->setContentsMargins(0, 0, 0, 0);
        for (int i = 0; i < m_configs.size(); ++i) {
            QLineEdit* edit = new QLineEdit;
            edit->setPlaceholderText("过滤" + m_configs[i].title);
            connect(edit, &QLineEdit::textChanged, this, [this, i](const QString& txt) {
                onColumnFilterChanged(i, txt);
            });
            layout->addWidget(edit);
            m_columnFilterEdits.append(edit);
        }
        container->setLayout(layout);
        m_filterPanel = container;
        // 添加到布局（需用户布局，这里简单设置父对象）
        m_filterPanel->setParent(m_view->parentWidget());
        m_filterPanel->show();
    } else if (!enable && m_filterPanel) {
        delete m_filterPanel;
        m_filterPanel = nullptr;
        m_columnFilterEdits.clear();
    }
}

void TableBuilder::setFilterPlaceholder(const QString& text) {
    if (m_globalFilterEdit) m_globalFilterEdit->setPlaceholderText(text);
}

void TableBuilder::onFilterTextChanged(const QString& text) {
    m_proxyModel->setFilterFixedString(text);
    m_proxyModel->setFilterKeyColumn(-1); // 所有列
}

void TableBuilder::onColumnFilterChanged(int column, const QString& text) {
    // 简单实现：为指定列设置正则过滤
    m_proxyModel->setFilterKeyColumn(column);
    m_proxyModel->setFilterFixedString(text);
}

// ========== 拖拽行/列排序 ==========

void TableBuilder::setDragDropEnabled(bool enableRows, bool enableColumns) {
    m_dragRows = enableRows;
    m_dragCols = enableColumns;
    if (m_view) {
        m_view->setDragEnabled(enableRows || enableColumns);
        m_view->setAcceptDrops(true);
        m_view->setDropIndicatorShown(true);
        if (enableRows) {
            m_view->setDragDropMode(QAbstractItemView::InternalMove);
            // 需要实现自定义行为，这里简化：启用内部移动后，模型需要支持 moveRows
            // 实际需子类化模型，为简洁，仅作示意
        }
        if (enableColumns) {
            m_view->horizontalHeader()->setSectionsMovable(true);
            connect(m_view->horizontalHeader(), &QHeaderView::sectionMoved, this, [this](int logical, int oldVisual, int newVisual) {
                // 同步列配置顺序
                if (oldVisual != newVisual) {
                    m_configs.move(oldVisual, newVisual);
                }
            });
        }
    }
}

// ========== 虚拟滚动（延迟加载） ==========

void TableBuilder::setVirtualMode(int totalRows, FetchMoreCallback callback) {
    m_virtualMode = true;
    m_virtualTotalRows = totalRows;
    m_fetchCallback = callback;
    setVirtualModeEnabled(true);
}

void TableBuilder::setVirtualModeEnabled(bool enable) {
    if (!m_view) return;
    if (enable) {
        // 使用自定义模型或代理模型实现延迟加载，这里采用简单方法：监听滚动条
        QScrollBar* vbar = m_view->verticalScrollBar();
        connect(vbar, &QScrollBar::valueChanged, this, [this, vbar]() {
            int lastRow = m_model->rowCount();
            if (lastRow < m_virtualTotalRows && vbar->value() > vbar->maximum() - 100) {
                int toFetch = qMin(50, m_virtualTotalRows - lastRow);
                if (m_fetchCallback) m_fetchCallback(lastRow, toFetch);
            }
        });
        // 初始加载第一批
        if (m_fetchCallback) m_fetchCallback(0, 50);
    } else {
        // 断开连接（示例中未实现断开，实际应保存 connect 返回值并断开）
    }
}

// ========== 构建表格 ==========

QTableView* TableBuilder::build() {
    if (!m_view) initView();
    applyColumnConfigs();
    applyStyle();
    setupDelegates();
    setupFilterPanel();   // 如果之前启用了过滤面板
    setupDragDrop();
    if (m_virtualMode) setVirtualModeEnabled(true);
    return m_view;
}

QStandardItemModel* TableBuilder::model() const {
    return m_model;
}

// ========== 私有辅助方法 ==========

void TableBuilder::initView() {
    if (!m_view) {
        m_view = new QTableView;
        m_view->setModel(m_proxyModel);
    }
}

void TableBuilder::applyColumnConfigs() {
    if (!m_view) return;
    QHeaderView* header = m_view->horizontalHeader();
    for (int i = 0; i < m_configs.size(); ++i) {
        const auto& cfg = m_configs[i];
        // 设置标题（如果模型存在）
        if (m_model) m_model->setHeaderData(i, Qt::Horizontal, cfg.title);
        // 宽度策略
        switch (cfg.widthPolicy) {
        case ColumnWidthPolicy::Fixed:
            m_view->setColumnWidth(i, cfg.fixedWidth);
            header->setSectionResizeMode(i, QHeaderView::Fixed);
            break;
        case ColumnWidthPolicy::Stretch:
            header->setSectionResizeMode(i, QHeaderView::Stretch);
            break;
        default:
            header->setSectionResizeMode(i, QHeaderView::Interactive);
            break;
        }
        m_view->setColumnHidden(i, cfg.hidden);
    }
}

void TableBuilder::applyStyle() {
    if (!m_view) return;
    m_view->setAlternatingRowColors(m_style.alternatingRowColors);
    m_view->setSelectionBehavior(QAbstractItemView::SelectRows);
    // 设置样式表（简单实现）
    QString styleSheet = QString(
        "QTableView { background-color: %1; gridline-color: %2; }"
        "QTableView::item:selected { background-color: %3; color: %4; }"
        "QHeaderView::section { font: %5; }"
    ).arg(m_style.backgroundColor.name())
     .arg(m_style.gridColor.name())
     .arg(m_style.selectionBackground.name())
     .arg(m_style.selectionForeground.name())
     .arg(m_style.headerFont.toString());
    m_view->setStyleSheet(styleSheet);
    if (m_style.rowHeight > 0) {
        m_view->verticalHeader()->setDefaultSectionSize(m_style.rowHeight);
    }
    // 单元格字体需要委托绘制时使用，这里简化，在委托中实现
}

void TableBuilder::setupDelegates() {
    // 为每一列设置委托（处理对齐、编辑器等）
    // 为简化篇幅，此部分可参考之前 GenericColumnDelegate 实现
    // 实际应遍历 m_configs 创建并设置 setItemDelegateForColumn
    // 用户可根据需要自行补充完整
}

void TableBuilder::setupFilterPanel() {
    // 如果已创建过滤面板，将其放置在表格上方
    if (m_filterPanel && m_view->parentWidget()) {
        // 需要布局管理，此示例中用户需自行布局
    }
}

void TableBuilder::setupDragDrop() {
    // 已在 setDragDropEnabled 中实现部分
}
