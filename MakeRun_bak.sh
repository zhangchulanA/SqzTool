#!/bin/bash
# ============================================================================
# 脚本名称: MakeRun.sh
# 功能: 打包 Sqz 库，生成 .tar.gz 和 .run 安装包
# 使用: 在 .pro 文件中添加 QMAKE_POST_LINK += $$PWD/MakeRun.sh $$VERSION $$PWD
# ============================================================================

set -e

VERSION=$1
PRO_PWD=$2

if [ -z "$VERSION" ] || [ -z "$PRO_PWD" ]; then
    echo "错误: 用法 $0 <版本号> <源码目录>"
    exit 1
fi

PACKAGE_NAME="Sqz"
SYS_NAME=$(uname -s)
TIMESTAMP=$(date +%Y%m%d)
BASE_NAME="${PACKAGE_NAME}_${SYS_NAME}_v${VERSION}"

OUTPUT_DIR="$PRO_PWD/../Packages"
TAR_FILE="$OUTPUT_DIR/${BASE_NAME}.tar.gz"
RUN_FILE="$OUTPUT_DIR/${BASE_NAME}.run"
WORK_DIR="$PRO_PWD/package_work"

mkdir -p "$OUTPUT_DIR"
rm -rf "$WORK_DIR"
mkdir -p "$WORK_DIR/$PACKAGE_NAME"

echo "========== 开始打包 Sqz =========="
echo "源码目录: $PRO_PWD"
echo "版本号: $VERSION"
echo "输出目录: $OUTPUT_DIR"
echo "包名: $BASE_NAME"

# 1. 收集所有 .h 文件（排除根目录，只保留子文件夹内的）
echo "收集头文件..."
cd "$PRO_PWD"
find . -mindepth 2 -name "*.h" -type f | while read header; do
    target_path="${header#./}"
    mkdir -p "$WORK_DIR/$PACKAGE_NAME/$(dirname "$target_path")"
    cp "$header" "$WORK_DIR/$PACKAGE_NAME/$target_path"
done

# 2. 收集所有 .cpp 文件（排除根目录，只保留子文件夹内的）
echo "收集源文件..."
find . -mindepth 2 -name "*.cpp" -type f | while read src; do
    target_path="${src#./}"
    mkdir -p "$WORK_DIR/$PACKAGE_NAME/$(dirname "$target_path")"
    cp "$src" "$WORK_DIR/$PACKAGE_NAME/$target_path"
done

