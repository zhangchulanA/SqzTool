int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    qRegisterMetaType<FlexData>();  // 注册元类型，以便在信号槽中使用

    // ========== 1. 基本构造与类型转换 ==========
    FlexData num = 42;
    qDebug() << "Int value:" << num.toInt();               // 42
    qDebug() << "As string:" << num.toString();            // "42"
    qDebug() << "As bool (non-zero):" << num.toBool();     // true

    FlexData text = "Hello";
    qDebug() << "String value:" << text.toString();        // "Hello"
    qDebug() << "As int (invalid):" << text.toInt(100);    // 100 (default)

    // ========== 2. Map 和 Array 基本操作 ==========
    FlexData config;
    config["server"]["host"] = "127.0.0.1";   // 自动创建嵌套 Map
    config["server"]["port"] = 8080;
    config["tags"].append("fast");
    config["tags"].append("secure");
    qDebug() << "Config dump:\n" << config.dump();

    // ========== 3. 路径访问 ==========
    config.set("/server/timeout", 30);        // 使用路径设置
    qDebug() << "Server port:" << config.get("/server/port").toInt();   // 8080
    qDebug() << "Has /server/ssl:" << config.has("/server/ssl");        // false
    config.removePath("/tags/0");             // 删除第一个 tag
    qDebug() << "After remove:\n" << config.dump();

    // ========== 4. 链式构造器 ==========
    FlexData person = FlexData::build()
        .set("name", "Alice")                 // 普通 key
        .set("age", 30)
        .set("/address/city", "Beijing")      // 路径，自动创建中间节点
        .append("hobby", "reading")           // 追加到数组
        .append("hobby", "swimming")
        .done();                              // 完成构建
    qDebug() << "Person built:\n" << person.dump();

    // 实例链式方法（返回引用，可连续调用）
    person.chainSet("email", "alice@example.com")
          .chainSet("/contact/phone", "123456")
          .chainAppend("extra", "note");
    qDebug() << "After chain set:\n" << person.dump();

    // ========== 5. 新类型支持：二进制、日期时间、UUID ==========
    FlexData complex;
    complex["image"] = QByteArray("PNG...");
    complex["timestamp"] = QDateTime::currentDateTime();
    complex["id"] = QUuid::createUuid();
    qDebug() << "Complex JSON:\n" << complex.toJson();     // 自动转为 Base64/ISO 字符串

    // ========== 6. Schema 校验与默认值 ==========
    FlexData schema = FlexData::fromJson(R"({
        "name": { "type": "string", "required": true, "default": "unknown" },
        "age":  { "type": "int", "min": 0, "max": 150 },
        "role": { "type": "string", "enum": ["admin","user"] }
    })");
    FlexData user;
    user["name"] = "Bob";
    user["age"] = 25;
    user["role"] = "admin";
    QStringList errors;
    if (user.validate(schema, &errors))
        qDebug() << "User validation passed";
    else
        qDebug() << "User errors:" << errors;

    // 补全缺失字段
    FlexData incomplete;
    incomplete["age"] = 18;
    FlexData filled = incomplete.applyDefaults(schema);
    qDebug() << "Filled with defaults:\n" << filled.dump();

    // ========== 7. 合并与差量 ==========
    FlexData base;
    base["a"] = 1;
    base["b"]["c"] = 2;
    FlexData other;
    other["b"]["c"] = 3;
    other["d"] = 4;
    FlexData merged = base.merge(other);
    qDebug() << "Merged:\n" << merged.dump();

    FlexData target;
    target["a"] = 1;
    target["b"]["c"] = 4;
    FlexData patch = base.diff(target);
    qDebug() << "Patch:\n" << patch.dump();
    FlexData restored = base.patch(patch);
    qDebug() << "Restored (should equal target):\n" << restored.dump();
    qDebug() << "Equals target:" << (restored == target);

    // ========== 8. 并行快照 ==========
    FlexData original;
    original["value"] = 100;
    FlexData snapshot = original.freeze();   // 创建不可变快照
    original["value"] = 200;                 // 原对象修改
    qDebug() << "Original value:" << original["value"].toInt();   // 200
    qDebug() << "Snapshot value:" << snapshot["value"].toInt();   // 100

    // ========== 9. 序列化：JSON / XML / INI / 二进制 ==========
    FlexData data;
    data["name"] = "Test";
    data["nums"] = QList<FlexData>() << 1 << 2 << 3;
    qDebug() << "JSON:\n" << data.toJson();
    qDebug() << "XML:\n" << data.toXml("root");
    qDebug() << "INI:\n" << data.toIni();

    QByteArray bin = data.serialize();
    FlexData restoredBin;
    restoredBin.deserialize(bin);
    qDebug() << "Binary restored:\n" << restoredBin.dump();

    // ========== 10. QVariant 互转（用于信号槽） ==========
    QVariant var = person;
    FlexData fromVar = FlexData::fromVariant(var);
    qDebug() << "From QVariant, name:" << fromVar.get("/name").toString();

    return 0;
}





