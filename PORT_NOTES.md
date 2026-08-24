# Port: X@RTD2662 board + Toshiba LTM09C362V (1024x600)

This fork ports [KerJoe/ORTD2662](https://github.com/KerJoe/ORTD2662) to the
generic **X@RTD2662** controller board driving a **Toshiba LTM09C362V**
9.0" 1024x600 18-bit single-channel LVDS panel, used as a standalone HDMI
monitor.

Status: **fully working.** The firmware serves a native 1024x600@60.15 EDID,
the display is pixel-perfect 1:1 with correct colors. Verified with two
different HDMI sources with zero source-side configuration:

* Raspberry Pi 3B (Raspberry Pi OS, vc4-kms-v3d)
* Orange Pi PC2 (Armbian 26.2.5)

To our knowledge this is the first documented third-party port of ORTD2662
to a different board and panel.

This was a *very* long-dormant project: the board is date-stamped **2010/1**
on the back, bought as a board+panel kit shortly after release and left in a
drawer for about 17 years before finally being brought to life.

## Wiring (how to talk to the board)

The board has no dedicated programming header; it is flashed over the
scaler's I2C, which is exposed on the **VGA (D-Sub 15) connector's DDC
pins**. A Raspberry Pi drives that I2C through a bidirectional
**MOSFET logic-level shifter** (the common BSS138 type).

The board's I2C lines measured **4.8 V** (tested with a multimeter), so they
go on the shifter's **high-voltage (HV)** side; the Pi's 3.3 V I2C goes on
the **low-voltage (LV)** side.

```
[Pi GPIO]          [shifter LV]     [shifter HV]     [D-Sub 15]
3.3V  (pin 1) ───── LV
5V    (pin 2) ─────────────────────── HV
GND   (pin 6) ───── GND ───────────── GND ────────── pin 6  (GND)
SDA   (pin 3) ───── LV1          HV1 ─────────────── pin 12 (SDA / DDC data)
SCL   (pin 5) ───── LV2          HV2 ─────────────── pin 15 (SCL / DDC clock)
```

This uses the Pi's hardware I2C bus (`/dev/i2c-1`), which is why the
RTDMultiProg commands below use `-d 1`. The panel still needs its own 12 V
supply for the display/backlight; that is separate from this I2C link.

## Build & flash

Plain SDCC (tested 4.2.0), no other dependencies:

```
make firmware          # -> output/firmware.bin (32 KB)
```

Known-good build md5: `dab06e957b8bca27de369c9ab5abb3cb`

Flash with [RTDMultiProg](https://github.com/KerJoe/RTDMultiProg) (here via a
Raspberry Pi's I2C on the board's HDMI DDC pins):

```
python3 rtdmultiprog.py -i i2cdev -d 1 -w firmware.bin
python3 rtdmultiprog.py -i i2cdev -d 1 -r verify.bin -z 32768 && md5sum verify.bin
```

Keep a full dump of your vendor firmware before flashing
(`-r dump.bin -z 524288`); restoring it is a single `-w dump.bin`.

## Changed files

* `config/panel_config.h` — LTM09C362V timing (1024x600, DCLK 49.088 MHz,
  HTotal 1312 / VTotal 622, 18-bit, no bit-order swap).
* `config/board_config.h`, `config/misc_config.h` — board specifics.
* `peripherals/ddc.c` — `testEDID` replaced with a panel-native
  1024x600@60.15 EDID (DTD: 49.09 MHz, H 1024/40/104/144, V 600/3/10/9).
* `core/main.c` — backlight/panel-power GPIO bring-up for this board,
  gamma LUT + dither initialization ported from the vendor reference code,
  vertical capture-window offset (V start = 18 lines).
* `scaler/scaling.c` — at 1:1 the UZD line buffer is bypassed
  (`BUFFER_MODE=00`, `SBUFF_EXT=0`); the buffered path with `SBUFF_EXT`
  is only configured when actually downscaling (see notes).
* `scaler/scaler.c` — `SetDPLLFrequncy()` uses a high-VCO configuration
  (DPN=8, output Div4, VCO ≈ 400 MHz) for the 10–100 MHz range, matching
  the configuration the vendor firmware runs (measured on live hardware).
* `scaler/scaler_tables.c` — LVDS control register init, including **the
  single bit that cost us a whole day** (finding 1 below).

## Findings that may save you days

1. **TCON register 0xA3 (LVDS_CTRL3) bit 0 = BMTS, the LVDS bit-mapping
   table select. It is panel-dependent, and the reset default may be
   wrong for your panel.** The LTM09C362V needs Table 2 (bit0 = 1). With
   the default Table 1, *only intermediate pixel values are corrupted* —
   colored per-pixel speckle on antialiased text — while pure 0x00/0xFF
   content (1-px stripe patterns, solid color bars) renders **perfectly**,
   because any bit-position permutation is invisible when all bits of a
   channel are equal. This makes the bug almost immune to classic test
   patterns. If your panel shows colored noise on text but passes every
   stripe test you throw at it: try the other BMTS table.

2. **LVDS_CTRL1 (TCON 0xA1) lower bits are not don't-care.** Upstream's
   `DisplayInitTable` writes `0xC0` (the two clock-polarity inversion
   bits), which also clears STSTL (default 010) and the LVDS output
   common-mode setting (default 100). On this panel that produced visible
   flicker; restoring the datasheet default lower bits (`0xD4`) fixed it.
   The vendor firmware runs `0xD7` (common-mode 111); both are stable here.

3. **UZD "2-tap" (page 6, 0xE3 bit 4) is not an image filter — it is
   SBUFF_EXT**, a line-buffer width extension (960 → 1920 px, RTD2660
   datasheet p150-151). In the buffered path it is mandatory for widths
   over 960 (clearing it wraps the image); at 1:1 the correct
   configuration is to take the buffer out of the path entirely
   (`BUFFER_MODE=00`), which is what the vendor firmware does.

4. **The firmware-served EDID is enough.** With a valid DTD both Raspberry
   Pi OS (KMS) and Armbian pick the panel-native mode with no
   `config.txt` / kernel-cmdline overrides. If you inherited
   `hdmi_timings=`, `hdmi_ignore_edid=` or `drm.edid_firmware=` overrides
   from earlier experiments, remove them — a stale 592-line override cost
   us hours (it silently re-enables vertical scaling and smears text).

5. **Backlight on this board is not the usual PCB800099-style P3_3/P3_4**
   (those pins are a bit-banged I2C bus here). A group of scaler GPIOs
   must be configured as outputs and driven low; see `core/main.c`.

## Known cosmetic issue

Input (60.15 Hz) and output (60.00 Hz, VTotal 625) are not frame-locked;
a slowly drifting beat line can be seen on a 1-px checkerboard test
pattern (invisible in normal content). Deriving the output VTotal from
the input (622) should lock them — left as a TODO.

## Thanks

Huge thanks to **KerJoe** for ORTD2662 and RTDMultiProg. A from-scratch,
plain-SDCC, readable firmware for these scalers is what made this port
possible at all — the vendor Keil source tree does not even build without
a paid toolchain. 感謝！

---

# 日本語版 / Japanese

# 移植記録: X@RTD2662 基板 + 東芝 LTM09C362V (1024x600)

このフォークは [KerJoe/ORTD2662](https://github.com/KerJoe/ORTD2662) を、
汎用 **X@RTD2662** コントローラ基板 + **東芝 LTM09C362V**
(9.0インチ 1024x600 18bit 1ch LVDS パネル) の組み合わせに移植し、
単体HDMIモニタとして使えるようにしたものです。

ステータス: **完全動作。** ファームがネイティブの 1024x600@60.15 EDID を
提供し、表示は1:1ピクセルパーフェクト、色も正常です。ソース側の設定を
一切変更せず、2種類の異なるHDMIソースで動作を確認しました:

* Raspberry Pi 3B (Raspberry Pi OS, vc4-kms-v3d)
* Orange Pi PC2 (Armbian 26.2.5)

確認できた範囲では、これはORTD2662を別の基板・別のパネルに移植して
文書化した最初の事例です。

これは *非常に* 長く塩漬けにされたプロジェクトでした。基板の裏には
**2010/1** の製造刻印があり、発売直後に基板+パネルのキットとして購入した後、
約17年間引き出しで眠っていたものを、ようやく蘇らせました。

## 配線 (基板との通信方法)

この基板には専用の書き込みヘッダがなく、スケーラのI2C経由で書き込む。
そのI2Cは **VGA (D-Sub 15) コネクタのDDCピン** に出ている。Raspberry Pi
から、双方向の **MOSFETロジックレベル変換モジュール** (よくあるBSS138タイプ)
を介してこのI2Cを駆動する。

基板側のI2Cラインはテスターで **4.8V** を実測したため、変換モジュールの
**高圧側 (HV)** に接続する。Piの3.3V I2Cは **低圧側 (LV)** へ。

```
[Pi GPIO]          [変換 LV側]      [変換 HV側]      [D-Sub 15]
3.3V  (pin 1) ───── LV
5V    (pin 2) ─────────────────────── HV
GND   (pin 6) ───── GND ───────────── GND ────────── pin 6  (GND)
SDA   (pin 3) ───── LV1          HV1 ─────────────── pin 12 (SDA / DDC data)
SCL   (pin 5) ───── LV2          HV2 ─────────────── pin 15 (SCL / DDC clock)
```

Piのハードウェア I2C バス (`/dev/i2c-1`) を使用しており、これが下記の
RTDMultiProg コマンドで `-d 1` を指定している理由。表示/バックライトのため
パネルには別途12V電源が必要で、これはこのI2C接続とは別系統。

## ビルドと書き込み

素のSDCC (4.2.0で確認) のみ、他の依存なし:

```
make firmware          # -> output/firmware.bin (32 KB)
```

動作確認済みビルドの md5: `dab06e957b8bca27de369c9ab5abb3cb`

書き込みは [RTDMultiProg](https://github.com/KerJoe/RTDMultiProg) を使用
(ここでは Raspberry Pi のI2Cを基板のHDMI DDCピンに接続):

```
python3 rtdmultiprog.py -i i2cdev -d 1 -w firmware.bin
python3 rtdmultiprog.py -i i2cdev -d 1 -r verify.bin -z 32768 && md5sum verify.bin
```

書き込み前に純正ファームの完全ダンプを必ず取得しておくこと
(`-r dump.bin -z 524288`)。復元は `-w dump.bin` 一発です。

## 変更したファイル

* `config/panel_config.h` — LTM09C362Vのタイミング (1024x600、DCLK 49.088 MHz、
  HTotal 1312 / VTotal 622、18bit、ビット順スワップなし)。
* `config/board_config.h`, `config/misc_config.h` — 基板固有の設定。
* `peripherals/ddc.c` — `testEDID` をパネルネイティブの
  1024x600@60.15 EDID に置き換え (DTD: 49.09 MHz, H 1024/40/104/144, V 600/3/10/9)。
* `core/main.c` — この基板用のバックライト/パネル電源GPIOの起動、
  純正リファレンスコードから移植したガンマLUT + ディザ初期化、
  垂直取り込み窓のオフセット (V開始 = 18行)。
* `scaler/scaling.c` — 1:1のときUZDラインバッファをバイパス
  (`BUFFER_MODE=00`, `SBUFF_EXT=0`)。`SBUFF_EXT` を使うバッファ経路は
  実際に縮小するときだけ設定する (下記参照)。
* `scaler/scaler.c` — `SetDPLLFrequncy()` を高VCO構成
  (DPN=8, 出力Div4, VCO ≈ 400 MHz) に変更 (10–100 MHz範囲)。
  純正ファームが実機で使っている構成に合わせた (実機実測による)。
* `scaler/scaler_tables.c` — LVDS制御レジスタの初期化。**丸一日を溶かした
  あの1ビット** を含む (下記の知見1)。

## 何日も節約できるかもしれない知見

1. **TCONレジスタ 0xA3 (LVDS_CTRL3) の bit0 = BMTS、LVDSビットマッピング
   テーブル選択。これはパネル依存で、リセット既定値があなたのパネルには
   間違っている可能性がある。** LTM09C362VはTable 2 (bit0 = 1) が必要。
   既定のTable 1では *中間階調のピクセルだけが化ける* —— アンチエイリアス
   された文字にピクセル単位の色ノイズが乗る —— のに、純粋な 0x00/0xFF の
   コンテンツ (1px縞パターン、ベタ色帯) は **完璧に** 映る。チャンネルの
   全ビットが等しいときはビット位置の入れ替えが不可視になるため。この
   バグは古典的なテストパターンにほぼ引っかからない。文字には色ノイズが
   出るのにあらゆる縞テストを通過してしまう場合は、BMTSのもう一方の
   テーブルを試すこと。

2. **LVDS_CTRL1 (TCON 0xA1) の下位ビットはdon't-careではない。** 上流の
   `DisplayInitTable` は `0xC0` (クロック極性反転の2ビット) を書くが、
   これは同時にSTSTL (既定010) とLVDS出力コモンモード設定 (既定100) を
   ゼロにしてしまう。このパネルではそれが目に見えるフリッカーを生じた。
   データシート既定の下位ビットを復元 (`0xD4`) すると解消。純正ファームは
   `0xD7` (コモンモード111) で動作しており、こちらでは両方とも安定。

3. **UZDの「2-tap」(page 6, 0xE3 bit4) は画像フィルタではなく SBUFF_EXT** で、
   ラインバッファ幅の拡張 (960 → 1920 px、RTD2660データシート p150-151)。
   バッファ経路では960pxを超える幅に必須 (クリアすると画像が巻き込む)。
   1:1のときの正しい構成は、バッファを経路から完全に外すこと
   (`BUFFER_MODE=00`)。純正ファームもそうしている。

4. **ファームが提供するEDIDだけで十分。** 有効なDTDがあれば、Raspberry
   Pi OS (KMS) も Armbian も `config.txt` やカーネルcmdlineの上書きなしで
   パネルネイティブモードを選ぶ。以前の実験で `hdmi_timings=`、
   `hdmi_ignore_edid=`、`drm.edid_firmware=` の上書きが残っていたら削除する
   こと —— 古い592行の上書きが残っていて数時間溶かした (垂直スケーリングを
   こっそり再有効化して文字をにじませる)。

5. **この基板のバックライトは、よくあるPCB800099系のP3_3/P3_4ではない**
   (このピンはここではビットバンギングのI2Cバス)。スケーラのGPIO群を
   出力に設定してLowに駆動する必要がある。`core/main.c` 参照。

## 既知の軽微な問題

入力 (60.15 Hz) と出力 (60.00 Hz, VTotal 625) がフレームロックしておらず、
1pxの市松テストパターンでゆっくり流れるうなり線が見える (通常のコンテンツ
では不可視)。出力VTotalを入力から導出する (622) とロックするはず ——
TODOとして残している。

## 謝辞

**KerJoe** 氏の ORTD2662 と RTDMultiProg に心から感謝します。これらの
スケーラ向けに、ゼロから書かれた・素のSDCCでビルドできる・読みやすい
ファームウェアが存在したことが、この移植を可能にした全てです ——
純正のKeilソースツリーは有償ツールチェーンなしではビルドすらできません。
感謝！
