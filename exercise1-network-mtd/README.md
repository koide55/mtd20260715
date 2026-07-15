# 演習1: ネットワークレベルMTD（URL/ポート・シャッフリング）
# Exercise 1: Network-level MTD (URL / Port Shuffling)

Webシステムのネットワーク識別子（ここではポート番号）を実行時に変化させることで、
攻撃者の偵察・攻撃を難しくする「動く標的」を体験する。

*Experience a "moving target" by changing a web system's network identifier (here, the
port number) at run time, making an attacker's reconnaissance and attack harder.*

## 必要なもの / Requirements

- Python 3.8 以降（標準ライブラリのみ / standard library only）
- `curl`（動作確認用 / for testing）

## 実行 / Run

```console
# 方法A: デモスクリプトで一括体験 / one-shot demo
$ ./demo.sh

# 方法B: 手動で / manually
$ python3 mtdnet.py 8123           # 初期ポート8123で起動 / start on port 8123
```

別の端末から / from another terminal:

```console
$ curl localhost:8123                        # → "This is a response. cnt=1000"
$ curl "localhost:8123/restart?port=9999"    # 次回から 9999 へ移動 / move to 9999
$ curl localhost:8123                         # → 接続拒否（標的が移動）/ refused (target moved)
$ curl localhost:9999                         # → OK
$ curl "localhost:9999/restart?port=8111"    # さらに 8111 へ / move again to 8111
$ curl localhost:8111                         # → OK
$ curl localhost:8111/shutdown               # サーバ停止 / stop the server
```

## 仕組み / How it works

`mtdnet.py` は単純なHTTPサーバで、次の3つの振る舞いを持つ：

| リクエスト / Request | 動作 / Behavior |
|---|---|
| `GET /` | 通常のレスポンスを返す / normal response |
| `GET /restart?port=<N>` | レスポンス後、次のアクセスからポート `<N>` へ移動 / move to port `<N>` |
| `GET /shutdown` | レスポンス後、サーバを停止 / stop the server |

`MainServer.run_forever()` は単一スレッドのループで、移動要求が来ると現在のリスナを
閉じ、新しいポートで再バインドする（レースのない決定的な実装）。

- **what** : ポート番号（URLの構成要素）/ the port number (a URL component)
- **when** : 正規ユーザのアクセス時 / on legitimate access
- **how** : クライアントと共有する「次のポート」規則で / by a "next port" rule shared with the client

## グループ演習で考えること / Group discussion

1. 複数サーバが互いにネットワークレベルMTDで通信しあう実験環境の設計・実装。
2. 評価指標（例: 平均攻撃成功時間間隔 MTTC、可用性への影響）と測定実験の方法。
3. 正規ユーザの余分な負担が少ないURL指定方法の評価。
4. 正規ユーザ側から安全にURLを変更する方法（本実装は平文の `?port=` なので改善余地あり）。
5. 演習2（システムコールMTD）と組み合わせたときの「平均攻撃成功時間間隔」の変化。
6. 企業内の相互通信環境で、ネットワークレベルMTDがマルウェア侵入検知に役立つ理由。

## 元実装からの変更点 / Changes from the original

旧サンプル（Gist `mtdnet2.py`, 2021）を以下の点で更新：

- `Content-Length` ヘッダを文字列（バイト長）で送信し、Python 3.12+ でも警告なく動作。
- スレッド起動によるポート移動の**競合を排除**し、単一スレッドの再バインドループに変更。
- `/shutdown` を追加し、リソースリークなく停止できるように。
