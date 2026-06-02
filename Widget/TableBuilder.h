#ifndef TABLEBUILDER_H
#define TABLEBUILDER_H

#include <QObject>
#include <QTableView>
#include <QStandardItemModel>
#include <QHeaderView>
#include <QJsonObject>
#include <QJsonArray>
#include <QList>
#include <QString>
#include <QFont>
#include <QColor>
#include <QStyledItemDelegate>
#include <QLineEdit>
#include <QComboBox>
#include <functional>

// 前向声明
class QSortFilterProxyModel;

/**
 * @class ColumnWidthPolicy
 * @brief 列宽策略枚举
 */
enum class ColumnWidthPolicy {
    Default,    ///< 默认可由用户调整大小（Interactive）
    Fixed,      ///< 固定像素宽度，用户不可调整
    Stretch     ///< 自动拉伸以填满可用空间
};

/**
 * @struct ColumnConfig
 * @brief 单列的所有配置信息
 */
struct ColumnConfig {
    QString title;                              ///< 列标题文本
    ColumnWidthPolicy widthPolicy = ColumnWidthPolicy::Default; ///< 宽度策略
    int fixedWidth = -1;                        ///< 固定宽度值（仅当 policy == Fixed 时有效）
    int stretchFactor = 1;                      ///< 拉伸系数（仅当 policy == Stretch 时有效，目前未使用）
    Qt::Alignment alignment = Qt::AlignLeft | Qt::AlignVCenter; ///< 单元格内容对齐方式
    bool editable = true;                       ///< 单元格是否可编辑
    bool sortable = false;                      ///< 是否允许排序（用于 QHeaderView 排序指示器）
    bool hidden = false;                        ///< 列是否初始隐藏
    int dataRole = Qt::DisplayRole;             ///< 数据角色（用于自定义模型）

    // 编辑器工厂：用于创建单元格编辑控件（如 QSpinBox, QComboBox 等）
    using EditorCreator = std::function<QWidget*(QWidget*)>;
    EditorCreator editorCreator;                ///< 编辑器创建函数

    // 自定义委托：若提供则完全接管该列的绘制和编辑行为
    QAbstractItemDelegate* customDelegate = nullptr; ///< 自定义委托指针
    bool delegateOwnership = false;             ///< 是否由 Builder 负责删除该委托
};

/**
 * @struct TableStyle
 * @brief 表格整体的样式配置（外观、字体、颜色等）
 */
struct TableStyle {
    QColor backgroundColor;                     ///< 表格背景色
    QColor alternateBackgroundColor;            ///< 交替行背景色（需启用 alternateRowColors）
    QColor gridColor;                           ///< 网格线颜色
    QColor selectionBackground;                 ///< 选中项背景色
    QColor selectionForeground;                 ///< 选中项前景色（文字颜色）
    QFont headerFont;                           ///< 表头字体
    QFont cellFont;                             ///< 单元格字体
    int rowHeight = -1;                         ///< 行高（像素），-1 表示默认
    bool showGrid = true;                       ///< 是否显示网格线
    bool alternatingRowColors = false;          ///< 是否启用交替行颜色
    QHeaderView::ResizeMode defaultColumnResizeMode = QHeaderView::Interactive; ///< 默认列宽调整模式（未使用，可扩展）

    /// 主题预设枚举
    enum Theme { LightTheme, DarkTheme };

    /**
     * @brief 应用内置主题到当前样式对象
     * @param theme 主题类型 (LightTheme 或 DarkTheme)
     */
    void applyTheme(Theme theme);
};

