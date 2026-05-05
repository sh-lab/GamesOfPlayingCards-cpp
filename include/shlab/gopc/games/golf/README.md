# Golf

`shlab::gopc::games::golf::Golf` は、Golf の状態を
`Card -> Position` の不変写像として表現し、`can_loop` の設定を保持します。

## 公開 API

- umbrella header: `shlab/gopc/games/golf.hpp`
- namespace: `shlab::gopc::games::golf`
- public types: `Golf`, `Position`, `Lane`

## 状態モデル

- `Golf::state_type` は `std::unordered_map<Card, Position>`
- `Position` は `Hand`, `Deck`, `Field` の `std::variant`
- `Hand` と `Deck` は `number` を持つ
- `Field` は `lane` と `number` を持つ
- `can_loop()` で K-A の循環を含むルール設定を取得できる

## 主要操作

- `deal(std::span<const Card>, bool can_loop)`
- `move_to_hand(const Card&)`
- `deck_count()`
- `is_win()`
- `is_lose()`

すべての遷移 API は既存状態を変更せず、新しい `Golf` を返します。

## 現在の実装範囲

- deck / field から hand への遷移を `move_to_hand()` で表現する
- `can_loop` ルールの有無を切り替えられる
- joker あり / なしの deck を扱える