# 3. 复制 SqzLib 全部文件（库文件）
echo "收集库文件..."
if [ -d "$PRO_PWD/SqzLib" ]; then
    mkdir -p "$WORK_DIR/$PACKAGE_NAME/SqzLib"
    cp -r "$PRO_PWD/SqzLib"/* "$WORK_DIR/$PACKAGE_NAME/SqzLib/" 2>/dev/null || true
    echo "已复制 SqzLib 内容"
else
    echo "错误: 找不到 SqzLib 目录"
    exit 1
fi

# 4. 生成 Sqz.prf（修正 INCLUDEPATH 路径，基于临时目录结构）
echo "生成 Sqz.prf..."
cat > "$WORK_DIR/Sqz.prf" << 'EOF'
# Sqz.prf - 使用方法: CONFIG += Sqz
Sqz_ROOT = /opt/Sqz
INCLUDEPATH += $$Sqz_ROOT/include
EOF

# 遍历临时目录下的所有子目录（排除 SqzLib，因为它是库目录，不包含头文件）
cd "$WORK_DIR/$PACKAGE_NAME"
find . -mindepth 1 -type d ! -path "./SqzLib*" | while read dir; do
    clean_dir="${dir#./}"
    if find "$dir" -maxdepth 1 -name "*.h" | grep -q .; then
        echo "INCLUDEPATH += \$\$Sqz_ROOT/include/$clean_dir" >> "$WORK_DIR/Sqz.prf"
    fi
done

cat >> "$WORK_DIR/Sqz.prf" << 'EOF'
LIBS += -L$$Sqz_ROOT/lib -lSqz
EOF

# 5. 生成 install.sh（增加了赋予 /opt/Sqz 全部权限的步骤）
echo "生成 install.sh..."
cat > "$WORK_DIR/install.sh" << 'EOF'
#!/bin/bash
# Sqz 安装脚本（增强版，确保 CONFIG += Sqz 可用）

INSTALL_DIR="/opt/Sqz"
INCLUDE_DIR="$INSTALL_DIR/include"
LIB_DIR="$INSTALL_DIR/lib"

echo "=========================================="
echo "安装 Sqz 到 $INSTALL_DIR"
echo "=========================================="

sudo mkdir -p "$INCLUDE_DIR" "$LIB_DIR"

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

echo "安装头文件..."
cd "$SCRIPT_DIR/Sqz"
find . -name "*.h" -type f | while read header; do
    target_path="${header#./}"
    target_dir="$INCLUDE_DIR/$(dirname "$target_path")"
    sudo mkdir -p "$target_dir"
    sudo cp "$header" "$target_dir/"
done

echo "安装库文件..."
sudo cp -r "$SCRIPT_DIR/Sqz/SqzLib"/* "$LIB_DIR/" 2>/dev/null || true

sudo cp "$SCRIPT_DIR/Sqz.prf" "$INSTALL_DIR/"

echo "配置 Qt feature (CONFIG += Sqz) ..."

find_all_qmake() {
    if command -v qmake &>/dev/null; then
        which qmake
    fi
    for prefix in "$HOME/Qt" "/opt/Qt" "/usr/local/Qt" "/usr/lib/qt" "/usr/lib/qt5" "/usr"; do
        if [ -d "$prefix" ]; then
            find "$prefix" -type f -name "qmake" -perm /111 2>/dev/null | head -5
        fi
    done
    if command -v locate &>/dev/null; then
        locate qmake 2>/dev/null | grep -E "/(bin|gcc_64/bin)/qmake$" | head -3
    fi
}

get_features_dir() {
    local qmake_path="$1"
    local features_dir=""
    local qt_data=$("$qmake_path" -query QT_INSTALL_DATA 2>/dev/null)
    if [ -n "$qt_data" ] && [ -d "$qt_data/mkspecs/features" ]; then
        features_dir="$qt_data/mkspecs/features"
    fi
    if [ -z "$features_dir" ]; then
        local qmake_bin_dir=$(dirname "$qmake_path")
        local parent_dir=$(dirname "$qmake_bin_dir")
        if [ -d "$parent_dir/mkspecs/features" ]; then
            features_dir="$parent_dir/mkspecs/features"
        fi
    fi
    if [ -z "$features_dir" ]; then
        local qt_prefix=$("$qmake_path" -query QT_INSTALL_PREFIX 2>/dev/null)
        if [ -n "$qt_prefix" ] && [ -d "$qt_prefix/mkspecs/features" ]; then
            features_dir="$qt_prefix/mkspecs/features"
        fi
    fi
    echo "$features_dir"
}

declare -A features_map
for qmake_path in $(find_all_qmake | sort -u); do
    if [ -x "$qmake_path" ]; then
        feat_dir=$(get_features_dir "$qmake_path")
        if [ -n "$feat_dir" ] && [ -d "$feat_dir" ]; then
            features_map["$feat_dir"]="$qmake_path"
        fi
    fi
done

if [ -n "$QT_FEATURES_DIR" ] && [ -d "$QT_FEATURES_DIR" ]; then
    features_map["$QT_FEATURES_DIR"]="user_specified"
fi

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

INSTALLED_COUNT=0
for feat_dir in "${!features_map[@]}"; do
    echo "找到 features 目录: $feat_dir"
    if sudo cp "$SCRIPT_DIR/Sqz.prf" "$feat_dir/"; then
        echo "✓ 已安装 Sqz.prf 到 $feat_dir"
        ((INSTALLED_COUNT++))
    else
        echo "警告: 复制到 $feat_dir 失败"
    fi
done

if [ $INSTALLED_COUNT -gt 0 ]; then
    echo "✅ Qt feature 配置成功！现在可以在 .pro 中使用 CONFIG += Sqz"
else
    echo "====================================================="
    echo "❌ 未能自动找到 Qt features 目录，CONFIG += Sqz 将不可用"
    echo "您可以手动将 $SCRIPT_DIR/Sqz.prf 复制到 Qt features 目录"
    echo "或在 .pro 中写: include($INSTALL_DIR/Sqz.prf)"
    echo "====================================================="
fi

echo "配置系统库路径..."
echo "$LIB_DIR" | sudo tee /etc/ld.so.conf.d/Sqz.conf > /dev/null
sudo ldconfig

if ! grep -q "Sqz" ~/.bashrc 2>/dev/null; then
    echo "" >> ~/.bashrc
    echo "# Sqz" >> ~/.bashrc
    echo "export LD_LIBRARY_PATH=$LIB_DIR:\$LD_LIBRARY_PATH" >> ~/.bashrc
    echo "已添加 LD_LIBRARY_PATH 到 ~/.bashrc"
fi

# 赋予 /opt/Sqz 全部权限（所有用户可读、写、执行）
echo "赋予 $INSTALL_DIR 全部权限..."
sudo chmod -R 777 "$INSTALL_DIR"

echo "=========================================="
echo "安装完成！"
echo "头文件: $INCLUDE_DIR"
echo "库文件: $LIB_DIR"
if [ $INSTALLED_COUNT -gt 0 ]; then
    echo "✅ 使用方法：在 .pro 文件中写入  CONFIG += Sqz"
else
    echo "⚠️  使用方法：在 .pro 文件中写入  include(/opt/Sqz/Sqz.prf)"
fi
echo "=========================================="
EOF

chmod +x "$WORK_DIR/install.sh"

# 6. 打包
echo "创建 tar.gz..."
cd "$WORK_DIR"
tar -czf "$TAR_FILE" Sqz/ install.sh Sqz.prf

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
rm -rf Sqz/ install.sh Sqz.prf
exit 0
__ARCHIVE_BELOW__
EOF
cat "$TAR_FILE" >> "$RUN_FILE"
chmod +x "$RUN_FILE"

# 7. 清理
rm -rf "$WORK_DIR"

# 8. 同步
sync

echo "========== 打包完成 =========="
echo "输出文件:"
ls -lh "$TAR_FILE" "$RUN_FILE"
echo ""
echo "安装方法: sudo ./$RUN_FILE"