/**
 * @class TableBuilder
 * @brief 强大的表格构建器，支持 QTableView，整合列配置、样式、数据绑定、过滤、排序、拖拽、虚拟滚动等高级功能。
 *
 * 主要功能：
 * - **链式列配置**：addColumn() -> setWidth() -> setAlignment() -> ...
 * - **列预设与模板**：注册/使用常用列模板，避免重复代码。
 * - **动态列管理**：运行时插入、删除、移动列，自动同步模型和视图。
 * - **表格样式**：背景色、字体、行高、主题（亮色/暗色）等。
 * - **数据绑定**：支持二维 QVariant 列表或外部 QAbstractItemModel，自动刷新。
 * - **列配置导入/导出**：保存/加载列配置为 JSON，便于持久化。
 * - **全局过滤 & 列过滤**：基于 QSortFilterProxyModel，支持实时文本过滤。
 * - **拖拽行/列排序**：列拖拽自动重排；行拖拽需用户扩展（当前仅示意，行拖拽可能导致行消失，建议禁用）。
 * - **虚拟滚动（延迟加载）**：监听滚动条，动态加载数据，适用于大表格。
 *
 * 使用示例：
 * @code
 * TableBuilder builder;
 * builder.addColumn("ID").setWidth(80).setAlignment(Qt::AlignCenter)
 *        .addColumn("Name").setStretch(1)
 *        .addColumn("Age").setWidth(100);
 * builder.applyTheme(TableStyle::DarkTheme).setRowHeight(30);
 * builder.setDataSource({{1, "Alice", 25}, {2, "Bob", 30}});
 * builder.enableGlobalFilter(true);
 * QTableView* view = builder.build();
 * @endcode
 *
 * 注意：
 * - 内部使用 QStandardItemModel 和 QSortFilterProxyModel。
 * - 析构时自动清理视图和委托（若拥有所有权）。
 * - 行拖拽（setDragDropEnabled(true, false)）当前不完善，可能导致行消失，建议仅启用列拖拽。
 */
class TableBuilder : public QObject {
    Q_OBJECT
public:
    /**
     * @brief 构造函数
     * @param parent QObject 父对象，用于内存管理
     */
    explicit TableBuilder(QObject* parent = nullptr);

    /**
     * @brief 析构函数，负责清理拥有的委托及视图
     */
    ~TableBuilder();

    // ========== 1. 列配置（链式） ==========

    /**
     * @brief 添加新列并开始配置
     * @param title 列标题
     * @return 自身引用，用于链式调用
     * @note 后续的 setXxx() 方法将作用于这一列，直到下一个 addColumn()
     */
    TableBuilder& addColumn(const QString& title);

    /// 设置当前列的固定宽度（像素）
    TableBuilder& setWidth(int width);
    /// 设置当前列为拉伸模式，占用剩余空间（factor 为拉伸系数，预留）
    TableBuilder& setStretch(int factor = 1);
    /// 设置当前列为默认宽度（用户可手动调整）
    TableBuilder& setDefaultWidth();
    /// 设置当前列的对齐方式（例如 Qt::AlignCenter）
    TableBuilder& setAlignment(Qt::Alignment align);
    /// 设置当前列是否可编辑
    TableBuilder& setEditable(bool editable);
    /// 设置当前列是否可排序（仅影响表头排序指示器，实际排序由 proxy model 处理）
    TableBuilder& setSortable(bool sortable);
    /// 设置当前列是否隐藏
    TableBuilder& setHidden(bool hidden);
    /**
     * @brief 设置当前列的自定义编辑器（工厂函数）
     * @param creator 函数对象，接受父控件指针，返回编辑器控件指针
     * @code
     * builder.setEditor([](QWidget* p){ return new QSpinBox(p); });
     * @endcode
     */
    TableBuilder& setEditor(ColumnConfig::EditorCreator creator);
    /**
     * @brief 设置当前列的自定义委托
     * @param delegate 委托对象指针
     * @param takeOwnership 若为 true，则 Builder 析构时 delete 该委托；否则由用户管理生命周期
     */
    TableBuilder& setDelegate(QAbstractItemDelegate* delegate, bool takeOwnership = false);
    /// 设置当前列的数据角色（用于自定义模型，默认 Qt::DisplayRole）
    TableBuilder& setDataRole(int role);

    // ========== 2. 表格样式设置 ==========

    /// 整体替换表格样式
    TableBuilder& setStyle(const TableStyle& style);
    /// 设置表格背景色
    TableBuilder& setBackgroundColor(const QColor& color);
    /// 启用/禁用交替行颜色
    TableBuilder& setAlternatingRowColors(bool enable);
    /// 设置网格线颜色
    TableBuilder& setGridColor(const QColor& color);
    /// 设置选中行背景色和前景色
    TableBuilder& setSelectionColors(const QColor& background, const QColor& foreground);
    /// 设置表头字体
    TableBuilder& setHeaderFont(const QFont& font);
    /// 设置单元格字体（需在委托中实现，当前版本仅占位）
    TableBuilder& setCellFont(const QFont& font);
    /// 设置默认行高（像素）
    TableBuilder& setRowHeight(int height);
    /// 应用内置主题（亮色/暗色）
    TableBuilder& applyTheme(TableStyle::Theme theme);

