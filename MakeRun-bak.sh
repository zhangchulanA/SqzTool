#!/bin/bash
# ============================================================================
# 脚本名称: MakeRun.sh
# 功能: 打包 SqzTool 库，生成 .tar.gz 和 .run 安装包
# 使用: 在 .pro 文件中添加 QMAKE_POST_LINK += $$PWD/MakeRun.sh $$OUT_PWD $$PWD
# ============================================================================

set -e  # 出错立即停止

# 接收参数
OUT_PWD=$1
PRO_PWD=$2

# 配置
PACKAGE_NAME="SqzTool"                     # 工作目录名（保持不变）
SYS_NAME=$(uname -s)                       # 系统名，如 Linux
TIMESTAMP=$(date +%Y%m%d)             # 时间戳，如 20260517
BASE_NAME="${PACKAGE_NAME}_${SYS_NAME}_${TIMESTAMP}"   # 最终包名

OUTPUT_DIR="$PRO_PWD/../Packages"
TAR_FILE="$OUTPUT_DIR/${BASE_NAME}.tar.gz"
RUN_FILE="$OUTPUT_DIR/${BASE_NAME}.run"
WORK_DIR="$PRO_PWD/package_work"

mkdir -p "$OUTPUT_DIR"
rm -rf "$WORK_DIR"
mkdir -p "$WORK_DIR/$PACKAGE_NAME"

echo "========== 开始打包 SqzTool =========="
echo "源码目录: $PRO_PWD"
echo "输出目录: $OUTPUT_DIR"
echo "包名: $BASE_NAME"

# 1. 收集所有 .h 文件
echo "收集头文件..."
cd "$PRO_PWD"
find . -name "*.h" -type f | while read header; do
    target_path="${header#./}"
    mkdir -p "$WORK_DIR/$PACKAGE_NAME/$(dirname "$target_path")"
    cp "$header" "$WORK_DIR/$PACKAGE_NAME/$target_path"
done

