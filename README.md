# wxFreeCell

A cross-platform port of the classic Microsoft FreeCell solitaire card game, built with [wxWidgets](https://www.wxwidgets.org/).

Originally written by Jim Horne in June 1991, this port preserves the original game logic and shuffle algorithm exactly -- game numbers produce identical card layouts across all platforms.

## Screenshots

- Windows:

![](/screenshots/wxfreecell_win.png)

- macOS:

![](/screenshots/wxfreecell_macOS.png)

- Linux:

![](/screenshots/wxfreecell_linux.png)

## Features

- All 1,000,000 game numbers from the original FreeCell
- Identical card deals to the classic Windows version
- Undo support
- Win/loss statistics with streak tracking
- Configurable options (animation speed, double-click behavior, move messages)

## Building

### Prerequisites

- CMake 3.14 or later
- A C++11 compiler
- wxWidgets (with `core`, `base`, `xrc`, `html`, and `xml` components)
- The `wxrc` tool (included with wxWidgets)

### macOS

```sh
brew install wxwidgets
mkdir build && cd build
cmake ..
make
```

The output is a `wxFreeCell.app` bundle.

### Linux

```sh
# Debian/Ubuntu
sudo apt install libwxgtk3.2-dev wx-common

mkdir build && cd build
cmake ..
make
sudo make install
```

### Windows

```sh
mkdir build && cd build
cmake .. -DwxWidgets_ROOT_DIR=C:/path/to/wxWidgets
cmake --build . --config Release
```

## How to Play

The goal is to move all 52 cards to the four foundation piles (top-right), building each suit up from Ace to King.

- **Columns** -- build down in alternating colors (red on black, black on red).
- **Free cells** (top-left) -- each holds one card temporarily.
- **Foundations** (top-right) -- build up by suit from Ace to King.

Cards are moved by clicking the source card, then clicking the destination. The number of cards you can move at once depends on the number of empty free cells and empty columns available.

### Keyboard Shortcuts

| Key | Action |
|---|---|
| F2 | New game |
| F3 | Select game by number |
| F4 | Statistics |
| F5 | Options |
| F10 | Undo |

## License

GPL-2.0
