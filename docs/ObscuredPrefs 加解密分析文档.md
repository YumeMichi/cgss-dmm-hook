# ObscuredPrefs 加解密分析文档

## 项目背景

**游戏名称：** 偶像大师灰姑娘女孩星光舞台 (`imascgstage`)  
**平台：** Windows / Android  
**目标：** 结合 Windows 注册表样本与 Android `dump.cs` / `libil2cpp.so`，还原可实际复现的注册表键值加解密实现。

---

## 一、最终结论

目前已经可以完整实现当前 Windows 注册表样本对应的 `ObscuredPrefs` 加解密。

已确认结论如下：

1. 游戏使用 `CodeStage.AntiCheat.ObscuredTypes.ObscuredPrefs`。
2. `ObscuredPrefs.encryptionKey = "e806f6"`。
3. 注册表键名主干的算法是：
   - `plainKey` UTF-8
   - 与 `e806f6` 循环 XOR
   - Base64 编码
4. 注册表键名的 `_h1234567890` 后缀**不属于 ACTk 本体**。
   - 它与 Unity Windows `PlayerPrefs` 的注册表 value name 后缀行为一致。
   - 对当前 9 个样本，后缀都能被 `djb2_xor(Base64主干)` 精确复现。
5. 注册表值的实际布局是：
   - `payload`
   - `DataType`
   - `VERSION(=2)`
   - `deviceLockLevel`
   - `checksum`
   - 如果 `deviceLockLevel != 0`，则 `payload` 后、类型字节前还会多 4 字节 `deviceIDHash`
6. 当前样本的 `payload` 加密 key 不是单独的 `e806f6`，而是：
   - `plainKey + "e806f6"`
7. 当前样本的 `checksum` 是：
   - `xxHash32(明文 payload bytes, seed = 0)`
   - 4 字节小端序存储
8. 以上规则已经能**逐字节复现 9 个 Windows 注册表样本**。

---

## 二、证据来源

### 2.1 Android dump

- `dump.cs`: `/home/aya/workspace/cgss/data/android/dump.cs`
- `libil2cpp.so`: `/home/aya/workspace/cgss/data/android/libil2cpp.so`
- `script.json`: `/home/aya/workspace/cgss/data/android/script.json`

### 2.2 关键方法

`dump.cs` / `script.json` 可确认以下方法存在：

- `ObscuredPrefs.EncryptKey`
- `ObscuredPrefs.GetEncryptedPrefsString`
- `ObscuredPrefs.EncryptData`
- `ObscuredPrefs.DecryptData`
- `ObscuredPrefs.CalculateChecksum`
- `ObscuredPrefs.EncryptDecryptBytes`
- `xxHash.CalculateHash`

### 2.3 关键字段

`dump.cs` 中 `ObscuredPrefs` 静态字段布局如下：

```csharp
private static string encryptionKey; // 0x0
private static bool foreignSavesReported; // 0x8
private static string deviceID; // 0x10
private static uint deviceIDHash; // 0x18
public static Action onAlterationDetected; // 0x20
public static bool preservePlayerPrefs; // 0x28
public static Action onPossibleForeignSavesDetected; // 0x30
public static ObscuredPrefs.DeviceLockLevel lockToDevice; // 0x38
public static bool readForeignSaves; // 0x39
public static bool emergencyMode; // 0x3A
private static string deprecatedDeviceID; // 0x40
```

这直接解释了 `DecryptData` 中对 `0x38 / 0x39 / 0x3A` 的读取含义。

---

## 三、键名算法

### 3.1 ACTk 部分

`ObscuredPrefs.EncryptKey` 在 Android `libil2cpp.so` 中的行为已经确认：

1. 读取 `ObscuredPrefs.encryptionKey`
2. 调用 `ObscuredString.EncryptDecrypt(key, encryptionKey)`
3. 对结果调用 `Convert.ToBase64String`
4. 返回 Base64 字符串

也就是说，**ACTk 本体只负责生成 Base64 主干**，不负责 `_h...` 后缀。

### 3.2 Windows 注册表后缀

当前 Windows 样本的最终键名是：

```text
<base64_stem>_h<decimal_hash>
```

对 9 个样本全部验证后，后缀满足：

```text
hash = djb2_xor(base64_stem)
```

其中：

```text
init = 5381
for each byte:
    hash = ((hash << 5) + hash) ^ byte
```

32 位无符号溢出。

例如 `VIEWER_ID`：

```text
plainKey = VIEWER_ID
XOR key  = e806f6
XOR 后   = 33 71 75 61 23 64 3a 71 74
Base64   = M3F1YSNkOnF0
djb2_xor(Base64) = 4073495316
最终键名 = M3F1YSNkOnF0_h4073495316
```

### 3.3 关于 `_h...` 来源的判断

这部分并非从 Android ACTk 代码直接读出，而是结合以下事实作出的结论：

1. `EncryptKey()` 本体不拼接 `_h...`
2. 当前 9 个样本都与 `djb2_xor(Base64主干)` 完全一致
3. `ObscuredPrefs.GetEncryptedPrefsString()` / `SetInt()` 最终调用的是 Unity `PlayerPrefs`

因此可判断：