    // ========== 3. 列预设与模板复用 ==========

    /// 列模板函数类型：接收 TableBuilder 引用和自定义标题，内部调用 addColumn 及设置方法
    using ColumnTemplate = std::function<void(TableBuilder&, const QString&)>;

    /**
     * @brief 注册一个列模板
     * @param name 模板名称（全局，所有 Builder 实例共享）
     * @param tmpl 模板函数
     * @note 通常在程序启动时注册一次
     */
    static void registerColumnTemplate(const QString& name, ColumnTemplate tmpl);

    /**
     * @brief 从已注册的模板添加列
     * @param templateName 模板名称
     * @param customTitle 自定义列标题，若为空则使用模板名称作为标题
     * @return 自身引用
     */
    TableBuilder& addColumnFromTemplate(const QString& templateName, const QString& customTitle = QString());

    // ========== 4. 动态列管理（运行时） ==========

    /**
     * @brief 在指定位置插入新列
     * @param index 插入位置（0 到当前列数）
     * @param title 列标题
     * @return 是否成功
     */
    bool insertColumn(int index, const QString& title);

    /// 移除指定列（索引有效时）
    bool removeColumn(int index);

    /**
     * @brief 移动列（交换两列的位置）
     * @param from 源列索引
     * @param to 目标列索引
     * @return 是否成功
     */
    bool moveColumn(int from, int to);

    /// 清空所有列配置
    void clearColumns();

    // ========== 5. 数据绑定与自动刷新 ==========

    /**
     * @brief 设置数据源为二维 QVariant 列表
     * @param data 行列表，每行为列数据列表
     * @note 调用后会立即刷新表格，使用内部 QStandardItemModel
     */
    void setDataSource(const QList<QList<QVariant>>& data);

    /**
     * @brief 设置数据源为外部 QAbstractItemModel
     * @param model 外部模型指针（调用者负责生命周期）
     * @note Builder 会监听模型的 dataChanged、rowsInserted 等信号自动刷新视图
     */
    void setDataSource(QAbstractItemModel* model);

    /// 手动刷新数据（当外部数据源变化且未自动更新时调用）
    void refreshData();

    /**
     * @brief 绑定到外部模型并建立信号连接
     * @param model 外部模型
     * @note 通常 setDataSource(model) 已调用本方法，无需单独调用
     */
    void bindToModel(QAbstractItemModel* model);

    // ========== 6. 导出/导入列配置（JSON） ==========

    /**
     * @brief 导出当前列配置为 JSON 对象
     * @return QJsonObject 包含所有列配置信息
     */
    QJsonObject exportColumnConfig() const;

    /**
     * @brief 从 JSON 对象导入列配置
     * @param config 之前导出的 JSON 对象
     * @return 是否成功（需包含 "columns" 数组）
     */
    bool importColumnConfig(const QJsonObject& config);

    // ========== 7. 高级排序与过滤面板 ==========

    /**
     * @brief 启用/禁用全局过滤输入框
     * @param enable true 显示一个全局文本框，输入内容将在所有列中进行匹配
     * @note 实际过滤由 QSortFilterProxyModel 完成，所有列均参与匹配
     */
    void enableGlobalFilter(bool enable = true);

    /**
     * @brief 启用/禁用每列独立的过滤框（简化版）
     * @param enable true 则在表头上方为每列添加一个 QLineEdit，用于单独过滤该列
     * @note 当前实现中多个列的过滤会互相覆盖（仅最后一个生效），建议使用全局过滤
     */
    void enableColumnFilter(bool enable = true);

    /// 设置全局过滤框的占位文本
    void setFilterPlaceholder(const QString& text);

    // ========== 8. 拖拽行/列排序 ==========

