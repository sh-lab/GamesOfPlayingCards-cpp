# FreeCell

`shlab::gopc::games::freecell::FreeCell` は、FreeCell の状態を
`Card -> Position` の不変写像として表現します。

## 公開 API

- umbrella header: `shlab/gopc/games/freecell.hpp`
- namespace: `shlab::gopc::games::freecell`
- public types: `FreeCell`, `Position`, `Column`

## 状態モデル

- `FreeCell::state_type` は `shlab::gopc::utility::CardState<Position, 52>`
- `Position` は `Cell`, `Foundation`, `Tableau` を保持する tagged union 型で、`kind()` / `is<T>()` / `get<T>()` / `get_if<T>()` / `visit(...)` で扱える
- `Cell` は `number` を持つ
- `Tableau` は `column` と `number` を持つ

## 主要操作

- `deal(std::span<const Card>)`
- `move_to_cell(const Card&)`
- `move_to_foundation(const Card&)`
- `move_to_tableau(const Card&, Column)`

すべての遷移 API は既存状態を変更せず、新しい `FreeCell` を返します。

## 現在の実装範囲

- cells、foundations、tableau を公開状態として扱う
- 勝利判定は `is_win()` で取得できる
- tableau / cell / foundation 間の移動は `can_*` と対応する `move_*` の組で公開する
