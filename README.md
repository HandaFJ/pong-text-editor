Markdown

# Juiced Pong Text Editor

ただのテキストエディタではありません。タイピングの「打鍵感（Juice）」と、アーケードゲームの「Pong（ポン）」が融合した、唯一無二のデジタルアート・エディタです。

キーボードを叩くたびに文字が跳ね回り、文字同士が衝突してスパークが散る、五感で味わうタイピング空間を提供します。

---

## 特徴 (Features)

- **タイピングとPong物理の融合**: 入力した文字が画面の「お化け（物理オブジェクト）」として実体化し、壁や他の文字と衝突しながら画面中を跳ね回ります。
- **完全同期のバッファ管理**: バックスペースで文字を消去すると、画面上の対応するPong文字も連動してプチ爆発とともに消滅します。
- **ジュシーな演出 (Juiciness)**:
  - 打鍵時の画面シェイク（スクリーンシェイク）
  - パーティクル爆発エフェクト
  - ポップアップ拡大アニメーション
  - 文字同士の弾性衝突とスパーク演出
- **エディタとしての基本機能**: 自動折り返し（Word Wrap）に対応。

---

## 技術スタック (Tech Stack)

- **言語**: C++ (C++17)
- **ビルドシステム**: CMake (v3.16以上)
- **グラフィック・ウィンドウ**: SFML 3 (Simple and Fast Multimedia Library)
- **フォント**: IPAexゴシック (`ipaexg.ttf`) （※OS標準フォントへのフォールバック対応）

---

## 再現手順 (Getting Started)

### 1. 前提条件（環境構築）

お使いの環境（macOS / Windows）に **CMake** と **SFML 3** がインストールされている必要があります。

- **macOS (Homebrew)**:
  ```bash
  brew install cmake sfml
  Windows:
  公式サイトやパッケージマネージャーから CMake および SFML 3 をインストールしてください。
  ```

2. ファイルの配置
   リポジトリのルートディレクトリに、以下のファイル構造で配置します。

- main.cpp
- CMakeLists.txt
- assets/ipaexg.ttf （※なくてもOS標準フォントにフォールバックします）

3. CMakeによるビルド (Build)
   ターミナル（またはコマンドプロンプト）でリポジトリのルートディレクトリを開き、以下のコマンドを実行します。

```Bash
# 1. ビルド用ディレクトリを作成して移動

mkdir build
cd build

# 2. CMakeの構成（macOSでHomebrew版SFMLが見つからない場合は -DSFML_DIR を指定してください）

cmake .

# 3. コンパイル実行

cmake --build .
(※ macOSの場合: cmake -DSFML_DIR=/opt/homebrew/lib/cmake/SFML ..)
```

4. 実行 (Run)
   ビルドが成功すると、build ディレクトリ内（またはその中の Debug / Release フォルダ内）に実行ファイルが生成されます。

```Bash
# macOS / Linux
./juiced_editor

# Windows (コマンドプロンプト等)
.\juiced_editor.exe
```

## ライセンス (License)
このプロジェクトは MIT ライセンスの下で公開されています。詳細については LICENSE ファイルをご確認ください。

※ 本プロジェクトで使用しているフォント（ipaexg.ttf）は、独立行政法人情報処理推進機構（IPA）が定める 「IPAフォントライセンス v1.0」 に準拠しています。