```text
_h...` 后缀是 Windows PlayerPrefs 注册表 value name 规则，而不是 ACTk 自己定义的附加校验段。
```

---

## 四、值算法

### 4.1 值结构

`EncryptData` / `DecryptData` 结合反汇编可以还原出当前版本结构：

当 `deviceLockLevel == 0` 时：

```text
payload | dataType | version | deviceLock | checksum
```

当 `deviceLockLevel != 0` 时：

```text
payload | deviceIDHash | dataType | version | deviceLock | checksum
```

然后整体做 Base64，最终以 ASCII 写入注册表，再补一个结尾 `00` 字节。

### 4.2 payload 的 XOR key

这是值加密实现里的关键点。

`EncryptData` 在进入 `EncryptDecryptBytes` 之前，会先做：

```text
String.Concat(plainKey, encryptionKey)
```

再把这个结果作为 XOR key。

因此真实规则不是：

```text
payload ^= "e806f6"
```

而是：

```text
payload ^= (plainKey + "e806f6")
```

### 4.3 checksum

`DecryptData` 的校验流程可确认：

1. 从尾部取 4 字节 checksum
2. 取出 payload
3. 先把 payload XOR 解密成明文 bytes
4. 对**明文 bytes**做 `xxHash.CalculateHash(payload, len, 0)`
5. 与尾部 checksum 比较

因此当前版本的 checksum 为：

```text
xxHash32(plainPayloadBytes, seed = 0)
```

并按小端序写入尾部。

### 4.4 `CalculateChecksum(string)` 的实际作用

Android dump 中确实存在：

```csharp
private static uint CalculateChecksum(string input);
```

但它不是当前 value trailer 的生成函数。

从调用关系看，它用于：

- `DeviceIDHash`
- `SetNewCryptoKey`
- 若干设备锁相关逻辑

而不是 `EncryptData` / `DecryptData` 里这 4 字节 payload checksum。

因此，当前 value 尾部 checksum 应直接归为 `xxHash32(plainPayloadBytes)`，而不是 `CalculateChecksum(string)` 的结果。

---

## 五、样本验证结果

### 5.1 当前 9 个样本可完全复现

按以下规则：

1. key stem = `Base64(XOR(plainKey, "e806f6"))`
2. key suffix = `"_h" + djb2_xor(key stem)`
3. int payload = `struct.pack("<i", value)`
4. encrypted payload = `XOR(payload, plainKey + "e806f6")`
5. checksum = `xxHash32(payload)`
6. raw = `encryptedPayload + 05 + 02 + 00 + checksum_le`
7. value = `hex(ASCII(Base64(raw))) + 00`

可以逐字节复现全部 9 组样本。

### 5.2 样本对应的明文值

当前样本的真实 payload 明文如下：

| 键名 | 明文值 |
|------|--------|
| `VIEWER_ID` | `589089289` |
| `TUTORIAL_STEP` | `1000` |
| `TUTORIAL_LOADED_FROM_SERVER_FLAG` | `0` |
| `TUTORIAL_IS_DIRTY_LOCAL_DATA` | `0` |
| `POLICY_PRE_ANNOUNCE_DATE` | `1753452686` |
| `POLICY_ANNOUNCE_DATE` | `1753452686` |
| `BN_CONTENT_RUN` | `2` |
| `BN_CONTENT_AGREE_ANALYSIS` | `1` |
| `BN_CONTENT_AGREE_ADVERTISEMENT` | `1` |

其中两个时间值转为 UTC 时间是：

```text
1753452686 -> 2025-07-25T14:11:26Z
```

这些结果与字段语义能够直接对应，可作为当前样本的 payload 明文解释。

---

## 六、关于 ObscuredInt 的关系

已确认：

- `ObscuredInt.cryptoKey = 444444`
- `ObscuredInt.Encrypt/Decrypt(value, key) = value ^ key`

但就**当前 Windows 注册表格式实现**来说，不需要再额外引入 `ObscuredInt`。

原因是：

1. 当前样本的 payload 明文已经能直接解释为合理业务值
2. 现有样本的写回复现不需要额外 `^ 444444`
3. 我们已经能逐字节复现游戏样本

因此，对“如何完整实现当前注册表加解密”这个目标来说，答案已经足够闭合。

`ObscuredInt` 更像是业务对象在内存中的字段包装，而不是当前注册表 payload 必须再做的一层存储编码。

---

## 七、仓库中的实现

当前实现文件：

- `obscured_prefs.py`

相关辅助脚本：

- `decrypt_all.py`
- `decrypt_tool.py`
- `encrypt_tool.py`
- `verify_roundtrip.py`
- `analyze_registry.py`
- `analyze_values.py`

这些脚本都基于本文确认的同一套规则实现。

---

## 八、可直接使用的实现摘要

### 8.1 键名

```text
stem = Base64(XOR(plainKey, "e806f6"))
registryKey = stem + "_h" + djb2_xor(stem)
```

### 8.2 Int 值

```text
plainBytes = little_endian_int32(value)
xorKey = plainKey + "e806f6"
payload = XOR(plainBytes, xorKey)
checksum = xxHash32(plainBytes)
raw = payload + [0x05, 0x02, 0x00] + little_endian_uint32(checksum)
registryValue = hex(ASCII(Base64(raw))) + "00"
```

### 8.3 解密

```text
raw = Base64Decode(ASCII(hexBytesWithoutTrailing00))
payload = raw[:-7]
type = raw[-7]
version = raw[-6]
deviceLock = raw[-5]
checksum = little_endian_uint32(raw[-4:])
plainBytes = XOR(payload, plainKey + "e806f6")
verify xxHash32(plainBytes) == checksum
```

---

## 九、结论

这次分析后，以下问题都已经闭合：

1. 键名主干如何生成
2. `_h...` 后缀如何生成
3. 值的真实布局
4. payload 的真实 XOR key
5. checksum 的真实算法
6. 现有 9 个 Windows 样本如何逐字节复现

因此，针对当前游戏 Windows 端注册表样本，已经可以认为：

```text
ObscuredPrefs 加解密实现已完整还原。
```