# 2. 复制 SqzLib 全部文件
echo "收集库文件..."
if [ -d "$PRO_PWD/SqzLib" ]; then
    mkdir -p "$WORK_DIR/$PACKAGE_NAME/SqzLib"
    cp -r "$PRO_PWD/SqzLib"/* "$WORK_DIR/$PACKAGE_NAME/SqzLib/" 2>/dev/null || true
    echo "已复制 SqzLib 内容"
else
    echo "错误: 找不到 SqzLib 目录"
    exit 1
fi

# 3. 生成 sqztool.prf（Qt feature 文件）
echo "生成 sqztool.prf..."
cat > "$WORK_DIR/sqztool.prf" << 'EOF'
# sqztool.prf - 使用方法: CONFIG += sqztool
SQZTOOL_ROOT = /opt/SqzTool
INCLUDEPATH += $$SQZTOOL_ROOT/include
EOF

# 动态添加所有包含 .h 的子目录
cd "$PRO_PWD"
find . -type d | while read dir; do
    clean_dir="${dir#./}"
    if [ -n "$clean_dir" ] && [ "$clean_dir" != "." ]; then
        if ls "$dir"/*.h >/dev/null 2>&1; then
            echo "INCLUDEPATH += \$\$SQZTOOL_ROOT/include/$clean_dir" >> "$WORK_DIR/sqztool.prf"
        fi
    fi
done

cat >> "$WORK_DIR/sqztool.prf" << 'EOF'
LIBS += -L$$SQZTOOL_ROOT/lib -lSqzTool
EOF

# 4. 生成 install.sh（增强版，稳定支持 CONFIG += sqztool）
echo "生成 install.sh..."
cat > "$WORK_DIR/install.sh" << 'EOF'
#!/bin/bash
# SqzTool 安装脚本（增强版，确保 CONFIG += sqztool 可用）

INSTALL_DIR="/opt/SqzTool"
INCLUDE_DIR="$INSTALL_DIR/include"
LIB_DIR="$INSTALL_DIR/lib"

echo "=========================================="
echo "安装 SqzTool 到 $INSTALL_DIR"
echo "=========================================="

# 创建目录
sudo mkdir -p "$INCLUDE_DIR" "$LIB_DIR"

# 获取脚本所在目录
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

# 安装头文件
echo "安装头文件..."
cd "$SCRIPT_DIR/SqzTool"
find . -name "*.h" -type f | while read header; do
    target_path="${header#./}"
    target_dir="$INCLUDE_DIR/$(dirname "$target_path")"
    sudo mkdir -p "$target_dir"
    sudo cp "$header" "$target_dir/"
done

# 安装库文件
echo "安装库文件..."
sudo cp -r "$SCRIPT_DIR/SqzTool/SqzLib"/* "$LIB_DIR/" 2>/dev/null || true
sudo ldconfig

# 安装 sqztool.prf 到 /opt/SqzTool（备用 include 方式）
sudo cp "$SCRIPT_DIR/sqztool.prf" "$INSTALL_DIR/"

# ========== 核心：安装 Qt feature 以支持 CONFIG += sqztool ==========
echo "配置 Qt feature (CONFIG += sqztool) ..."

# 函数：查找所有可能的 qmake
find_all_qmake() {
    # 1. PATH 中的 qmake
    if command -v qmake &>/dev/null; then
        which qmake
    fi
    # 2. 常见安装目录
    for prefix in "$HOME/Qt" "/opt/Qt" "/usr/local/Qt" "/usr/lib/qt" "/usr/lib/qt5" "/usr"; do
        if [ -d "$prefix" ]; then
            find "$prefix" -type f -name "qmake" -perm /111 2>/dev/null | head -5
        fi
    done
    # 3. locate（如果可用）
    if command -v locate &>/dev/null; then
        locate qmake 2>/dev/null | grep -E "/(bin|gcc_64/bin)/qmake$" | head -3
    fi
}

# 函数：根据 qmake 路径获取 features 目录
get_features_dir() {
    local qmake_path="$1"
    local features_dir=""
    
    # 方法1: qmake -query QT_INSTALL_DATA
    local qt_data=$("$qmake_path" -query QT_INSTALL_DATA 2>/dev/null)
    if [ -n "$qt_data" ] && [ -d "$qt_data/mkspecs/features" ]; then
        features_dir="$qt_data/mkspecs/features"
    fi
    
    # 方法2: 根据路径推断（例如 .../gcc_64/bin/qmake -> .../gcc_64/mkspecs/features）
    if [ -z "$features_dir" ]; then
        local qmake_bin_dir=$(dirname "$qmake_path")
        local parent_dir=$(dirname "$qmake_bin_dir")
        if [ -d "$parent_dir/mkspecs/features" ]; then
            features_dir="$parent_dir/mkspecs/features"
        fi
    fi
    
    # 方法3: qmake -query QT_INSTALL_PREFIX
    if [ -z "$features_dir" ]; then
        local qt_prefix=$("$qmake_path" -query QT_INSTALL_PREFIX 2>/dev/null)
        if [ -n "$qt_prefix" ] && [ -d "$qt_prefix/mkspecs/features" ]; then
            features_dir="$qt_prefix/mkspecs/features"
        fi
    fi
    
    echo "$features_dir"
}

# 收集所有可能的 qmake 和对应的 features 目录
declare -A features_map
for qmake_path in $(find_all_qmake | sort -u); do
    if [ -x "$qmake_path" ]; then
        feat_dir=$(get_features_dir "$qmake_path")
        if [ -n "$feat_dir" ] && [ -d "$feat_dir" ]; then
            features_map["$feat_dir"]="$qmake_path"
        fi
    fi
done

# 如果用户通过环境变量指定了目录，优先使用
if [ -n "$QT_FEATURES_DIR" ] && [ -d "$QT_FEATURES_DIR" ]; then
    features_map["$QT_FEATURES_DIR"]="user_specified"
fi

# 如果没有找到任何 features 目录，尝试系统常用路径（硬编码）
if [ ${#features_map[@]} -eq 0 ]; then
    for try_dir in "/usr/lib/x86_64-linux-gnu/qt5/mkspecs/features" \
                   "/usr/lib64/qt5/mkspecs/features" \
                   "/usr/share/qt5/mkspecs/features" \
                   "/usr/lib/qt5/mkspecs/features" \
                   "/usr/local/share/qt5/mkspecs/features"; do
        if [ -d "$try_dir" ]; then
            features_map["$try_dir"]="system_guess"
            break
        fi
    done
fi

# 安装 .prf 文件到找到的每个 features 目录（避免多个 Qt 版本混乱，通常只有一个有效）
INSTALLED_COUNT=0
for feat_dir in "${!features_map[@]}"; do
    echo "找到 features 目录: $feat_dir"
    if sudo cp "$SCRIPT_DIR/sqztool.prf" "$feat_dir/"; then
        echo "✓ 已安装 sqztool.prf 到 $feat_dir"
        ((INSTALLED_COUNT++))
    else
        echo "警告: 复制到 $feat_dir 失败（权限或路径错误）"
    fi
done

if [ $INSTALLED_COUNT -gt 0 ]; then
    echo "✅ Qt feature 配置成功！现在可以在 .pro 中使用 CONFIG += sqztool"
else
    echo "====================================================="
    echo "❌ 未能自动找到 Qt features 目录，CONFIG += sqztool 将不可用"
    echo "但您可以手动将以下文件复制到您的 Qt features 目录："
    echo "  源文件: $SCRIPT_DIR/sqztool.prf"
    echo "  目标目录: 例如 /usr/lib/x86_64-linux-gnu/qt5/mkspecs/features/"
    echo ""
    echo "或者，您也可以使用备选方案（总是有效）："
    echo "  在 .pro 文件中写: include($INSTALL_DIR/sqztool.prf)"
    echo "====================================================="
fi

# 配置系统库路径
echo "配置系统库路径..."
echo "$LIB_DIR" | sudo tee /etc/ld.so.conf.d/sqztool.conf > /dev/null
sudo ldconfig

# 添加用户环境变量（可选）
if ! grep -q "SqzTool" ~/.bashrc 2>/dev/null; then
    echo "" >> ~/.bashrc
    echo "# SqzTool" >> ~/.bashrc
    echo "export LD_LIBRARY_PATH=$LIB_DIR:\$LD_LIBRARY_PATH" >> ~/.bashrc
    echo "已添加 LD_LIBRARY_PATH 到 ~/.bashrc"
fi

echo "=========================================="
echo "安装完成！"
echo ""
echo "头文件: $INCLUDE_DIR"
echo "库文件: $LIB_DIR"
echo ""
if [ $INSTALLED_COUNT -gt 0 ]; then
    echo "✅ 使用方法：在 .pro 文件中写入  CONFIG += sqztool"
else
    echo "⚠️  使用方法：在 .pro 文件中写入  include(/opt/SqzTool/sqztool.prf)"
fi
echo ""
echo "然后代码中直接 #include \"你的头文件.h\""
echo "=========================================="
EOF

chmod +x "$WORK_DIR/install.sh"

# 5. 打包
echo "创建 tar.gz..."
cd "$WORK_DIR"
tar -czf "$TAR_FILE" SqzTool/ install.sh sqztool.prf

echo "创建 .run 自解压包..."
cat > "$RUN_FILE" << 'EOF'
#!/bin/bash
ARCHIVE=$(awk '/^__ARCHIVE_BELOW__/ {print NR + 1; exit 0;}' "$0")
tail -n +$ARCHIVE "$0" | tar -xzv
if [ -f install.sh ]; then
    chmod +x install.sh
    ./install.sh
else
    echo "错误: install.sh 不存在"
    exit 1
fi
rm -rf SqzTool/ install.sh sqztool.prf
exit 0
__ARCHIVE_BELOW__
EOF
cat "$TAR_FILE" >> "$RUN_FILE"
chmod +x "$RUN_FILE"

# 6. 清理
rm -rf "$WORK_DIR"

echo "========== 打包完成 =========="
echo "输出文件:"
ls -lh "$TAR_FILE" "$RUN_FILE"
echo ""
echo "安装方法: sudo ./$RUN_FILE"