    /**
     * @brief 设置拖拽排序功能
     * @param enableRows 是否启用行拖拽（不完整，可能导致行消失，建议 false）
     * @param enableColumns 是否启用列拖拽（完整支持，可安全开启）
     * @note 列拖拽会同步更新 m_configs 中的列顺序；行拖拽因 QStandardItemModel 限制，需要额外实现，当前示例中行拖拽会导致数据消失，请谨慎使用。
     */
    void setDragDropEnabled(bool enableRows = true, bool enableColumns = true);

    // ========== 9. 虚拟滚动（延迟加载） ==========

    /// 数据获取回调：当需要加载更多行时调用，参数 start 起始行索引，count 需要加载的行数
    using FetchMoreCallback = std::function<void(int start, int count)>;

    /**
     * @brief 设置虚拟滚动模式（延迟加载）
     * @param totalRows 总行数（预估）
     * @param callback 回调函数，用于加载指定范围的行数据
     * @note 启用后，滚动条滚动到底部附近时会触发回调，用户应在回调中向模型追加数据
     */
    void setVirtualMode(int totalRows, FetchMoreCallback callback);

    /// 启用或禁用虚拟滚动（通常由 setVirtualMode 自动调用）
    void setVirtualModeEnabled(bool enable);

    // ========== 构建与获取控件 ==========

    /**
     * @brief 构建表格控件，应用所有配置
     * @return QTableView* 构建好的表格视图指针（由 Builder 内部管理，析构时自动删除）
     * @note 在调用 build() 之前的所有配置将在此时一次性生效
     */
    QTableView* build();

    /// 获取内部使用的 QStandardItemModel（可能在切换到外部模型后变为 nullptr）
    QStandardItemModel* model() const;

private slots:
    /// 全局过滤文本框内容变化时的槽函数
    void onFilterTextChanged(const QString& text);
    /// 某列过滤文本框内容变化时的槽函数
    void onColumnFilterChanged(int column, const QString& text);

private:
    // 内部辅助函数
    void initView();                ///< 初始化视图和模型（如果尚未创建）
    void applyColumnConfigs();     ///< 将列配置应用到视图（标题、宽度、隐藏等）
    void applyStyle();             ///< 应用表格样式（颜色、字体、行高等）
    void setupDelegates();         ///< 为每列设置委托（需实现 GenericColumnDelegate）
    void setupFilterPanel();       ///< 将过滤面板放置在合适位置（布局相关）
    void setupDragDrop();          ///< 配置拖拽相关选项（目前未完整实现）

    // ---- 成员变量 ----
    QTableView* m_view = nullptr;               ///< 表格视图控件
    QStandardItemModel* m_model = nullptr;      ///< 内部数据模型（当使用外部模型时可能为 nullptr）
    QSortFilterProxyModel* m_proxyModel = nullptr; ///< 代理模型，用于过滤和排序
    QWidget* m_filterPanel = nullptr;           ///< 列过滤面板容器（仅当 enableColumnFilter 时创建）
    QLineEdit* m_globalFilterEdit = nullptr;    ///< 全局过滤输入框
    QList<QLineEdit*> m_columnFilterEdits;      ///< 每列独立的过滤输入框列表

    QList<ColumnConfig> m_configs;              ///< 所有列配置的列表
    int m_currentCol = -1;                      ///< 当前正在配置的列索引（-1 表示未开始或无效）
    TableStyle m_style;                         ///< 当前表格样式

    // 数据绑定相关
    QList<QList<QVariant>> m_cachedData;        ///< 当使用 setDataSource(QList) 时的数据缓存
    bool m_useExternalModel = false;            ///< 是否使用外部模型
    QAbstractItemModel* m_externalModel = nullptr; ///< 外部模型指针（不负责释放）

    // 虚拟滚动相关
    bool m_virtualMode = false;                 ///< 是否启用虚拟滚动模式
    int m_virtualTotalRows = 0;                 ///< 虚拟模式下的总行数
    FetchMoreCallback m_fetchCallback;          ///< 加载数据的回调函数

    // 拖拽相关标志
    bool m_dragRows = false;                    ///< 是否启用行拖拽
    bool m_dragCols = false;                    ///< 是否启用列拖拽

    // 静态列模板注册表（所有 TableBuilder 实例共享）
    static QHash<QString, ColumnTemplate> s_templates;
};

#endif // TABLEBUILDER_H