// ======================= CRC16-CCITT 表 =======================
static const quint16 crc16_table[256] = {
    0x0000,0x1021,0x2042,0x3063,0x4084,0x50A5,0x60C6,0x70E7,
    0x8108,0x9129,0xA14A,0xB16B,0xC18C,0xD1AD,0xE1CE,0xF1EF,
    0x1231,0x0210,0x3273,0x2252,0x52B5,0x4294,0x72F7,0x62D6,
    0x9339,0x8318,0xB37B,0xA35A,0xD3BD,0xC39C,0xF3FF,0xE3DE,
    0x2462,0x3443,0x0420,0x1401,0x64E6,0x74C7,0x44A4,0x5485,
    0xA56A,0xB54B,0x8528,0x9509,0xE5EE,0xF5CF,0xC5AC,0xD58D,
    0x3653,0x2672,0x1611,0x0630,0x76D7,0x66F6,0x5695,0x46B4,
    0xB75B,0xA77A,0x9719,0x8738,0xF7DF,0xE7FE,0xD79D,0xC7BC,
    0x48C4,0x58E5,0x6886,0x78A7,0x0840,0x1861,0x2802,0x3823,
    0xC9CC,0xD9ED,0xE98E,0xF9AF,0x8948,0x9969,0xA90A,0xB92B,
    0x5AF5,0x4AD4,0x7AB7,0x6A96,0x1A71,0x0A50,0x3A33,0x2A12,
    0xDBFD,0xCBDC,0xFBBF,0xEBFE,0x9B79,0x8B58,0xBB3B,0xAB1A,
    0x6CA6,0x7C87,0x4CE4,0x5CC5,0x2C22,0x3C03,0x0C60,0x1C41,
    0xEDAE,0xFD8F,0xCDEC,0xDDCD,0xAD2A,0xBD0B,0x8D68,0x9D49,
    0x7E97,0x6EB6,0x5ED5,0x4EF4,0x3E13,0x2E32,0x1E51,0x0E70,
    0xFF9F,0xEFBE,0xDFDD,0xCFFC,0xBF1B,0xAF3A,0x9F59,0x8F78,
    0x9188,0x81A9,0xB1CA,0xA1EB,0xD10C,0xC12D,0xF14E,0xE16F,
    0x1080,0x00A1,0x30C2,0x20E3,0x5004,0x4025,0x7046,0x6067,
    0x83B9,0x9398,0xA3FB,0xB3DA,0xC33D,0xD31C,0xE37F,0xF35E,
    0x02B1,0x1290,0x22F3,0x32D2,0x4235,0x5214,0x6277,0x7256,
    0xB5EA,0xA5CB,0x95A8,0x8585,0xF56E,0xE54F,0xD52C,0xC50D,
    0x34E2,0x24C3,0x14A0,0x0481,0x7466,0x6447,0x5424,0x4405,
    0xA7DB,0xB7FA,0x8799,0x97B8,0xE75F,0xF77E,0xC71D,0xD73C,
    0x26D3,0x36F2,0x0691,0x16B0,0x6657,0x7676,0x4615,0x5634,
    0xD94C,0xC96D,0xF90E,0xE92F,0x99C8,0x89E9,0xB98A,0xA9AB,
    0x5844,0x4865,0x7806,0x6827,0x18C0,0x08E1,0x38B2,0x2893,
    0xCB7D,0xDB5C,0xEB3F,0xFB1E,0x8BF9,0x9BD8,0xABBB,0xBB9A,
    0x4A75,0x5A54,0x6A37,0x7A16,0x0AF1,0x1AD0,0x2AB3,0x3A92,
    0xFD2E,0xED0F,0xDD6C,0xCD4D,0xBDAA,0xAD8B,0x9DE8,0x8DC9,
    0x7C26,0x6C07,0x5C64,0x4C45,0x3CA2,0x2C83,0x1CE0,0x0CC1,
    0xEF1F,0xFF3E,0xCF5D,0xDF7C,0xAF9B,0xBFBA,0x8FD9,0x9FF8,
    0x6E17,0x7E36,0x4E55,0x5E74,0x2E93,0x3EB2,0x0ED1,0x1EF0
};
