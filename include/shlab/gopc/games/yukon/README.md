# Yukon

`shlab::gopc::games::yukon::Yukon` は、Yukon の状態を
`Card -> Position` の不変写像として表現します。

## 公開 API

- umbrella header: `shlab/gopc/games/yukon.hpp`
- namespace: `shlab::gopc::games::yukon`
- public types: `Yukon`, `Position`, `Column`

## 状態モデル

- `Yukon::state_type` は `shlab::gopc::utility::CardState<Position, 52>`
- `Position` は `Foundation` と `Tableau` を保持する tagged union 型で、`kind()` / `is<T>()` / `get<T>()` / `get_if<T>()` / `visit(...)` で扱える
- `Tableau` は `column`, `number`, `open` を持つ

## 主要操作

- `deal(std::span<const Card>)`
- `open(const Card&)`
- `move_to_foundation(const Card&)`
- `move_to_tableau(const Card&, Column)`
- `is_moved_foundation_no_problem(const Card&)`

すべての遷移 API は既存状態を変更せず、新しい `Yukon` を返します。

## 現在の実装範囲

- この実装では stock / waste は持たない
- open な tableau カードから下の tail をまとめて移動できる
- tableau の開閉状態は `Tableau::open` で保持する
